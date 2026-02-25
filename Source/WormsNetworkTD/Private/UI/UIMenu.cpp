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

		// HandleBeaconCreated est connecté ici (une seule fois) pour rebinder OnLobbyUpdated
		// si le BeaconClient est recréé après un CustomJoinSession
		SessionSubsystem->OnBeaconClientCreated.AddDynamic(this, &UUIMenu::HandleBeaconCreated);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UUIMenu::NativeConstruct: SessionSubsystem introuvable."));
	}

	SetupMenu();
}

// ============================================================
//  Setup des bindings
// ============================================================

void UUIMenu::SetupMenu()
{
	// === MENU PRINCIPAL ===
	if (Btn_CreateRoom)
		Btn_CreateRoom->OnClicked.AddDynamic(this, &UUIMenu::OnCreateRoomClicked);

	if (Btn_JoinRoom)
		Btn_JoinRoom->OnClicked.AddDynamic(this, &UUIMenu::OnJoinRoomClicked);

	if (Btn_FindRoom)
		Btn_FindRoom->OnClicked.AddDynamic(this, &UUIMenu::OnFindRoomClicked);

	if (Btn_Settings)
		Btn_Settings->OnClicked.AddDynamic(this, &UUIMenu::OnSettingsClicked);

	if (Btn_Quit)
		Btn_Quit->OnClicked.AddDynamic(this, &UUIMenu::OnQuitClicked);

	// === CREATE ROOM ===
	if (Btn_CloseCreateRoomSettings)
		Btn_CloseCreateRoomSettings->OnClicked.AddDynamic(this, &UUIMenu::OnCloseCreateRoomSettingsClicked);

	if (Btn_OpenRoom)
		Btn_OpenRoom->OnClicked.AddDynamic(this, &UUIMenu::OnOpenRoomClicked);

	if (Btn_CloseRoom)
		Btn_CloseRoom->OnClicked.AddDynamic(this, &UUIMenu::OnCloseRoomClicked);

	if (Btn_StartGame)
		Btn_StartGame->OnClicked.AddDynamic(this, &UUIMenu::OnStartGameClicked);

	if (GameModeChoice)
		GameModeChoice->OnSelectionChanged.AddDynamic(this, &UUIMenu::OnGameModeChanged);

	if (WaterRisingChoice)
		WaterRisingChoice->OnSelectionChanged.AddDynamic(this, &UUIMenu::OnWaterRisingChanged);

	if (UnitLifeChoice)
		UnitLifeChoice->OnSelectionChanged.AddDynamic(this, &UUIMenu::OnUnitLifeChanged);

	if (UnitCountChoice)
		UnitCountChoice->OnSelectionChanged.AddDynamic(this, &UUIMenu::OnUnitCountChanged);

	// Btn_QuitLobby : bindé ici une seule fois, pas dans HideRoomSettingsForJoiningPlayer
	if (Btn_QuitLobby)
		Btn_QuitLobby->OnClicked.AddDynamic(this, &UUIMenu::OnQuitLobbyClicked);

	// === FIND ROOM ===
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

	// Affiche le menu principal et le curseur
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
	// Quick Join : on cherche les sessions et on rejoint la première trouvée
	bIsQuickJoin = true;
	if (SessionSubsystem)
	{
		SessionSubsystem->FindSessions(LobbyConstants::MaxSearchResults, true);
	}
}

void UUIMenu::OnFindRoomClicked()
{
	ShowFindRoom();

	// Réinitialise le filtre sur "Tous"
	OnCheckBoxAllClicked(true);

	if (SessionSubsystem)
	{
		SessionSubsystem->FindSessions(LobbyConstants::MaxSearchResults, true);
	}
}

void UUIMenu::OnSettingsClicked()
{
	// TODO: implémenter le panneau de settings
	UE_LOG(LogTemp, Warning, TEXT("Settings: non implemente."));
}

void UUIMenu::OnQuitClicked()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		UKismetSystemLibrary::QuitGame(GetWorld(), PC, EQuitPreference::Quit, false);
	}
}

