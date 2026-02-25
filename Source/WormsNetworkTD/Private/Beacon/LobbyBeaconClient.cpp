#include "Beacon/LobbyBeaconClient.h"
#include "Beacon/LobbyBeaconHostObject.h"
#include "GameFramework/PlayerController.h"

ALobbyBeaconClient::ALobbyBeaconClient(const FObjectInitializer& Initializer)
	: Super(Initializer)
{
	UE_LOG(LogTemp, Warning, TEXT("ALobbyBeaconClient: acteur cree (%p)"), this);
}

// ============================================================
//  Connexion / Déconnexion
// ============================================================

void ALobbyBeaconClient::OnConnected()
{
	Super::OnConnected();
	UE_LOG(LogTemp, Warning, TEXT("ALobbyBeaconClient: connecte au beacon host."));

	// Dès que la connexion est établie, on demande une place
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (LocalPlayer)
	{
		Server_RequestReservation(LocalPlayer->GetPreferredUniqueNetId());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ALobbyBeaconClient::OnConnected: aucun LocalPlayer trouve."));
		// Signale l'échec au subsystem
		if (OnRequestValidate.IsBound())
		{
			OnRequestValidate.Execute(false);
		}
	}
}

void ALobbyBeaconClient::OnFailure()
{
	Super::OnFailure();
	UE_LOG(LogTemp, Error, TEXT("ALobbyBeaconClient: echec de connexion au beacon host."));

	if (OnRequestValidate.IsBound())
	{
		OnRequestValidate.Execute(false);
	}
}

bool ALobbyBeaconClient::ConnectToServer(FURL& Url)
{
	return InitClient(Url);
}

// ============================================================
//  Réservation
// ============================================================

void ALobbyBeaconClient::Server_RequestReservation_Implementation(const FUniqueNetIdRepl& PlayerNetId)
{
	ALobbyBeaconHostObject* Host = Cast<ALobbyBeaconHostObject>(GetBeaconOwner());
	if (!Host)
	{
		UE_LOG(LogTemp, Error, TEXT("Server_RequestReservation: HostObject introuvable."));
		Client_ReservationDenied();
		return;
	}

	if (Host->ReservedSlots >= Host->MaxSlots)
	{
		UE_LOG(LogTemp, Warning, TEXT("Server_RequestReservation: lobby plein (%d/%d)."),
			Host->ReservedSlots, Host->MaxSlots);
		Client_ReservationDenied();
		return;
	}

	Host->ReservedSlots++;
	UE_LOG(LogTemp, Warning, TEXT("Server_RequestReservation: reservation accordee (%d/%d)."),
		Host->ReservedSlots, Host->MaxSlots);

	Client_ReservationAccepted();
}

void ALobbyBeaconClient::Client_ReservationAccepted_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("ALobbyBeaconClient: reservation acceptee."));

	// 1. Notifie le subsystem (qui informera l'UI via OnBeaconClientCreated)
	if (OnRequestValidate.IsBound())
	{
		OnRequestValidate.Execute(true);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Client_ReservationAccepted: OnRequestValidate non binde."));
	}

	// 2. Envoie les infos du joueur au host INDÉPENDAMMENT du delegate.
	//    PendingPlayerInfo est pré-rempli par ConnectHostAsClient() pour l'hôte.
	//    Pour un client normal, il contient les valeurs par défaut — à remplacer
	//    par les vraies données de profil (GameInstance / SaveGame).
	if (PendingPlayerInfo.PlayerId == 0)
	{
		// Génère un ID pseudo-unique si non fourni
		PendingPlayerInfo.PlayerId = static_cast<int32>(FPlatformTime::Cycles() & 0x7FFFFFFF);
	}

	Server_SendLobbyInfo(PendingPlayerInfo);
}

void ALobbyBeaconClient::Client_ReservationDenied_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("ALobbyBeaconClient: reservation refusee."));

	if (OnRequestValidate.IsBound())
	{
		OnRequestValidate.Execute(false);
	}
}

// ============================================================
//  Lobby info
// ============================================================

void ALobbyBeaconClient::Server_SendLobbyInfo_Implementation(const FPlayerLobbyInfo& PlayerInfo)
{
	ALobbyBeaconHostObject* Host = Cast<ALobbyBeaconHostObject>(GetBeaconOwner());
	if (!Host)
	{
		UE_LOG(LogTemp, Error, TEXT("Server_SendLobbyInfo: HostObject introuvable."));
		return;
	}

	Host->RegisterOrUpdatePlayer(PlayerInfo);
}

void ALobbyBeaconClient::Client_ReceiveLobbyUpdate_Implementation(const TArray<FPlayerLobbyInfo>& Players)
{
	UE_LOG(LogTemp, Log, TEXT("Client_ReceiveLobbyUpdate: %d joueur(s) dans le lobby."), Players.Num());
	OnLobbyUpdated.Broadcast(Players);
}