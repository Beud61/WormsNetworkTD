#include "Beacon/LobbyBeaconHostObject.h"
#include "Beacon/LobbyBeaconClient.h"

ALobbyBeaconHostObject::ALobbyBeaconHostObject(const FObjectInitializer& Initializer)
	: Super(Initializer)
{
	ClientBeaconActorClass = ALobbyBeaconClient::StaticClass();
	BeaconTypeName = ClientBeaconActorClass->GetName();
}

// ============================================================
//  Connexion d'un client
// ============================================================

void ALobbyBeaconHostObject::OnClientConnected(AOnlineBeaconClient* NewClientActor,
	UNetConnection* ClientConnection)
{
	Super::OnClientConnected(NewClientActor, ClientConnection);

	ALobbyBeaconClient* LobbyClient = Cast<ALobbyBeaconClient>(NewClientActor);
	if (!LobbyClient)
	{
		UE_LOG(LogTemp, Error, TEXT("OnClientConnected: cast ALobbyBeaconClient echoue."));
		return;
	}

	ConnectedClients.Add(LobbyClient);
	UE_LOG(LogTemp, Warning, TEXT("OnClientConnected: client ajoute (%d connecte(s))."),
		ConnectedClients.Num());

	// Envoie immédiatement l'état actuel du lobby au nouveau client
	LobbyClient->Client_ReceiveLobbyUpdate(ConnectedPlayers);
}

// ============================================================
//  Déconnexion d'un client
// ============================================================

void ALobbyBeaconHostObject::NotifyClientDisconnected(AOnlineBeaconClient* LeavingClientActor)
{
	Super::NotifyClientDisconnected(LeavingClientActor);

	ALobbyBeaconClient* LobbyClient = Cast<ALobbyBeaconClient>(LeavingClientActor);
	if (LobbyClient)
	{
		ConnectedClients.Remove(LobbyClient);

		UE_LOG(LogTemp, Warning, TEXT("NotifyClientDisconnected: client retire (%d restant(s))."),
			ConnectedClients.Num());
	}
}

// ============================================================
//  Spawn du beacon client côté serveur
// ============================================================

AOnlineBeaconClient* ALobbyBeaconHostObject::SpawnBeaconActor(UNetConnection* ClientConnection)
{
	// Comportement par défaut : spawn via ClientBeaconActorClass
	return Super::SpawnBeaconActor(ClientConnection);
}

// ============================================================
//  Gestion des joueurs
// ============================================================

void ALobbyBeaconHostObject::RegisterOrUpdatePlayer(const FPlayerLobbyInfo& PlayerInfo)
{
	// L'hôte impose son propre nombre d'unités à tous les joueurs
	FPlayerLobbyInfo CorrectedInfo = PlayerInfo;
	CorrectedInfo.UnitNB = RoomUnitCount;

	const int32 ExistingIndex = ConnectedPlayers.IndexOfByPredicate(
		[&](const FPlayerLobbyInfo& P) { return P.PlayerId == CorrectedInfo.PlayerId; }
	);

	if (ExistingIndex != INDEX_NONE)
	{
		// Mise à jour d'un joueur existant
		ConnectedPlayers[ExistingIndex] = CorrectedInfo;
		UE_LOG(LogTemp, Log, TEXT("RegisterOrUpdatePlayer: mise a jour de '%s'."), *CorrectedInfo.PlayerName);
	}
	else
	{
		// Nouveau joueur
		ConnectedPlayers.Add(CorrectedInfo);
		UE_LOG(LogTemp, Warning, TEXT("RegisterOrUpdatePlayer: '%s' ajoute (%d joueur(s))."),
			*CorrectedInfo.PlayerName, ConnectedPlayers.Num());
	}

	BroadcastLobbyUpdate();
}

void ALobbyBeaconHostObject::UnregisterPlayer(int32 PlayerId)
{
	const int32 RemovedCount = ConnectedPlayers.RemoveAll(
		[PlayerId](const FPlayerLobbyInfo& P) { return P.PlayerId == PlayerId; }
	);

	if (RemovedCount > 0)
	{
		ReservedSlots = FMath::Max(0, ReservedSlots - 1);
		UE_LOG(LogTemp, Warning, TEXT("UnregisterPlayer: joueur %d retire (%d restant(s))."),
			PlayerId, ConnectedPlayers.Num());
		BroadcastLobbyUpdate();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UnregisterPlayer: joueur %d introuvable."), PlayerId);
	}
}

// ============================================================
//  Diffusion
// ============================================================

void ALobbyBeaconHostObject::BroadcastLobbyUpdate()
{
	// On itère sur une copie pour être robuste si un client se déconnecte pendant la boucle
	TArray<ALobbyBeaconClient*> ClientsCopy = ConnectedClients;
	for (ALobbyBeaconClient* Client : ClientsCopy)
	{
		if (IsValid(Client))
		{
			Client->Client_ReceiveLobbyUpdate(ConnectedPlayers);
		}
	}
}