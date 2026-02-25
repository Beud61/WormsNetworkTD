#include "Actors/ProjectileBase.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

AProjectileBase::AProjectileBase()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicateMovement(true);

	// === Collision ===
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	RootComponent = Collision;
	Collision->InitSphereRadius(15.f);
	Collision->SetCollisionProfileName("BlockAll");
	Collision->OnComponentHit.AddDynamic(this, &AProjectileBase::OnHit);

	// === Mesh ===
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// === Movement ===
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = true;
	ProjectileMovement->Bounciness = 0.6f;
	ProjectileMovement->ProjectileGravityScale = 1.f;
}

void AProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		if (FuseTime > 0.f)
		{
			GetWorldTimerManager().SetTimerForNextTick([this]()
				{
					FTimerHandle Timer;
					GetWorldTimerManager().SetTimer(
						Timer,
						this,
						&AProjectileBase::Explode,
						FuseTime,
						false
					);
				});
		}
	}
}

void AProjectileBase::InitVelocity(const FVector& LaunchVelocity)
{
	if (ProjectileMovement)
	{
		ProjectileMovement->Velocity = LaunchVelocity;
	}
}

void AProjectileBase::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (!HasAuthority())
		return;

	if (bExplodeOnImpact && !bHasExploded)
	{
		Explode();
	}
}

void AProjectileBase::Explode()
{
	if (!HasAuthority() || bHasExploded)
		return;

	bHasExploded = true;

	Multicast_ExplodeFX();
	ApplyExplosionDamage();

	Destroy();
}

void AProjectileBase::ApplyExplosionDamage()
{
	TArray<AActor*> Ignored;
	Ignored.Add(this);

	UGameplayStatics::ApplyRadialDamage(
		this,
		Damage,
		GetActorLocation(),
		ExplosionRadius,
		nullptr,
		Ignored,
		this,
		GetInstigatorController(),
		true
	);

	// Knockback
	TArray<FOverlapResult> Overlaps;

	FCollisionShape Sphere = FCollisionShape::MakeSphere(ExplosionRadius);

	bool bHit = GetWorld()->OverlapMultiByChannel(
		Overlaps,
		GetActorLocation(),
		FQuat::Identity,
		ECC_Pawn,
		Sphere
	);

	if (bHit)
	{
		for (auto& Result : Overlaps)
		{
			if (AActor* HitActor = Result.GetActor())
			{
				UPrimitiveComponent* RootComp = Cast<UPrimitiveComponent>(HitActor->GetRootComponent());

				if (RootComp && RootComp->IsSimulatingPhysics())
				{
					FVector Direction = (HitActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
					RootComp->AddImpulse(Direction * KnockbackStrength);
				}
			}
		}
	}
}

void AProjectileBase::Multicast_ExplodeFX_Implementation()
{
	// Ici tu peux spawn particules / son
	UE_LOG(LogTemp, Warning, TEXT("BOOM"));
}

void AProjectileBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AProjectileBase, bHasExploded);
}