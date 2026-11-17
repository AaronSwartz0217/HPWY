#include "UI/CC_PlayerHealthHUDWidget.h"
#include "Progression/CC_ProgressionComponent.h"

TArray<UCC_ConsumableDefinition*> UCC_PlayerHealthHUDWidget::GetItemChoices() const
{
 TArray<UCC_ConsumableDefinition*> Choices;
 if(const auto* Progression=BoundProgression.Get())
 {
  for(const auto& Stack:Progression->ItemStacks)
  {
   if(Stack.Item && Stack.Count>0 && Stack.Item->Effect!=ECC_ConsumableEffect::TravelScroll)
   {
    Choices.AddUnique(Stack.Item);
   }
  }
 }
 return Choices;
}

void UCC_PlayerHealthHUDWidget::RefreshItemCooldown()
{
 const auto* Progression=BoundProgression.Get();
 DisplayedItemCooldown=Progression?Progression->GetEquippedItemCooldownRemaining():0.f;
}
