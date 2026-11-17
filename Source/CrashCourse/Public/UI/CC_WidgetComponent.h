#pragma once
#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Components/WidgetComponent.h"
#include "CC_WidgetComponent.generated.h"

class UAbilitySystemComponent;
class UCC_AttributeSet;
class ACC_BaseCharacter;
class UCC_AttributeWidget;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CRASHCOURSE_API UCC_WidgetComponent : public UWidgetComponent
{
 GENERATED_BODY()
public:
 UCC_WidgetComponent();
 virtual void InitWidget() override;
 virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
 // Also useful after changing WidgetClass / AttributeMap at runtime.
 UFUNCTION(BlueprintCallable, Category="Crash|UI") void RefreshBindings();
protected:
 virtual void BeginPlay() override;
 UPROPERTY(EditAnywhere, Category="Crash|UI") TMap<FGameplayAttribute, FGameplayAttribute> AttributeMap;
private:
 TWeakObjectPtr<ACC_BaseCharacter> CrashCharacter;
 TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
 TWeakObjectPtr<UCC_AttributeSet> AttributeSet;
 TWeakObjectPtr<UUserWidget> BoundWidget;
 TMap<FGameplayAttribute, FDelegateHandle> AttributeHandles;
 bool bBinding = false;
 void ClearBindings();
 UFUNCTION() void OnASCInitialized(UAbilitySystemComponent* ASC, UAttributeSet* AS);
 UFUNCTION() void BindToAttributeChanges();
 void RefreshWidgets(FGameplayAttribute ChangedAttribute = FGameplayAttribute(), float OldValue = 0.f);
};
