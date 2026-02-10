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
}

void ACustomPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateAnimations();
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
	float MovementVector = Value.Get<float>();
	FVector Forward = MyPlayer->GetActorForwardVector();
	MyPlayer->AddMovementInput(Forward, MovementVector);

	if (MyPlayer->PlayerAnimState != EPlayerState::Jumping)
		MyPlayer->PlayerAnimState = EPlayerState::Running;

	float Dir = FMath::Sign(MovementVector);
	MyPlayer->GetSprite()->SetRelativeScale3D(FVector(Dir, 1.f, 1.f));
}

void ACustomPlayerController::Jump(const FInputActionValue& Value)
{
	//if (MyPlayer->GetCharacterMovement()->IsMovingOnGround())
	//	MyPlayer->LaunchCharacter({ 0.f, 0.f, MyPlayer->JumpVelocity }, false, true);
	//else if (!MyPlayer->HasDoubleJumped)
	//{
	//	MyPlayer->LaunchCharacter({ 0.f, 0.f, MyPlayer->JumpVelocity }, false, true);
	//	MyPlayer->HasDoubleJumped = true;
	//}

	MyPlayer->Jump();//Tmp
	MyPlayer->PlayerAnimState = EPlayerState::Jumping;
}

void ACustomPlayerController::UpdateAnimations()
{
	if (MyPlayer->GetVelocity().Z < 0)
		MyPlayer->PlayerAnimState = EPlayerState::Falling;

	if (FMath::IsNearlyZero(MyPlayer->GetVelocity().Length()))
		MyPlayer->PlayerAnimState = EPlayerState::Idle;

	switch (MyPlayer->PlayerAnimState)
	{
	case EPlayerState::Idle:
		MyPlayer->CurrentAnim = MyPlayer->IdleAnim;
		break;
	case EPlayerState::Running:
		MyPlayer->CurrentAnim = MyPlayer->RunAnim;
		break;
	case EPlayerState::Jumping:
		MyPlayer->CurrentAnim = MyPlayer->JumpAnim;
		break;
	case EPlayerState::Falling:
		MyPlayer->CurrentAnim = MyPlayer->FallAnim;
		break;
	default:
		MyPlayer->CurrentAnim = MyPlayer->IdleAnim;
		break;
	}

	MyPlayer->GetSprite()->SetFlipbook(MyPlayer->CurrentAnim);
}
