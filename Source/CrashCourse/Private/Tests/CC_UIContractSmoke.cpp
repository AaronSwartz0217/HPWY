#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Player/CC_PlayerController.h"
#include "Player/CC_PlayerState.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Characters/CC_PatrolEnemyCharacter.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Progression/CC_SkillComponent.h"
#include "UI/CC_ProgressionMenu.h"
#include "UI/CC_AttributeWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
namespace {
void RunUIContractSmoke(UWorld* World) {
 if (!World) return;
 FTimerHandle Start;
 World->GetTimerManager().SetTimer(Start,[World]() {
  bool Pass=true;
  auto Check=[&Pass](bool Value,const TCHAR* Label) { Pass &= Value; UE_LOG(LogTemp,Display,TEXT("UI_CONTRACT_%s: %d"),Label,Value); };
  auto* PC=Cast<ACC_PlayerController>(World->GetFirstPlayerController());
  auto* PS=PC ? PC->GetPlayerState<ACC_PlayerState>() : nullptr;
  auto* Player=PC ? Cast<ACC_PlayerCharacter>(PC->GetPawn()) : nullptr;
  Check(PC && PS && Player,TEXT("player_ready"));
  if (PC && PS && Player) {
   for (TActorIterator<ACC_PatrolEnemyCharacter> It(World);It;++It) It->StopPatrol();
   auto* P=PS->Progression.Get();
   FCC_FormStage Preview;
   Check(P->GetActiveForm()==P->NativeForm,TEXT("active_native_form"));
   Check(P->GetStagePreview(nullptr,0,Preview) && Preview.MaxHealth==75.f && Preview.Attack==11.f,TEXT("initial_ink_stage_resolves_balance"));
   auto* Nature=LoadObject<UCC_FormDefinition>(nullptr,TEXT("/Game/HPWH/Forms/DA_Form_Native.DA_Form_Native"));
   Check(Nature && P->GetStagePreview(Nature,1,Preview) && Preview.MaxHealth==250.f && Preview.Attack==37.5f,TEXT("nature_stage_resolves_balance"));
   Check(P->GetStagePreview(P->ArtifactForms[0],2,Preview) && Preview.MaxHealth==500.f && Preview.Attack==75.f,TEXT("locked_bronze_can_be_previewed"));
   Check(!P->GetStagePreview(nullptr,3,Preview),TEXT("invalid_preview_tier_rejected"));
   Check(!P->GetStagePreview(P->ArtifactForms[2],0,Preview),TEXT("unavailable_route_hidden"));
   const auto PreviousClass=PC->ProgressionMenuClass;
   PC->CloseProgressionMenu(); PC->ProgressionMenuClass=nullptr;
   PC->OpenProgressionMenu(); auto* Menu=PC->GetProgressionMenuWidget();
   Check(Menu && Menu->IsA<UCC_ProgressionMenu>() && PC->IsProgressionMenuOpen(),TEXT("native_menu_fallback"));
   PC->OpenProgressionMenu(); Check(PC->GetProgressionMenuWidget()==Menu,TEXT("open_is_idempotent"));
   auto HasText=[Menu](const TCHAR* Word) {
    bool Found=false;
    if (Menu && Menu->WidgetTree) Menu->WidgetTree->ForEachWidget([&Found,Word](UWidget* W) { if (auto* T=Cast<UTextBlock>(W)) Found |= T->GetText().ToString().Contains(Word); });
    return Found;
   };
   P->EquipSkill(-1,P->SkillCatalog[0]); Check(HasText(TEXT("无效的技能槽位")),TEXT("invalid_slot_result_reaches_ui"));
   P->EquipSkill(0,nullptr); Check(HasText(TEXT("技能未在可装备目录中")),TEXT("invalid_catalog_result_reaches_ui"));
   P->EquipSkill(0,P->SkillCatalog[0]); Check(HasText(TEXT("技能配置成功")) && Player->Skills->CurrentSlots[0].Skill==P->SkillCatalog[0],TEXT("success_result_and_actual_loadout"));
   P->ResetSkills(); Check(HasText(TEXT("技能配置成功")) && P->ManualSkills.IsEmpty(),TEXT("reset_result_reaches_ui"));
   PC->CloseProgressionMenu(); PC->CloseProgressionMenu();
   Check(!PC->IsProgressionMenuOpen() && !PC->GetProgressionMenuWidget() && !PC->IsMoveInputIgnored() && !PC->IsLookInputIgnored(),TEXT("close_is_idempotent_and_unlocks"));
   PC->ProgressionMenuClass=UCC_AttributeWidget::StaticClass();
   PC->SetIgnoreMoveInput(true);
   PC->OpenProgressionMenu();
   Check(PC->GetProgressionMenuWidget() && PC->GetProgressionMenuWidget()->IsA<UCC_AttributeWidget>(),TEXT("custom_user_widget_class_used"));
   Check(PC->IsMoveInputIgnored() && PC->bShowMouseCursor,TEXT("custom_menu_uses_same_input_flow"));
   PC->CloseProgressionMenu(); Check(PC->IsMoveInputIgnored(),TEXT("close_preserves_external_input_lock")); PC->SetIgnoreMoveInput(false);
   PC->ProgressionMenuClass=PreviousClass;
   Check(PC->FindFunction(TEXT("CloseProgressionMenu"))->HasAnyFunctionFlags(FUNC_BlueprintCallable),TEXT("close_exposed_to_blueprint"));
  }
  FFileHelper::SaveStringToFile(Pass ? TEXT("{\"passed\":true}") : TEXT("{\"passed\":false}"),*(FPaths::ProjectSavedDir()/TEXT("UIContractSmoke.json")));
  UE_LOG(LogTemp,Display,TEXT("UI_CONTRACT_SMOKE_%s"),Pass ? TEXT("PASS") : TEXT("FAIL"));
  GEngine->Exec(World,TEXT("QUIT"));
 },3.f,false);
}
FAutoConsoleCommandWithWorld UIContractSmoke(TEXT("CC.UIContractSmokeTest"),TEXT("Verify designer menu injection, lifecycle, skill result callbacks and evolution preview; no assets or saves modified."),FConsoleCommandWithWorldDelegate::CreateStatic(&RunUIContractSmoke));
}
#endif
