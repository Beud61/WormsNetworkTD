#include "Network/OnlineSessionSubsystem.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystemUtils.h"
#include "Beacon/LobbyBeaconHostObject.h"
#include "Beacon/LobbyBeaconClient.h"
#include "Beacon/LobbyTypes.h"
#include "OnlineBeaconHost.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

// ============================================================
//  Initialisation / Nettoyage
// ============================================================

void UOnlineSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Session = Online::GetSessionInterface(GetWorld());
}

void UOnlineSessionSubsystem::Deinitialize()
{
	CleanupBeaconClient();
	Super::Deinitialize();
}

void UOnlineSessionSubsystem::CleanupBeaconClient()
{
	if (LobbyBeaconClient)
	{
		LobbyBeaconClient->OnLobbyUpdated.RemoveDynamic(this, &UOnlineSessionSubsystem::HandleLobbyUpdated_Internal);
		LobbyBeaconClient->DestroyBeacon();
		LobbyBeaconClient = nullptr;
	}
	bBeaconConnecting = false;
}

// ============================================================
//  Création de session
// ============================================================

void UOnlineSessionSubsystem::CreateSession(const FString& SessionName, int32 NumPublicConnections,
	bool bIsLanMatch, const FString& GameMode, int32 UnitLife, int32 UnitCount, int32 TurnsBeforeWater)
{
	if (!Session.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("CreateSession: Session interface invalide."));
		return;
	}

	MaxPlayers = NumPublicConnections;

	LastSessionSettings = MakeShareable(new FOnlineSessionSettings());
	LastSessionSettings->NumPublicConnections = NumPublicConnections;
	LastSessionSettings->bAllowJoinInProgress = true;
	LastSessionSettings->bAllowJoinViaPresence = true;
	LastSessionSettings->bIsDedicated = false;
	LastSessionSettings->bUsesPresence = true;
	LastSessionSettings->bIsLANMatch = bIsLanMatch;
	LastSessionSettings->bShouldAdvertise = true;

	LastSessionSettings->Set(LobbyConstants::Key_SessionName, SessionName, EOnlineDataAdvertisementType::ViaOnlineService);
	LastSessionSettings->Set(LobbyConstants::Key_GameMode, GameMode, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	LastSessionSettings->Set(LobbyConstants::Key_UnitLife, UnitLife, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	LastSessionSettings->Set(LobbyConstants::Key_UnitCount, UnitCount, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	LastSessionSettings->Set(LobbyConstants::Key_TurnsBeforeWater, TurnsBeforeWater, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	CreateHandle = Session->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UOnlineSessionSubsystem::OnCreateSessionCompleted)
	);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!Session->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *LastSessionSettings))
	{
		UE_LOG(LogTemp, Error, TEXT("CreateSession: appel a CreateSession() echoue."));
		Session->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
	}
}

void UOnlineSessionSubsystem::OnCreateSessionCompleted(FName SessionName, bool Successful)
{
	Session->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);

	if (!Successful)
	{
		UE_LOG(LogTemp, Error, TEXT("OnCreateSessionCompleted: echec de creation de session."));
		return;
	}

	// Enregistre le joueur local dans la session
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (LocalPlayer && Session.IsValid())
	{
		Session->RegisterPlayer(NAME_GameSession, *LocalPlayer->GetPreferredUniqueNetId(), false);
	}

	// Session prête -> on démarre le beacon host
	CreateHostBeacon();
}

// ============================================================
//  Recherche de sessions
// ============================================================

void UOnlineSessionSubsystem::FindSessions(int32 MaxSearchResults, bool bIsLANQuery)
{
	if (!Session.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("FindSessions: Session interface invalide."));
		return;
	}

	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	LastSessionSearch->MaxSearchResults = MaxSearchResults;
	LastSessionSearch->bIsLanQuery = bIsLANQuery;
	LastSessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);

	FindHandle = Session->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UOnlineSessionSubsystem::OnFindSessionsCompleted)
	);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!Session->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef()))
	{
		UE_LOG(LogTemp, Error, TEXT("FindSessions: appel a FindSessions() echoue."));
		Session->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
	}
}

