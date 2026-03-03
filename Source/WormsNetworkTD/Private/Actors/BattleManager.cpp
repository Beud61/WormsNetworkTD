#include "Actors/BattleManager.h"
#include "Actors/CustomPaperCharacter.h"
#include "Actors/CustomPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"

// ============================================================
//  Constructor / Lifecycle
// ============================================================

ABattleManager::ABattleManager()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
}

void ABattleManager::BeginPlay()
{
	Super::BeginPlay();
}

void ABattleManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABattleManager, ActiveUnit);
	DOREPLIFETIME(ABattleManager, TurnTimeRemaining);
	DOREPLIFETIME(ABattleManager, bBattleActive);
}

// ============================================================
//  Tick - timer (server only)
// ============================================================

void ABattleManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority() || !bBattleActive)
		return;

	TurnTimeRemaining -= DeltaTime;
	TurnTimeRemaining = FMath::Max(TurnTimeRemaining, 0.f);

	if (TurnTimeRemaining <= 0.f)
	{
		UE_LOG(LogTemp, Log, TEXT("[BattleManager] Timer écoulé pour %s -> tour suivant."), ActiveUnit ? *ActiveUnit->GetName() : TEXT("NULL"));
		EndCurrentTurn();
	}
}

// ============================================================
//  Registration
// ============================================================

void ABattleManager::RegisterPlayerTeam(ACustomPlayerController* Controller,
	const TArray<ACustomPaperCharacter*>& Units)
{
	if (!HasAuthority()) return;
	if (!Controller)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleManager] RegisterPlayerTeam : controller null, ignoré."));
		return;
	}

	FPlayerTeam Team;
	Team.Controller = Controller;
	for (ACustomPaperCharacter* Unit : Units)
	{
		if (IsValid(Unit))
			Team.Units.Add(Unit);
	}

	PlayerTeams.Add(Team);
	UE_LOG(LogTemp, Log, TEXT("[BattleManager] PlayerTeam registered : %s (%d units)."), *Controller->GetName(), Team.Units.Num());
}

// ============================================================
//  StartBattle
// ============================================================

void ABattleManager::StartBattle()
{
	if (!HasAuthority()) return;
	if (PlayerTeams.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleManager] No team Registered !"));
		return;
	}

	//Random Order
	for (int32 i = PlayerTeams.Num() - 1; i > 0; --i)
	{
		int32 j = FMath::RandRange(0, i);
		PlayerTeams.Swap(i, j);
	}

	UE_LOG(LogTemp, Log, TEXT("[BattleManager] player Order random :"));
	for (int32 i = 0; i < PlayerTeams.Num(); ++i)
	{
		UE_LOG(LogTemp, Log, TEXT("  [%d] %s"), i, *PlayerTeams[i].Controller->GetName());
	}

	BuildTurnQueue();

	bBattleActive = true;
	CurrentQueueIndex = INDEX_NONE;

	AdvanceTurn();
}

// ============================================================
//  BuildTurnQueue
//  Pattern : J1U1, J2U1, J3U1, J1U2, J2U2, J3U2, ...
// ============================================================

void ABattleManager::BuildTurnQueue()
{
	TurnQueue.Empty();

	//To replace with room settings
	int32 MaxUnits = 0;
	for (const FPlayerTeam& Team : PlayerTeams)
		MaxUnits = FMath::Max(MaxUnits, Team.Units.Num());


	for (int32 UnitIdx = 0; UnitIdx < MaxUnits; ++UnitIdx)
	{
		for (const FPlayerTeam& Team : PlayerTeams)
		{
			if (Team.Units.IsValidIndex(UnitIdx) && IsValid(Team.Units[UnitIdx]))
			{
				TurnQueue.Add(Team.Units[UnitIdx]);
			}
		}
	}
}

// ============================================================
//  AdvanceTurn - cherche la prochaine unit vivante
// ============================================================

void ABattleManager::AdvanceTurn()
{
	if (!HasAuthority()) return;


	if (ACustomPlayerController* Winner = CheckWinCondition())
	{
		bBattleActive = false;
		ActiveUnit = nullptr;
		UE_LOG(LogTemp, Log, TEXT("[BattleManager] Victoire ! Gagnant : %s"), *Winner->GetName());
		Multicast_BattleEnded(Winner);
		OnBattleFinished.Broadcast(Winner);
		return;
	}

	const int32 QueueSize = TurnQueue.Num();
	if (QueueSize == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[BattleManager] La file de tour est vide !"));
		bBattleActive = false;
		return;
	}


	int32 StartIndex = CurrentQueueIndex;
	int32 Attempts = 0;

	do
	{
		CurrentQueueIndex = (CurrentQueueIndex + 1) % QueueSize;
		++Attempts;

		if (Attempts > QueueSize)
		{
			UE_LOG(LogTemp, Error, TEXT("[BattleManager] Aucune unit vivante trouvée dans la file !"));
			bBattleActive = false;
			return;
		}
	} while (!IsValid(TurnQueue[CurrentQueueIndex]));

	ActivateTurn(TurnQueue[CurrentQueueIndex]);
}

