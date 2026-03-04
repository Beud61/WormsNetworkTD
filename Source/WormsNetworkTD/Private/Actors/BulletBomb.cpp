#include "Actors/BulletBomb.h"
#include "Actors/DestructibleMap.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"


ABulletBomb::ABulletBomb()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(12.f);
	CollisionSphere->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	CollisionSphere->SetNotifyRigidBodyCollision(true);
	RootComponent = CollisionSphere;


	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 0.f;
	ProjectileMovement->MaxSpeed = 3000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 1.5f;

	ProjectileMovement->bConstrainToPlane = true;
	ProjectileMovement->SetPlaneConstraintNormal(FVector(0.f, 1.f, 0.f));

	ProjectileMovement->bShouldBounce = false;
}

// ============================================================
//  BeginPlay
// ============================================================

void ABulletBomb::BeginPlay()
{
	Super::BeginPlay();


	if (HasAuthority())
	{
		CollisionSphere->OnComponentHit.AddDynamic(this, &ABulletBomb::OnHit);


		SetLifeSpan(LifeSpan);
	}


	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADestructibleMap::StaticClass(), Found);
	if (Found.Num() > 0)
	{
		DestructibleMap = Cast<ADestructibleMap>(Found[0]);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[BulletBomb] Aucune DestructibleMap trouvée dans le niveau !"));
	}
}

// ============================================================
//  Launch
// ============================================================

void ABulletBomb::Launch(FVector Direction)
{

	Direction.Y = 0.f;
	Direction.Normalize();

	ProjectileMovement->Velocity = Direction * LaunchSpeed;

	UE_LOG(LogTemp, Log, TEXT("[BulletBomb] Launch -> Dir=(%.2f, %.2f) Speed=%.0f"),
		Direction.X, Direction.Z, LaunchSpeed);
}

// ============================================================
//  OnHit — server only
// ============================================================

void ABulletBomb::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bHasExploded) return;
	bHasExploded = true;

	const FVector HitLocation = Hit.ImpactPoint;

	UE_LOG(LogTemp, Log, TEXT("[BulletBomb] Impact sur %s en (%.1f, %.1f, %.1f)"),
		OtherActor ? *OtherActor->GetName() : TEXT("World"),
		HitLocation.X, HitLocation.Y, HitLocation.Z);


	if (IsValid(DestructibleMap))
	{
		const FVector2D ExplosionPos(HitLocation.X, HitLocation.Z);
		DestructibleMap->ApplyExplosion(ExplosionPos, ExplosionRadius);
	}

	Destroy();
}