#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/IConsoleManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Components/InputComponent.h"
#include "Player/CC_PlayerController.h"
#include "Player/CC_PlayerState.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Progression/CC_ProgressionHUD.h"
#include "Progression/CC_FormPowerOrb.h"
#include "UI/CC_PlayerHealthHUDWidget.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

static void ChromaticInkSmoke(UWorld* W)
{
 if(!W||!W->IsGameWorld()||W->GetNetMode()!=NM_Standalone)return;
 FTimerHandle Timer;W->GetTimerManager().SetTimer(Timer,FTimerDelegate::CreateLambda([W]()
 {
  bool Pass=true;auto Report=MakeShared<FJsonObject>();
  auto Check=[&](bool B,const TCHAR* Name){Pass&=B;Report->SetBoolField(Name,B);UE_LOG(LogTemp,Display,TEXT("CHROMATIC_CHECK %s: %d"),Name,B);};
  auto* PC=Cast<ACC_PlayerController>(W->GetFirstPlayerController());
  auto* PS=PC?PC->GetPlayerState<ACC_PlayerState>():nullptr;
  auto* Player=PC?Cast<ACC_PlayerCharacter>(PC->GetPawn()):nullptr;
  auto* HUD=PC?Cast<ACC_ProgressionHUD>(PC->GetHUD()):nullptr;
  auto* Widget=HUD?HUD->PlayerHealthHUD.Get():nullptr;
  Check(PC&&PS&&Player&&Widget,TEXT("real_player_and_hud"));
  if(PC&&PS&&Player&&Widget)
  {
   Widget->TakeWidget(); // NullRHI has no viewport paint pass to construct Slate for us.
   auto* P=PS->Progression.Get();P->ApplyDeveloperPreset(false);
   auto Key=[&](){for(auto& B:PC->InputComponent->KeyBindings)if(B.Chord.Key==EKeys::Q&&B.KeyEvent==IE_Pressed){B.KeyDelegate.Execute(EKeys::Q);return true;}return false;};
   auto* ASC=Player->GetAbilitySystemComponent();
   const float HP=ASC->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute());
   const float Attack=ASC->GetNumericAttribute(UCC_AttributeSet::GetAttackAttribute());
   Check(Key()&&Player->IsChromaticMaterialApplied(),TEXT("Q_binding_enables_burning_preview"));
   Check(Widget->IsRainbowHealthDisplayed(),TEXT("Q_preview_updates_actual_rainbow_health_widget"));
   Check(!PC->SetCombatKey(0,EKeys::Q),TEXT("Q_reserved_from_combat_remapping"));
   Check(Key()&&!Player->IsChromaticMaterialApplied()&&!Widget->IsRainbowHealthDisplayed(),TEXT("second_Q_restores_original_visuals"));
   Check(ASC->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute())==HP&&ASC->GetNumericAttribute(UCC_AttributeSet::GetAttackAttribute())==Attack,TEXT("Q_does_not_change_combat_stats"));
   for(FName Id:{FName(TEXT("Native")),FName(TEXT("Artifact_01")),FName(TEXT("Artifact_02"))})
   {
    auto Found=P->ArtifactForms.FindByPredicate([&](const UCC_FormDefinition* F){return F&&F->FormId==Id;});
    if(!Found){Check(false,TEXT("element_catalog"));continue;}
    auto* Orb=W->SpawnActor<ACC_FormPowerOrb>();Orb->Recipient=PS;Orb->RewardForm=*Found;Orb->ExperienceValue=0;
    Orb->SetActorLocation(Player->GetActorLocation());Orb->BeginSeeking();
    Check(Orb->Phase==ECC_ExperienceOrbPhase::Collected||Orb->TryCollect(Player),*FString::Printf(TEXT("collect_%s"),*Id.ToString()));
   }
   Check(P->HasChromaticUnlock()&&Player->IsChromaticMaterialApplied()&&Widget->IsRainbowHealthDisplayed(),TEXT("three_elements_automatically_enable_outline_and_rainbow_bar"));
   Key();Check(!Player->IsChromaticMaterialApplied()&&Widget->IsRainbowHealthDisplayed(),TEXT("Q_hides_outline_but_keeps_unlocked_rainbow_bar"));
   Key();Check(Player->IsChromaticMaterialApplied(),TEXT("Q_reenables_unlocked_outline"));
   PC->OpenProgressionMenu();const bool Before=P->IsChromaticAppearanceEnabled();Key();
   Check(P->IsChromaticAppearanceEnabled()==Before,TEXT("Q_ignored_while_menu_open"));PC->CloseProgressionMenu();
   Check(ASC->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute())==HP&&ASC->GetNumericAttribute(UCC_AttributeSet::GetAttackAttribute())==Attack,TEXT("cosmetic_unlock_preserves_health_and_attack"));
  }
  Report->SetBoolField(TEXT("passed"),Pass);FString Text;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Text));
  FFileHelper::SaveStringToFile(Text,*(FPaths::ProjectSavedDir()/TEXT("ChromaticInkSmoke.json")));
  GEngine->Exec(W,TEXT("QUIT"));
 }),3.f,false);
}
static FAutoConsoleCommandWithWorld Command(TEXT("CC.ChromaticInkSmoke"),TEXT("Verify Q, collection and rainbow health on the real player without writing saves."),FConsoleCommandWithWorldDelegate::CreateStatic(&ChromaticInkSmoke));
#endif
