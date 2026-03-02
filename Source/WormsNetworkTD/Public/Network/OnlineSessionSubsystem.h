#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Beacon/LobbyTypes.h"
#include "OnlineSessionSubsystem.generated.h"

class ALobbyBeaconClient;
class AOnlineBeaconHost;

// ============================================================
//  Struct session exposée à l'UI
// ============================================================
USTRUCT(BlueprintType)
struct FCustomSessionInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly) FString SessionName = TEXT("");
	UPROPERTY(BlueprintReadOnly) int32   CurrentPlayers = 0;
	UPROPERTY(BlueprintReadOnly) int32   MaxPlayers = 0;
	UPROPERTY(BlueprintReadOnly) int32   Ping = 0;
	UPROPERTY(BlueprintReadOnly) int32   SessionSearchResultIndex = 0;
	UPROPERTY(BlueprintReadOnly) FString GameMode = TEXT("");
	UPROPERTY(BlueprintReadOnly) FString HostIP = TEXT("");  // <- NOUVEAU
};

// ============================================================
//  Delegates
// ============================================================
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFindGameSessionsComplete,
	const TArray<FCustomSessionInfo>&, SessionResults, bool, Successful);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionJoinCompleted,
	bool, bWasSuccessful);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBeaconClientCreated,
	ALobbyBeaconClient*, BeaconClient);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLobbysUpdated,
	const TArray<FPlayerLobbyInfo>&, Players);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHostBeaconReady);

// ============================================================
//  Subsystem
// ============================================================
UCLASS()
class WORMSNETWORKTD_API UOnlineSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	// ============================================================
	//  API publique
	// ============================================================
	bool bIsHost = false;

	UFUNCTION(BlueprintCallable, Category = "Session")
	void CreateSession(const FString& SessionName, int32 NumPublicConnections, bool bIsLanMatch,
		const FString& GameMode, int32 UnitLife, int32 UnitCount, int32 TurnsBeforeWater);

	UFUNCTION(BlueprintCallable, Category = "Session")
	void FindSessions(int32 MaxSearchResults, bool bIsLANQuery);

	/**
	 * Rejoint une session via Beacon.
	 * Résout l'IP depuis les settings de session (Key_HostIP) pour
	 * fonctionner correctement avec le NULL OSS sur vrai réseau LAN.
	 */
	UFUNCTION(BlueprintCallable, Category = "Session")
	void JoinLobby(const FCustomSessionInfo& SessionInfo);

	UFUNCTION(BlueprintCallable, Category = "Session")
	void DestroySession();

	/** Lance la partie (ServerTravel) — appelé uniquement par l'hôte. */
	UFUNCTION(BlueprintCallable, Category = "Session")
	void StartGame();

	/** Déconnecte le client du beacon et quitte le lobby proprement. */
	UFUNCTION(BlueprintCallable, Category = "Session")
	void LeaveBeaconLobby();

	/** Mise à jour d'un setting custom (template — défini dans le .h pour éviter les erreurs de linker). */
	template<typename ValueType>
	void UpdateCustomSetting(const FName& KeyName, const ValueType& Value,
		EOnlineDataAdvertisementType::Type InType)
	{
		if (!Session.IsValid() || !LastSessionSettings.IsValid())
			return;

		TSharedPtr<FOnlineSessionSettings> UpdatedSettings =
			MakeShareable(new FOnlineSessionSettings(*LastSessionSettings));
		UpdatedSettings->Set(KeyName, Value, InType);

		UpdateHandle = Session->AddOnUpdateSessionCompleteDelegate_Handle(
			FOnUpdateSessionCompleteDelegate::CreateUObject(
				this, &UOnlineSessionSubsystem::OnUpdateSessionCompleted));

		if (!Session->UpdateSession(NAME_GameSession, *UpdatedSettings))
		{
			Session->ClearOnUpdateSessionCompleteDelegate_Handle(UpdateHandle);
			return;
		}

		// Mis à jour uniquement si UpdateSession accepte la requête
		LastSessionSettings = UpdatedSettings;
	}

	void SetHostPlayerInfo(const FPlayerLobbyInfo& Info) { PendingHostPlayerInfo = Info; }
	ALobbyBeaconClient* GetLobbyBeaconClient() const { return LobbyBeaconClient; }

	// ============================================================
	//  Delegates publics
	// ============================================================

	UPROPERTY(BlueprintAssignable) FOnFindGameSessionsComplete OnFindSessionsCompleteEvent;
	UPROPERTY(BlueprintAssignable) FOnSessionJoinCompleted     OnSessionJoinCompleted;
	UPROPERTY(BlueprintAssignable) FOnBeaconClientCreated      OnBeaconClientCreated;
	UPROPERTY(BlueprintAssignable) FOnLobbysUpdated            OnLobbysUpdated;
	UPROPERTY(BlueprintAssignable) FOnHostBeaconReady          OnHostBeaconReady;

	// ============================================================
	//  État
	// ============================================================
	IOnlineSessionPtr                  Session;
	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;
	TArray<FOnlineSessionSearchResult> SearchResults;
	int32                              MaxPlayers = 0;

private:
	// ----- Beacon -----
	UPROPERTY() AOnlineBeaconHost* BeaconHost = nullptr;
	UPROPERTY() ALobbyBeaconClient* LobbyBeaconClient = nullptr;
	bool                              bBeaconConnecting = false;

	// ----- Delegates handles -----
	FDelegateHandle CreateHandle;
	FDelegateHandle FindHandle;
	FDelegateHandle DestroyHandle;
	FDelegateHandle UpdateHandle;

	// ----- État interne -----
	FPlayerLobbyInfo                   PendingHostPlayerInfo;
	TSharedPtr<FOnlineSessionSearch>   LastSessionSearch;

	// ----- Callbacks internes -----
	void OnCreateSessionCompleted(FName SessionName, bool Successful);
	void OnFindSessionsCompleted(bool Successful);
	void OnDestroySessionCompleted(FName SessionName, bool Successful);
	void OnUpdateSessionCompleted(FName SessionName, bool Successful);

	void CreateHostBeacon();
	void ConnectAsBeaconClient(const FString& HostIP, const FPlayerLobbyInfo& PlayerInfo);
	void CleanupBeaconClient();

	UFUNCTION()
	void HandleLobbyUpdated_Internal(const TArray<FPlayerLobbyInfo>& Players);
};