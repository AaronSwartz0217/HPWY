#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "InputCoreTypes.h"
#include "CC_ControlSettingsSave.generated.h"

UCLASS()
class CRASHCOURSE_API UCC_ControlSettingsSave : public USaveGame
{
	GENERATED_BODY()
public:
	UPROPERTY(SaveGame) TArray<FKey> CombatKeys;
};
