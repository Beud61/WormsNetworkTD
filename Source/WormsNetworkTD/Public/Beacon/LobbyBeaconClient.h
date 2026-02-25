#pragma once

#include "CoreMinimal.h"
#include "OnlineBeaconClient.h"
#include "Beacon/LobbyTypes.h"
#include "LobbyBeaconClient.generated.h"

DECLARE_DELEGATE_OneParam(FOnRequestValidate, bool /*bValidated*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLobbyUpdated, const TArray<FPlayerLobbyInfo>&, Players);

UCLASS()
class WORMSNETWORKTD_API ALobbyBeaconClient : public AOnlineBeaconClient
{
	GENERATED_BODY()

public:
	ALobbyBeaconClient(const FObjectInitializer& Initializer);

	// ----- Surcharges AOnlineBeaconClient -----
	virtual void OnConnected() override;
	virtual void OnFailure() override;

	bool ConnectToServer(FURL& Url);

	// ----- Réservation -----

	/** (Client -> Serveur) Demande une place dans le lobby. */
	UFUNCTION(Server, Reliable)
	void Server_RequestReservation(const FUniqueNetIdRepl& PlayerNetId);

	/** (Serveur -> Client) Réservation accordée. */
	UFUNCTION(Client, Reliable)
	void Client_ReservationAccepted();

	/** (Serveur -> Client) Réservation refusée (lobby plein). */
	UFUNCTION(Client, Reliable)
	void Client_ReservationDenied();

	/**
	 * Delegate appelé une seule fois après la tentative de réservation.
	 * true  = accepté, false = refusé ou connexion échouée.
	 */
	FOnRequestValidate OnRequestValidate;

	/**
	 * Informations du joueur à envoyer après la réservation.
	 * Pré-rempli par ConnectHostAsClient() pour l'hôte,
	 * ou laissé à ses valeurs par défaut pour un client normal
	 * (le client normal devra idéalement les remplir depuis son profil).
	 */
	FPlayerLobbyInfo PendingPlayerInfo;

	// ----- Informations de lobby -----

	/** (Client -> Serveur) Envoie les infos du joueur au host. */
	UFUNCTION(Server, Reliable)
	void Server_SendLobbyInfo(const FPlayerLobbyInfo& PlayerInfo);

	/** (Serveur -> Client) Reçoit la liste complète du lobby mise à jour. */
	UFUNCTION(Client, Reliable)
	void Client_ReceiveLobbyUpdate(const TArray<FPlayerLobbyInfo>& Players);

	/** Diffusé à chaque mise à jour de la liste des joueurs. */
	UPROPERTY(BlueprintAssignable)
	FOnLobbyUpdated OnLobbyUpdated;
};