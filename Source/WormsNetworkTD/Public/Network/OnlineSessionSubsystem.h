#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FCustomSessionInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString SessionName = "";
	UPROPERTY(BlueprintReadOnly)
	int32 CurrentPlayers = 0;
	UPROPERTY(BlueprintReadOnly)
	int32 MaxPlayers = 0;
	UPROPERTY(BlueprintReadOnly)
	int32 Ping = 0;
	UPROPERTY(BlueprintReadOnly)
	int32 SessionSearchResultIndex = 0;
	UPROPERTY(BlueprintReadOnly)
	FString GameMode = "";
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFindGameSessionsComplete, const TArray<FCustomSessionInfo>&, SessionResults, bool, Successful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionJoinCompleted, bool, bWasSuccessful);

UCLASS()
class WORMSNETWORKTD_API UOnlineSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

public:
	IOnlineSessionPtr Session;
	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;

	FDelegateHandle CreateHandle;
	void OnCreateSessionCompleted(FName SessionName, bool Successful);

	FDelegateHandle FindHandle;
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;
	TArray<FOnlineSessionSearchResult> SearchResults;
	void OnFindSessionsCompleted(bool Successful);

	void JoinGameSession(const FOnlineSessionSearchResult& SessionResult);
	FDelegateHandle JoinHandle;
	void OnJoinSessionCompleted(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	FDelegateHandle DestroyHandle;
	void OnDestroySessionCompleted(FName SessionName, bool Successful);

	FDelegateHandle UpdateHandle;
	void OnUpdateSessionCompleted(FName SessionName, bool Successful);

public:
	UFUNCTION(BlueprintCallable, Category = "Session")
	void CreateSession(const FString& SessionName, int32 NumPublicConnections, bool isLanMatch, const FString& GameMode, int32 UnitLife, int32 UnitCount, int32 TurnsBeforeWater);
	UFUNCTION(BlueprintCallable, Category = "Session")
	void FindSessions(int32 MaxSearchResults, bool IsLANQuery);
	UPROPERTY(BlueprintAssignable, Category = "Session")
	FOnFindGameSessionsComplete OnFindSessionsCompleteEvent;
	UFUNCTION(BlueprintCallable, Category = "Session")
	void CustomJoinSession(const FCustomSessionInfo& SessionInfo, int32 BeaconPOrt, bool bPortOverride);
	UFUNCTION(BlueprintCallable, Category = "Session")
	void DestroySession();
	int32 MaxPlayers = 0;

	template<typename ValueType>
	void UpdateCustomSetting(const FName& KeyName, const ValueType& Value, EOnlineDataAdvertisementType::Type InType);

	UFUNCTION(BlueprintCallable)
	void CreateHostBeacon(int32 ListenPort, bool bOverridePort);

	UPROPERTY(BlueprintCallable)
	FOnSessionJoinCompleted OnSessionJoinCompleted;
};