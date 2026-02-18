#include "Beacon/LobbyBeaconClient.h"
#include "Beacon/LobbyBeaconHostObject.h"
#include "Network/OnlineSessionSubsystem.h"

ALobbyBeaconClient::ALobbyBeaconClient(const FObjectInitializer& Initializer) : Super(Initializer)
{
}

void ALobbyBeaconClient::OnConnected()
{
	Super::OnConnected();
	UE_LOG(LogTemp, Warning, TEXT("CONNECTED TO HOST BEACON"));
	const ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(GetWorld()->GetFirstLocalPlayerFromController());
	if (LocalPlayer)
	{
		Server_RequestReservation(LocalPlayer->GetPreferredUniqueNetId());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("No LocalPlayer found!"));
	}
}

void ALobbyBeaconClient::OnFailure()
{
	Super::OnFailure();
	UE_LOG(LogTemp, Warning, TEXT("FAILED TO CONNECT TO HOST BEACON"));
}

bool ALobbyBeaconClient::ConnectToServer(FURL& Url)
{
	return InitClient(Url);
}

// ---- Reservation ----
void ALobbyBeaconClient::Server_RequestReservation_Implementation(const FUniqueNetIdRepl& PlayerNetId)
{
	ALobbyBeaconHostObject* Host = Cast<ALobbyBeaconHostObject>(GetBeaconOwner());
	if (!Host)
	{
		Client_ReservationDenied();
		return;
	}
	if (Host->ReservedSlots >= Host->MaxSlots)
	{
		Client_ReservationDenied();
		return;
	}

	Host->ReservedSlots++;
	Client_ReservationAccepted();
}

void ALobbyBeaconClient::Client_ReservationAccepted_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("RESERVATION ACCEPTED"));
	OnRequestValidate.Execute(true);
}

void ALobbyBeaconClient::Client_ReservationDenied_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("RESERVATION DENIED"));
	OnRequestValidate.Execute(false);
}

// ---- Lobby Info ----
void ALobbyBeaconClient::Server_SendLobbyInfo_Implementation(const FPlayerLobbyInfo& PlayerInfo)
{
	if (ALobbyBeaconHostObject* Host = Cast<ALobbyBeaconHostObject>(GetBeaconOwner()))
	{
		Host->RegisterOrUpdatePlayer(PlayerInfo);
	}
}

void ALobbyBeaconClient::Client_ReceiveLobbyUpdate_Implementation(const TArray<FPlayerLobbyInfo>& Players)
{
	OnLobbyUpdated.Broadcast(Players);
}

void ALobbyBeaconClient::AddPlayerAndBroadcast(const FPlayerLobbyInfo& PlayerInfo)
{
	OnLobbyUpdated.Broadcast({ PlayerInfo });
}