#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "BattleManager.generated.h"

class ACustomPaperCharacter;
class ACustomPlayerController;

// ============================================================
//  Struct : équipe d'un joueur
// ============================================================

USTRUCT(BlueprintType)
struct FPlayerTeam
{
	GENERATED_BODY()


	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	TObjectPtr<ACustomPlayerController> Controller;


	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	TArray<TObjectPtr<ACustomPaperCharacter>> Units;
};

// ============================================================
//  Delegates
// ============================================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTurnStarted, ACustomPaperCharacter*, ActiveUnit, float, Duration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBattleFinished, ACustomPlayerController*, Winner);

// ============================================================
//  ABattleManager
// ============================================================

UCLASS()
class WORMSNETWORKTD_API ABattleManager : public AActor
{
	GENERATED_BODY()

public:
	ABattleManager();

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	virtual void Tick(float DeltaTime) override;



	//Use after spawning units and before battle start
	UFUNCTION(BlueprintCallable, Category = "Battle")
	void RegisterPlayerTeam(ACustomPlayerController* Controller, const TArray<ACustomPaperCharacter*>& Units);

	UFUNCTION(BlueprintCallable, Category = "Battle")
	void StartBattle();


	UFUNCTION(BlueprintCallable, Category = "Battle")
	void EndCurrentTurn();


	UFUNCTION(BlueprintCallable, Category = "Battle")
	void OnUnitDied(ACustomPaperCharacter* DeadUnit);


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|Settings")
	float TurnDuration = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|Settings")
	float CameraBlendTime = 0.6f;

	UPROPERTY(ReplicatedUsing = OnRep_ActiveUnit, BlueprintReadOnly, Category = "Battle|State")
	TObjectPtr<ACustomPaperCharacter> ActiveUnit;


	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Battle|State")
	float TurnTimeRemaining = 0.f;


	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Battle|State")
	bool bBattleActive = false;




	UPROPERTY(BlueprintAssignable, Category = "Battle|Events")
	FOnTurnStarted OnTurnStarted;


	UPROPERTY(BlueprintAssignable, Category = "Battle|Events")
	FOnBattleFinished OnBattleFinished;

protected:


	UPROPERTY()
	TArray<FPlayerTeam> PlayerTeams;

	//Order : [J1_U1, J2_U1, J3_U1, J1_U2, J2_U2, J3_U2,...]
	UPROPERTY()
	TArray<TObjectPtr<ACustomPaperCharacter>> TurnQueue;


	int32 CurrentQueueIndex = INDEX_NONE;

private:



	void BuildTurnQueue();


	void AdvanceTurn();


	void ActivateTurn(ACustomPaperCharacter* Unit);


	ACustomPlayerController* CheckWinCondition() const;


	bool TeamHasAliveUnits(const FPlayerTeam& Team) const;



	UFUNCTION()
	void OnRep_ActiveUnit();

	//RPCs

	/**
	 * Notifie all clients that a new turn has started :
	 *  - put cams on active client
	 *  - turn on input only for playing client
	 */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_TurnStarted(ACustomPaperCharacter* NewActiveUnit, float Duration);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_BattleEnded(ACustomPlayerController* Winner);
};