// ============================================================
//  ActivateTurn
// ============================================================

void ABattleManager::ActivateTurn(ACustomPaperCharacter* Unit)
{
	ActiveUnit = Unit;
	TurnTimeRemaining = TurnDuration;

	UE_LOG(LogTemp, Log, TEXT("[BattleManager] -> Tour de %s (%.0f s)"), *Unit->GetName(), TurnDuration);

	// Notifie tous les clients (caméra + input)
	Multicast_TurnStarted(Unit, TurnDuration);

	// Broadcast Blueprint côté serveur
	OnTurnStarted.Broadcast(Unit, TurnDuration);
}

// ============================================================
//  EndCurrentTurn
// ============================================================

void ABattleManager::EndCurrentTurn()
{
	if (!HasAuthority() || !bBattleActive) return;
	AdvanceTurn();
}

// ============================================================
//  OnUnitDied
// ============================================================

void ABattleManager::OnUnitDied(ACustomPaperCharacter* DeadUnit)
{
	if (!HasAuthority() || !IsValid(DeadUnit)) return;

	bool bWasActiveTurn = (ActiveUnit == DeadUnit);


	for (TObjectPtr<ACustomPaperCharacter>& Slot : TurnQueue)
	{
		if (Slot == DeadUnit)
		{
			Slot = nullptr;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[BattleManager] Unit dead : %s%s"), *DeadUnit->GetName(), bWasActiveTurn ? TEXT(" (skiped turn)") : TEXT(""));


	if (bWasActiveTurn)
	{
		ActiveUnit = nullptr;
		AdvanceTurn();
	}
}

// ============================================================
//  CheckWinCondition
// ============================================================

ACustomPlayerController* ABattleManager::CheckWinCondition() const
{
	ACustomPlayerController* LastAliveController = nullptr;
	int32 TeamsAlive = 0;

	for (const FPlayerTeam& Team : PlayerTeams)
	{
		if (TeamHasAliveUnits(Team))
		{
			++TeamsAlive;
			LastAliveController = Team.Controller;
		}
	}


	return (TeamsAlive <= 1) ? LastAliveController : nullptr;
}

bool ABattleManager::TeamHasAliveUnits(const FPlayerTeam& Team) const
{
	for (const TObjectPtr<ACustomPaperCharacter>& Unit : Team.Units)
	{
		if (IsValid(Unit) && TurnQueue.Contains(Unit))
			return true;
	}
	return false;
}

// ============================================================
//  OnRep_ActiveUnit (client)
// ============================================================

void ABattleManager::OnRep_ActiveUnit()
{
	// Côté client : met à jour les delegates Blueprint (UI, HUD, etc.)
	if (IsValid(ActiveUnit))
	{
		OnTurnStarted.Broadcast(ActiveUnit, TurnTimeRemaining);
	}
}

// ============================================================
//  Multicast_TurnStarted
// ============================================================

void ABattleManager::Multicast_TurnStarted_Implementation(ACustomPaperCharacter* NewActiveUnit, float Duration)
{
	if (!IsValid(NewActiveUnit)) return;

	UWorld* World = GetWorld();
	if (!World) return;

	UE_LOG(LogTemp, Log, TEXT("[BattleManager][Client] Multicast_TurnStarted -> %s"), *NewActiveUnit->GetName());

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		ACustomPlayerController* PC = Cast<ACustomPlayerController>(It->Get());
		if (!PC) continue;

		// -Caméra : tout le monde braqué sur l'unit active
		PC->SetViewTargetWithBlend(
			NewActiveUnit,
			CameraBlendTime,
			EViewTargetBlendFunction::VTBlend_Cubic);

		// -Input : uniquement le propriétaire de l'unit peut agir
		const bool bIsOwner = (NewActiveUnit->GetController() == PC);

		if (bIsOwner)
		{
			PC->EnableInput(PC);
			UE_LOG(LogTemp, Log, TEXT("  -> Input ACTIVÉ pour %s"), *PC->GetName());
		}
		else
		{
			PC->DisableInput(PC);
			UE_LOG(LogTemp, Log, TEXT("  -> Input DÉSACTIVÉ pour %s"), *PC->GetName());
		}
	}
}

// ============================================================
//  Multicast_BattleEnded
// ============================================================

void ABattleManager::Multicast_BattleEnded_Implementation(ACustomPlayerController* Winner)
{
	UE_LOG(LogTemp, Log, TEXT("[BattleManager][Client] Multicast_BattleEnded -> Gagnant : %s"), Winner ? *Winner->GetName() : TEXT("Aucun"));

	// Réactive l'input pour tout le monde
	UWorld* World = GetWorld();
	if (!World) return;

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (ACustomPlayerController* PC = Cast<ACustomPlayerController>(It->Get()))
		{
			PC->EnableInput(PC);
		}
	}

	// Broadcast Blueprint pour afficher l'écran de fin de partie
	OnBattleFinished.Broadcast(Winner);
}