void UOnlineSessionSubsystem::OnFindSessionsCompleted(bool Successful)
{
	Session->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
	SearchResults = LastSessionSearch->SearchResults;

	TArray<FCustomSessionInfo> SessionInfos;
	for (int32 i = 0; i < SearchResults.Num(); i++)
	{
		const FOnlineSessionSearchResult& Result = SearchResults[i];
		FCustomSessionInfo Info;
		Result.Session.SessionSettings.Get(LobbyConstants::Key_SessionName, Info.SessionName);
		Result.Session.SessionSettings.Get(LobbyConstants::Key_GameMode, Info.GameMode);
		Info.CurrentPlayers = Result.Session.SessionSettings.NumPublicConnections
			- Result.Session.NumOpenPublicConnections;
		Info.MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
		Info.Ping = Result.PingInMs;
		Info.SessionSearchResultIndex = i;
		SessionInfos.Add(Info);
	}

	UE_LOG(LogTemp, Warning, TEXT("FindSessions termine: %d resultat(s), succes=%d"),
		SearchResults.Num(), Successful);

	OnFindSessionsCompleteEvent.Broadcast(SessionInfos, Successful);
}

// ============================================================
//  Join session classique (ServerTravel)
// ============================================================

void UOnlineSessionSubsystem::JoinGameSession(const FOnlineSessionSearchResult& SessionResult)
{
	if (!Session.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("JoinGameSession: Session interface invalide."));
		return;
	}

	JoinHandle = Session->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &UOnlineSessionSubsystem::OnJoinSessionCompleted)
	);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!Session->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SessionResult))
	{
		UE_LOG(LogTemp, Error, TEXT("JoinGameSession: appel a JoinSession() echoue."));
		Session->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
	}
}

void UOnlineSessionSubsystem::OnJoinSessionCompleted(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	Session->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);

	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		UE_LOG(LogTemp, Error, TEXT("OnJoinSessionCompleted: echec (%d). Abandon du voyage."), (int32)Result);
		OnSessionJoinCompleted.Broadcast(false);
		return;
	}

	FString ConnectString;
	if (!Session->GetResolvedConnectString(NAME_GameSession, ConnectString))
	{
		UE_LOG(LogTemp, Error, TEXT("OnJoinSessionCompleted: impossible de resoudre l'adresse de connexion."));
		OnSessionJoinCompleted.Broadcast(false);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("OnJoinSessionCompleted: succes, voyage vers %s"), *ConnectString);

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController)
	{
		PlayerController->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
	}
}

// ============================================================
//  Custom join via Beacon (lobby pré-game)
// ============================================================

void UOnlineSessionSubsystem::CustomJoinSession(const FCustomSessionInfo& SessionInfo)
{
	UE_LOG(LogTemp, Warning, TEXT("CustomJoinSession: demarrage pour l'index %d"), SessionInfo.SessionSearchResultIndex);

	if (!Session.IsValid() || !SearchResults.IsValidIndex(SessionInfo.SessionSearchResultIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("CustomJoinSession: session invalide ou index hors limites."));
		OnSessionJoinCompleted.Broadcast(false);
		return;
	}

	if (bBeaconConnecting)
	{
		UE_LOG(LogTemp, Warning, TEXT("CustomJoinSession: connexion beacon deja en cours, ignore."));
		return;
	}

	// Nettoie un éventuel beacon client résiduel
	CleanupBeaconClient();

	// Crée le beacon client
	LobbyBeaconClient = GetWorld()->SpawnActor<ALobbyBeaconClient>();
	if (!LobbyBeaconClient)
	{
		UE_LOG(LogTemp, Error, TEXT("CustomJoinSession: impossible de spawner le BeaconClient."));
		OnSessionJoinCompleted.Broadcast(false);
		return;
	}

	LobbyBeaconClient->SetActorHiddenInGame(true);
	LobbyBeaconClient->SetActorEnableCollision(false);
	LobbyBeaconClient->SetReplicates(true);

	// Bind les événements AVANT la connexion pour ne rien manquer
	LobbyBeaconClient->OnLobbyUpdated.AddDynamic(this, &UOnlineSessionSubsystem::HandleLobbyUpdated_Internal);

	const FOnlineSessionSearchResult& TempResult = SearchResults[SessionInfo.SessionSearchResultIndex];

	LobbyBeaconClient->OnRequestValidate.BindLambda(
		[this, TempResult](bool bValidated)
		{
			bBeaconConnecting = false;
			if (bValidated)
			{
				UE_LOG(LogTemp, Warning, TEXT("CustomJoinSession: beacon valide."));
				// Notifie l'UI que le beacon est prêt
				OnBeaconClientCreated.Broadcast(LobbyBeaconClient);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("CustomJoinSession: validation beacon echouee."));
				CleanupBeaconClient();
				OnSessionJoinCompleted.Broadcast(false);
			}
		}
	);

	// Résolution de l'adresse IP de l'hôte depuis l'OSS.
	// GetResolvedConnectString retourne quelque chose comme "192.168.1.10:7777".
	// On extrait l'IP et on remplace le port par celui du beacon.
	FString HostIP = TEXT("127.0.0.1"); // fallback PIE/LAN local
	{
		FString ConnectString;
		if (Session->GetResolvedConnectString(TempResult, NAME_GameSession, ConnectString))
		{
			// ConnectString format : "IP:Port" — on garde seulement l'IP
			FString Port;
			if (ConnectString.Split(TEXT(":"), &HostIP, &Port))
			{
				UE_LOG(LogTemp, Warning, TEXT("CustomJoinSession: IP hote resolue = %s"), *HostIP);
			}
			else
			{
				// Pas de ":" -> ConnectString est déjà une IP pure
				HostIP = ConnectString;
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("CustomJoinSession: GetResolvedConnectString echoue, fallback 127.0.0.1"));
		}
	}

	FURL Destination(nullptr, *HostIP, TRAVEL_Absolute);
	Destination.Port = LobbyConstants::BeaconPort;
	UE_LOG(LogTemp, Warning, TEXT("CustomJoinSession: connexion beacon a %s:%d"), *Destination.Host, Destination.Port);

	bBeaconConnecting = true;
	LobbyBeaconClient->ConnectToServer(Destination);
}

