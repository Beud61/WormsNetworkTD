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
	SessionSubsystem = GetGameInstance()->GetSubsystem<UOnlineSessionSubsystem>();
	if (SessionSubsystem)
	{
		SessionSubsystem->OnFindSessionsCompleteEvent.AddDynamic(this, &UUIMenu::HandleFindSessionsCompleted);
	}
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

	if (Btn_CloseRoom)
	{
		Btn_CloseRoom->OnClicked.AddDynamic(this, &UUIMenu::OnCloseRoomClicked);
	}

	if (Btn_StartGame)
	{
		Btn_StartGame->OnClicked.AddDynamic(this, &UUIMenu::OnStartGameClicked);
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


	// === BIND FIND ROOM SETTINGS ===
	if (Btn_CloseFindRoom)
		Btn_CloseFindRoom->OnClicked.AddDynamic(this, &UUIMenu::OnCloseFindRoomClicked);

	if (Btn_Refresh)
		Btn_Refresh->OnClicked.AddDynamic(this, &UUIMenu::OnRefreshRoomsClicked);

	if (CheckBox_All)
		CheckBox_All->OnCheckStateChanged.AddDynamic(this, &UUIMenu::OnCheckBoxAllClicked);

	if (CheckBox_1V1)
		CheckBox_1V1->OnCheckStateChanged.AddDynamic(this, &UUIMenu::OnCheckBox1V1Clicked);

	if (CheckBox_2V2)
		CheckBox_2V2->OnCheckStateChanged.AddDynamic(this, &UUIMenu::OnCheckBox2V2Clicked);

	if (CheckBox_FFA)
		CheckBox_FFA->OnCheckStateChanged.AddDynamic(this, &UUIMenu::OnCheckBoxFFAClicked);


	// Afficher le menu principal au d�marrage
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
	if (!FoundSessions.IsValidIndex(SelectedSessionIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("No session selected."));
		return;
	}
	SessionSubsystem->CustomJoinSession(FoundSessions[SelectedSessionIndex], 7787, true);
}

void UUIMenu::OnFindRoomClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("Find Room clicked"));

	ShowFindRoom();
	OnCheckBoxAllClicked(true);
	SessionSubsystem->FindSessions(10000, true);
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
	int32 MaxPlayers = GetMaxPlayersFromGameMode();
	SessionSubsystem->CreateSession(
		TEXT("MyGameSession"),
		MaxPlayers,
		true,
		SelectedGameMode,
		SelectedUnitLife,
		SelectedUnitCount,
		SelectedTurnsBeforeWater
	);

	if (Txt_Status)
	{
		Txt_Status->SetText(FText::FromString(TEXT("Room Status : Open")));
		Txt_Status->SetColorAndOpacity(FLinearColor::Green);
	}

	if (Txt_PlayerNb)
	{
		FString StmpMaxPlayer = *SelectedGameMode;
		int ItmpMaxPlayer = 1;

		if (StmpMaxPlayer == "1V1")
			ItmpMaxPlayer = 2;
		else if (StmpMaxPlayer == "2V2" || StmpMaxPlayer == "FFA")
			ItmpMaxPlayer = 4;
		
		StmpMaxPlayer = FString::Printf(TEXT("1/%d"), ItmpMaxPlayer);
		Txt_PlayerNb->SetText(FText::FromString(StmpMaxPlayer));
		Txt_PlayerNb->SetColorAndOpacity(FLinearColor::White);
	}

	if (VB_PlayersInfos && PLayerInfoWidgetClass)
	{
		UUserInfoTemplate* PlayerInfoWidget = CreateWidget<UUserInfoTemplate>(GetWorld(), PLayerInfoWidgetClass);

		if (PlayerInfoWidget)
		{
			PlayerInfoWidget->UnitNB = SelectedUnitCount;
			PlayerInfoWidget->PlayerName = TEXT("Player 1");

			VB_PlayersInfos->AddChildToVerticalBox(PlayerInfoWidget);
			PlayersInfosUI.Add(PlayerInfoWidget);
		}
	}

	if (Settings)
		Settings->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (Btn_OpenRoom)
		Btn_OpenRoom->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (Btn_CloseRoom)
		Btn_CloseRoom->SetVisibility(ESlateVisibility::Visible);
}

