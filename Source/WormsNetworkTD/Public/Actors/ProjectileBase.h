#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectileBase.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

UCLASS()
class WORMSNETWORKTD_API AProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	AProjectileBase();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void InitVelocity(const FVector& LaunchVelocity);

protected:
	virtual void BeginPlay() override;

	// === COMPONENTS ===
	UPROPERTY(VisibleAnywhere)
	USphereComponent* Collision;
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Mesh;
	UPROPERTY(VisibleAnywhere)
	UProjectileMovementComponent* ProjectileMovement;

	// === SETTINGS ===
	UPROPERTY(EditDefaultsOnly, Category = "Explosion")
	float Damage = 50.f;
	UPROPERTY(EditDefaultsOnly, Category = "Explosion")
	float ExplosionRadius = 300.f;
	UPROPERTY(EditDefaultsOnly, Category = "Explosion")
	float KnockbackStrength = 1500.f;
	UPROPERTY(EditDefaultsOnly, Category = "Explosion")
	float FuseTime = 3.f;
	UPROPERTY(EditDefaultsOnly, Category = "Explosion")
	bool bExplodeOnImpact = false;

	// === REPLICATION ===
	UPROPERTY(Replicated)
	bool bHasExploded = false;

	// === LOGIC ===
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse,
		const FHitResult& Hit);

	void Explode();
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ExplodeFX();
	void ApplyExplosionDamage();
};