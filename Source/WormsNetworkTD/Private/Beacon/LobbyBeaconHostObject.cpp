#include "Beacon/LobbyBeaconHostObject.h"
#include "Beacon/LobbyBeaconClient.h"

ALobbyBeaconHostObject::ALobbyBeaconHostObject(const FObjectInitializer& Initializer) : Super(Initializer)
{
	ClientBeaconActorClass = ALobbyBeaconClient::StaticClass();
	BeaconTypeName = ClientBeaconActorClass->GetName();
}

void ALobbyBeaconHostObject::OnClientConnected(AOnlineBeaconClient* NewClientActor, UNetConnection* ClientConnection)
{
	Super::OnClientConnected(NewClientActor, ClientConnection);
	if (NewClientActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("CONNECTED CLIENT VALID"));
	}
	else
		UE_LOG(LogTemp, Warning, TEXT("CONNECTED CLIENT INVALID"));
}

AOnlineBeaconClient* ALobbyBeaconHostObject::SpawnBeaconActor(UNetConnection* ClientConnection)
{
	return Super::SpawnBeaconActor(ClientConnection);
}