// ============================================================
//  CALLBACKS — CREATE ROOM
// ============================================================

void UUIMenu::OnCloseCreateRoomSettingsClicked()
{
	ShowMainMenu();
}

void UUIMenu::OnOpenRoomClicked()
{
	if (!SessionSubsystem)
		return;

	const int32 MaxPlayers = GetMaxPlayersForGameMode(SelectedGameMode);

	// Prépare les infos de l'hôte AVANT CreateSession() pour qu'elles soient
	// disponibles dès que le beacon host est prêt et se connecte à lui-même.
	// TODO: remplacer PlayerName par le vrai nom depuis le GameInstance / profil.
	FPlayerLobbyInfo HostInfo;
	HostInfo.PlayerName = TEXT("Player 1");
	HostInfo.UnitNB = SelectedUnitCount;
	HostInfo.ProfileIcon = 0;
	HostInfo.TeamIcon = 0;
	HostInfo.PlayerId = static_cast<int32>(FPlatformTime::Cycles() & 0x7FFFFFFF);
	SessionSubsystem->SetHostPlayerInfo(HostInfo);

	SessionSubsystem->CreateSession(
		TEXT("MyGameSession"),
		MaxPlayers,
		true,
		SelectedGameMode,
		SelectedUnitLife,
		SelectedUnitCount,
		SelectedTurnsBeforeWater
	);

	// L'UI des joueurs sera peuplée par HandleLobbyUpdated() dès que le beacon
	// de l'hôte aura validé sa connexion locale et diffusé ConnectedPlayers.
	// On n'ajoute donc plus le widget hôte manuellement ici.

	// Statut de la room : en attente de la confirmation beacon (on reste à 0
	// joueur visuellement jusqu'au premier HandleLobbyUpdated).
	UpdateRoomStatusUI(true, 0);

	// Passe en mode "room ouverte" : verrouille les settings, affiche Fermer
	if (Settings)
		Settings->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (Btn_OpenRoom)
		Btn_OpenRoom->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (Btn_CloseRoom)
		Btn_CloseRoom->SetVisibility(ESlateVisibility::Visible);
}

void UUIMenu::OnCloseRoomClicked()
{
	// TODO : kick des joueurs présents dans la room avant de détruire

	if (VB_PlayersInfos)
		VB_PlayersInfos->ClearChildren();
	PlayersInfosUI.Empty();
	FoundSessions.Empty();

	UpdateRoomStatusUI(false, 0);

	// Remet la room en mode "fermé"
	if (Settings)
		Settings->SetVisibility(ESlateVisibility::Visible);
	if (Btn_OpenRoom)
		Btn_OpenRoom->SetVisibility(ESlateVisibility::Visible);
	if (Btn_CloseRoom)
		Btn_CloseRoom->SetVisibility(ESlateVisibility::HitTestInvisible);

	if (SessionSubsystem)
		SessionSubsystem->DestroySession();
}

void UUIMenu::OnStartGameClicked()
{
	// TODO : vérifier que la room est pleine avant de lancer la partie.
	CloseMenu();
	UWorld* World = GetWorld();
	if (!World) return;

	// Notifie tous les PlayerControllers connectés AVANT le ServerTravel.
	// Chaque PC pose son flag bGameStarted et cache son menu via le RPC
	// Client_NotifyGameStarting, ce qui évite que BeginPlay recrée le menu
	// sur la nouvelle map (le PlayerController survit au travel non-seamless).
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (ACustomPlayerController* PC = Cast<ACustomPlayerController>(It->Get()))
		{
			PC->Client_NotifyGameStarting();
		}
	}


	World->ServerTravel(TEXT("/Game/Maps/Lobby?listen"));
}

void UUIMenu::OnQuitLobbyClicked()
{
	// TODO : envoyer un message au host pour libérer le slot, puis nettoyer
	UE_LOG(LogTemp, Warning, TEXT("OnQuitLobbyClicked: quitter le lobby."));
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
	OnCheckBoxAllClicked(true);
}

