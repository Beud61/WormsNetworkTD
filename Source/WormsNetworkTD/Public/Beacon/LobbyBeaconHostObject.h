#pragma once

#include "CoreMinimal.h"
#include "OnlineBeaconHostObject.h"
#include "Beacon/LobbyTypes.h"
#include "Beacon/LobbyBeaconClient.h"
#include "LobbyBeaconHostObject.generated.h"

UCLASS()
class WORMSNETWORKTD_API ALobbyBeaconHostObject : public AOnlineBeaconHostObject
{
	GENERATED_BODY()
	
public:
	ALobbyBeaconHostObject(const FObjectInitializer& Initializer);
	virtual void OnClientConnected(AOnlineBeaconClient* NewClientActor, UNetConnection* ClientConnection) override;
	virtual AOnlineBeaconClient* SpawnBeaconActor(UNetConnection* ClientConnection) override;
	int32 ReservedSlots = 0;
	int32 MaxSlots = 4;
	UPROPERTY()
	TArray<FPlayerLobbyInfo> ConnectedPlayers;
	void RegisterOrUpdatePlayer(const FPlayerLobbyInfo& PlayerInfo);
	void BroadcastLobbyUpdate();
	UPROPERTY()
	TArray<ALobbyBeaconClient*> ConnectedClients;
};