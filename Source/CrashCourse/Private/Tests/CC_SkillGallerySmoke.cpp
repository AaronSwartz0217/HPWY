#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/IConsoleManager.h"
#include "HAL/FileManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Player/CC_PlayerController.h"
#include "Player/CC_PlayerState.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Characters/CC_PatrolEnemyCharacter.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Progression/CC_SkillComponent.h"
#include "UI/CC_MuralMenuWidget.h"
#include "UI/CC_MenuArtData.h"
#include "UI/CC_SkillGalleryData.h"
#include "UI/CC_SkillGalleryWidget.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
namespace {
void RunSkillGallerySmoke(UWorld* W) {
 if (!W || !W->IsGameWorld() || W->GetNetMode()!=NM_Standalone) return;
 FTimerHandle Start; W->GetTimerManager().SetTimer(Start,[W]() {
  bool Pass=true; auto Check=[&](bool V,const TCHAR* Label) { Pass &= V; UE_LOG(LogTemp,Display,TEXT("GALLERY_%s: %d"),Label,V); };
  auto* PC=Cast<ACC_PlayerController>(W->GetFirstPlayerController()); auto* PS=PC ? PC->GetPlayerState<ACC_PlayerState>() : nullptr; auto* C=PC ? Cast<ACC_PlayerCharacter>(PC->GetPawn()) : nullptr;
  Check(PC && PS && C,TEXT("player_ready"));
  if (!PC || !PS || !C) { GEngine->Exec(W,TEXT("QUIT")); return; }
  for (TActorIterator<ACC_PatrolEnemyCharacter> It(W);It;++It) It->StopPatrol();
  PC->OpenProgressionMenu(); auto* M=Cast<UCC_MuralMenuWidget>(PC->GetProgressionMenuWidget()); auto* G=M ? M->GetSkillGallery() : nullptr;
  Check(G && G->PresentationData,TEXT("gallery_data_configured"));
  if (!G || !G->PresentationData) { GEngine->Exec(W,TEXT("QUIT")); return; }
  auto* P=PS->Progression.Get(); const auto Manual=P->ManualSkills; const auto Active=P->GetActiveForm(); const int32 XP=P->Experience; const auto Slots=C->Skills->CurrentSlots;
  auto* D=G->PresentationData.Get(); Check(D->Entries.Num()==18 && D->Categories.Num()==4 && D->CategoryTabs.Num()==4,TEXT("18_entries_four_categories"));
  bool AllIcons=true; for (const auto& E:D->Entries) AllIcons &= E.Icon.LoadSynchronous()!=nullptr && !E.DisplayName.IsEmpty() && !E.Description.IsEmpty();
  Check(AllIcons,TEXT("all_icons_and_descriptions_available"));
  const int32 Counts[]={6,4,4,4}; for (int32 I=0;I<4;++I) { G->SelectCategory(I); Check(G->GetVisibleCardCount()==Counts[I],*FString::Printf(TEXT("category_%d_count"),I)); }
  G->SelectCategory(1); Check(D->Entries.IsValidIndex(G->GetSelectedEntry()) && D->Entries[G->GetSelectedEntry()].DesignId==TEXT("bronze_charged_hit"),TEXT("charged_hit_default_selection"));
  const auto& E=D->Entries[G->GetSelectedEntry()]; Check(E.StatValues.Num()==4 && E.StatValues[0].ToString().Contains(TEXT("400%")) && E.StatValues[2].ToString()==TEXT("12秒"),TEXT("design_stat_table_loaded"));
  G->SelectPreviewSlot(3); G->PreviewEquip(); Check(G->GetPreviewEntry(3)!=G->GetSelectedEntry(),TEXT("locked_school_equipment_rejected"));
  bool Unchanged=P->ManualSkills==Manual && P->GetActiveForm()==Active && P->Experience==XP && C->Skills->CurrentSlots.Num()==Slots.Num();
  for (int32 I=0;I<Slots.Num();++I) Unchanged &= C->Skills->CurrentSlots[I].Skill==Slots[I].Skill && C->Skills->CurrentSlots[I].Level==Slots[I].Level;
  Check(Unchanged,TEXT("locked_request_does_not_change_combat_or_progress"));
  // Capture a fresh gallery after verifying the server-backed school restriction.
  PC->CloseProgressionMenu(); PC->OpenProgressionMenu();
  M=Cast<UCC_MuralMenuWidget>(PC->GetProgressionMenuWidget()); G=M->GetSkillGallery();
  M->ShowPage(ECC_MuralMenuPage::Skills);
  for (int32 I=0;I<4;++I) {
   FTimerHandle Capture; W->GetTimerManager().SetTimer(Capture,[G,I]() { G->SelectCategory(I); FTimerHandle Wait; G->GetWorld()->GetTimerManager().SetTimer(Wait,[I]() { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Screenshots/SkillGallery/Category%d.png"),I),true,false); },.4f,false); },.6f+I*1.1f,false);
  }
  FTimerHandle Finish; W->GetTimerManager().SetTimer(Finish,[W,PC,Pass]() {
   PC->CloseProgressionMenu(); bool Result=Pass && !PC->IsProgressionMenuOpen() && !PC->IsMoveInputIgnored();
   for (int32 I=0;I<4;++I) Result &= IFileManager::Get().FileSize(*(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Screenshots/SkillGallery/Category%d.png"),I)))>0;
   FFileHelper::SaveStringToFile(Result ? TEXT("{\"passed\":true}") : TEXT("{\"passed\":false}"),*(FPaths::ProjectSavedDir()/TEXT("SkillGallerySmoke.json")));
   UE_LOG(LogTemp,Display,TEXT("SKILL_GALLERY_SMOKE_%s"),Result ? TEXT("PASS") : TEXT("FAIL")); GEngine->Exec(W,TEXT("QUIT"));
  },5.6f,false);
 },3.f,false);
}
FAutoConsoleCommandWithWorld GallerySmoke(TEXT("CC.SkillGallerySmokeTest"),TEXT("Presentation-only skill gallery, no combat changes, four category screenshots."),FConsoleCommandWithWorldDelegate::CreateStatic(&RunSkillGallerySmoke));
}
#endif
