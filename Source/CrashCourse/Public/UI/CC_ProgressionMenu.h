#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ComboBoxString.h"
#include "Framework/Commands/InputChord.h"
#include "Progression/CC_ProgressionComponent.h"
#include "CC_ProgressionMenu.generated.h"
class UTextBlock;
class UButton;
class UVerticalBox;

UCLASS()
class CRASHCOURSE_API UCC_ProgressionMenu : public UUserWidget
{
	GENERATED_BODY()
protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
private:
	UPROPERTY() TObjectPtr<UTextBlock> Details;
	UPROPERTY() TObjectPtr<UTextBlock> Feedback;
	UPROPERTY() TObjectPtr<UButton> EvolveButton;
	UPROPERTY() TObjectPtr<UCC_ProgressionComponent> Progression;
	UPROPERTY() TObjectPtr<class UScrollBox> ContentScroll;
	int32 Page = 0;
 bool bUpdatingKeySelector = false;
 UPROPERTY() TObjectPtr<UVerticalBox> SkillControls;
 UPROPERTY() TObjectPtr<UVerticalBox> KeyControls;
 UPROPERTY() TObjectPtr<UComboBoxString> SkillChoice;
 UPROPERTY() TObjectPtr<UComboBoxString> SlotChoice;
 UPROPERTY() TObjectPtr<UComboBoxString> KeyActionChoice;
 UPROPERTY() TObjectPtr<class UInputKeySelector> KeySelector;
 UPROPERTY() TObjectPtr<UButton> EquipButton;
 UFUNCTION() void SkillsPage();
 UFUNCTION() void SettingsPage();
 UFUNCTION() void Equip();
 UFUNCTION() void ResetSkills();
 UFUNCTION() void ResetKeys();
 UFUNCTION() void SaveProgress();
 UFUNCTION() void LoadProgress();
 UFUNCTION() void KeyActionChanged(FString Item, ESelectInfo::Type Type);
 UFUNCTION() void KeySelected(FInputChord Chord);
	UButton* AddButton(UVerticalBox* Box, const FString& Label);
	void Refresh();
	UFUNCTION() void Inventory();
	UFUNCTION() void Evolution();
	UFUNCTION() void Evolve();
	UFUNCTION() void Close();
	UFUNCTION() void PurchaseResult(ECC_PurchaseResult Result);
 UFUNCTION() void SkillConfigurationResult(ECC_SkillConfigurationResult Result);
};
