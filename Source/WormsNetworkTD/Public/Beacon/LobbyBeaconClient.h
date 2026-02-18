#pragma once

#include "CoreMinimal.h"
#include "OnlineBeaconClient.h"
#include "Beacon/LobbyTypes.h"
#include "LobbyBeaconClient.generated.h"

DECLARE_DELEGATE_OneParam(FOnRequestValidate, bool);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLobbyUpdated, const TArray<FPlayerLobbyInfo>&, Players);

UCLASS()
class WORMSNETWORKTD_API ALobbyBeaconClient : public AOnlineBeaconClient
{
	GENERATED_BODY()
	
public:
	ALobbyBeaconClient(const FObjectInitializer& Initializer);
	virtual void OnConnected() override;
	virtual void OnFailure() override;
	bool ConnectToServer(FURL& Url);
	UFUNCTION(Server, Reliable)
	void Server_RequestReservation(const FUniqueNetIdRepl& PlayerNetId);
	UFUNCTION(Client, Reliable)
	void Client_ReservationAccepted();
	UFUNCTION(Client, Reliable)
	void Client_ReservationDenied();
	FOnRequestValidate OnRequestValidate;
	UFUNCTION(Server, Reliable)
	void Server_SendLobbyInfo(const FPlayerLobbyInfo& PlayerInfo);
	UFUNCTION(Client, Reliable)
	void Client_ReceiveLobbyUpdate(const TArray<FPlayerLobbyInfo>& Players);

	UPROPERTY(BlueprintAssignable)
	FOnLobbyUpdated OnLobbyUpdated;
};
