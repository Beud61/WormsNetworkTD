#include "UI/UIMenu.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ComboBoxString.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Beacon/LobbyBeaconClient.h"
#include "Actors/CustomPlayerController.h"

// ============================================================
//  Initialisation
// ============================================================

void UUIMenu::NativeConstruct()
{
	Super::NativeConstruct();

	SessionSubsystem = GetGameInstance()->GetSubsystem<UOnlineSessionSubsystem>();
	if (SessionSubsystem)
	{
		SessionSubsystem->OnFindSessionsCompleteEvent.AddDynamic(this, &UUIMenu::HandleFindSessionsCompleted);
		SessionSubsystem->OnLobbysUpdated.AddDynamic(this, &UUIMenu::HandleLobbyUpdated);
		SessionSubsystem->OnBeaconClientCreated.AddDynamic(this, &UUIMenu::HandleBeaconCreated);
		SessionSubsystem->OnSessionJoinCompleted.AddDynamic(this, &UUIMenu::HandleSessionJoinCompleted);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UUIMenu::NativeConstruct: SessionSubsystem introuvable."));
	}

	SetupMenu();
}

// ============================================================
//  Setup des bindings boutons
// ============================================================

void UUIMenu::SetupMenu()
{
	// Menu principal
	if (Btn_CreateRoom) Btn_CreateRoom->OnClicked.AddDynamic(this, &UUIMenu::OnCreateRoomClicked);
	if (Btn_JoinRoom)   Btn_JoinRoom->OnClicked.AddDynamic(this, &UUIMenu::OnJoinRoomClicked);
	if (Btn_FindRoom)   Btn_FindRoom->OnClicked.AddDynamic(this, &UUIMenu::OnFindRoomClicked);
	if (Btn_Settings)   Btn_Settings->OnClicked.AddDynamic(this, &UUIMenu::OnSettingsClicked);
	if (Btn_Quit)       Btn_Quit->OnClicked.AddDynamic(this, &UUIMenu::OnQuitClicked);

	// Create Room / Lobby
	if (Btn_CloseCreateRoomSettings) Btn_CloseCreateRoomSettings->OnClicked.AddDynamic(this, &UUIMenu::OnCloseCreateRoomSettingsClicked);
	if (Btn_OpenRoom)                Btn_OpenRoom->OnClicked.AddDynamic(this, &UUIMenu::OnOpenRoomClicked);
	if (Btn_CloseRoom)               Btn_CloseRoom->OnClicked.AddDynamic(this, &UUIMenu::OnCloseRoomClicked);
	if (Btn_StartGame)               Btn_StartGame->OnClicked.AddDynamic(this, &UUIMenu::OnStartGameClicked);
	if (Btn_QuitLobby)               Btn_QuitLobby->OnClicked.AddDynamic(this, &UUIMenu::OnQuitLobbyClicked);

	if (GameModeChoice)   GameModeChoice->OnSelectionChanged.AddDynamic(this, &UUIMenu::OnGameModeChanged);
	if (WaterRisingChoice) WaterRisingChoice->OnSelectionChanged.AddDynamic(this, &UUIMenu::OnWaterRisingChanged);
	if (UnitLifeChoice)   UnitLifeChoice->OnSelectionChanged.AddDynamic(this, &UUIMenu::OnUnitLifeChanged);
	if (UnitCountChoice)  UnitCountChoice->OnSelectionChanged.AddDynamic(this, &UUIMenu::OnUnitCountChanged);

	// Find Room
	if (Btn_CloseFindRoom) Btn_CloseFindRoom->OnClicked.AddDynamic(this, &UUIMenu::OnCloseFindRoomClicked);
	if (Btn_Refresh)       Btn_Refresh->OnClicked.AddDynamic(this, &UUIMenu::OnRefreshRoomsClicked);
	if (CheckBox_All)      CheckBox_All->OnCheckStateChanged.AddDynamic(this, &UUIMenu::OnCheckBoxAllClicked);
	if (CheckBox_1V1)      CheckBox_1V1->OnCheckStateChanged.AddDynamic(this, &UUIMenu::OnCheckBox1V1Clicked);
	if (CheckBox_2V2)      CheckBox_2V2->OnCheckStateChanged.AddDynamic(this, &UUIMenu::OnCheckBox2V2Clicked);
	if (CheckBox_FFA)      CheckBox_FFA->OnCheckStateChanged.AddDynamic(this, &UUIMenu::OnCheckBoxFFAClicked);

	ShowMainMenu();

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->bShowMouseCursor = true;
		PC->SetInputMode(FInputModeUIOnly());
	}
}

