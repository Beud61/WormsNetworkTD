// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/UIMenu.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UUIMenu::NativeConstruct()
{
	Super::NativeConstruct();
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		SessionSubsystem = GameInstance->GetSubsystem<UOnlineSessionSubsystem>();
		UE_LOG(LogTemp, Warning, TEXT("Session Subsystem: %s"), *GetNameSafe(SessionSubsystem));
	}
	SetupMenu();
}

void UUIMenu::SetupMenu()
{
	// Bind Events
	if (Btn_CreateRoom)
	{
		Btn_CreateRoom->OnClicked.AddDynamic(this, &UUIMenu::OnCreateRoomClicked);
	}

	if (Btn_JoinRoom)
	{
		Btn_JoinRoom->OnClicked.AddDynamic(this, &UUIMenu::OnJoinRoomClicked);
	}

	if (Btn_FindRoom)
	{		
		Btn_FindRoom->OnClicked.AddDynamic(this, &UUIMenu::OnFindRoomClicked);
	}

	if (Btn_Settings)
	{
		Btn_Settings->OnClicked.AddDynamic(this, &UUIMenu::OnSettingsClicked);
	}

	if (Btn_Quit)
	{
		Btn_Quit->OnClicked.AddDynamic(this, &UUIMenu::OnQuitClicked);
	}

	// Afficher le curseur
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->bShowMouseCursor = true;
		PC->SetInputMode(FInputModeUIOnly());
	}
	// S'abonner aux événements du subsystem
	if (SessionSubsystem)
	{
		SessionSubsystem->OnFindSessionsCompleteEvent.AddDynamic(this, &UUIMenu::OnSessionsFound);
	}
}

void UUIMenu::OnCreateRoomClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("Create Room clicked"));
	
	if (!SessionSubsystem) return;

	FString SessionName = TEXT("DefaultRoom");
	/*if (SessionNameInput)
	{
		SessionName = SessionNameInput->GetText().ToString();
	}*/

	// Créer une session avec 4 joueurs max, en LAN
	SessionSubsystem->CreateSession(SessionName, 4, true);

	// Créer le beacon host
	SessionSubsystem->CreateHostBeacon(7787, true);

	UE_LOG(LogTemp, Warning, TEXT("Session creation initiated with name: %s"), *SessionName);
	
}

void UUIMenu::OnJoinRoomClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("Join Room clicked"));
	
	if (!SessionSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("SessionSubsystem est null"));
		return;
	}

	if (FoundSessions.Num() > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Tentative de rejoindre la session: %s"),
			*FoundSessions[0].SessionName);

		// Utiliser directement JoinGameSession au lieu de CustomJoinSession
		const FOnlineSessionSearchResult& SessionResult = SessionSubsystem->SearchResults[FoundSessions[0].SessionSearchResultIndex];
		SessionSubsystem->JoinGameSession(SessionResult);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("FoundSessions est vide ! Lancez 'Find Room' d'abord."));
	}
}

void UUIMenu::OnFindRoomClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("Find Room clicked"));

	if (!SessionSubsystem) return;

	// Chercher jusqu'à 20 sessions en LAN
	SessionSubsystem->FindSessions(20, true);
}

void UUIMenu::OnSettingsClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("Settings clicked"));
	
	// TODO: Settings
}

void UUIMenu::OnQuitClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("Quit clicked!"));
	
	// Quit
	if (APlayerController* PC = GetOwningPlayer())
	{
		UKismetSystemLibrary::QuitGame(GetWorld(), PC, EQuitPreference::Quit, false);
	}
}

void UUIMenu::CloseMenu()
{
	RemoveFromParent();

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}
}

void UUIMenu::OnSessionsFound(const TArray<FCustomSessionInfo>& Sessions, bool bSuccess)
{
	FoundSessions = Sessions;

	//if (!SessionListBox) return;

	// Vider la liste
	//SessionListBox->ClearChildren();

	// Afficher les sessions trouvées
	for (int32 i = 0; i < Sessions.Num(); i++)
	{
		// Créer un widget pour chaque session
		// Vous devrez créer un widget Blueprint pour afficher les infos de session
		// Pour l'instant, voici un exemple simplifié

		UE_LOG(LogTemp, Log, TEXT("Session trouvée: %s (%d/%d) - Ping: %d"),
			*Sessions[i].SessionName,
			Sessions[i].CurrentPlayers,
			Sessions[i].MaxPlayers,
			Sessions[i].Ping);
	}
}