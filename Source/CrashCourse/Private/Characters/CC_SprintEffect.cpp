#include "Characters/CC_SprintEffect.h"
#include "AbilitySystem/CC_AttributeSet.h"

const FName UCC_SprintEffect::MultiplierName(TEXT("SprintMultiplier"));
UCC_SprintEffect::UCC_SprintEffect()
{
 DurationPolicy=EGameplayEffectDurationType::Infinite;
 FSetByCallerFloat Multiplier;Multiplier.DataName=MultiplierName;
 FGameplayModifierInfo Modifier;Modifier.Attribute=UCC_AttributeSet::GetMoveSpeedAttribute();
 Modifier.ModifierOp=EGameplayModOp::Multiplicitive;
 Modifier.ModifierMagnitude=FGameplayEffectModifierMagnitude(Multiplier);Modifiers.Add(Modifier);
}
