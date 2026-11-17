#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HPWH_HUD.generated.h"

class UTexture2D;

/** 原型 HUD：受击次数、百分比、提示、离体与触摸按键。 */
UCLASS()
class CRASHCOURSE_API UHPWH_HUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:
	UPROPERTY() TObjectPtr<UTexture2D> JoystickRingTexture;
	UPROPERTY() TObjectPtr<UTexture2D> OrbTexture;
	UPROPERTY() TObjectPtr<UTexture2D> MenuBackgroundTexture;
	mutable int32 PaintCount = 0;
};
