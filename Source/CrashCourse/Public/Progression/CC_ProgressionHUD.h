#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CC_ProgressionHUD.generated.h"

// Minimal keyboard-operated test panel; final art/UI can subscribe to progression delegates.
UCLASS()
class CRASHCOURSE_API ACC_ProgressionHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;
 UPROPERTY(Transient,BlueprintReadOnly,Category="Crash|UI") TObjectPtr<class UCC_PlayerHealthHUDWidget> PlayerHealthHUD;
 UPROPERTY(Transient,BlueprintReadOnly,Category="Crash|UI") TObjectPtr<class UCC_FirstPersonScanWidget> FirstPersonScanHUD;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crash|UI") bool bShowDebugPanel = true;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crash|Interaction") TSoftObjectPtr<class UTexture2D> PickupPromptTexture;
 UFUNCTION(BlueprintPure, Category="Crash|Interaction") bool IsPickupPromptVisible() const;
protected:
 UPROPERTY(Transient) TObjectPtr<class UTexture2D> LoadedPickupPrompt;
	virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
};
