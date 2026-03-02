#pragma once

#include "CoreMinimal.h"
#include "LobbyTypes.generated.h"

// ============================================================
//  Constantes
// ============================================================
namespace LobbyConstants
{
	static constexpr int32 BeaconPort = 7787;
	static constexpr int32 MaxSearchResults = 100;

	static const FName Key_SessionName = TEXT("SETTING_SESSIONNAME");
	static const FName Key_GameMode = TEXT("GAME_MODE");
	static const FName Key_UnitLife = TEXT("UNIT_LIFE");
	static const FName Key_UnitCount = TEXT("UNIT_COUNT");
	static const FName Key_TurnsBeforeWater = TEXT("TURNS_BEFORE_WATER");
	static const FName Key_HostIP = TEXT("HOST_IP");   // <- NOUVEAU : IP de l'hôte stockée dans la session

	static const FString GameMode_1V1 = TEXT("1V1");
	static const FString GameMode_2V2 = TEXT("2V2");
	static const FString GameMode_FFA = TEXT("FFA");

	static const FString GameMapPath = TEXT("/Game/Maps/Lobby");
}

// ============================================================
//  Helpers
// ============================================================
inline int32 GetMaxPlayersForGameMode(const FString& GameMode)
{
	if (GameMode == LobbyConstants::GameMode_1V1) return 2;
	if (GameMode == LobbyConstants::GameMode_2V2) return 4;
	if (GameMode == LobbyConstants::GameMode_FFA) return 4;
	return 2;
}

inline int32 GetGameModeID(const FString& GameMode)
{
	if (GameMode == LobbyConstants::GameMode_1V1) return 0;
	if (GameMode == LobbyConstants::GameMode_2V2) return 1;
	if (GameMode == LobbyConstants::GameMode_FFA) return 2;
	return 0;
}

// ============================================================
//  Structures
// ============================================================
USTRUCT(BlueprintType)
struct FPlayerLobbyInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite) FString PlayerName = TEXT("Player");
	UPROPERTY(BlueprintReadWrite) int32   UnitNB = 1;
	UPROPERTY(BlueprintReadWrite) int32   ProfileIcon = 0;
	UPROPERTY(BlueprintReadWrite) int32   TeamIcon = 0;
	UPROPERTY(BlueprintReadWrite) int32   PlayerId = 0;
};