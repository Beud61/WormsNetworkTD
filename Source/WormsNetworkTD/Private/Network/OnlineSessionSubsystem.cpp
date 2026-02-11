#include "Network/OnlineSessionSubsystem.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystemUtils.h"
#include "Beacon/LobbyBeaconHostObject.h"
#include "OnlineBeaconHost.h"
#include "Beacon/LobbyBeaconClient.h"

void UOnlineSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Session = Online::GetSessionInterface(GetWorld());
}

void UOnlineSessionSubsystem::CreateSession(const FString& SessionName, int32 NumPublicConnections, bool isLanMatch)
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
	LastSessionSettings->Set("SETTING_SESSIONSTATE", 0, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	CreateHandle = Session->AddOnCreateSessionCompleteDelegate_Handle
	(FOnCreateSessionCompleteDelegate::CreateUObject(this, &UOnlineSessionSubsystem::OnCreateSessionCompleted));

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();

	if (!Session->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *LastSessionSettings))
	{
		Session->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
		return;
	}
}

void UOnlineSessionSubsystem::OnCreateSessionCompleted(FName SessionName, bool Successful)
{
	if (Session)
		Session->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
	if (!Successful)
		return;
	GetWorld()->ServerTravel("/Game/Maps/Lobby?listen");
}

void UOnlineSessionSubsystem::FindSessions(int32 MaxSearchResults, bool IsLANQuery)
{
	if (!Session.IsValid())
		return;
	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	LastSessionSearch->MaxSearchResults = MaxSearchResults;
	LastSessionSearch->bIsLanQuery = IsLANQuery;
	LastSessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	FindHandle = Session->AddOnFindSessionsCompleteDelegate_Handle
	(FOnFindSessionsCompleteDelegate::CreateUObject(this, &UOnlineSessionSubsystem::OnFindSessionsCompleted));
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!Session->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef()))
	{
		Session->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
		return;
	}
}

void UOnlineSessionSubsystem::OnFindSessionsCompleted(bool Successful)
{
	if (Session)
		Session->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
	SearchResults = LastSessionSearch->SearchResults;
	if (SearchResults.IsEmpty())
	{
		OnFindSessionsCompleteEvent.Broadcast(TArray<FCustomSessionInfo>(), Successful);
		return;
	}
	TArray<FCustomSessionInfo> SessionInfos;
	for (int i = 0; i < SearchResults.Num(); i++)
	{
		FOnlineSessionSearchResult Result = SearchResults[i];
		FCustomSessionInfo SessionInfo;
		FString SessionName;
		Result.Session.SessionSettings.Get("SETTING_SESSIONNAME", SessionName);
		SessionInfo.SessionName = SessionName;
		SessionInfo.CurrentPlayers = Result.Session.SessionSettings.NumPublicConnections - Result.Session.NumOpenPublicConnections;
		SessionInfo.MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
		SessionInfo.Ping = Result.PingInMs;
		SessionInfo.SessionSearchResultIndex = i;
		SessionInfos.Add(SessionInfo);
	}
	OnFindSessionsCompleteEvent.Broadcast(SessionInfos, Successful);
}

void UOnlineSessionSubsystem::JoinGameSession(const FOnlineSessionSearchResult& SessionResult)
{
	if (!Session.IsValid())
		return;
	JoinHandle = Session->AddOnJoinSessionCompleteDelegate_Handle
	(FOnJoinSessionCompleteDelegate::CreateUObject(this, &UOnlineSessionSubsystem::OnJoinSessionCompleted));
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!Session->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SessionResult))
	{
		Session->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
		return;
	}
}

void UOnlineSessionSubsystem::OnJoinSessionCompleted(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	FString ConnectString;
	if (!Session)
		return;
	Session->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
	if (Result != EOnJoinSessionCompleteResult::Success || !Session->GetResolvedConnectString(NAME_GameSession, ConnectString))
	{
		Session->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
		return;
	}
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	PlayerController->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
}

void UOnlineSessionSubsystem::CustomJoinSession(const FCustomSessionInfo& SessionInfo, int32 BeaconPOrt, bool bPortOverride)
{
	const FOnlineSessionSearchResult TempResult = SearchResults[SessionInfo.SessionSearchResultIndex];
	FString ConnectString;
	ALobbyBeaconClient* BeaconClient = GetWorld()->SpawnActor<ALobbyBeaconClient>();
	FURL Destination = FURL(nullptr, *ConnectString, ETravelType::TRAVEL_Absolute);
	Destination.Port = 7787;
	UE_LOG(LogTemp, Warning, TEXT("TRYING TO CONNECT TO : %s:%d"), *Destination.Host, Destination.Port);
	BeaconClient->ConnectToServer(Destination);
	BeaconClient->OnRequestValidate.BindLambda([this, TempResult](bool bValidated)
		{
			if (bValidated)
			{
				JoinGameSession(TempResult);
			}
			else
			{
				OnSessionJoinCompleted.Broadcast(false);
			}
		});
}

void UOnlineSessionSubsystem::DestroySession()
{
	if (!Session.IsValid())
		return;
	DestroyHandle = Session->AddOnDestroySessionCompleteDelegate_Handle
	(FOnDestroySessionCompleteDelegate::CreateUObject(this, &UOnlineSessionSubsystem::OnDestroySessionCompleted));
	if (!Session->DestroySession(NAME_GameSession))
	{
		Session->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
		return;
	}
}

void UOnlineSessionSubsystem::OnDestroySessionCompleted(FName SessionName, bool Successful)
{
	if (Session)
		Session->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
}

template<typename ValueType>
void UOnlineSessionSubsystem::UpdateCustomSetting(const FName& KeyName, const ValueType& Value, EOnlineDataAdvertisementType::Type InType)
{
	if (!Session.IsValid())
		return;
	if (!LastSessionSettings.IsValid())
		return;
	TSharedPtr<FOnlineSessionSettings> UpdatedSessionSettings = MakeShareable(new FOnlineSessionSettings(*LastSessionSettings));
	UpdatedSessionSettings->Set(KeyName, Value, InType);
	UpdateHandle = Session->AddOnUpdateSessionCompleteDelegate_Handle(
		FOnUpdateSessionCompleteDelegate::CreateUObject(this, &UOnlineSessionSubsystem::OnUpdateSessionCompleted));
	if (!Session->UpdateSession(NAME_GameSession, *UpdatedSessionSettings))
	{
		Session->ClearOnUpdateSessionCompleteDelegate_Handle(UpdateHandle);
		return;
	}
	LastSessionSettings = UpdatedSessionSettings;
}

void UOnlineSessionSubsystem::OnUpdateSessionCompleted(FName SessionName, bool Successful)
{
	if (Session)
		Session->ClearOnUpdateSessionCompleteDelegate_Handle(UpdateHandle);
}

void UOnlineSessionSubsystem::CreateHostBeacon(int32 ListenPort, bool bOverridePort)
{
	AOnlineBeaconHost* BeaconHost = GetWorld()->SpawnActor<AOnlineBeaconHost>();
	if (BeaconHost->InitHost())
	{
		BeaconHost->PauseBeaconRequests(false);
		if (ALobbyBeaconHostObject* HostObject = GetWorld()->SpawnActor<ALobbyBeaconHostObject>())
		{
			HostObject->ReservedSlots++;
			HostObject->MaxSlots = MaxPlayers;
			BeaconHost->RegisterHost(HostObject);
			UE_LOG(LogTemp, Warning, TEXT("Host created, port listening : %d"), BeaconHost->ListenPort);
		}
	}
}