void UUIMenu::OnCheckBoxAllClicked(bool bIsChecked)
{
	bCheckBoxAll = true;
	bCheckBox1V1 = false;
	bCheckBox2V2 = false;
	bCheckBoxFFA = false;

	if (CheckBox_All) CheckBox_All->SetIsChecked(bCheckBoxAll);
	if (CheckBox_1V1) CheckBox_1V1->SetIsChecked(bCheckBox1V1);
	if (CheckBox_2V2) CheckBox_2V2->SetIsChecked(bCheckBox2V2);
	if (CheckBox_FFA) CheckBox_FFA->SetIsChecked(bCheckBoxFFA);
}

void UUIMenu::OnCheckBox1V1Clicked(bool bIsChecked)
{
	bCheckBox1V1 = true;
	bCheckBoxAll = false;

	// Si tous les modes spécifiques sont cochés -> retombe sur "Tous"
	if (bCheckBox1V1 && bCheckBox2V2 && bCheckBoxFFA)
	{
		OnCheckBoxAllClicked(true);
		return;
	}

	if (CheckBox_All) CheckBox_All->SetIsChecked(bCheckBoxAll);
	if (CheckBox_1V1) CheckBox_1V1->SetIsChecked(bCheckBox1V1);
	if (CheckBox_2V2) CheckBox_2V2->SetIsChecked(bCheckBox2V2);
	if (CheckBox_FFA) CheckBox_FFA->SetIsChecked(bCheckBoxFFA);
}

void UUIMenu::OnCheckBox2V2Clicked(bool bIsChecked)
{
	bCheckBox2V2 = true;
	bCheckBoxAll = false;

	if (bCheckBox1V1 && bCheckBox2V2 && bCheckBoxFFA)
	{
		OnCheckBoxAllClicked(true);
		return;
	}

	if (CheckBox_All) CheckBox_All->SetIsChecked(bCheckBoxAll);
	if (CheckBox_1V1) CheckBox_1V1->SetIsChecked(bCheckBox1V1);
	if (CheckBox_2V2) CheckBox_2V2->SetIsChecked(bCheckBox2V2);
	if (CheckBox_FFA) CheckBox_FFA->SetIsChecked(bCheckBoxFFA);
}

void UUIMenu::OnCheckBoxFFAClicked(bool bIsChecked)
{
	bCheckBoxFFA = true;
	bCheckBoxAll = false;

	if (bCheckBox1V1 && bCheckBox2V2 && bCheckBoxFFA)
	{
		OnCheckBoxAllClicked(true);
		return;
	}

	if (CheckBox_All) CheckBox_All->SetIsChecked(bCheckBoxAll);
	if (CheckBox_1V1) CheckBox_1V1->SetIsChecked(bCheckBox1V1);
	if (CheckBox_2V2) CheckBox_2V2->SetIsChecked(bCheckBox2V2);
	if (CheckBox_FFA) CheckBox_FFA->SetIsChecked(bCheckBoxFFA);
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

	SelectedSessionIndex = Index;
	SessionSubsystem->CustomJoinSession(FoundSessions[SelectedSessionIndex]);
	HideRoomSettingsForJoiningPlayer();
}

// ============================================================
//  FONCTIONS UTILITAIRES — Affichage des panneaux
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
	if (HostSettingsSecurity) HostSettingsSecurity->SetVisibility(ESlateVisibility::Collapsed);
	if (Btn_QuitLobby)        Btn_QuitLobby->SetVisibility(ESlateVisibility::Collapsed);
	if (Btn_StartGame)        Btn_StartGame->SetVisibility(ESlateVisibility::Visible);
	if (Btn_CloseCreateRoomSettings) Btn_CloseCreateRoomSettings->SetVisibility(ESlateVisibility::Visible);

	// Statut initial : room fermée
	UpdateRoomStatusUI(false, 0);
}

void UUIMenu::ShowFindRoom()
{
	if (MenuPanel)          MenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	if (FindRoom)           FindRoom->SetVisibility(ESlateVisibility::Visible);
	if (CreateRoomSettings) CreateRoomSettings->SetVisibility(ESlateVisibility::Collapsed);
}

