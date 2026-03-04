#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "InputAction.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "CustomPaperCharacter.h"
#include "UI/UIMenu.h"
#include "WormsGameInstance.h"
#include "Actors/BulletBomb.h"
#include "CustomPlayerController.generated.h"

USTRUCT(BlueprintType)
struct FInputActionSetup
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inputs")
	TObjectPtr<UInputAction> Action;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inputs")
	ETriggerEvent Event = ETriggerEvent::Triggered;

	UPROPERTY(EditAnywhere, meta = (FunctionReference, PrototypeFunction = "/Script/WormsNetworkTD.CustomPlayerController.Prototype_InputAction"))
	FMemberReference ActionName;
};

UCLASS()
class WORMSNETWORKTD_API ACustomPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupInputComponent() override;

	virtual void ClientTravelInternal_Implementation(const FString& URL,
		ETravelType TravelType, bool bSeamless,
		const FGuid& MapPackageGuid);


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inputs")
	TObjectPtr<class UInputMappingContext> MappingContextBase = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inputs")
	TArray<FInputActionSetup> IA_Setup;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inputs")
	TObjectPtr<UInputAction> IA_Fire;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data")
	TObjectPtr<ACustomPaperCharacter> MyPlayer = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUIMenu> MenuWidgetClass;

	UPROPERTY()
	TObjectPtr<UUIMenu> MenuWidgetInstance;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<ABulletBomb> BulletBombClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	float SpawnOffset = 60.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	float FireCooldown = 0.5f;

private:
	float LastFireTime = -999.f;

#if WITH_EDITOR
	UFUNCTION(BlueprintInternalUseOnly)
	void Prototype_InputAction(const FInputActionValue& Value) {};
#endif

public:


	UFUNCTION(BlueprintCallable)
	void Move(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable)
	void Jump(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void Fire(const FInputActionValue& Value);


	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowMainMenu();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void HideMainMenu();

	UFUNCTION(Client, Reliable)
	void Client_NotifyGameStarting();

private:

	void SpawnProjectile(FVector ProjectileSpawnPos, FVector Direction);
};