// ============================================================
//  Beacon host (côté serveur)
// ============================================================

void UOnlineSessionSubsystem::CreateHostBeacon()
{
	// Idempotent : ne respawne pas si déjà actif
	if (BeaconHost)
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateHostBeacon: beacon host deja actif, ignore."));
		return;
	}

	BeaconHost = GetWorld()->SpawnActor<AOnlineBeaconHost>();
	if (!BeaconHost)
	{
		UE_LOG(LogTemp, Error, TEXT("CreateHostBeacon: impossible de spawner AOnlineBeaconHost."));
		return;
	}

	if (!BeaconHost->InitHost())
	{
		UE_LOG(LogTemp, Error, TEXT("CreateHostBeacon: InitHost() echoue."));
		BeaconHost->Destroy();
		BeaconHost = nullptr;
		return;
	}

	BeaconHost->PauseBeaconRequests(false);

	ALobbyBeaconHostObject* HostObject = GetWorld()->SpawnActor<ALobbyBeaconHostObject>();
	if (!HostObject)
	{
		UE_LOG(LogTemp, Error, TEXT("CreateHostBeacon: impossible de spawner ALobbyBeaconHostObject."));
		return;
	}

	// ReservedSlots démarre à 0 : le slot de l'hôte sera accordé via le handshake
	// beacon (Server_RequestReservation), identique à n'importe quel client.
	HostObject->ReservedSlots = 0;
	HostObject->MaxSlots = MaxPlayers;

	int32 UnitCount = 1;
	if (LastSessionSettings.IsValid())
	{
		LastSessionSettings->Get(LobbyConstants::Key_UnitCount, UnitCount);
	}
	HostObject->RoomUnitCount = UnitCount;

	BeaconHost->RegisterHost(HostObject);

	UE_LOG(LogTemp, Warning, TEXT("CreateHostBeacon: host actif sur le port %d."), BeaconHost->ListenPort);

	// L'hôte se connecte à son propre beacon en tant que client pour envoyer
	// ses FPlayerLobbyInfo et apparaître dans la liste du lobby.
	ConnectHostAsClient(PendingHostPlayerInfo);
}

// ============================================================
//  Connexion de l'hôte à son propre beacon (listen-server)
// ============================================================

