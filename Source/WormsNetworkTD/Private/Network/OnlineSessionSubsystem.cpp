#include "Network/OnlineSessionSubsystem.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystemUtils.h"
#include "Beacon/LobbyBeaconHostObject.h"
#include "OnlineBeaconHost.h"
#include "Beacon/LobbyBeaconClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"

void UOnlineSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Session = Online::GetSessionInterface(GetWorld());
}

// ---- SESSION CREATION ----
void UOnlineSessionSubsystem::CreateSession(const FString& SessionName, int32 NumPublicConnections, bool isLanMatch, const FString& GameMode, int32 UnitLife, int32 UnitCount, int32 TurnsBeforeWater)
{
	if (!Session.IsValid())
		return;

	MaxPlayers = NumPublicConnections;

	LastSessionSettings = MakeShareable(new FOnlineSessionSettings());
	LastSessionSettings->NumPublicConnections = NumPublicConnections;
	LastSessionSettings->bAllowJoinInProgress = true;
	LastSessionSettings->bAllowJoinViaPresence = true;
	LastSessionSettings->bIsDedicated = false;
	LastSessionSettings->bUsesPresence = true;
	LastSessionSettings->bIsLANMatch = isLanMatch;
	LastSessionSettings->bShouldAdvertise = true;

	LastSessionSettings->Set("SETTING_SESSIONNAME", SessionName, EOnlineDataAdvertisementType::ViaOnlineService);
	LastSessionSettings->Set("GAME_MODE", GameMode, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	LastSessionSettings->Set("UNIT_LIFE", UnitLife, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	LastSessionSettings->Set("UNIT_COUNT", UnitCount, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	LastSessionSettings->Set("TURNS_BEFORE_WATER", TurnsBeforeWater, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	CreateHandle = Session->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UOnlineSessionSubsystem::OnCreateSessionCompleted)
	);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!Session->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *LastSessionSettings))
	{
		Session->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
	}
}

void UOnlineSessionSubsystem::OnCreateSessionCompleted(FName SessionName, bool Successful)
{
	Session->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
	if (!Successful)
		return;

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (LocalPlayer && Session.IsValid())
	{
		Session->RegisterPlayer(NAME_GameSession, *LocalPlayer->GetPreferredUniqueNetId(), false);
	}

	// Session ready -> spawn Beacon host
	CreateHostBeacon(7787, true);
}

// ---- SESSION SEARCH ----
void UOnlineSessionSubsystem::FindSessions(int32 MaxSearchResults, bool IsLANQuery)
{
	if (!Session.IsValid())
		return;

	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	LastSessionSearch->MaxSearchResults = MaxSearchResults;
	LastSessionSearch->bIsLanQuery = IsLANQuery;
	LastSessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);

	FindHandle = Session->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UOnlineSessionSubsystem::OnFindSessionsCompleted)
	);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!Session->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef()))
	{
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
		FCustomSessionInfo SessionInfo;
		Result.Session.SessionSettings.Get("SETTING_SESSIONNAME", SessionInfo.SessionName);
		Result.Session.SessionSettings.Get("GAME_MODE", SessionInfo.GameMode);
		SessionInfo.CurrentPlayers = Result.Session.SessionSettings.NumPublicConnections - Result.Session.NumOpenPublicConnections;
		SessionInfo.MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
		SessionInfo.Ping = Result.PingInMs;
		SessionInfo.SessionSearchResultIndex = i;
		SessionInfos.Add(SessionInfo);
	}

	OnFindSessionsCompleteEvent.Broadcast(SessionInfos, Successful);
	UE_LOG(LogTemp, Warning, TEXT("Find sessions completed with %d results, success : %d"), SearchResults.Num(), Successful);
}

// ---- JOIN SESSION ----
void UOnlineSessionSubsystem::JoinGameSession(const FOnlineSessionSearchResult& SessionResult)
{
	if (!Session.IsValid())
		return;

	JoinHandle = Session->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &UOnlineSessionSubsystem::OnJoinSessionCompleted)
	);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!Session->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SessionResult))
	{
		Session->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
	}
}

void UOnlineSessionSubsystem::OnJoinSessionCompleted(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	Session->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);

	FString ConnectString;
	if (Result != EOnJoinSessionCompleteResult::Success || !Session->GetResolvedConnectString(NAME_GameSession, ConnectString))
	{
		// Fallback PIE/Null OSS
		ConnectString = FString::Printf(TEXT("127.0.0.1:%d"), 7787);
	}
	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		UE_LOG(LogTemp, Warning, TEXT("JoinSession SUCCESS"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("JoinSession FAILED"));
	}

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController)
	{
		PlayerController->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
	}
}

