#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ComboBoxString.h"
#include "Components/CanvasPanel.h"
#include "Components/Overlay.h"
#include "Components/VerticalBox.h"
#include "Components/CheckBox.h"
#include "Components/ScrollBox.h"
#include "UI/UserInfoTemplate.h"
#include "UI/RoomInfoTemplate.h"
#include "Network/OnlineSessionSubsystem.h"
#include "UIMenu.generated.h"


UCLASS()
class WORMSNETWORKTD_API UUIMenu : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

public:

	// === MENU PRINCIPAL ===
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> MenuPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_CreateRoom;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_JoinRoom;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_FindRoom;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Settings;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Quit;

	// === CREATE ROOM SETTINGS ===
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> CreateRoomSettings;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> Settings;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_CloseCreateRoomSettings;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_OpenRoom;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_CloseRoom;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_StartGame;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_Status;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_PlayerNb;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> GameModeChoice;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> WaterRisingChoice;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> UnitLifeChoice;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> UnitCountChoice;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VB_PlayersInfos;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> PLayerInfoWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TArray<TObjectPtr<UUserInfoTemplate>> PlayersInfosUI;


	// === FIND ROOM SETTINGS === //

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_CloseFindRoom;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Refresh;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> CheckBox_All;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> CheckBox_1V1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> CheckBox_2V2;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> CheckBox_FFA;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> FindRoomScrollBox;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> RoomInfoWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TArray<TObjectPtr<URoomInfoTemplate>> RoomInfosUI;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> FindRoom;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	bool bCheckBoxAll;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	bool bCheckBox1V1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	bool bCheckBox2V2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	bool bCheckBoxFFA;

	// Variables pour stocker les choix
	UPROPERTY(BlueprintReadOnly, Category = "Settings")
	FString SelectedGameMode = "1v1";

	UPROPERTY(BlueprintReadOnly, Category = "Settings")
	int32 SelectedTurnsBeforeWater = 10;

	UPROPERTY(BlueprintReadOnly, Category = "Settings")
	int32 SelectedUnitLife = 100;

	UPROPERTY(BlueprintReadOnly, Category = "Settings")
	int32 SelectedUnitCount = 1;

	// Game instance pour acc�der au subsystem
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UOnlineSessionSubsystem> SessionSubsystem;

	// === CALLBACKS MENU PRINCIPAL ===
	UFUNCTION()
	void OnCreateRoomClicked();

	UFUNCTION()
	void OnJoinRoomClicked();

	UFUNCTION()
	void OnFindRoomClicked();

	UFUNCTION()
	void OnSettingsClicked();

	UFUNCTION()
	void OnQuitClicked();

	// === CALLBACKS CREATE ROOM ===
	UFUNCTION()
	void OnCloseCreateRoomSettingsClicked();

	UFUNCTION()
	void OnOpenRoomClicked();

	UFUNCTION()
	void OnCloseRoomClicked();

	UFUNCTION()
	void OnStartGameClicked();

	UFUNCTION()
	void OnGameModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void OnWaterRisingChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void OnUnitLifeChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void OnUnitCountChanged(FString SelectedItem, ESelectInfo::Type SelectionType);



	// === CALLBACKS FIND ROOM ===
	UFUNCTION()
	void OnCloseFindRoomClicked();

	UFUNCTION()
	void OnCheckBoxAllClicked(bool bIsChecked);

	UFUNCTION()
	void OnCheckBox1V1Clicked(bool bIsChecked);

	UFUNCTION()
	void OnCheckBox2V2Clicked(bool bIsChecked);

	UFUNCTION()
	void OnCheckBoxFFAClicked(bool bIsChecked);

	UFUNCTION()
	void OnRefreshRoomsClicked();





	// === FONCTIONS UTILITAIRES ===
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void SetupMenu();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void CloseMenu();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void ShowMainMenu();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void ShowCreateRoomSettings();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void ShowFindRoom();

	UFUNCTION(BlueprintCallable, Category = "Room")
	void AddRoomInfoUI(FString RoomName, int32 RoomModeID, int32 PlayerInRoom, int32 MaxPlayerInRoom, int32 RoomPing);

	UFUNCTION()
	int32 GetMaxPlayersFromGameMode() const;
};