void UOnlineSessionSubsystem::ConnectHostAsClient(const FPlayerLobbyInfo& HostInfo)
{
	if (bBeaconConnecting)
	{
		UE_LOG(LogTemp, Warning, TEXT("ConnectHostAsClient: connexion deja en cours, ignore."));
		return;
	}

	// On nettoie un éventuel client résiduel avant de recréer
	CleanupBeaconClient();

	LobbyBeaconClient = GetWorld()->SpawnActor<ALobbyBeaconClient>();
	if (!LobbyBeaconClient)
	{
		UE_LOG(LogTemp, Error, TEXT("ConnectHostAsClient: impossible de spawner ALobbyBeaconClient."));
		return;
	}

	LobbyBeaconClient->SetActorHiddenInGame(true);
	LobbyBeaconClient->SetActorEnableCollision(false);
	LobbyBeaconClient->SetReplicates(true);

	// Bind les événements AVANT la connexion
	LobbyBeaconClient->OnLobbyUpdated.AddDynamic(this, &UOnlineSessionSubsystem::HandleLobbyUpdated_Internal);

	// Capture HostInfo par valeur pour l'utiliser dans le lambda
	LobbyBeaconClient->OnRequestValidate.BindLambda(
		[this, HostInfo](bool bValidated)
		{
			bBeaconConnecting = false;
			if (bValidated)
			{
				UE_LOG(LogTemp, Warning, TEXT("ConnectHostAsClient: hote enregistre dans le lobby."));
				OnBeaconClientCreated.Broadcast(LobbyBeaconClient);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("ConnectHostAsClient: validation echouee pour l'hote."));
				CleanupBeaconClient();
			}
		}
	);

	// On surcharge ici le PlayerInfo qui sera envoyé par Client_ReservationAccepted_Implementation.
	// On stocke les infos hôte dans le client beacon pour qu'il les utilise à la place des valeurs par défaut.
	LobbyBeaconClient->PendingPlayerInfo = HostInfo;

	// Connexion locale (l'hôte se connecte à son propre beacon)
	FURL Destination(nullptr, TEXT("127.0.0.1"), TRAVEL_Absolute);
	Destination.Port = LobbyConstants::BeaconPort;
	UE_LOG(LogTemp, Warning, TEXT("ConnectHostAsClient: connexion locale a %s:%d"), *Destination.Host, Destination.Port);

	bBeaconConnecting = true;
	LobbyBeaconClient->ConnectToServer(Destination);
}

// ============================================================
//  Destroy session
// ============================================================

void UOnlineSessionSubsystem::DestroySession()
{
	if (!Session.IsValid())
		return;

	// On nettoie aussi le beacon host
	if (BeaconHost)
	{
		BeaconHost->Destroy();
		BeaconHost = nullptr;
	}

	DestroyHandle = Session->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &UOnlineSessionSubsystem::OnDestroySessionCompleted)
	);

	if (!Session->DestroySession(NAME_GameSession))
	{
		Session->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
	}
}

void UOnlineSessionSubsystem::OnDestroySessionCompleted(FName SessionName, bool Successful)
{
	if (Session)
		Session->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);

	UE_LOG(LogTemp, Warning, TEXT("OnDestroySessionCompleted: %s"), Successful ? TEXT("succes") : TEXT("echec"));
}

// ============================================================
//  Update de setting custom
// ============================================================

template<typename ValueType>
void UOnlineSessionSubsystem::UpdateCustomSetting(const FName& KeyName, const ValueType& Value,
	EOnlineDataAdvertisementType::Type InType)
{
	if (!Session.IsValid() || !LastSessionSettings.IsValid())
		return;

	TSharedPtr<FOnlineSessionSettings> UpdatedSettings = MakeShareable(new FOnlineSessionSettings(*LastSessionSettings));
	UpdatedSettings->Set(KeyName, Value, InType);

	UpdateHandle = Session->AddOnUpdateSessionCompleteDelegate_Handle(
		FOnUpdateSessionCompleteDelegate::CreateUObject(this, &UOnlineSessionSubsystem::OnUpdateSessionCompleted)
	);

	if (!Session->UpdateSession(NAME_GameSession, *UpdatedSettings))
	{
		Session->ClearOnUpdateSessionCompleteDelegate_Handle(UpdateHandle);
		return;
	}

	LastSessionSettings = UpdatedSettings;
}

void UOnlineSessionSubsystem::OnUpdateSessionCompleted(FName SessionName, bool Successful)
{
	if (Session)
		Session->ClearOnUpdateSessionCompleteDelegate_Handle(UpdateHandle);
}

// ============================================================
//  Relais lobby
// ============================================================

void UOnlineSessionSubsystem::HandleLobbyUpdated_Internal(const TArray<FPlayerLobbyInfo>& Players)
{
	OnLobbysUpdated.Broadcast(Players);
}