// ---- CUSTOM JOIN SESSION (BEACON) ----
void UOnlineSessionSubsystem::CustomJoinSession(const FCustomSessionInfo& SessionInfo, int32 BeaconPort, bool bPortOverride)
{
	UE_LOG(LogTemp, Error, TEXT("CustomJoinSession CALLED"));
	if (!Session.IsValid() || !SearchResults.IsValidIndex(SessionInfo.SessionSearchResultIndex))
	{
		OnSessionJoinCompleted.Broadcast(false);
		return;
	}

	const FOnlineSessionSearchResult& TempResult = SearchResults[SessionInfo.SessionSearchResultIndex];

	FString ConnectString;
	if (!Session->GetResolvedConnectString(TempResult, NAME_GameSession, ConnectString))
	{
		// Fallback si Null OSS / PIE
		ConnectString = FString::Printf(TEXT("127.0.0.1:%d"), BeaconPort);
	}

	// Spawn Beacon client si pas déjà existant
	if (!LobbyBeaconClient)
	{
		LobbyBeaconClient = GetWorld()->SpawnActor<ALobbyBeaconClient>();
		LobbyBeaconClient->OnLobbyUpdated.AddDynamic(
			this,
			&UOnlineSessionSubsystem::HandleLobbyUpdated_Internal
		);
		UE_LOG(LogTemp, Warning, TEXT("BeaconClient CREATED %p"), LobbyBeaconClient);

		// Attendre que l'acteur soit pleinement initialisé
		LobbyBeaconClient->SetActorHiddenInGame(true);
		LobbyBeaconClient->SetActorEnableCollision(false);
		LobbyBeaconClient->SetReplicates(true);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Beacon already exists, skipping creation"));
	}

	// Connexion différée
	
	/*if (!LobbyBeaconClient)
	{
		UE_LOG(LogTemp, Error, TEXT("Impossible de creer le BeaconClient"));
		OnSessionJoinCompleted.Broadcast(false);
		return;
	}*/
	//OnBeaconClientCreated.Broadcast(LobbyBeaconClient);

	if (bBeaconConnecting)
	{
		UE_LOG(LogTemp, Warning, TEXT("Already connecting to beacon"));
		return;
	}
	bBeaconConnecting = true;
	// Destination URL
	FURL Destination(nullptr, TEXT("127.0.0.1"), TRAVEL_Absolute);
	Destination.Port = BeaconPort;
	UE_LOG(LogTemp, Warning, TEXT("TRYING TO CONNECT TO : %s:%d"),
		*Destination.Host, Destination.Port);

	UE_LOG(LogTemp, Warning, TEXT("TRYING TO CONNECT TO : %s:%d"), *Destination.Host, Destination.Port);
	LobbyBeaconClient->ConnectToServer(Destination);
	// Callback validation Beacon
	LobbyBeaconClient->OnRequestValidate.BindLambda(
		[this, TempResult](bool bValidated)
		{
			if (bValidated)
			{
				UE_LOG(LogTemp, Warning, TEXT("Beacon validated"));
				//JoinGameSession(TempResult);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Validation du beacon echouee"));
				OnSessionJoinCompleted.Broadcast(false);
			}
		}
	);

	/*FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this, &Destination]()
		{
			if (LobbyBeaconClient && !bBeaconConnecting)
			{
				bBeaconConnecting = true;
				LobbyBeaconClient->ConnectToServer(Destination);
			}
		}, 0.1f, false);*/
}

// ---- BEACON HOST ----
void UOnlineSessionSubsystem::CreateHostBeacon(int32 ListenPort, bool bOverridePort)
{
	AOnlineBeaconHost* BeaconHost = GetWorld()->SpawnActor<AOnlineBeaconHost>();
	if (BeaconHost->InitHost())
	{
		BeaconHost->PauseBeaconRequests(false);

		ALobbyBeaconHostObject* HostObject = GetWorld()->SpawnActor<ALobbyBeaconHostObject>();
		if (HostObject)
		{
			HostObject->ReservedSlots++;
			HostObject->MaxSlots = MaxPlayers;
			BeaconHost->RegisterHost(HostObject);
			UE_LOG(LogTemp, Warning, TEXT("Host created, port listening : %d"), BeaconHost->ListenPort);
			UE_LOG(LogTemp, Warning, TEXT("BeaconHost NetDriver: %s"),
				BeaconHost->GetNetDriver() ? TEXT("VALID") : TEXT("NULL"));
			int32 UnitCount = 1;
			LastSessionSettings->Get("UNIT_COUNT", UnitCount);
			HostObject->RoomUnitCount = UnitCount;
		}
	}
}

// ---- SESSION DESTROY ----
void UOnlineSessionSubsystem::DestroySession()
{
	if (!Session.IsValid())
		return;

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
}

// ---- CUSTOM SETTING ----
template<typename ValueType>
void UOnlineSessionSubsystem::UpdateCustomSetting(const FName& KeyName, const ValueType& Value, EOnlineDataAdvertisementType::Type InType)
{
	if (!Session.IsValid() || !LastSessionSettings.IsValid())
		return;

	TSharedPtr<FOnlineSessionSettings> UpdatedSessionSettings = MakeShareable(new FOnlineSessionSettings(*LastSessionSettings));
	UpdatedSessionSettings->Set(KeyName, Value, InType);

	UpdateHandle = Session->AddOnUpdateSessionCompleteDelegate_Handle(
		FOnUpdateSessionCompleteDelegate::CreateUObject(this, &UOnlineSessionSubsystem::OnUpdateSessionCompleted)
	);

	if (!Session->UpdateSession(NAME_GameSession, *UpdatedSessionSettings))
	{
		Session->ClearOnUpdateSessionCompleteDelegate_Handle(UpdateHandle);
	}

	LastSessionSettings = UpdatedSessionSettings;
}

void UOnlineSessionSubsystem::OnUpdateSessionCompleted(FName SessionName, bool Successful)
{
	if (Session)
		Session->ClearOnUpdateSessionCompleteDelegate_Handle(UpdateHandle);
}
