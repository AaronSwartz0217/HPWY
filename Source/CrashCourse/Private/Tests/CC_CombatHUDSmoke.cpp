#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "Components/InputComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Player/CC_PlayerController.h"
#include "Player/CC_PlayerState.h"
#include "Characters/CC_BaseCharacter.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Framework/Application/SlateApplication.h"
#include "Progression/CC_ProgressionHUD.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Progression/CC_SkillComponent.h"
#include "UI/CC_PlayerHealthHUDWidget.h"
#include "UI/CC_SkillGalleryData.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/CC_AttributeSet.h"
namespace {
struct FHUDProbe {
 bool Pass=true;float FirstCD=0;
 TWeakObjectPtr<ACC_PlayerController> PC;
 TWeakObjectPtr<UCC_PlayerHealthHUDWidget> HUD;
 TSharedPtr<FJsonObject> Checks=MakeShared<FJsonObject>();
 void Check(bool B,const TCHAR* Name){Pass&=B;Checks->SetBoolField(Name,B);UE_LOG(LogTemp,Display,TEXT("COMBAT_HUD_CHECK %s: %d"),Name,B);}
 UCC_SkillComponent* Skills(){auto* C=PC.IsValid()?Cast<ACC_BaseCharacter>(PC->GetPawn()):nullptr;return C?C->Skills.Get():nullptr;}
 bool Key(FKey K,EInputEvent Event=IE_Pressed){if(!PC.IsValid()||!PC->InputComponent)return false;for(auto& B:PC->InputComponent->KeyBindings)if(B.Chord.Key==K&&B.KeyEvent==Event){B.KeyDelegate.Execute(K);return true;}return false;}
};
void CombatHUDSmoke(UWorld* W)
{
 if(!W||!W->IsGameWorld()||W->GetNetMode()!=NM_Standalone)return;
 auto Q=MakeShared<FHUDProbe>();
 auto Later=[W](float Delay,TFunction<void()> Fn){FTimerHandle T;W->GetTimerManager().SetTimer(T,MoveTemp(Fn),Delay,false);};
 Later(3.f,[W,Q,Later](){
  Q->PC=Cast<ACC_PlayerController>(W->GetFirstPlayerController());auto* H=Q->PC.IsValid()?Cast<ACC_ProgressionHUD>(Q->PC->GetHUD()):nullptr;Q->HUD=H?H->PlayerHealthHUD.Get():nullptr;
  Q->Check(Q->HUD.IsValid()&&Q->HUD->IsInViewport(),TEXT("widget_exists_in_real_map"));
  if(!Q->HUD.IsValid()||!Q->Skills()){GEngine->Exec(W,TEXT("QUIT"));return;}
  Q->Check(!Q->HUD->IsExpanded(),TEXT("starts_collapsed"));
  Q->Check(!Q->Key(EKeys::N),TEXT("n_has_no_standalone_travel_binding"));
  Q->Check(!Q->Key(EKeys::LeftControl)&&!Q->Key(EKeys::RightControl),TEXT("control_has_no_sprint_or_menu_binding"));
  Q->Check(!Q->PC->SetCombatKey(0,EKeys::RightMouseButton),TEXT("right_mouse_cannot_conflict_with_skill_binding"));
  Q->Key(EKeys::MouseScrollDown);Q->Check(Q->HUD->GetSelectedSlot()==0,TEXT("wheel_ignored_when_collapsed"));
  auto* PS=Q->PC->GetPlayerState<ACC_PlayerState>();PS->Progression->ApplyDeveloperPreset(true);
  auto* P=PS->Progression.Get();P->RestoreExperienceFromBackend(120);
  Q->Check(Q->HUD->GetDisplayedExperience()==120,TEXT("experience_restoration_updates_hud"));
  P->AddExperience(30);Q->Check(Q->HUD->GetDisplayedExperience()==150,TEXT("earned_experience_updates_hud"));
  const int32 Coins=P->CopperCoins;P->AddCopperCoins(7);Q->Check(Q->HUD->GetDisplayedCopperCoins()==Coins+7,TEXT("earned_copper_updates_hud"));
  P->RestoreExperienceFromBackend(100);Q->Check(Q->HUD->GetDisplayedExperience()==100,TEXT("decreased_experience_updates_hud"));
  Q->Check(Q->HUD->GetExperienceGoal()==0,TEXT("initial_ink_has_no_fake_level_goal"));
  bool GoalChecked=false;
  if(!P->Forms.IsEmpty())
  {
   auto* OldForm=P->CurrentForm.Get();const int32 OldTier=P->Forms[0].Tier;P->Forms[0].Tier=0;P->CurrentForm=P->Forms[0].Form;P->OnChanged.Broadcast();
   GoalChecked=Q->HUD->GetExperienceGoal()==P->GetEvolutionCost()&&Q->HUD->GetExperienceGoal()>0;
   P->Forms[0].Tier=OldTier;P->CurrentForm=OldForm;P->OnChanged.Broadcast();
  }
  Q->Check(GoalChecked,TEXT("evolution_progress_uses_actual_form_cost"));
  P->RestoreExperienceFromBackend(120);P->CopperCoins=35;P->OnChanged.Broadcast();
  P->bRemoteShopUnlocked=true;
  auto* Potion=LoadObject<UCC_ConsumableDefinition>(nullptr,TEXT("/Game/HPWH/Items/DA_Item_XP_Small.DA_Item_XP_Small"));
  if(Potion)P->BuyItem(Potion,2);Q->Check(Potion&&P->GetItemCount(Potion)==2,TEXT("item_slot_has_two_equipped_potions"));

  auto* Gallery=LoadObject<UCC_SkillGalleryData>(nullptr,TEXT("/Game/UI/MuralMenu/DA_SkillGallery.DA_SkillGallery"));
  const FName IDs[]={TEXT("bronze_guard"),TEXT("bronze_charged_hit"),TEXT("field_regrowth"),TEXT("temple_lamps")};
  bool Equipped=Gallery!=nullptr;
  for(int32 I=0;I<4&&Gallery;++I){const auto* Entry=Gallery->Entries.FindByPredicate([&](const FCC_SkillGalleryEntry& E){return E.DesignId==IDs[I];});Equipped&=Entry!=nullptr;if(Entry){PS->Progression->EquipSkill(I,Entry->ExecutableSkill);Equipped&=Q->Skills()->CurrentSlots.IsValidIndex(I)&&Q->Skills()->CurrentSlots[I].Skill==Entry->ExecutableSkill;}}
  Q->Check(Equipped,TEXT("four_actual_skills_equipped"));
  auto* ASC=Cast<ACC_BaseCharacter>(Q->PC->GetPawn())->GetAbilitySystemComponent();ASC->SetNumericAttributeBase(UCC_AttributeSet::GetMaxHealthAttribute(),188.f);ASC->SetNumericAttributeBase(UCC_AttributeSet::GetHealthAttribute(),150.f);
  Q->Check(Q->Key(EKeys::RightMouseButton)&&Q->HUD->IsExpanded(),TEXT("right_mouse_expands_combat_hud"));
  Q->Check(!Q->PC->IsProgressionMenuOpen()&&!Q->PC->IsMoveInputIgnored()&&!Q->PC->IsLookInputIgnored()&&!Q->PC->bShowMouseCursor,TEXT("expansion_keeps_gameplay_input"));
  Q->Key(EKeys::MouseScrollDown);Q->Check(Q->HUD->GetSelectedSlot()==1,TEXT("wheel_selects_item_category"));
  Q->Key(EKeys::Enter);Q->Key(EKeys::Enter,IE_Released);
  const int32 BeforeXP=P->Experience;
  Q->Key(EKeys::LeftMouseButton);Q->Key(EKeys::LeftMouseButton,IE_Released);
  Q->Check(Potion&&P->GetItemCount(Potion)==1&&P->Experience==BeforeXP+50&&P->GetItemCooldownRemaining()>14.9f,TEXT("left_click_uses_selected_item_once_with_fifteen_second_cooldown"));
  Q->Check(Q->Skills()->GetCooldownRemaining(4)==0,TEXT("item_click_does_not_also_basic_attack"));
  Q->Key(EKeys::LeftMouseButton);Q->Key(EKeys::LeftMouseButton,IE_Released);Q->Check(Potion&&P->GetItemCount(Potion)==1,TEXT("item_cooldown_rejects_second_click"));
  Q->Key(EKeys::RightMouseButton);Q->Key(EKeys::MouseScrollUp);Q->Check(Q->HUD->GetSelectedSlot()==0,TEXT("wheel_selects_skill_category"));
  Q->Key(EKeys::Enter);Q->Key(EKeys::Enter,IE_Released);
  Q->Check(Q->HUD->GetDisplayedCooldown(4)>14.9f,TEXT("wheel_selection_preserves_item_cooldown"));
  Q->Check(Q->Key(EKeys::LeftMouseButton),TEXT("left_click_activates_selected_skill"));Q->Key(EKeys::LeftMouseButton,IE_Released);
  Q->FirstCD=Q->Skills()->GetCooldownRemaining(0);Q->Check(Q->FirstCD>17.f,TEXT("selected_guard_uses_real_cooldown"));
  Later(.7f,[Q](){
   const float CD=Q->Skills()->GetCooldownRemaining(0);
   Q->Check(CD<Q->FirstCD&&FMath::Abs(CD-Q->HUD->GetDisplayedCooldown(0))<.25f,TEXT("cooldown_display_counts_down"));
   TArray<UWidget*> Widgets;Q->HUD->WidgetTree->GetAllWidgets(Widgets);int32 IconCount=0;bool HasCDText=false;
   for(auto* Widget:Widgets){if(auto* I=Cast<UImage>(Widget);I&&I->GetBrush().GetResourceObject()&&I->GetVisibility()!=ESlateVisibility::Hidden)++IconCount;if(auto* T=Cast<UTextBlock>(Widget);T&&T->GetText().ToString().Contains(TEXT("冷却 ")))HasCDText=true;}
   Q->Check(IconCount==4,TEXT("four_skill_icons_visible"));Q->Check(!HasCDText,TEXT("cooldown_seconds_hidden"));
   FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/CombatHUD/Expanded.png"),true,false);
  });
  Later(1.f,[Q](){Q->Key(EKeys::MouseScrollDown);Q->Check(Q->HUD->GetSelectedSlot()==1,TEXT("wheel_selects_next_skill"));Q->Key(EKeys::LeftMouseButton);});
  Later(1.2f,[Q](){Q->Key(EKeys::Enter,IE_Released);Q->Check(Q->Skills()->IsSlotHeld(1),TEXT("enter_release_does_not_release_mouse_charge"));});
  Later(1.4f,[Q](){Q->Key(EKeys::LeftMouseButton,IE_Released);});
  Later(1.6f,[Q](){Q->Check(!Q->Skills()->IsSlotHeld(1)&&Q->Skills()->GetCooldownRemaining(1)>11.f,TEXT("enter_release_finishes_selected_charge"));Q->Key(EKeys::RightMouseButton);Q->Check(Q->HUD->IsExpanded(),TEXT("right_mouse_returns_to_categories"));Q->Key(EKeys::RightMouseButton);Q->Check(!Q->HUD->IsExpanded(),TEXT("right_mouse_collapses_to_badge"));});
  Later(1.8f,[Q](){Q->Key(EKeys::Tab);Q->Check(Q->PC->IsProgressionMenuOpen(),TEXT("tab_opens_full_menu"));Q->Check(!Cast<ACC_PlayerCharacter>(Q->PC->GetPawn())->IsSprinting(),TEXT("tab_menu_does_not_trigger_sprint"));Q->Check(!Q->Key(EKeys::Escape),TEXT("escape_no_longer_has_game_menu_binding"));Q->Check(!Q->HUD->IsExpanded(),TEXT("full_menu_collapses_combat_hud"));Q->PC->CloseProgressionMenu();Q->Key(EKeys::RightMouseButton);Q->Key(EKeys::Enter);Q->Key(EKeys::Enter,IE_Released);});
  Later(2.4f,[Q](){Q->Check(!Q->Skills()->IsSlotHeld(1),TEXT("selection_does_not_reuse_charge_slot"));Q->Key(EKeys::Enter,IE_Released);Q->Key(EKeys::RightMouseButton);Q->Key(EKeys::RightMouseButton);FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/CombatHUD/Collapsed.png"),true,false);});
  Later(2.5f,[Q](){Q->Key(EKeys::Tab);Q->Check(Q->PC->IsProgressionMenuOpen(),TEXT("tab_reopens_full_menu"));});
  Later(2.6f,[Q](){FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(EKeys::Tab,FModifierKeysState(),0,false,0,0));Q->Check(!Q->PC->IsProgressionMenuOpen(),TEXT("tab_closes_focused_menu"));if(Q->PC->IsProgressionMenuOpen())Q->PC->CloseProgressionMenu();
   auto* C=Cast<ACC_PlayerCharacter>(Q->PC->GetPawn());C->ResetSprintState();Q->Key(EKeys::LeftShift);Q->Check(!C->IsSprinting()&&!Q->PC->IsProgressionMenuOpen(),TEXT("left_shift_does_not_start_sprint_immediately"));Q->Key(EKeys::LeftShift,IE_Released);Q->Check(!C->IsSprinting(),TEXT("left_shift_release_stops_sprint"));
   C->ResetSprintState();Q->Key(EKeys::RightShift);Q->Check(!C->IsSprinting()&&!Q->PC->IsProgressionMenuOpen(),TEXT("right_shift_does_not_start_sprint_immediately"));Q->Key(EKeys::RightShift,IE_Released);Q->Check(!C->IsSprinting(),TEXT("right_shift_release_stops_sprint"));
  });
  Later(2.7f,[Q](){
   Q->Skills()->CancelActiveSkills();Q->Skills()->ClearDesignStates();Cast<ACC_BaseCharacter>(Q->PC->GetPawn())->GetAbilitySystemComponent()->RemoveActiveEffects(FGameplayEffectQuery());
   Q->Skills()->RefreshSkillCooldowns();Q->Key(EKeys::RightMouseButton);Q->Key(EKeys::Enter);Q->Key(EKeys::Enter,IE_Released);Q->Key(EKeys::MouseScrollDown);Q->Key(EKeys::Enter);
  });
  Later(3.1f,[Q](){Q->Check(Q->Skills()->IsSlotHeld(1),TEXT("enter_holds_charge_before_collapse"));Q->Key(EKeys::RightMouseButton);Q->Check(!Q->Skills()->IsSlotHeld(1)&&Q->Skills()->GetCooldownRemaining(1)>2.f&&Q->Skills()->GetCooldownRemaining(1)<3.1f,TEXT("collapse_cancels_charge_without_firing"));});
  Later(3.4f,[Q](){Q->Skills()->CancelActiveSkills();Q->Skills()->ClearDesignStates();Cast<ACC_BaseCharacter>(Q->PC->GetPawn())->GetAbilitySystemComponent()->RemoveActiveEffects(FGameplayEffectQuery());Q->Skills()->RefreshSkillCooldowns();Q->Key(EKeys::Enter);Q->Key(EKeys::Enter,IE_Released);Q->Key(EKeys::MouseScrollDown);Q->Key(EKeys::Enter);});
  Later(3.8f,[Q](){Q->Key(EKeys::MouseScrollDown);Q->Check(Q->HUD->GetSelectedSlot()==2&&!Q->Skills()->IsSlotHeld(1)&&Q->Skills()->GetCooldownRemaining(1)>2.f&&Q->Skills()->GetCooldownRemaining(1)<3.1f,TEXT("wheel_cancels_previous_charge_without_firing"));Q->Key(EKeys::Enter,IE_Released);Q->Key(EKeys::RightMouseButton);});
  Later(4.2f,[Q](){
   Q->PC->CloseProgressionMenu();Q->HUD->SetExpanded(false);Q->Key(EKeys::RightMouseButton);
   Q->Key(EKeys::MouseScrollDown);Q->Key(EKeys::MouseScrollDown);Q->Key(EKeys::Enter);
   FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/CombatHUD/Forms.png"),true,false);
  });
  Later(5.2f,[W,Q](){auto Root=MakeShared<FJsonObject>();Root->SetBoolField(TEXT("passed"),Q->Pass);Root->SetObjectField(TEXT("checks"),Q->Checks);FString Json;FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Json));FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("CombatHUDSmoke.json")));UE_LOG(LogTemp,Display,TEXT("COMBAT_HUD_SMOKE_%s"),Q->Pass?TEXT("PASS"):TEXT("FAIL"));GEngine->Exec(W,TEXT("QUIT"));});
 });
}
FAutoConsoleCommandWithWorld CombatHUDProbe(TEXT("CC.CombatHUDSmokeTest"),TEXT("Explicit real-map RMB/Tab/wheel/Enter/cooldown and menu smoke test; exits its separate process."),FConsoleCommandWithWorldDelegate::CreateStatic(&CombatHUDSmoke));
}
#endif
