#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "CC_ProgressionSave.generated.h"

UCLASS()
class CRASHCOURSE_API UCC_ProgressionSave : public USaveGame
{
	GENERATED_BODY()
public:
	UPROPERTY(SaveGame) int32 Version = 4;
 UPROPERTY(SaveGame) bool bChromaticSuppressed=false;
 UPROPERTY(SaveGame) bool bRemoteShopUnlocked=false;
 UPROPERTY(SaveGame) bool bDeveloperInventory=false;
 UPROPERTY(SaveGame) TMap<FName,int32> ConsumableCounts;
 UPROPERTY(SaveGame) FName EquippedConsumableId;
 UPROPERTY(SaveGame) float ItemCooldownRemaining=0.f;
 UPROPERTY(SaveGame) int64 ItemCooldownSavedUtcTicks=0;
	UPROPERTY(SaveGame) TArray<FName> VisitedRegions;
	// Stable identity disambiguates native tiers after changing the starting form.
	UPROPERTY(SaveGame) FName NativeFormId;
	UPROPERTY(SaveGame) TArray<FName> ManualSkillIds;
	UPROPERTY(SaveGame) int32 Experience = 0;
 UPROPERTY(SaveGame) int32 CopperCoins = 0;
	UPROPERTY(SaveGame) int32 NativeTier = 0;
	UPROPERTY(SaveGame) FName SelectedForm;
	UPROPERTY(SaveGame) TMap<FName, int32> FormTiers;
	UPROPERTY(SaveGame) TMap<FName, int32> UpgradeLevels;
};