void UUIMenu::OnCloseRoomClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("Room CLOSED"));

	//TODO : Kick players in the room

	if (Txt_Status)
	{
		Txt_Status->SetText(FText::FromString(TEXT("Room Status : Closed")));
		Txt_Status->SetColorAndOpacity(FLinearColor::Red);
	}

	if (Txt_PlayerNb)
	{
		Txt_PlayerNb->SetText(FText::FromString(TEXT("0/0")));
		Txt_PlayerNb->SetColorAndOpacity(FLinearColor::White);
	}

	if (VB_PlayersInfos)
	{
		VB_PlayersInfos->ClearChildren();
	}

	PlayersInfosUI.Empty();

	if (Settings)
		Settings->SetVisibility(ESlateVisibility::Visible);
	if (Btn_OpenRoom)
		Btn_OpenRoom->SetVisibility(ESlateVisibility::Visible);
	if (Btn_CloseRoom)
		Btn_CloseRoom->SetVisibility(ESlateVisibility::HitTestInvisible);

	FoundSessions.Empty();
	SessionSubsystem->DestroySession();
}

void UUIMenu::OnStartGameClicked()
{
	// If room is Full

	//Travel to Selected Map
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

void UUIMenu::OnCloseFindRoomClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("Close FindRoomSettings clicked"));
	ShowMainMenu();
	OnCheckBoxAllClicked(true);
}

void UUIMenu::OnCheckBoxAllClicked(bool bIsChecked)
{
	UE_LOG(LogTemp, Warning, TEXT("ALL clicked"));

	bCheckBoxAll = true;
	bCheckBox1V1 = false;
	bCheckBox2V2 = false;
	bCheckBoxFFA = false;

	CheckBox_All->SetIsChecked(bCheckBoxAll);
	CheckBox_1V1->SetIsChecked(bCheckBox1V1);
	CheckBox_2V2->SetIsChecked(bCheckBox2V2);
	CheckBox_FFA->SetIsChecked(bCheckBoxFFA);

	OnRefreshRoomsClicked();
}

void UUIMenu::OnCheckBox1V1Clicked(bool bIsChecked)
{
	UE_LOG(LogTemp, Warning, TEXT("1V1 clicked"));

	bCheckBoxAll = false;
	bCheckBox1V1 = true;
	if (bCheckBox2V2 && bCheckBoxFFA)
	{
		bCheckBoxAll = true;
		bCheckBox1V1 = false;
		bCheckBox2V2 = false;
		bCheckBoxFFA = false;
	}

	CheckBox_All->SetIsChecked(bCheckBoxAll);
	CheckBox_1V1->SetIsChecked(bCheckBox1V1);
	CheckBox_2V2->SetIsChecked(bCheckBox2V2);
	CheckBox_FFA->SetIsChecked(bCheckBoxFFA);

	OnRefreshRoomsClicked();
}

void UUIMenu::OnCheckBox2V2Clicked(bool bIsChecked)
{
	UE_LOG(LogTemp, Warning, TEXT("2V2 clicked"));

	bCheckBoxAll = false;
	bCheckBox2V2 = true;
	if (bCheckBox1V1 && bCheckBoxFFA)
	{
		bCheckBoxAll = true;
		bCheckBox1V1 = false;
		bCheckBox2V2 = false;
		bCheckBoxFFA = false;
	}

	CheckBox_All->SetIsChecked(bCheckBoxAll);
	CheckBox_1V1->SetIsChecked(bCheckBox1V1);
	CheckBox_2V2->SetIsChecked(bCheckBox2V2);
	CheckBox_FFA->SetIsChecked(bCheckBoxFFA);

	OnRefreshRoomsClicked();
}

void UUIMenu::OnCheckBoxFFAClicked(bool bIsChecked)
{
	UE_LOG(LogTemp, Warning, TEXT("FFA clicked"));

	bCheckBoxAll = false;
	bCheckBoxFFA = true;
	if (bCheckBox1V1 && bCheckBox2V2)
	{
		bCheckBoxAll = true;
		bCheckBox1V1 = false;
		bCheckBox2V2 = false;
		bCheckBoxFFA = false;
	}

	CheckBox_All->SetIsChecked(bCheckBoxAll);
	CheckBox_1V1->SetIsChecked(bCheckBox1V1);
	CheckBox_2V2->SetIsChecked(bCheckBox2V2);
	CheckBox_FFA->SetIsChecked(bCheckBoxFFA);

	OnRefreshRoomsClicked();
}

void UUIMenu::OnRefreshRoomsClicked()
{
	//TODO DELETE ROOMS AND FIND NEW ROOMS WITH FILTERS

	if (!FindRoomScrollBox) return;

	FindRoomScrollBox->ClearChildren();
	RoomInfosUI.Empty();

	for (int32 i = 0; i < FoundSessions.Num(); i++)
	{
		const FCustomSessionInfo& Session = FoundSessions[i];

		if (!PassFilter(Session))
			continue;

		AddRoomInfoUI(
			Session.SessionName,
			ConvertGameModeToID(Session.GameMode),
			Session.CurrentPlayers,
			Session.MaxPlayers,
			Session.Ping
		);

		//RoomInfosUI.Last()->SessionIndex = i;
	}
}