// ============================================================
//  CALLBACKS — MENU PRINCIPAL
// ============================================================

void UUIMenu::OnCreateRoomClicked()
{
	ShowCreateRoomSettings();
}

void UUIMenu::OnJoinRoomClicked()
{
	// Quick Join : rejoint la première session disponible
	bIsQuickJoin = true;
	if (SessionSubsystem)
		SessionSubsystem->FindSessions(LobbyConstants::MaxSearchResults, true);
}

void UUIMenu::OnFindRoomClicked()
{
	ShowFindRoom();
	OnCheckBoxAllClicked(true);

	if (SessionSubsystem)
		SessionSubsystem->FindSessions(LobbyConstants::MaxSearchResults, true);
}

void UUIMenu::OnSettingsClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("Settings: non implemente."));
}

void UUIMenu::OnQuitClicked()
{
	if (APlayerController* PC = GetOwningPlayer())
		UKismetSystemLibrary::QuitGame(GetWorld(), PC, EQuitPreference::Quit, false);
}

// ============================================================
//  CALLBACKS — CREATE ROOM / LOBBY
// ============================================================

void UUIMenu::OnCloseCreateRoomSettingsClicked()
{
	ShowMainMenu();
}

void UUIMenu::OnOpenRoomClicked()
{
	if (!SessionSubsystem)
		return;

	// Infos hôte — TODO: lire depuis le GameInstance / profil utilisateur
	FPlayerLobbyInfo HostInfo;
	HostInfo.PlayerName = TEXT("Player 1");
	HostInfo.UnitNB = SelectedUnitCount;
	HostInfo.ProfileIcon = 0;
	HostInfo.TeamIcon = 0;
	HostInfo.PlayerId = FMath::RandRange(1, INT32_MAX);
	SessionSubsystem->SetHostPlayerInfo(HostInfo);

	SessionSubsystem->CreateSession(
		TEXT("MyGameSession"),
		GetMaxPlayersForGameMode(SelectedGameMode),
		true,
		SelectedGameMode,
		SelectedUnitLife,
		SelectedUnitCount,
		SelectedTurnsBeforeWater
	);

	// Statut visuel : en attente de la confirmation beacon
	UpdateRoomStatusUI(true, 0);

	if (Settings)    Settings->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (Btn_OpenRoom) Btn_OpenRoom->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (Btn_CloseRoom) Btn_CloseRoom->SetVisibility(ESlateVisibility::Visible);
}

void UUIMenu::OnCloseRoomClicked()
{
	if (VB_PlayersInfos) VB_PlayersInfos->ClearChildren();
	PlayersInfosUI.Empty();
	FoundSessions.Empty();

	UpdateRoomStatusUI(false, 0);

	if (Settings)    Settings->SetVisibility(ESlateVisibility::Visible);
	if (Btn_OpenRoom) Btn_OpenRoom->SetVisibility(ESlateVisibility::Visible);
	if (Btn_CloseRoom) Btn_CloseRoom->SetVisibility(ESlateVisibility::HitTestInvisible);

	if (SessionSubsystem)
		SessionSubsystem->DestroySession();
}

void UUIMenu::OnStartGameClicked()
{
	if (!SessionSubsystem)
		return;

	SessionSubsystem->StartGame();
}

void UUIMenu::OnQuitLobbyClicked()
{
	if (SessionSubsystem)
		SessionSubsystem->LeaveBeaconLobby();

	// Nettoie l'UI du lobby
	if (VB_PlayersInfos) VB_PlayersInfos->ClearChildren();
	PlayersInfosUI.Empty();

	ShowMainMenu();
}

void UUIMenu::OnGameModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	SelectedGameMode = SelectedItem;
}

void UUIMenu::OnWaterRisingChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	SelectedTurnsBeforeWater = FCString::Atoi(*SelectedItem);
}

void UUIMenu::OnUnitLifeChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	SelectedUnitLife = FCString::Atoi(*SelectedItem);
}

void UUIMenu::OnUnitCountChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	SelectedUnitCount = FCString::Atoi(*SelectedItem);
}

// ============================================================
//  CALLBACKS — FIND ROOM
// ============================================================

void UUIMenu::OnCloseFindRoomClicked()
{
	ShowMainMenu();
}

void UUIMenu::OnCheckBoxAllClicked(bool bIsChecked)
{
	bCheckBoxAll = true;
	bCheckBox1V1 = false;
	bCheckBox2V2 = false;
	bCheckBoxFFA = false;
	SyncCheckBoxVisuals();
}

