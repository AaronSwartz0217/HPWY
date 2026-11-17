#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CC_AnimationSetupLibrary.generated.h"
class UAnimBlueprint;
class UAnimSequence;
class UBlueprint;
class UMaterialInterface;

UCLASS()
class CRASHCOURSE_API UCC_AnimationSetupLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
 UFUNCTION(BlueprintCallable,Category="Crash|Editor") static int32 RepairLegacyWidgetPins(UBlueprint* Blueprint);
 UFUNCTION(BlueprintPure,Category="Crash|Editor") static bool IsNaniteBlendSupported(UMaterialInterface* Material);
	UFUNCTION(BlueprintCallable, Category="Crash|Editor") static FString DescribeBlueprintGraphs(UBlueprint* Blueprint);
    UFUNCTION(BlueprintCallable,Category="Crash|Editor") static int32 TuneLocomotionBlends(UAnimBlueprint* Blueprint);
    // Returns the selected spine bone, or None if the skeleton/graph is unsupported.
    UFUNCTION(BlueprintCallable,Category="Crash|Editor") static FName ConfigureUpperBodyAttack(UAnimBlueprint* Blueprint);
	// Editor-only implementation; builds six states plus a DefaultSlot montage layer.
	UFUNCTION(BlueprintCallable, Category="Crash|Editor")
	static bool BuildLocomotionGraph(UAnimBlueprint* Blueprint, UAnimSequence* Idle, UAnimSequence* Run, UAnimSequence* Jump, UAnimSequence* Fall, UAnimSequence* Land, UAnimSequence* Death);
};