// === FONCTIONS UTILITAIRES ===

void UUIMenu::ShowMainMenu()
{
	if (MenuPanel)
		MenuPanel->SetVisibility(ESlateVisibility::Visible);

	if (CreateRoomSettings)
		CreateRoomSettings->SetVisibility(ESlateVisibility::Collapsed);

	if (FindRoom)
		FindRoom->SetVisibility(ESlateVisibility::Collapsed);
}

void UUIMenu::ShowCreateRoomSettings()
{
	if (MenuPanel)
		MenuPanel->SetVisibility(ESlateVisibility::Collapsed);

	if (FindRoom)
		FindRoom->SetVisibility(ESlateVisibility::Collapsed);

	if (CreateRoomSettings)
		CreateRoomSettings->SetVisibility(ESlateVisibility::Visible);

	// Reset le statut
	if (Txt_Status)
	{
		Txt_Status->SetText(FText::FromString(TEXT("Room Status : Closed")));
		Txt_Status->SetColorAndOpacity(FLinearColor::Red);
	}
}

void UUIMenu::ShowFindRoom()
{
	if (MenuPanel)
		MenuPanel->SetVisibility(ESlateVisibility::Collapsed);

	if (FindRoom)
		FindRoom->SetVisibility(ESlateVisibility::Visible);

	if (CreateRoomSettings)
		CreateRoomSettings->SetVisibility(ESlateVisibility::Collapsed);
}

//RoomMode ID => 0 = 1V1 | 1 = 2V2 | 2 = FFA
void UUIMenu::AddRoomInfoUI(FString RoomName, int32 RoomModeID, int32 PlayerInRoom, int32 MaxPlayerInRoom, int32 RoomPing)
{
	if (FindRoomScrollBox && RoomInfoWidgetClass)
	{
		URoomInfoTemplate* RoomInfoWidget = CreateWidget<URoomInfoTemplate>(GetWorld(), RoomInfoWidgetClass);

		if (RoomInfoWidget)
		{
			RoomInfoWidget->RoomName = RoomName;

			if(RoomModeID <= 2 && RoomModeID >= 0)
				RoomInfoWidget->RoomModeID = RoomModeID;

			switch (RoomModeID)
			{
			case 0:
				RoomInfoWidget->RoomModeText = "GameMode : 1V1";
				break;
			case 1:
				RoomInfoWidget->RoomModeText = "GameMode : 2V2";
				break;
			case 2:
				RoomInfoWidget->RoomModeText = "GameMode : FFA";
				break;
			default:
				break;
			}
			RoomInfoWidget->PlayerInRoom = PlayerInRoom;
			RoomInfoWidget->MaxPlayerInRoom = MaxPlayerInRoom;
			RoomInfoWidget->PlayersText = FString::Printf(TEXT("Players : %d/%d"), PlayerInRoom, MaxPlayerInRoom);
			RoomInfoWidget->RoomPing = RoomPing;

			FindRoomScrollBox->AddChild(RoomInfoWidget);
			RoomInfosUI.Add(RoomInfoWidget);
		}
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

int32 UUIMenu::GetMaxPlayersFromGameMode() const
{
	if (SelectedGameMode == "1V1")
	{
		return 2;
	}
	else if (SelectedGameMode == "2V2")
	{
		return 4;
	}
	else if (SelectedGameMode == "FFA")
	{
		return 4;
	}
	return 2; // s�curit� par d�faut
}

void UUIMenu::HandleFindSessionsCompleted(const TArray<FCustomSessionInfo>& Sessions, bool bWasSuccessful)
{
	if (!bWasSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("FindSessions failed"));
		return;
	}

	FoundSessions = Sessions;
	OnRefreshRoomsClicked();
}

int32 UUIMenu::ConvertGameModeToID(const FString& GameMode) const
{
	if (GameMode == "1V1")
		return 0;
	if (GameMode == "2V2")
		return 1;
	if (GameMode == "FFA")
		return 2;

	return 0;
}

bool UUIMenu::PassFilter(const FCustomSessionInfo& Session) const
{
	if (bCheckBoxAll)
		return true;

	if (bCheckBox1V1 && Session.GameMode == "1V1")
		return true;

	if (bCheckBox2V2 && Session.GameMode == "2V2")
		return true;

	if (bCheckBoxFFA && Session.GameMode == "FFA")
		return true;

	return false;
}