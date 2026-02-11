#pragma once

#include "CoreMinimal.h"
#include "OnlineBeaconHostObject.h"
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
};