void UUIMenu::OnCheckBox1V1Clicked(bool bIsChecked)
{
	bCheckBox1V1 = bIsChecked;  // <- respecte l'état réel (peut décocher)
	if (bIsChecked) bCheckBoxAll = false;

	// Si rien n'est coché -> retombe sur "Tous"
	if (!bCheckBox1V1 && !bCheckBox2V2 && !bCheckBoxFFA)
	{
		bCheckBoxAll = true;
	}
	SyncCheckBoxVisuals();
}

void UUIMenu::OnCheckBox2V2Clicked(bool bIsChecked)
{
	bCheckBox2V2 = bIsChecked;
	if (bIsChecked) bCheckBoxAll = false;

	if (!bCheckBox1V1 && !bCheckBox2V2 && !bCheckBoxFFA)
		bCheckBoxAll = true;

	SyncCheckBoxVisuals();
}

void UUIMenu::OnCheckBoxFFAClicked(bool bIsChecked)
{
	bCheckBoxFFA = bIsChecked;
	if (bIsChecked) bCheckBoxAll = false;

	if (!bCheckBox1V1 && !bCheckBox2V2 && !bCheckBoxFFA)
		bCheckBoxAll = true;

	SyncCheckBoxVisuals();
}

void UUIMenu::OnRefreshRoomsClicked()
{
	if (!SessionSubsystem)
		return;

	if (FindRoomScrollBox)
	{
		FindRoomScrollBox->ClearChildren();
		RoomInfosUI.Empty();
	}

	SessionSubsystem->FindSessions(LobbyConstants::MaxSearchResults, true);
}

void UUIMenu::OnJoinLobbyClicked(int32 Index)
{
	if (!FoundSessions.IsValidIndex(Index) || !SessionSubsystem)
		return;

	// On désactive le bouton pour éviter un double-clic pendant la connexion
	if (Btn_FindRoom) Btn_FindRoom->SetIsEnabled(false);

	SessionSubsystem->JoinLobby(FoundSessions[Index]);
	// L'UI lobby est affichée dans HandleSessionJoinCompleted (après confirmation beacon)
}

// ============================================================
//  AFFICHAGE DES PANNEAUX
// ============================================================

void UUIMenu::ShowMainMenu()
{
	if (MenuPanel)          MenuPanel->SetVisibility(ESlateVisibility::Visible);
	if (CreateRoomSettings) CreateRoomSettings->SetVisibility(ESlateVisibility::Collapsed);
	if (FindRoom)           FindRoom->SetVisibility(ESlateVisibility::Collapsed);
}

void UUIMenu::ShowCreateRoomSettings()
{
	if (MenuPanel)          MenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	if (FindRoom)           FindRoom->SetVisibility(ESlateVisibility::Collapsed);
	if (CreateRoomSettings) CreateRoomSettings->SetVisibility(ESlateVisibility::Visible);

	// Visibilité par défaut (hôte)
	if (HostSettingsSecurity)          HostSettingsSecurity->SetVisibility(ESlateVisibility::Collapsed);
	if (Btn_QuitLobby)                 Btn_QuitLobby->SetVisibility(ESlateVisibility::Collapsed);
	if (Btn_StartGame)                 Btn_StartGame->SetVisibility(ESlateVisibility::Visible);
	if (Btn_CloseCreateRoomSettings)   Btn_CloseCreateRoomSettings->SetVisibility(ESlateVisibility::Visible);
	if (Btn_OpenRoom)                  Btn_OpenRoom->SetVisibility(ESlateVisibility::Visible);
	if (Btn_CloseRoom)                 Btn_CloseRoom->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (Settings)                      Settings->SetVisibility(ESlateVisibility::Visible);

	UpdateRoomStatusUI(false, 0);
}

void UUIMenu::ShowFindRoom()
{
	if (MenuPanel)          MenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	if (FindRoom)           FindRoom->SetVisibility(ESlateVisibility::Visible);
	if (CreateRoomSettings) CreateRoomSettings->SetVisibility(ESlateVisibility::Collapsed);
	if (Btn_FindRoom)       Btn_FindRoom->SetIsEnabled(true);
}

