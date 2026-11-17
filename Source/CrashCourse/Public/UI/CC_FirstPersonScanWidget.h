#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CC_FirstPersonScanWidget.generated.h"
class UBorder;
class UTextBlock;
UCLASS()
class CRASHCOURSE_API UCC_FirstPersonScanWidget:public UUserWidget
{
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintPure,Category="Interaction HUD") AActor* GetDisplayedTarget()const;
 UFUNCTION(BlueprintPure,Category="Interaction HUD") bool IsInteractionReady()const;
protected:
 virtual TSharedRef<SWidget> RebuildWidget()override;
 virtual void NativeTick(const FGeometry& Geometry,float Delta)override;
 virtual int32 NativePaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& Cull,FSlateWindowElementList& Elements,int32 Layer,const FWidgetStyle& Style,bool bParentEnabled)const override;
private:
 TWeakObjectPtr<AActor> DisplayedTarget;
 TWeakObjectPtr<AActor> ObservationTarget;
 double ObservationEndsAt = 0.;
 int32 ObservationRevision = 0;
 bool bInteractionReady=false;
 UPROPERTY(Transient) TObjectPtr<UBorder> Card;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> Title;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> Body;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> Hint;
 UPROPERTY(Transient) TObjectPtr<UBorder> HeavenGuide;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> HeavenGuideText;
};
