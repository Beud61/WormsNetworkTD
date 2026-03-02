#include "Network/OnlineSessionSubsystem.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystemUtils.h"
#include "Beacon/LobbyBeaconHostObject.h"
#include "Beacon/LobbyBeaconClient.h"
#include "Beacon/LobbyTypes.h"
#include "OnlineBeaconHost.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "SocketSubsystem.h"

// ============================================================
//  Initialisation
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

// ============================================================
//  Création de session (côté hôte)
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

	LastSessionSettings->Set(LobbyConstants::Key_SessionName, SessionName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	LastSessionSettings->Set(LobbyConstants::Key_GameMode, GameMode, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	LastSessionSettings->Set(LobbyConstants::Key_UnitLife, UnitLife, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	LastSessionSettings->Set(LobbyConstants::Key_UnitCount, UnitCount, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	LastSessionSettings->Set(LobbyConstants::Key_TurnsBeforeWater, TurnsBeforeWater, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	// ── Clé HOST_IP ──────────────────────────────────────────────────────────
	// On stocke l'IP locale de l'hôte dans les settings de session pour que
	// les clients puissent la lire directement depuis le SearchResult, sans
	// dépendre de GetResolvedConnectString (qui échoue avec le NULL OSS LAN).
	// On récupère l'IP locale via ISocketSubsystem.
	FString HostIP = TEXT("127.0.0.1");
	bool    bCanBindAll = false;
	TSharedPtr<FInternetAddr> LocalAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalHostAddr(*GLog, bCanBindAll);
	if (LocalAddr.IsValid())
	{
		HostIP = LocalAddr->ToString(false); // false = sans le port
		UE_LOG(LogTemp, Warning, TEXT("CreateSession: IP locale detectee = %s"), *HostIP);
	}
	LastSessionSettings->Set(LobbyConstants::Key_HostIP, HostIP, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	// ─────────────────────────────────────────────────────────────────────────

	CreateHandle = Session->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UOnlineSessionSubsystem::OnCreateSessionCompleted));
	bIsHost = true;
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!Session->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *LastSessionSettings))
	{
		UE_LOG(LogTemp, Error, TEXT("CreateSession: appel CreateSession() echoue."));
		Session->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
	}
}

void UOnlineSessionSubsystem::OnCreateSessionCompleted(FName SessionName, bool Successful)
{
	Session->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);

	if (!Successful)
	{
		UE_LOG(LogTemp, Error, TEXT("OnCreateSessionCompleted: echec."));
		return;
	}

	// Enregistre le joueur local dans la session OSS
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (LocalPlayer && Session.IsValid())
		Session->RegisterPlayer(NAME_GameSession, *LocalPlayer->GetPreferredUniqueNetId(), false);

	// Démarre le beacon host et connecte l'hôte comme premier client
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
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UOnlineSessionSubsystem::OnFindSessionsCompleted));

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!Session->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef()))
	{
		UE_LOG(LogTemp, Error, TEXT("FindSessions: appel FindSessions() echoue."));
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
		Result.Session.SessionSettings.Get(LobbyConstants::Key_HostIP, Info.HostIP);   // <- lit l'IP stockée

		Info.CurrentPlayers = Result.Session.SessionSettings.NumPublicConnections - Result.Session.NumOpenPublicConnections;
		Info.MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
		Info.Ping = Result.PingInMs;
		Info.SessionSearchResultIndex = i;

		SessionInfos.Add(Info);
	}

	UE_LOG(LogTemp, Warning, TEXT("FindSessions: %d resultat(s), succes=%d"), SearchResults.Num(), Successful);
	OnFindSessionsCompleteEvent.Broadcast(SessionInfos, Successful);
}

// ============================================================
//  Rejoindre un lobby via Beacon (côté client)
// ============================================================

void UOnlineSessionSubsystem::JoinLobby(const FCustomSessionInfo& SessionInfo)
{
	UE_LOG(LogTemp, Warning, TEXT("JoinLobby: tentative pour l'index %d, IP=%s"),
		SessionInfo.SessionSearchResultIndex, *SessionInfo.HostIP);

	if (bBeaconConnecting)
	{
		UE_LOG(LogTemp, Warning, TEXT("JoinLobby: connexion beacon deja en cours."));
		return;
	}

	// L'IP est lue directement depuis les settings de session (Key_HostIP),
	// pas besoin de GetResolvedConnectString → fonctionne avec le NULL OSS LAN.
	FString HostIP = SessionInfo.HostIP;
	if (HostIP.IsEmpty() || HostIP == TEXT("127.0.0.1"))
	{
		UE_LOG(LogTemp, Error, TEXT("JoinLobby: IP hote invalide ('%s'). Abandon."), *HostIP);
		OnSessionJoinCompleted.Broadcast(false);
		return;
	}

	// Infos joueur par défaut — l'UI les mettra à jour via SetHostPlayerInfo
	// ou directement via le beacon après connexion.
	FPlayerLobbyInfo ClientInfo;
	ClientInfo.PlayerName = TEXT("Player");
	ClientInfo.PlayerId = FMath::RandRange(1, INT32_MAX);

	ConnectAsBeaconClient(HostIP, ClientInfo);
}

// ============================================================
//  Lancer la partie (hôte uniquement)
// ============================================================

