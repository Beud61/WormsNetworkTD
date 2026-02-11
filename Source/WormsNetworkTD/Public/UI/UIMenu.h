#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "UIMenu.generated.h"


UCLASS()
class WORMSNETWORKTD_API UUIMenu : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

public:

	// Boutons
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


	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_Status;


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


	UFUNCTION(BlueprintCallable, Category = "Menu")
	void SetupMenu();


	UFUNCTION(BlueprintCallable, Category = "Menu")
	void CloseMenu();
};