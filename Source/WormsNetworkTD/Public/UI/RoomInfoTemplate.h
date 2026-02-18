// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "RoomInfoTemplate.generated.h"

/**
 * 
 */
UCLASS()
class WORMSNETWORKTD_API URoomInfoTemplate : public UUserWidget
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Info")
	int32 RoomPing = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Info")
	FString RoomName = "RoomName";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Info")
	FString RoomModeText = "GameMode : 1V1";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Info")
	FString PlayersText = "Players : 1/2";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Info")
	int32 RoomModeID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Info")
	int32 PlayerInRoom = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Info")
	int32 MaxPlayerInRoom = 2;


	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_JoinLobby;

	//To Update the UI Ingame when values change
	UFUNCTION(BlueprintNativeEvent)
	void UpdateValues();
	void UpdateValues_Implementation();


};
