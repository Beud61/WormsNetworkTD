#include "Actors/CustomPaperCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include <Net/UnrealNetwork.h>
#include "Components/SplineComponent.h"
#include <Kismet/GameplayStaticsTypes.h>
#include <Kismet/GameplayStatics.h>

ACustomPaperCharacter::ACustomPaperCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	/* ================= NETWORK ================= */

	bReplicates = true;
	SetReplicateMovement(true);

	/* ================= 2D SETUP ================= */

	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->SetPlaneConstraintNormal(FVector(0.f, 1.f, 0.f));
	GetCharacterMovement()->bUseFlatBaseForFloorChecks = true;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	/* ================= MOVEMENT TUNING ================= */

	GetCharacterMovement()->GravityScale = 2.5f;
	GetCharacterMovement()->JumpZVelocity = 800.f;
	GetCharacterMovement()->AirControl = 0.8f;
	GetCharacterMovement()->MaxWalkSpeed = 600.f;
	GetCharacterMovement()->BrakingFrictionFactor = 2.f;

	/* ================= CAMERA ================= */

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 500.f;
	SpringArm->bDoCollisionTest = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);
	Camera->bUsePawnControlRotation = false;

	/* ================= PROJECTILES ================ */
	TrajectorySpline = CreateDefaultSubobject<USplineComponent>("TrajectorySpline");
	TrajectorySpline->SetupAttachment(RootComponent);
}

void ACustomPaperCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Le serveur décide des animations
	if (HasAuthority())
	{
		UpdateAnimations();
	}
}

void ACustomPaperCharacter::UpdateAnimations()
{
	if (!GetCharacterMovement())
		return;

	EPlayerState NewState = PlayerAnimState;

	if (!GetCharacterMovement()->IsMovingOnGround())
	{
		if (GetVelocity().Z > 0)
			NewState = EPlayerState::Jumping;
		else
			NewState = EPlayerState::Falling;
	}
	else
	{
		if (FMath::IsNearlyZero(GetVelocity().X))
			NewState = EPlayerState::Idle;
		else
			NewState = EPlayerState::Running;
	}

	if (NewState != PlayerAnimState)
	{
		PlayerAnimState = NewState;
		OnRep_PlayerAnimState(); // Mise à jour serveur immédiate
	}
}

void ACustomPaperCharacter::OnRep_PlayerAnimState()
{
	switch (PlayerAnimState)
	{
	case EPlayerState::Idle:
		if (IdleAnim) GetSprite()->SetFlipbook(IdleAnim);
		break;

	case EPlayerState::Running:
		if (RunAnim) GetSprite()->SetFlipbook(RunAnim);
		break;

	case EPlayerState::Jumping:
		if (JumpAnim) GetSprite()->SetFlipbook(JumpAnim);
		break;

	case EPlayerState::Falling:
		if (FallAnim) GetSprite()->SetFlipbook(FallAnim);
		break;

	default:
		break;
	}
}

void ACustomPaperCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACustomPaperCharacter, PlayerAnimState);
	DOREPLIFETIME(ACustomPaperCharacter, FacingDirection);
}

void ACustomPaperCharacter::OnRep_FacingDirection()
{
	GetSprite()->SetRelativeScale3D(FVector(FacingDirection, 1.f, 1.f));
}

FVector ACustomPaperCharacter::CalculateLaunchVelocity() const
{
	float Radians = FMath::DegreesToRadians(ThrowAngle);

	FVector LaunchVelocity;
	LaunchVelocity.X = FMath::Cos(Radians) * CurrentThrowSpeed * FacingDirection;
	LaunchVelocity.Z = FMath::Sin(Radians) * CurrentThrowSpeed;
	LaunchVelocity.Y = 0.f;

	return LaunchVelocity;
}

void ACustomPaperCharacter::Server_SetFacingDirection_Implementation(float NewDirection)
{
	FacingDirection = FMath::Sign(NewDirection);
	OnRep_FacingDirection();
}

void ACustomPaperCharacter::UpdateTrajectory()
{
	FPredictProjectilePathParams Params;

	Params.StartLocation = GetActorLocation() + FVector(0, 0, 50.f);
	Params.LaunchVelocity = CalculateLaunchVelocity();
	Params.bTraceWithCollision = true;
	Params.ProjectileRadius = 10.f;
	Params.MaxSimTime = 5.f;
	Params.SimFrequency = 20.f;
	Params.TraceChannel = ECC_Visibility;

	FPredictProjectilePathResult Result;

	bool bHit = UGameplayStatics::PredictProjectilePath(
		this,
		Params,
		Result
	);

	TrajectorySpline->ClearSplinePoints(false);

	for (const FPredictProjectilePathPointData& Point : Result.PathData)
	{
		TrajectorySpline->AddSplinePoint(
			Point.Location,
			ESplineCoordinateSpace::World,
			false
		);
	}

	TrajectorySpline->UpdateSpline();
}