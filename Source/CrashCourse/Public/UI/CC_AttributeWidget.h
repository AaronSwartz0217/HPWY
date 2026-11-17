// Copyright Druid Mechanics

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AbilitySystem/CC_AttributeSet.h"

#include "CC_AttributeWidget.generated.h"

/**
 * 
 */
UCLASS()
class CRASHCOURSE_API UCC_AttributeWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UCC_AttributeWidget(const FObjectInitializer& ObjectInitializer);
	UPROPERTY(BlueprintReadOnly, Category="Crash|Attributes") float CurrentValue = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Crash|Attributes") float CurrentMaxValue = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Crash|Attributes") float ValuePercent = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crash|Attributes")
	FGameplayAttribute Attribute;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crash|Attributes")
	FGameplayAttribute MaxAttribute;

	void OnAttributeChange(const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair, UCC_AttributeSet* AttributeSet, float OldValue);
	bool MatchesAttributes(const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair) const;

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Attribute Change"))
	void BP_OnAttributeChange(float NewValue, float NewMaxValue, float OldValue);

	UPROPERTY(BlueprintReadOnly, Category = "Crash|Attributes")
	TWeakObjectPtr<AActor> AvatarActor;
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
private:
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> FallbackText;
};
