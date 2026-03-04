#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "BulletBomb.generated.h"

class ADestructibleMap;

UCLASS()
class WORMSNETWORKTD_API ABulletBomb : public AActor
{
	GENERATED_BODY()

public:
	ABulletBomb();

protected:
	virtual void BeginPlay() override;

public:


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;


	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile")
	float LaunchSpeed = 1500.f;


	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile")
	float ExplosionRadius = 150.f;


	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile")
	float LifeSpan = 8.f;


	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void Launch(FVector Direction);

private:


	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse,
		const FHitResult& Hit);


	UPROPERTY()
	TObjectPtr<ADestructibleMap> DestructibleMap;


	bool bHasExploded = false;
};