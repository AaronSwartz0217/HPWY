#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "HPWH/HPWH_Types.h"
#include "HPWH_SaveGame.generated.h"

/** 单局存档：记录层数、经验、强化与已探索格的解密状态。 */
UCLASS()
class CRASHCOURSE_API UHPWH_SaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY() int32 RunSeed = 20261117;
	UPROPERTY() int32 LayerIndex = 1;
	UPROPERTY() int32 PlayerLevel = 1;
	UPROPERTY() int32 Experience = 0;
	UPROPERTY() int32 ExperienceToNextLevel = 10;
	UPROPERTY() int32 KillCount = 0;
	UPROPERTY() int32 DeathCount = 0;
	UPROPERTY() float ChancePenalty = 0.f;
	UPROPERTY() int32 CrawlerSpeedLevel = 0;
	UPROPERTY() FVector PlayerLocation = FVector(0.f, 0.f, 120.f);
	UPROPERTY() float PlayerHealth = 30.f;
	UPROPERTY() bool bCoreCollected = false;

	UPROPERTY() TMap<FIntPoint, FHPWH_CellRecord> Records;
	UPROPERTY() TArray<FIntPoint> PuzzleAnchorCells;
	UPROPERTY() TArray<uint8> OwnedUpgradeTypes;
	UPROPERTY() TArray<uint8> OwnedUpgradeRarities;
	UPROPERTY() TArray<int32> OwnedUpgradeCounts;
	UPROPERTY() TArray<uint8> SolvedPuzzleTypes;

	UPROPERTY() float AttackBonus = 0.f;
	UPROPERTY() float MaxHealthBonus = 0.f;
	UPROPERTY() float MoveSpeedBonus = 0.f;
	UPROPERTY() float AttackRangeBonus = 0.f;
	UPROPERTY() float BulletCooldownReduction = 0.f;
	UPROPERTY() int32 ProjectileCount = 1;
	UPROPERTY() int32 SoulHitsRequired = 1;
	UPROPERTY() float SoulSpeedBonus = 0.f;
	UPROPERTY() float ReturnRangeBonus = 0.f;
	UPROPERTY() float InvulnBonus = 0.f;
	UPROPERTY() float KillHeal = 0.f;
	UPROPERTY() int32 CrawlerDistanceCells = 0;
	UPROPERTY() float CrawlerSlow = 0.f;
	UPROPERTY() float PuzzleTimeBonus = 0.f;
	UPROPERTY() float ChanceBonus = 0.f;
	UPROPERTY() float InteractRangeBonus = 0.f;
};