void UOnlineSessionSubsystem::StartGame()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC)
	{
		UE_LOG(LogTemp, Error, TEXT("StartGame: pas de PlayerController."));
		return;
	}

	// ServerTravel : emmène tous les clients connectés sur la map de jeu.
	// ?listen est requis pour que le serveur continue d'accepter les connexions.
	const FString TravelURL = LobbyConstants::GameMapPath + TEXT("?listen");
	UE_LOG(LogTemp, Warning, TEXT("StartGame: ServerTravel vers %s"), *TravelURL);

	GetWorld()->ServerTravel(TravelURL);
}

// ============================================================
//  Quitter le lobby (client)
// ============================================================

void UOnlineSessionSubsystem::LeaveBeaconLobby()
{
	UE_LOG(LogTemp, Warning, TEXT("LeaveBeaconLobby: deconnexion du lobby."));

	// TODO : envoyer un RPC au host pour libérer le slot avant de détruire le client.
	// Pour l'instant on détruit directement — le host détectera la déconnexion
	// via OnClientDisconnected dans LobbyBeaconHostObject.
	CleanupBeaconClient();
}

// ============================================================
//  Destroy session (hôte)
// ============================================================

void UOnlineSessionSubsystem::DestroySession()
{
	if (!Session.IsValid())
		return;

	// Nettoie le beacon client de l'hôte
	CleanupBeaconClient();

	// Détruit le beacon host -> déconnecte tous les clients
	if (BeaconHost)
	{
		BeaconHost->Destroy();
		BeaconHost = nullptr;
	}

	DestroyHandle = Session->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &UOnlineSessionSubsystem::OnDestroySessionCompleted));

	if (!Session->DestroySession(NAME_GameSession))
		Session->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);

	bIsHost = false;
}

void UOnlineSessionSubsystem::OnDestroySessionCompleted(FName SessionName, bool Successful)
{
	if (Session) Session->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
	UE_LOG(LogTemp, Warning, TEXT("OnDestroySessionCompleted: %s"), Successful ? TEXT("succes") : TEXT("echec"));
}

void UOnlineSessionSubsystem::OnUpdateSessionCompleted(FName SessionName, bool Successful)
{
	if (Session) Session->ClearOnUpdateSessionCompleteDelegate_Handle(UpdateHandle);
}

// ============================================================
//  Beacon Host (côté hôte)
// ============================================================

void UOnlineSessionSubsystem::CreateHostBeacon()
{
	if (BeaconHost)
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateHostBeacon: beacon host deja actif."));
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

	HostObject->ReservedSlots = 0;
	HostObject->MaxSlots = MaxPlayers;

	int32 UnitCount = 1;
	if (LastSessionSettings.IsValid())
		LastSessionSettings->Get(LobbyConstants::Key_UnitCount, UnitCount);
	HostObject->RoomUnitCount = UnitCount;

	BeaconHost->RegisterHost(HostObject);
	UE_LOG(LogTemp, Warning, TEXT("CreateHostBeacon: actif sur le port %d."), BeaconHost->ListenPort);

	// L'hôte se connecte à son propre beacon (127.0.0.1 est correct ici car c'est local)
	ConnectAsBeaconClient(TEXT("127.0.0.1"), PendingHostPlayerInfo);
}

// ============================================================
//  Connexion beacon client (hôte ET clients partagent ce chemin)
// ============================================================

void UOnlineSessionSubsystem::ConnectAsBeaconClient(const FString& HostIP, const FPlayerLobbyInfo& PlayerInfo)
{
	if (bBeaconConnecting)
	{
		UE_LOG(LogTemp, Warning, TEXT("ConnectAsBeaconClient: connexion deja en cours."));
		return;
	}

	CleanupBeaconClient();

	LobbyBeaconClient = GetWorld()->SpawnActor<ALobbyBeaconClient>();
	if (!LobbyBeaconClient)
	{
		UE_LOG(LogTemp, Error, TEXT("ConnectAsBeaconClient: impossible de spawner ALobbyBeaconClient."));
		OnSessionJoinCompleted.Broadcast(false);
		return;
	}

	LobbyBeaconClient->SetActorHiddenInGame(true);
	LobbyBeaconClient->SetActorEnableCollision(false);
	LobbyBeaconClient->SetReplicates(true);
	LobbyBeaconClient->PendingPlayerInfo = PlayerInfo;

	LobbyBeaconClient->OnLobbyUpdated.AddDynamic(this, &UOnlineSessionSubsystem::HandleLobbyUpdated_Internal);

	LobbyBeaconClient->OnRequestValidate.BindLambda(
		[this](bool bValidated)
		{
			bBeaconConnecting = false;
			if (bValidated)
			{
				UE_LOG(LogTemp, Warning, TEXT("ConnectAsBeaconClient: connexion validee."));
				OnBeaconClientCreated.Broadcast(LobbyBeaconClient);
				OnSessionJoinCompleted.Broadcast(true);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("ConnectAsBeaconClient: validation echouee."));
				CleanupBeaconClient();
				OnSessionJoinCompleted.Broadcast(false);
			}
		}
	);

	FURL Destination(nullptr, *HostIP, TRAVEL_Absolute);
	Destination.Port = LobbyConstants::BeaconPort;
	UE_LOG(LogTemp, Warning, TEXT("ConnectAsBeaconClient: connexion vers %s:%d"), *HostIP, Destination.Port);

	bBeaconConnecting = true;
	LobbyBeaconClient->ConnectToServer(Destination);
}

// ============================================================
//  Nettoyage beacon client
// ============================================================

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
//  Relais lobby
// ============================================================

void UOnlineSessionSubsystem::HandleLobbyUpdated_Internal(const TArray<FPlayerLobbyInfo>& Players)
{
	OnLobbysUpdated.Broadcast(Players);
}