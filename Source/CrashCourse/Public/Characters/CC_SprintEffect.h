#pragma once
#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "CC_SprintEffect.generated.h"

// Player-owned temporary multiplier; removal restores the current GAS speed, not a cached value.
UCLASS()
class CRASHCOURSE_API UCC_SprintEffect : public UGameplayEffect
{
 GENERATED_BODY()
public:
 UCC_SprintEffect();
 static const FName MultiplierName;
};
