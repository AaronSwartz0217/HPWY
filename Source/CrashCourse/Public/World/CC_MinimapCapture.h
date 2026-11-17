#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CC_MinimapCapture.generated.h"
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

UCLASS(Blueprintable)
class CRASHCOURSE_API ACC_MinimapCapture : public AActor
{
	GENERATED_BODY()
public:
	ACC_MinimapCapture();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<USceneCaptureComponent2D> Capture;
	UPROPERTY(Transient,BlueprintReadOnly) TObjectPtr<UTextureRenderTarget2D> RenderTarget;
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Minimap",meta=(ClampMin="64",ClampMax="1024")) int32 Resolution=256;
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Minimap",meta=(ClampMin="1",ClampMax="30")) float RefreshRate=10.f;
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Minimap",meta=(ClampMin="100")) float ViewWidth=3000.f;
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Minimap") float CaptureHeight=2500.f;
	UPROPERTY() TWeakObjectPtr<APlayerController> FollowingController;
	UFUNCTION(BlueprintCallable,Category="Minimap") void SetCaptureEnabled(bool bEnabled);
};
