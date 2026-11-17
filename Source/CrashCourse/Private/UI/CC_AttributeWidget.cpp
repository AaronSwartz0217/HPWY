// Copyright Druid Mechanics


#include "UI/CC_AttributeWidget.h"
#include "UI/CC_GameFont.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"

UCC_AttributeWidget::UCC_AttributeWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	Attribute = UCC_AttributeSet::GetHealthAttribute();
	MaxAttribute = UCC_AttributeSet::GetMaxHealthAttribute();
}

TSharedRef<SWidget> UCC_AttributeWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		FallbackText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AttributeValueText"));
		FallbackText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		FallbackText->SetShadowColorAndOpacity(FLinearColor::Black);
		FallbackText->SetShadowOffset(FVector2D(1.f, 1.f));
		FSlateFontInfo Font = CCGameFont::Kai(FallbackText->GetFont()); Font.Size = 14; FallbackText->SetFont(Font);
		WidgetTree->RootWidget = FallbackText;
	}
	return Super::RebuildWidget();
}

void UCC_AttributeWidget::OnAttributeChange(const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair, UCC_AttributeSet* AttributeSet, float OldValue)
{
	if (!IsValid(AttributeSet) || !Pair.Key.IsValid() || !Pair.Value.IsValid()) return;
	const float AttributeValue = Pair.Key.GetNumericValue(AttributeSet);
	const float MaxAttributeValue = Pair.Value.GetNumericValue(AttributeSet);
	CurrentValue = AttributeValue;
	CurrentMaxValue = MaxAttributeValue;
	ValuePercent = MaxAttributeValue > 0.f ? FMath::Clamp(AttributeValue / MaxAttributeValue, 0.f, 1.f) : 0.f;
	if (FallbackText)
	{
		const TCHAR* Label = Attribute == UCC_AttributeSet::GetHealthAttribute() ? TEXT("HP") : TEXT("Value");
		FallbackText->SetText(FText::FromString(FString::Printf(TEXT("%s %.0f / %.0f"), Label, AttributeValue, MaxAttributeValue)));
	}

	BP_OnAttributeChange(AttributeValue, MaxAttributeValue, OldValue);
}

bool UCC_AttributeWidget::MatchesAttributes(const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair) const
{
	return Attribute == Pair.Key && MaxAttribute == Pair.Value;
}