void UUIMenu::ShowLobbyAsClient()
{
	// Réutilise le panneau CreateRoomSettings mais en mode client (lecture seule)
	if (MenuPanel)          MenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	if (FindRoom)           FindRoom->SetVisibility(ESlateVisibility::Collapsed);
	if (CreateRoomSettings) CreateRoomSettings->SetVisibility(ESlateVisibility::Visible);

	// Masque les contrôles réservés à l'hôte
	if (HostSettingsSecurity)        HostSettingsSecurity->SetVisibility(ESlateVisibility::Visible);
	if (Btn_QuitLobby)               Btn_QuitLobby->SetVisibility(ESlateVisibility::Visible);
	if (Btn_StartGame)               Btn_StartGame->SetVisibility(ESlateVisibility::Collapsed);
	if (Btn_CloseCreateRoomSettings) Btn_CloseCreateRoomSettings->SetVisibility(ESlateVisibility::Collapsed);
	if (Btn_OpenRoom)                Btn_OpenRoom->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (Btn_CloseRoom)               Btn_CloseRoom->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (Settings)                    Settings->SetVisibility(ESlateVisibility::HitTestInvisible);

	UpdateRoomStatusUI(true, 0);
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

// ============================================================
//  ROOM INFO UI (Find Room)
// ============================================================

void UUIMenu::AddRoomInfoUI(FString RoomName, int32 RoomModeID, int32 PlayerInRoom,
	int32 MaxPlayerInRoom, int32 RoomPing, int32 SessionIndex)
{
	if (!FindRoomScrollBox || !RoomInfoWidgetClass)
		return;

	URoomInfoTemplate* RoomInfoWidget = CreateWidget<URoomInfoTemplate>(GetWorld(), RoomInfoWidgetClass);
	if (!RoomInfoWidget)
		return;

	RoomInfoWidget->RoomName = RoomName;
	RoomInfoWidget->PlayerInRoom = PlayerInRoom;
	RoomInfoWidget->MaxPlayerInRoom = MaxPlayerInRoom;
	RoomInfoWidget->RoomPing = RoomPing;
	RoomInfoWidget->SessionIndex = SessionIndex;
	RoomInfoWidget->PlayersText = FString::Printf(TEXT("Players : %d/%d"), PlayerInRoom, MaxPlayerInRoom);

	if (RoomModeID >= 0 && RoomModeID <= 2)
	{
		RoomInfoWidget->RoomModeID = RoomModeID;
		const FString ModeNames[] = { TEXT("GameMode : 1V1"), TEXT("GameMode : 2V2"), TEXT("GameMode : FFA") };
		RoomInfoWidget->RoomModeText = ModeNames[RoomModeID];
	}

	if (RoomInfoWidget->Btn_JoinLobby)
		RoomInfoWidget->OnJoinClicked.AddDynamic(this, &UUIMenu::OnJoinLobbyClicked);

	FindRoomScrollBox->AddChild(RoomInfoWidget);
	RoomInfosUI.Add(RoomInfoWidget);
}

// ============================================================
//  PLAYER INFO UI (Lobby)
// ============================================================

void UUIMenu::AddPlayerInfoUI(const FPlayerLobbyInfo& PlayerInfo)
{
	if (!VB_PlayersInfos || !PLayerInfoWidgetClass)
		return;

	UUserInfoTemplate* PlayerInfoWidget = CreateWidget<UUserInfoTemplate>(GetWorld(), PLayerInfoWidgetClass);
	if (!PlayerInfoWidget)
		return;

	PlayerInfoWidget->PlayerName = PlayerInfo.PlayerName;
	PlayerInfoWidget->UnitNB = PlayerInfo.UnitNB;
	PlayerInfoWidget->ProfileIcon = PlayerInfo.ProfileIcon;
	PlayerInfoWidget->TeamIcon = PlayerInfo.TeamIcon;

	VB_PlayersInfos->AddChild(PlayerInfoWidget);
	PlayersInfosUI.Add(PlayerInfoWidget);
}

// ============================================================
//  HELPERS PRIVÉS
// ============================================================

void UUIMenu::UpdatePlayerCountText(int32 CurrentPlayers)
{
	if (!Txt_PlayerNb)
		return;

	const int32 MaxPlayers = GetMaxPlayersForGameMode(SelectedGameMode);
	Txt_PlayerNb->SetText(FText::FromString(
		FString::Printf(TEXT("%d/%d"), CurrentPlayers, MaxPlayers)));
	Txt_PlayerNb->SetColorAndOpacity(FLinearColor::White);
}

void UUIMenu::UpdateRoomStatusUI(bool bIsOpen, int32 CurrentPlayers)
{
	if (Txt_Status)
	{
		Txt_Status->SetText(FText::FromString(
			bIsOpen ? TEXT("Room Status : Open") : TEXT("Room Status : Closed")));
		Txt_Status->SetColorAndOpacity(bIsOpen ? FLinearColor::Green : FLinearColor::Red);
	}

	if (bIsOpen)
	{
		UpdatePlayerCountText(CurrentPlayers);
	}
	else
	{
		if (Txt_PlayerNb)
		{
			Txt_PlayerNb->SetText(FText::FromString(TEXT("0/0")));
			Txt_PlayerNb->SetColorAndOpacity(FLinearColor::White);
		}
	}
}

void UUIMenu::SyncCheckBoxVisuals()
{
	if (CheckBox_All) CheckBox_All->SetIsChecked(bCheckBoxAll);
	if (CheckBox_1V1) CheckBox_1V1->SetIsChecked(bCheckBox1V1);
	if (CheckBox_2V2) CheckBox_2V2->SetIsChecked(bCheckBox2V2);
	if (CheckBox_FFA) CheckBox_FFA->SetIsChecked(bCheckBoxFFA);
}

// ============================================================
//  DELEGATES SUBSYSTEM
// ============================================================

void UUIMenu::HandleFindSessionsCompleted(const TArray<FCustomSessionInfo>& Sessions, bool bWasSuccessful)
{
	if (!bWasSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("HandleFindSessionsCompleted: recherche echouee."));
		bIsQuickJoin = false;
		return;
	}

	FoundSessions = Sessions;

	// Quick Join : rejoint automatiquement la première session disponible
	if (bIsQuickJoin)
	{
		bIsQuickJoin = false;
		if (FoundSessions.Num() > 0)
		{
			SessionSubsystem->JoinLobby(FoundSessions[0]);
			// L'UI lobby sera affichée dans HandleSessionJoinCompleted
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("HandleFindSessionsCompleted: aucune session pour Quick Join."));
		}
		return;
	}

	// Affichage normal dans la liste Find Room
	if (!FindRoomScrollBox)
		return;

	FindRoomScrollBox->ClearChildren();
	RoomInfosUI.Empty();

	for (int32 i = 0; i < FoundSessions.Num(); i++)
	{
		const FCustomSessionInfo& Session = FoundSessions[i];
		if (!PassFilter(Session))
			continue;

		AddRoomInfoUI(
			Session.SessionName,
			GetGameModeID(Session.GameMode),
			Session.CurrentPlayers,
			Session.MaxPlayers,
			Session.Ping,
			i
		);
	}
}

