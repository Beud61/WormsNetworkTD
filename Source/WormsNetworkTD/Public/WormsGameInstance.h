#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "WormsGameInstance.generated.h"

UCLASS()
class WORMSNETWORKTD_API UWormsGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/**
	 * Mis à true par Client_NotifyGameStarting() avant le ServerTravel.
	 * Survit à tous les travels — empêche BeginPlay du PlayerController
	 * de recréer le menu sur la nouvelle map.
	 */
	UPROPERTY()
	bool bGameStarted = false;
};