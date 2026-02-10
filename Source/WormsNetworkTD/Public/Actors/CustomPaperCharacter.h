// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PaperCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "PaperFlipbookComponent.h"
#include "CustomPaperCharacter.generated.h"


UENUM(BlueprintType)
enum class EPlayerState : uint8
{
	Idle     UMETA(DisplayName = "Idle"),
	Running  UMETA(DisplayName = "Running"),
	Jumping  UMETA(DisplayName = "Jumping"),
	Falling  UMETA(DisplayName = "Falling")
};

/**
 * 
 */
UCLASS()
class WORMSNETWORKTD_API ACustomPaperCharacter : public APaperCharacter
{
	GENERATED_BODY()

protected:

	ACustomPaperCharacter();


public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprite")
	EPlayerState PlayerAnimState = EPlayerState::Idle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprite")
	TObjectPtr<UPaperFlipbook> IdleAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprite")
	TObjectPtr<UPaperFlipbook> RunAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprite")
	TObjectPtr<UPaperFlipbook> JumpAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprite")
	TObjectPtr<UPaperFlipbook> FallAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprite")
	TObjectPtr<UPaperFlipbook> CurrentAnim;

};
