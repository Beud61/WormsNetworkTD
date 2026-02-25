#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Beacon/LobbyTypes.h"
#include "OnlineSessionSubsystem.generated.h"

// Forward declaration pour éviter l'inclusion circulaire
class ALobbyBeaconClient;
class AOnlineBeaconHost;

// ============================================================
//  Struct de session custom exposée à l'UI
// ============================================================
USTRUCT(BlueprintType)
struct FCustomSessionInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString SessionName = TEXT("");

	UPROPERTY(BlueprintReadOnly)
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 MaxPlayers = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 Ping = 0;

	// Index dans le tableau SearchResults — utilisé pour rejoindre la session
	UPROPERTY(BlueprintReadOnly)
	int32 SessionSearchResultIndex = 0;

	UPROPERTY(BlueprintReadOnly)
	FString GameMode = TEXT("");
};

// ============================================================
//  Delegates
// ============================================================
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFindGameSessionsComplete,
	const TArray<FCustomSessionInfo>&, SessionResults,
	bool, Successful);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionJoinCompleted,
	bool, bWasSuccessful);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBeaconClientCreated,
	ALobbyBeaconClient*, BeaconClient);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLobbysUpdated,
	const TArray<FPlayerLobbyInfo>&, Players);

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
	// ----- Interface de session -----
	IOnlineSessionPtr Session;
	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;

	// ----- Création de session -----
	FDelegateHandle CreateHandle;
	void OnCreateSessionCompleted(FName SessionName, bool Successful);

	// ----- Recherche de session -----
	FDelegateHandle FindHandle;
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;
	TArray<FOnlineSessionSearchResult> SearchResults;
	void OnFindSessionsCompleted(bool Successful);

	// ----- Join session (classique, voyage réseau) -----
	void JoinGameSession(const FOnlineSessionSearchResult& SessionResult);
	FDelegateHandle JoinHandle;
	void OnJoinSessionCompleted(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	// ----- Destroy session -----
	FDelegateHandle DestroyHandle;
	void OnDestroySessionCompleted(FName SessionName, bool Successful);

	// ----- Update session -----
	FDelegateHandle UpdateHandle;
	void OnUpdateSessionCompleted(FName SessionName, bool Successful);

public:
	// ----- API publique -----

	UFUNCTION(BlueprintCallable, Category = "Session")
	void CreateSession(const FString& SessionName, int32 NumPublicConnections, bool bIsLanMatch,
		const FString& GameMode, int32 UnitLife, int32 UnitCount, int32 TurnsBeforeWater);

	UFUNCTION(BlueprintCallable, Category = "Session")
	void FindSessions(int32 MaxSearchResults, bool bIsLANQuery);

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FOnFindGameSessionsComplete OnFindSessionsCompleteEvent;

	/**
	 * Rejoint une session via Beacon (pas de ServerTravel immédiat).
	 * Le beacon sert à valider la réservation et synchroniser le lobby.
	 */
	UFUNCTION(BlueprintCallable, Category = "Session")
	void CustomJoinSession(const FCustomSessionInfo& SessionInfo);

	UFUNCTION(BlueprintCallable, Category = "Session")
	void DestroySession();

	/** Crée/met à jour un setting custom dans la session en cours. */
	template<typename ValueType>
	void UpdateCustomSetting(const FName& KeyName, const ValueType& Value,
		EOnlineDataAdvertisementType::Type InType);

	/** Spawn le beacon host côté serveur. Idempotent (ne respawne pas si déjà actif). */
	UFUNCTION(BlueprintCallable)
	void CreateHostBeacon();

	/**
	 * Connecte l'hôte à son propre beacon en tant que client.
	 * Appelé automatiquement après CreateHostBeacon() pour que l'hôte
	 * envoie ses propres FPlayerLobbyInfo et apparaisse dans la liste du lobby.
	 * @param HostInfo  Informations du joueur hôte à enregistrer.
	 */
	void ConnectHostAsClient(const FPlayerLobbyInfo& HostInfo);

	// ----- Delegates publics -----

	UPROPERTY(BlueprintAssignable)
	FOnSessionJoinCompleted OnSessionJoinCompleted;

	UPROPERTY(BlueprintAssignable)
	FOnBeaconClientCreated OnBeaconClientCreated;

	UPROPERTY(BlueprintAssignable)
	FOnLobbysUpdated OnLobbysUpdated;

	// ----- Accesseurs beacon client -----

	UFUNCTION()
	ALobbyBeaconClient* GetLobbyBeaconClient() const { return LobbyBeaconClient; }

	// ----- État interne -----

	int32 MaxPlayers = 0;

private:
	/** Beacon host (côté serveur uniquement). */
	UPROPERTY()
	AOnlineBeaconHost* BeaconHost = nullptr;

	/** Beacon client (côté joueur rejoignant). */
	UPROPERTY()
	ALobbyBeaconClient* LobbyBeaconClient = nullptr;

	/** Évite les doubles connexions beacon. */
	bool bBeaconConnecting = false;

	/**
	 * Informations du joueur hôte, stockées entre CreateSession() et
	 * la connexion beacon. Doit être rempli par l'UI via SetHostPlayerInfo()
	 * AVANT d'appeler CreateSession().
	 */
	FPlayerLobbyInfo PendingHostPlayerInfo;

	/** Relais interne : propage les mises à jour lobby au delegate public. */
	UFUNCTION()
	void HandleLobbyUpdated_Internal(const TArray<FPlayerLobbyInfo>& Players);

	/** Nettoyage du beacon client (disconnect + destroy). */
	void CleanupBeaconClient();

public:
	/**
	 * Doit être appelé par l'UI avant CreateSession() pour que l'hôte
	 * apparaisse correctement dans la liste du lobby.
	 */
	void SetHostPlayerInfo(const FPlayerLobbyInfo& Info) { PendingHostPlayerInfo = Info; }
};