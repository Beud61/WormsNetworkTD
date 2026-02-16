#pragma once

#include "CoreMinimal.h"
#include "OnlineBeaconClient.h"
#include "LobbyBeaconClient.generated.h"

DECLARE_DELEGATE_OneParam(FOnRequestValidate, bool);

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
};
