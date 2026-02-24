#pragma once

#include "CoreMinimal.h"
#include "LobbyTypes.generated.h"
USTRUCT(BlueprintType)
struct FPlayerLobbyInfo
{
	GENERATED_BODY()

	FPlayerLobbyInfo()
		: PlayerName(TEXT(""))
		, UnitNB(0)
		, ProfileIcon(0)
		, TeamIcon(0)
		, PlayerId(-1)
	{
	}

	UPROPERTY(BlueprintReadWrite)
	FString PlayerName;

	UPROPERTY(BlueprintReadWrite)
	int32 UnitNB;

	UPROPERTY(BlueprintReadWrite)
	TArray<FString> UnitNames;

	UPROPERTY(BlueprintReadWrite)
	int32 ProfileIcon;

	UPROPERTY(BlueprintReadWrite)
	int32 TeamIcon;

	UPROPERTY(BlueprintReadWrite)
	int32 PlayerId;
};