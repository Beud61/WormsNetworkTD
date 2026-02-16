#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ComboBoxString.h"
#include "Components/CanvasPanel.h"
#include "Components/Overlay.h"
#include "Components/VerticalBox.h"
#include "UI/UserInfoTemplate.h"
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


	// Variables pour stocker les choix
	UPROPERTY(BlueprintReadOnly, Category = "Settings")
	FString SelectedGameMode = "1v1";

	UPROPERTY(BlueprintReadOnly, Category = "Settings")
	int32 SelectedTurnsBeforeWater = 10;

	UPROPERTY(BlueprintReadOnly, Category = "Settings")
	int32 SelectedUnitLife = 100;

	UPROPERTY(BlueprintReadOnly, Category = "Settings")
	int32 SelectedUnitCount = 1;




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

	// === FONCTIONS UTILITAIRES ===
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void SetupMenu();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void CloseMenu();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void ShowMainMenu();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void ShowCreateRoomSettings();
};