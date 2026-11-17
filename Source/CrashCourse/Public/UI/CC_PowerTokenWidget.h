#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CC_PowerTokenWidget.generated.h"
class UCC_FormDefinition;
class UCC_MenuArtData;
class UTextBlock;
class UImage;
class UButton;
class UBorder;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCC_PowerTokenSelected,UCC_FormDefinition*,Form);

// One inventory cell, holding a permanent power rather than a consumable stack.
UCLASS()
class CRASHCOURSE_API UCC_PowerTokenWidget : public UUserWidget
{
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintCallable,Category="Inventory") void SetEntry(UCC_FormDefinition* Form,int32 Tier,bool bActive,bool bSelected,UCC_MenuArtData* Art);
 UFUNCTION(BlueprintPure,Category="Inventory") UCC_FormDefinition* GetEntryForm() const {return EntryForm;}
 UPROPERTY(BlueprintAssignable,Category="Inventory") FCC_PowerTokenSelected OnSelected;
protected:
 virtual void NativeOnInitialized() override;
private:
 UPROPERTY(Transient) TObjectPtr<UCC_FormDefinition> EntryForm;
 UPROPERTY(Transient) TObjectPtr<UButton> CellButton;
 UPROPERTY(Transient) TObjectPtr<UImage> FrameImage;
 UPROPERTY(Transient) TObjectPtr<UImage> IconImage;
 UPROPERTY(Transient) TObjectPtr<UBorder> TokenSeal;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> TokenMark;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> NameLabel;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> StateLabel;
 UFUNCTION() void SelectEntry();
};
