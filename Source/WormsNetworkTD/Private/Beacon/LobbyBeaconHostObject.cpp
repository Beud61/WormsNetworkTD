#include "Beacon/LobbyBeaconHostObject.h"

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
	{
		UE_LOG(LogTemp, Warning, TEXT("CONNECTED CLIENT INVALID"));
	}

	ALobbyBeaconClient* LobbyClient = Cast<ALobbyBeaconClient>(NewClientActor);
	if (LobbyClient)
	{
		ConnectedClients.Add(LobbyClient);
		UE_LOG(LogTemp, Warning, TEXT("CLIENT ADDED TO ConnectedClients"));
		// Send current lobby info to the new client
		LobbyClient->Client_ReceiveLobbyUpdate(ConnectedPlayers);
	}
}

AOnlineBeaconClient* ALobbyBeaconHostObject::SpawnBeaconActor(UNetConnection* ClientConnection)
{
	return Super::SpawnBeaconActor(ClientConnection);
}

void ALobbyBeaconHostObject::RegisterOrUpdatePlayer(const FPlayerLobbyInfo& PlayerInfo)
{
	FPlayerLobbyInfo CorrectedInfo = PlayerInfo;
	//Le host impose le nombre d’unités
	CorrectedInfo.UnitNB = RoomUnitCount;

	int32 Index = ConnectedPlayers.IndexOfByPredicate(
		[&](const FPlayerLobbyInfo& P)
		{
			return P.PlayerId == CorrectedInfo.PlayerId;
		});

	if (Index != INDEX_NONE)
	{
		ConnectedPlayers[Index] = CorrectedInfo;
	}
	else
	{
		ConnectedPlayers.Add(CorrectedInfo);
	}

	BroadcastLobbyUpdate();
}

void ALobbyBeaconHostObject::BroadcastLobbyUpdate()
{
	for (ALobbyBeaconClient* Client : ConnectedClients)
	{
		if (Client)
		{
			Client->Client_ReceiveLobbyUpdate(ConnectedPlayers);
		}
	}
}
