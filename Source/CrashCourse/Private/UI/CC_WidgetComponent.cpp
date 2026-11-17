#include "UI/CC_WidgetComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "Blueprint/WidgetTree.h"
#include "Characters/CC_BaseCharacter.h"
#include "UI/CC_AttributeWidget.h"

UCC_WidgetComponent::UCC_WidgetComponent()
{
 AttributeMap.Add(UCC_AttributeSet::GetHealthAttribute(), UCC_AttributeSet::GetMaxHealthAttribute());
}
void UCC_WidgetComponent::BeginPlay()
{
 Super::BeginPlay();
 if (GetNetMode() == NM_DedicatedServer) return;
 InitWidget();
 RefreshBindings();
}
void UCC_WidgetComponent::InitWidget()
{
 Super::InitWidget();
 if (HasBegunPlay() && !bBinding && GetUserWidgetObject() != BoundWidget.Get()) RefreshBindings();
}
void UCC_WidgetComponent::ClearBindings()
{
 if (AbilitySystemComponent.IsValid())
  for (const auto& Entry : AttributeHandles)
   AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Entry.Key).Remove(Entry.Value);
 AttributeHandles.Empty();
 BoundWidget.Reset();
}
void UCC_WidgetComponent::RefreshBindings()
{
 if (GetNetMode() == NM_DedicatedServer || bBinding) return;
 ACC_BaseCharacter* Character = Cast<ACC_BaseCharacter>(GetOwner());
 if (!Character) { ClearBindings(); return; }
 if (CrashCharacter.IsValid() && CrashCharacter != Character) CrashCharacter->OnASCInitialized.RemoveDynamic(this, &ThisClass::OnASCInitialized);
 CrashCharacter = Character;
 Character->OnASCInitialized.AddUniqueDynamic(this, &ThisClass::OnASCInitialized);
 OnASCInitialized(Character->GetAbilitySystemComponent(), Character->GetAttributeSet());
}
void UCC_WidgetComponent::OnASCInitialized(UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
 ClearBindings();
 if (AttributeSet.IsValid()) AttributeSet->OnAttributesInitialized.RemoveDynamic(this, &ThisClass::BindToAttributeChanges);
 AbilitySystemComponent = ASC;
 AttributeSet = Cast<UCC_AttributeSet>(AS);
 if (!AbilitySystemComponent.IsValid() || !AttributeSet.IsValid()) return;
 AttributeSet->OnAttributesInitialized.AddUniqueDynamic(this, &ThisClass::BindToAttributeChanges);
 // Subscribe now, even if GE initialization hasn't executed yet. Changes and late initialization both refresh.
 BindToAttributeChanges();
}
void UCC_WidgetComponent::BindToAttributeChanges()
{
 if (bBinding || !AbilitySystemComponent.IsValid() || !AttributeSet.IsValid()) return;
 TGuardValue<bool> Guard(bBinding, true);
 ClearBindings();
 UUserWidget* UserWidget = GetUserWidgetObject();
 if (!IsValid(UserWidget)) return;
 UserWidget->TakeWidget(); // Ensures native fallback WidgetTree exists before initial values are sent.
 BoundWidget = UserWidget;
 for (const auto& Pair : AttributeMap)
 {
  const FGameplayAttribute Attributes[] = { Pair.Key, Pair.Value };
  for (const FGameplayAttribute& Attribute : Attributes)
  {
   if (!Attribute.IsValid() || AttributeHandles.Contains(Attribute)) continue;
   const FDelegateHandle Handle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Attribute).AddWeakLambda(this, [this](const FOnAttributeChangeData& Change)
   {
    RefreshWidgets(Change.Attribute, Change.OldValue);
   });
   AttributeHandles.Add(Attribute, Handle);
  }
 }
 RefreshWidgets();
}
void UCC_WidgetComponent::RefreshWidgets(FGameplayAttribute ChangedAttribute, float OldValue)
{
 UUserWidget* Root = GetUserWidgetObject();
 if (!IsValid(Root) || !AttributeSet.IsValid()) return;
 TSet<UCC_AttributeWidget*> Visited;
 auto Update = [this, ChangedAttribute, OldValue, &Visited](UWidget* WidgetObject)
 {
  UCC_AttributeWidget* AttributeWidget = Cast<UCC_AttributeWidget>(WidgetObject);
  if (!IsValid(AttributeWidget) || Visited.Contains(AttributeWidget)) return;
  Visited.Add(AttributeWidget);
  for (const auto& Pair : AttributeMap)
   if (Pair.Key.IsValid() && Pair.Value.IsValid() && AttributeWidget->MatchesAttributes(Pair)
       && (!ChangedAttribute.IsValid() || ChangedAttribute == Pair.Key || ChangedAttribute == Pair.Value))
   {
    AttributeWidget->AvatarActor = CrashCharacter;
    const float OldCurrentValue = ChangedAttribute == Pair.Key ? OldValue : AttributeWidget->CurrentValue;
    AttributeWidget->OnAttributeChange(Pair, AttributeSet.Get(), OldCurrentValue);
   }
 };
 Update(Root);
 if (Root->WidgetTree) Root->WidgetTree->ForEachWidget(Update);
}
void UCC_WidgetComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
 ClearBindings();
 if (CrashCharacter.IsValid()) CrashCharacter->OnASCInitialized.RemoveDynamic(this, &ThisClass::OnASCInitialized);
 if (AttributeSet.IsValid()) AttributeSet->OnAttributesInitialized.RemoveDynamic(this, &ThisClass::BindToAttributeChanges);
 AbilitySystemComponent.Reset(); AttributeSet.Reset(); CrashCharacter.Reset();
 Super::EndPlay(EndPlayReason);
}
