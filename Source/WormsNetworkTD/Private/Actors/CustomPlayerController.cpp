// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/CustomPlayerController.h"

void ACustomPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!MappingContextBase)
	{
		return;
	}

	if (GetLocalPlayer())
	{
		if (TObjectPtr<UEnhancedInputLocalPlayerSubsystem> InputSystem = GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			InputSystem->AddMappingContext(MappingContextBase, 0);
		}
	}

	MyPlayer = Cast<ACustomPaperCharacter>(GetPawn());
	ShowMainMenu();
}

void ACustomPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ACustomPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (TObjectPtr<UEnhancedInputComponent> EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		for (FInputActionSetup i : IA_Setup)
		{
			EnhancedInputComponent->BindAction(i.Action, i.Event, this, i.ActionName.GetMemberName());
		}
	}
}

void ACustomPlayerController::Move(const FInputActionValue& Value)
{
	float Movement = Value.Get<float>();
	if (!MyPlayer) return;

	MyPlayer->AddMovementInput(FVector::ForwardVector, Movement);

	if (Movement != 0.f)
	{
		MyPlayer->Server_SetFacingDirection(Movement);
	}
}

void ACustomPlayerController::Jump(const FInputActionValue& Value)
{
	if (!MyPlayer) return;
	MyPlayer->Jump();
}

void ACustomPlayerController::ShowMainMenu()
{
	if (!MenuWidgetClass) return;

	if (!MenuWidgetInstance)
	{
		MenuWidgetInstance = CreateWidget<UUIMenu>(this, MenuWidgetClass);
	}

	if (MenuWidgetInstance)
	{
		MenuWidgetInstance->AddToViewport();
	}
}

void ACustomPlayerController::HideMainMenu()
{
	if (MenuWidgetInstance)
	{
		MenuWidgetInstance->CloseMenu();
	}
}