void UUIMenu::HideRoomSettingsForJoiningPlayer()
{
	// Réutilise ShowCreateRoomSettings pour la base, puis adapte pour un non-hôte
	ShowCreateRoomSettings();

	// Masque les contrôles réservés à l'hôte
	if (HostSettingsSecurity) HostSettingsSecurity->SetVisibility(ESlateVisibility::Visible);
	if (Btn_QuitLobby)        Btn_QuitLobby->SetVisibility(ESlateVisibility::Visible);
	// Btn_QuitLobby est déjà bindé dans SetupMenu(), pas de AddDynamic ici
	if (Btn_StartGame)        Btn_StartGame->SetVisibility(ESlateVisibility::Collapsed);
	if (Btn_CloseCreateRoomSettings) Btn_CloseCreateRoomSettings->SetVisibility(ESlateVisibility::Collapsed);

	// Room déjà ouverte côté hôte
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
	UE_LOG(LogTemp, Warning, TEXT("UIMenu: menu ferme, retour au mode jeu."));
}

// ============================================================
//  FONCTIONS UTILITAIRES — Room info UI
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

	// Texte du mode de jeu via les constantes partagées
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
//  FONCTIONS UTILITAIRES — Player info UI
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
		FString::Printf(TEXT("%d/%d"), CurrentPlayers, MaxPlayers)
	));
	Txt_PlayerNb->SetColorAndOpacity(FLinearColor::White);
}

void UUIMenu::UpdateRoomStatusUI(bool bIsOpen, int32 CurrentPlayers)
{
	if (Txt_Status)
	{
		Txt_Status->SetText(FText::FromString(
			bIsOpen ? TEXT("Room Status : Open") : TEXT("Room Status : Closed")
		));
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

// ============================================================
//  CALLBACKS DELEGATES — Session / Beacon
// ============================================================

void UUIMenu::HandleFindSessionsCompleted(const TArray<FCustomSessionInfo>& Sessions, bool bWasSuccessful)
{
	if (!bWasSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("HandleFindSessionsCompleted: recherche echouee."));
		return;
	}

	FoundSessions = Sessions;

	// Quick Join : rejoindre automatiquement la première session disponible
	if (bIsQuickJoin)
	{
		bIsQuickJoin = false;
		if (FoundSessions.Num() > 0)
		{
			SessionSubsystem->CustomJoinSession(FoundSessions[0]);
			HideRoomSettingsForJoiningPlayer();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("HandleFindSessionsCompleted: aucune session pour Quick Join."));
		}
		return;
	}

	// Affichage normal dans la liste
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

	// Reconstruit l'UI de la liste des joueurs
	VB_PlayersInfos->ClearChildren();
	PlayersInfosUI.Empty();

	for (const FPlayerLobbyInfo& Player : Players)
	{
		AddPlayerInfoUI(Player);
	}

	UpdatePlayerCountText(Players.Num());
}

void UUIMenu::HandleBeaconCreated(ALobbyBeaconClient* BeaconClient)
{
	// BeaconClient est fourni directement par le delegate OnBeaconClientCreated.
	// On rebinde OnLobbyUpdated au cas où le client aurait été recréé.
	if (!IsValid(BeaconClient))
	{
		UE_LOG(LogTemp, Error, TEXT("HandleBeaconCreated: BeaconClient invalide."));
		return;
	}

	// Évite de doubler le binding si HandleLobbyUpdated est déjà connecté via OnLobbysUpdated
	// (le subsystem relaie déjà via HandleLobbyUpdated_Internal -> OnLobbysUpdated -> HandleLobbyUpdated)
	// Ce binding direct est conservé comme filet de sécurité si le subsystem n'est pas disponible.
	if (!BeaconClient->OnLobbyUpdated.IsAlreadyBound(this, &UUIMenu::HandleLobbyUpdated))
	{
		BeaconClient->OnLobbyUpdated.AddDynamic(this, &UUIMenu::HandleLobbyUpdated);
	}

	UE_LOG(LogTemp, Warning, TEXT("HandleBeaconCreated: beacon client binde (%p)."), BeaconClient);
}