#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CC_ExperienceFXSetupLibrary.generated.h"

class UNiagaraSystem;
class UMaterialInterface;

/** Small editor utility used by the repeatable experience pickup setup script. */
UCLASS()
class CRASHCOURSE_API UCC_ExperienceFXSetupLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** Builds one local-space, stationary sprite. Gameplay actors supply physics and attraction. */
	UFUNCTION(BlueprintCallable, Category="Crash|Editor")
	static bool BuildExperienceOrbSystem(UNiagaraSystem* System, UMaterialInterface* SpriteMaterial);
};
