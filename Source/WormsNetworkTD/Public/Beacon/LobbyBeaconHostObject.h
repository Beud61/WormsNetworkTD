#pragma once

#include "CoreMinimal.h"
#include "OnlineBeaconHostObject.h"
#include "Beacon/LobbyTypes.h"
#include "LobbyBeaconHostObject.generated.h"

// Forward declaration (évite d'inclure le .h complet ici)
class ALobbyBeaconClient;

UCLASS()
class WORMSNETWORKTD_API ALobbyBeaconHostObject : public AOnlineBeaconHostObject
{
	GENERATED_BODY()

public:
	ALobbyBeaconHostObject(const FObjectInitializer& Initializer);

	// ----- Surcharges AOnlineBeaconHostObject -----
	virtual void OnClientConnected(AOnlineBeaconClient* NewClientActor, UNetConnection* ClientConnection) override;
	virtual void NotifyClientDisconnected(AOnlineBeaconClient* LeavingClientActor) override;
	virtual AOnlineBeaconClient* SpawnBeaconActor(UNetConnection* ClientConnection) override;

	// ----- État du lobby -----

	/** Nombre de slots déjà réservés (hôte inclus). */
	UPROPERTY()
	int32 ReservedSlots = 0;

	/** Capacité maximale de la room (définie par le GameMode). */
	UPROPERTY()
	int32 MaxSlots = 4;

	/** Nombre d'unités imposé par l'hôte à tous les joueurs. */
	UPROPERTY()
	int32 RoomUnitCount = 1;

	/** Liste des joueurs connectés (état de référence côté serveur). */
	UPROPERTY()
	TArray<FPlayerLobbyInfo> ConnectedPlayers;

	// ----- Interface publique -----

	/**
	 * Ajoute ou met à jour un joueur dans ConnectedPlayers,
	 * puis diffuse la mise à jour à tous les clients.
	 */
	void RegisterOrUpdatePlayer(const FPlayerLobbyInfo& PlayerInfo);

	/**
	 * Retire un joueur de ConnectedPlayers par son PlayerId,
	 * libère un slot et diffuse la mise à jour.
	 */
	void UnregisterPlayer(int32 PlayerId);

private:
	/** Liste des clients beacon actuellement connectés. */
	UPROPERTY()
	TArray<ALobbyBeaconClient*> ConnectedClients;

	/** Diffuse ConnectedPlayers à tous les clients connectés. */
	void BroadcastLobbyUpdate();
};