bool UUIMenu::PassFilter(const FCustomSessionInfo& Session) const
{
	if (bCheckBoxAll)  return true;
	if (bCheckBox1V1 && Session.GameMode == LobbyConstants::GameMode_1V1) return true;
	if (bCheckBox2V2 && Session.GameMode == LobbyConstants::GameMode_2V2) return true;
	if (bCheckBoxFFA && Session.GameMode == LobbyConstants::GameMode_FFA) return true;
	return false;
}

void UUIMenu::HandleLobbyUpdated(const TArray<FPlayerLobbyInfo>& Players)
{
	if (!VB_PlayersInfos)
		return;

	VB_PlayersInfos->ClearChildren();
	PlayersInfosUI.Empty();

	for (const FPlayerLobbyInfo& Player : Players)
		AddPlayerInfoUI(Player);

	UpdatePlayerCountText(Players.Num());
}

void UUIMenu::HandleBeaconCreated(ALobbyBeaconClient* BeaconClient)
{
	// Le subsystem relaie déjà les mises à jour via OnLobbysUpdated → HandleLobbyUpdated.
	// Pas de binding direct supplémentaire ici pour éviter les callbacks dupliqués.
	if (!IsValid(BeaconClient))
	{
		UE_LOG(LogTemp, Error, TEXT("HandleBeaconCreated: BeaconClient invalide."));
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("HandleBeaconCreated: beacon client pret (%p)."), BeaconClient);
}

void UUIMenu::HandleSessionJoinCompleted(bool bWasSuccessful)
{
	if (Btn_FindRoom) Btn_FindRoom->SetIsEnabled(true);

	if (!bWasSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("HandleSessionJoinCompleted: echec de connexion au lobby."));
		ShowMainMenu();
		return;
	}

	// L'hôte reste sur son écran (déjà affiché via ShowCreateRoomSettings)
	// Seul le client bascule sur l'écran lobby en lecture seule
	if (SessionSubsystem && !SessionSubsystem->bIsHost)
	{
		ShowLobbyAsClient();
	}
}