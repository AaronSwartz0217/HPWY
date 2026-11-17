// Copyright Druid Mechanics

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"

#include "CC_PlayerState.generated.h"

class UAttributeSet;
class UAbilitySystemComponent;
class UCC_ProgressionComponent;

UCLASS()
class CRASHCOURSE_API ACC_PlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	ACC_PlayerState();
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Progression")
	TObjectPtr<UCC_ProgressionComponent> Progression;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UAttributeSet* GetAttributeSet() const { return AttributeSet; }
private:

	UPROPERTY(VisibleAnywhere, Category = "Crash|Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UAttributeSet> AttributeSet;
};
