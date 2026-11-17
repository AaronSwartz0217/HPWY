#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CC_PlayerHealthHUDWidget.generated.h"
class UAbilitySystemComponent;
class ACC_BaseCharacter;
class UProgressBar;
class UTextBlock;
class UBorder;
class UImage;
class UCC_SkillGalleryData;
class UCC_SkillDefinition;
class UCC_FormDefinition;
class UCC_ConsumableDefinition;
class UCC_SkillComponent;
class UCC_ProgressionComponent;
class UMaterialInterface;
UCLASS()
class CRASHCOURSE_API UCC_PlayerHealthHUDWidget:public UUserWidget
{
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintPure,Category="Health") float GetDisplayedHealth()const{return Health;}
 UFUNCTION(BlueprintPure,Category="Health") float GetDisplayedMaxHealth()const{return Maximum;}
 UFUNCTION(BlueprintCallable,Category="Combat HUD") void SetExpanded(bool bOpen);
 UFUNCTION(BlueprintCallable,Category="Combat HUD") void CycleSkill(int32 Direction);
 UFUNCTION(BlueprintPure,Category="Combat HUD") bool IsExpanded()const{return bExpanded;}
 UFUNCTION(BlueprintPure,Category="Combat HUD") int32 GetSelectedSlot()const{return SelectedSlot;}
 UFUNCTION(BlueprintPure,Category="Combat HUD") float GetDisplayedCooldown(int32 Index)const{return Index==4?DisplayedItemCooldown:Cooldowns.IsValidIndex(Index)?Cooldowns[Index]:0.f;}
 UFUNCTION(BlueprintPure,Category="Progression HUD") int32 GetDisplayedExperience()const{return DisplayedExperience;}
 UFUNCTION(BlueprintPure,Category="Progression HUD") int32 GetDisplayedCopperCoins()const{return DisplayedCoins;}
 UFUNCTION(BlueprintPure,Category="Progression HUD") int32 GetExperienceGoal()const{return ExperienceGoal;}
 bool PressSelectedSkill(FKey Source=FKey());
 void ToggleCommandMenu();
 void ReleaseSelectedSkill(FKey Source=FKey());
protected:
 virtual void NativeConstruct() override;
 virtual TSharedRef<SWidget> RebuildWidget() override;
 virtual int32 NativePaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& Cull,FSlateWindowElementList& Elements,int32 Layer,const FWidgetStyle& Style,bool bParentEnabled)const override;
 virtual void NativeTick(const FGeometry& Geometry,float Delta) override;
 virtual void NativeDestruct() override;
private:
 struct FCommandRow
 {
  UBorder* Frame;
  UImage* Icon;
  UTextBlock* Name;
  UTextBlock* State;
  UProgressBar* Cooldown;
 };
 FCommandRow BuildCommandRow();
 static void StyleCommandRow(UBorder* Frame, bool bSelected);
 TArray<UCC_ConsumableDefinition*> GetItemChoices() const;
 void RefreshItemCooldown();
 UPROPERTY(Transient) TObjectPtr<UProgressBar> HealthBar;
 UPROPERTY(Transient) TObjectPtr<UMaterialInterface> RainbowHealthMaterial;
 UPROPERTY(EditDefaultsOnly, Category="Health") TSoftObjectPtr<UMaterialInterface> RainbowHealthMaterialAsset{FSoftObjectPath(TEXT("/Game/HPWH/Effects/ChromaticInk/M_ChromaticHealth.M_ChromaticHealth"))};
public:
 UFUNCTION(BlueprintPure,Category="Health") bool IsRainbowHealthDisplayed() const {return bRainbowHealth;}
private:
 bool bRainbowHealth=false;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> ValueText;
 TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
 TWeakObjectPtr<ACC_BaseCharacter> BoundCharacter;
 FDelegateHandle HealthHandle,MaxHandle;
 float Health=0,Maximum=0;
 bool bPreviousWorldLabelVisibility=true;
 UPROPERTY(Transient) TObjectPtr<class UCanvasPanel> SkillPanel;
 UPROPERTY(Transient) TObjectPtr<UBorder> CommandBadge;
 UPROPERTY(Transient) TObjectPtr<UBorder> DescriptionFrame;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> DescriptionText;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> CommandTitle;
 UPROPERTY(Transient) TObjectPtr<UBorder> ScrollTrack;
 UPROPERTY(Transient) TObjectPtr<UBorder> ScrollThumb;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> FormText;
 enum class ECommandPage { Main, Skills, Items, Forms, Travel };
 ECommandPage CommandPage = ECommandPage::Main;
 int32 CategoryIndex = 0;
 int32 ScrollOffset = 0;
 FText CommandFeedback;
 int32 GetCommandCount() const;
 void SetCommandDescription(const FText& Detail);
 TArray<UCC_FormDefinition*> GetFormChoices() const;
 void RefreshCommandPage();
 bool ActivateCommandChoice();
 UPROPERTY(Transient) TArray<TObjectPtr<UBorder>> Rows;
 UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> Names;
 UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> States;
 UPROPERTY(Transient) TArray<TObjectPtr<UImage>> Icons;
 UPROPERTY(Transient) TArray<TObjectPtr<UProgressBar>> CooldownBars;
 UPROPERTY(Transient) TArray<TObjectPtr<UCC_SkillDefinition>> ShownDefinitions;
 UPROPERTY(Transient) TObjectPtr<UCC_SkillGalleryData> Gallery;
 TWeakObjectPtr<UCC_SkillComponent> HeldSkills;
 TArray<float> Cooldowns;
 int32 SelectedSlot=0,HeldSlot=INDEX_NONE;
 FKey HeldInputKey;
 float DisplayedItemCooldown=0;
 bool bExpanded=false;
 float SkillRefreshElapsed=0;
 void BuildSkillPanel(class UCanvasPanel* Root);
 void RefreshSkills();
 TWeakObjectPtr<UCC_ProgressionComponent> BoundProgression;
 int32 DisplayedExperience=0,DisplayedCoins=0,ExperienceGoal=0;
 UFUNCTION() void RefreshProgression();
 void BindProgression();
 void Unbind();
 void Refresh();
};
