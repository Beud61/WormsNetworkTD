#include "UI/UIMenu.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ComboBoxString.h"
#include "Components/CanvasPanel.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UUIMenu::NativeConstruct()
{
	Super::NativeConstruct();

	SetupMenu();
}

void UUIMenu::SetupMenu()
{
	// === BIND MENU PRINCIPAL ===
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

	// === BIND CREATE ROOM SETTINGS ===
	if (Btn_CloseCreateRoomSettings)
	{
		Btn_CloseCreateRoomSettings->OnClicked.AddDynamic(this, &UUIMenu::OnCloseCreateRoomSettingsClicked);
	}

	if (Btn_OpenRoom)
	{
		Btn_OpenRoom->OnClicked.AddDynamic(this, &UUIMenu::OnOpenRoomClicked);
	}

	if (GameModeChoice)
	{
		GameModeChoice->OnSelectionChanged.AddDynamic(this, &UUIMenu::OnGameModeChanged);
	}

	if (WaterRisingChoice)
	{
		WaterRisingChoice->OnSelectionChanged.AddDynamic(this, &UUIMenu::OnWaterRisingChanged);
	}

	if (UnitLifeChoice)
	{
		UnitLifeChoice->OnSelectionChanged.AddDynamic(this, &UUIMenu::OnUnitLifeChanged);
	}

	if (UnitCountChoice)
	{
		UnitCountChoice->OnSelectionChanged.AddDynamic(this, &UUIMenu::OnUnitCountChanged);
	}

	// Afficher le menu principal au démarrage
	ShowMainMenu();

	// Afficher le curseur
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->bShowMouseCursor = true;
		PC->SetInputMode(FInputModeUIOnly());
	}
}

// === CALLBACKS MENU PRINCIPAL ===

void UUIMenu::OnCreateRoomClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("Create Room clicked"));
	ShowCreateRoomSettings();
}

void UUIMenu::OnJoinRoomClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("Join Room clicked"));

	//TODO: Menu pour joindre une room
}

void UUIMenu::OnFindRoomClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("Find Room clicked"));

	//TODO: Chercher les rooms disponibles
}

void UUIMenu::OnSettingsClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("Settings clicked"));

	// TODO: Settings
}

void UUIMenu::OnQuitClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("Quit clicked!"));

	if (APlayerController* PC = GetOwningPlayer())
	{
		UKismetSystemLibrary::QuitGame(GetWorld(), PC, EQuitPreference::Quit, false);
	}
}

// === CALLBACKS CREATE ROOM SETTINGS ===

void UUIMenu::OnCloseCreateRoomSettingsClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("Close CreateRoomSettings clicked"));
	ShowMainMenu();
}

void UUIMenu::OnOpenRoomClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("Open Room clicked - GameMode: %s, Turns: %d, Life: %d, Count: %d"),
		*SelectedGameMode, SelectedTurnsBeforeWater, SelectedUnitLife, SelectedUnitCount);

	// TODO: Créer la session réseau avec les paramètres choisis

	if (Txt_Status)
	{
		Txt_Status->SetText(FText::FromString(TEXT("Room Status : Open")));
		Txt_Status->SetColorAndOpacity(FLinearColor::Green);
	}
}

void UUIMenu::OnGameModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	SelectedGameMode = SelectedItem;
	UE_LOG(LogTemp, Log, TEXT("Game Mode changed to: %s"), *SelectedGameMode);
}

void UUIMenu::OnWaterRisingChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	SelectedTurnsBeforeWater = FCString::Atoi(*SelectedItem);
	UE_LOG(LogTemp, Log, TEXT("Turns Before Water changed to: %d"), SelectedTurnsBeforeWater);
}

void UUIMenu::OnUnitLifeChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	SelectedUnitLife = FCString::Atoi(*SelectedItem);
	UE_LOG(LogTemp, Log, TEXT("Unit Life changed to: %d"), SelectedUnitLife);
}

void UUIMenu::OnUnitCountChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	SelectedUnitCount = FCString::Atoi(*SelectedItem);
	UE_LOG(LogTemp, Log, TEXT("Unit Count changed to: %d"), SelectedUnitCount);
}

// === FONCTIONS UTILITAIRES ===

void UUIMenu::ShowMainMenu()
{
	if (MenuPanel)
	{
		MenuPanel->SetVisibility(ESlateVisibility::Visible);
	}

	if (CreateRoomSettings)
	{
		CreateRoomSettings->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UUIMenu::ShowCreateRoomSettings()
{
	if (MenuPanel)
	{
		MenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (CreateRoomSettings)
	{
		CreateRoomSettings->SetVisibility(ESlateVisibility::Visible);
	}

	// Reset le statut
	if (Txt_Status)
	{
		Txt_Status->SetText(FText::FromString(TEXT("Room Status : Closed")));
		Txt_Status->SetColorAndOpacity(FLinearColor::Red);
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