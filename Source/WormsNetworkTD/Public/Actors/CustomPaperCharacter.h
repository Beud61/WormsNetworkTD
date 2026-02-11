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

UCLASS()
class WORMSNETWORKTD_API ACustomPaperCharacter : public APaperCharacter
{
	GENERATED_BODY()

public:

	ACustomPaperCharacter();

	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

	void UpdateAnimations();

	UFUNCTION()
	void OnRep_PlayerAnimState();

public:

	/* ================= CAMERA ================= */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	/* ================= ANIMATION ================= */

	UPROPERTY(ReplicatedUsing = OnRep_PlayerAnimState, BlueprintReadOnly, Category = "Sprite")
	EPlayerState PlayerAnimState = EPlayerState::Idle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprite")
	TObjectPtr<UPaperFlipbook> IdleAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprite")
	TObjectPtr<UPaperFlipbook> RunAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprite")
	TObjectPtr<UPaperFlipbook> JumpAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprite")
	TObjectPtr<UPaperFlipbook> FallAnim;

	UPROPERTY(ReplicatedUsing = OnRep_FacingDirection)
	float FacingDirection = 1.f;

	UFUNCTION()
	void OnRep_FacingDirection();

	UFUNCTION(Server, Reliable)
	void Server_SetFacingDirection(float NewDirection);
};