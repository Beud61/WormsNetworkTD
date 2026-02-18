// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UserInfoTemplate.generated.h"

/**
 * 
 */
UCLASS()
class WORMSNETWORKTD_API UUserInfoTemplate : public UUserWidget
{
	GENERATED_BODY()
	
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Info")
    int32 UnitNB;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Info")
    FString PlayerName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Info")
	int32 ProfileIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Info")
	int32 TeamIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Info")
	int32 PlayerId;

};
