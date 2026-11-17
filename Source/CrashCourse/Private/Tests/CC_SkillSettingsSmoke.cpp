#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Characters/CC_PatrolEnemyCharacter.h"
#include "Player/CC_PlayerController.h"
#include "Player/CC_PlayerState.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Progression/CC_ArtifactComponent.h"
#include "Progression/CC_SkillComponent.h"
#include "Progression/CC_SkillProjectile.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UI/CC_MuralMenuWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

namespace {
struct FCC_SettingsProbe {
 bool Pass=true;
 TWeakObjectPtr<ACC_PlayerController> PC;
 TWeakObjectPtr<ACC_PlayerCharacter> Player;
 TWeakObjectPtr<ACC_PatrolEnemyCharacter> Enemy;
 FKey OldKey;
 FString ControlsPath;
 TArray<uint8> ControlBytes;
 bool HadControlSave=false;
 void Check(bool V,const TCHAR* Label) { Pass &= V; UE_LOG(LogTemp,Display,TEXT("SETTINGS_%s: %d"),Label,V); }
};
void RunSettingsSmoke(UWorld* W) {
 if (!W || !W->IsGameWorld() || W->GetNetMode()!=NM_Standalone) {
  UE_LOG(LogTemp,Warning,TEXT("CC.SkillSettingsSmokeTest requires a dedicated standalone -game process; no editor worlds or network clients."));
  return;
 }
 auto Probe=MakeShared<FCC_SettingsProbe>();
 FTimerHandle Start; W->GetTimerManager().SetTimer(Start,[W,Probe]() {
  auto* PC=Cast<ACC_PlayerController>(W->GetFirstPlayerController()); auto* Player=PC ? Cast<ACC_PlayerCharacter>(PC->GetPawn()) : nullptr;
  if (!PC || !Player) { GEngine->Exec(W,TEXT("QUIT")); return; }
  Probe->PC=PC; Probe->Player=Player;
  for (TActorIterator<ACC_PatrolEnemyCharacter> It(W);It;++It) It->StopPatrol();
  auto* P=PC->GetPlayerState<ACC_PlayerState>()->Progression.Get();
  Probe->Check(P->NativeForm && P->NativeForm->FormId==TEXT("InitialInk") && !P->CurrentForm,TEXT("initial_ink_default"));
  Probe->Check(P->SkillCatalog.Num()>=22,TEXT("22_catalog_entries"));
  for (const auto& S:P->SkillCatalog) Probe->Check(S && !S->Description.IsEmpty(),TEXT("skill_description"));
  // Ink's attack now depends on its held experience. Acquire Nature into the
  // inventory and explicitly use it so this probe retains its 15 * .75 hit.
  UCC_FormDefinition* Nature=LoadObject<UCC_FormDefinition>(nullptr,TEXT("/Game/HPWH/Forms/DA_Form_Native.DA_Form_Native"));
  AActor* NatureArtifact=nullptr;
  for (TActorIterator<AActor> It(W);It;++It) if (auto* A=It->FindComponentByClass<UCC_ArtifactComponent>(); A && A->Form==Nature && A->bEnabled && !It->IsHidden()) { NatureArtifact=*It; break; }
  bool CanAcquire=false;
  if (NatureArtifact && Nature) {
   auto* Artifact=NatureArtifact->FindComponentByClass<UCC_ArtifactComponent>();
   for (const FVector& Offset : {FVector(160,0,100),FVector(-160,0,100),FVector(0,160,100),FVector(0,-160,100)}) {
    Player->SetActorLocation(NatureArtifact->GetActorLocation()+Offset,false,nullptr,ETeleportType::TeleportPhysics);
    if (Artifact->CanInteract(Player)) { CanAcquire=true; break; }
   }
   auto* Before=P->GetActiveForm();
   if (CanAcquire) P->Interact(NatureArtifact);
   Probe->Check(P->GetActiveForm()==Before,TEXT("acquisition_does_not_transform"));
   Probe->Check(P->Forms.ContainsByPredicate([Nature](const FCC_FormProgress& F) { return F.Form==Nature; }),TEXT("nature_stored_in_inventory"));
   P->SwitchForm(Nature);
  }
  Probe->Check(CanAcquire && P->GetActiveForm()==Nature && P->CurrentTier==0,TEXT("explicit_inventory_use_selects_nature"));
  const auto* SourceAS=Cast<UCC_AttributeSet>(Player->GetAttributeSet());
  Probe->Check(SourceAS && FMath::IsNearlyEqual(SourceAS->GetAttack(),15.f),TEXT("nature_attack_baseline_15"));
  Player->SetActorLocation(FVector(3500,-3000,100),false,nullptr,ETeleportType::TeleportPhysics);
  PC->OpenProgressionMenu();
  TArray<UUserWidget*> Widgets; UWidgetBlueprintLibrary::GetAllWidgetsOfClass(W,Widgets,UCC_MuralMenuWidget::StaticClass(),true);
  Probe->Check(Widgets.Num()==1,TEXT("menu_open"));
  if (Widgets.Num()!=1) { GEngine->Exec(W,TEXT("QUIT")); return; }
  auto* Menu=Widgets[0]; Menu->ProcessEvent(Menu->FindFunction(TEXT("SkillsPage")),nullptr);
  bool Numeric=false; Menu->WidgetTree->ForEachWidget([&Numeric](UWidget* Widget) { if (auto* T=Cast<UTextBlock>(Widget)) Numeric |= T->GetText().ToString().Contains(TEXT("75.0%")) && T->GetText().ToString().Contains(TEXT("冷却")); });
  Probe->Check(Numeric,TEXT("panel_numeric_descriptions"));
  Menu->ProcessEvent(Menu->FindFunction(TEXT("Equip")),nullptr);
  Probe->Check(Player->Skills->CurrentSlots[0].Skill==P->SkillCatalog[0],TEXT("panel_equips_particle"));
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/SkillPanel.png"),true,false);
  FTimerHandle Settings; W->GetTimerManager().SetTimer(Settings,[Probe,W]() {
   auto* PC=Probe->PC.Get(); auto* Player=Probe->Player.Get();
   TArray<UUserWidget*> Menus; UWidgetBlueprintLibrary::GetAllWidgetsOfClass(W,Menus,UCC_MuralMenuWidget::StaticClass(),true);
   if (Menus.Num()) Menus[0]->ProcessEvent(Menus[0]->FindFunction(TEXT("Settings")),nullptr);
   Probe->ControlsPath=FPaths::ProjectSavedDir()/TEXT("SaveGames/SpiritTalesControls.sav");
   Probe->HadControlSave=FFileHelper::LoadFileToArray(Probe->ControlBytes,*Probe->ControlsPath);
   Probe->OldKey=PC->GetCombatKey(0);
   Probe->Check(!PC->SetCombatKey(0,EKeys::W),TEXT("movement_key_protected"));
   Probe->Check(!PC->SetCombatKey(0,PC->GetCombatKey(1)),TEXT("duplicate_key_rejected"));
   bool Rebound=false; FKey NewKey;
   for (FKey K : {EKeys::Q,EKeys::R,EKeys::T,EKeys::F,EKeys::G}) if (PC->SetCombatKey(0,K)) { Rebound=true; NewKey=K; break; }
   Probe->Check(Rebound && PC->GetCombatKey(0)==NewKey,TEXT("rebind_and_persist"));
   PC->CloseProgressionMenu();
   Player->SetActorLocation(FVector(3500,-3000,100)); Player->SetActorRotation(FRotator::ZeroRotator); Player->GetCharacterMovement()->DisableMovement(); PC->SetControlRotation(FRotator::ZeroRotator);
   FTransform Transform(FRotator(0,180,0),FVector(4000,-3000,100));
   UClass* Class=LoadClass<ACC_PatrolEnemyCharacter>(nullptr,TEXT("/Game/HPWH/Blueprints/BP_Ceramic_Elite.BP_Ceramic_Elite_C"));
   auto* Enemy=Class ? W->SpawnActorDeferred<ACC_PatrolEnemyCharacter>(Class,Transform,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn) : nullptr;
   Probe->Enemy=Enemy;
   if (Enemy) {
    Enemy->bStartPatrolling=false; Enemy->ExperienceReward=0; Enemy->FinishSpawning(Transform); Enemy->GetCharacterMovement()->DisableMovement();
    Probe->Check(Enemy->GetAbilitySystemComponent()->GetNumericAttribute(UCC_AttributeSet::GetMaxHealthAttribute())==1050,TEXT("ceramic_elite_live_health"));
    Probe->Check(Enemy->GetMesh()->GetRelativeScale3D().Equals(Enemy->OriginalProfileMeshScale*1.2f,.001f),TEXT("elite_live_scale_1_2"));
   } else Probe->Check(false,TEXT("spawn_elite"));
   bool KeyExecuted=false;
   for (const auto& Binding:PC->InputComponent->KeyBindings) if (Binding.Chord.Key==NewKey && Binding.KeyEvent==IE_Pressed) { Binding.KeyDelegate.Execute(NewKey); KeyExecuted=true; break; }
   Probe->Check(KeyExecuted,TEXT("new_key_dispatches_skill"));
   PC->SetCombatKey(0,Probe->OldKey);
   if (Probe->HadControlSave) FFileHelper::SaveArrayToFile(Probe->ControlBytes,*Probe->ControlsPath); else UGameplayStatics::DeleteGameInSlot(TEXT("SpiritTalesControls"),0);
  },.5f,false);
  FTimerHandle Finish; W->GetTimerManager().SetTimer(Finish,[W,Probe]() {
   auto* Enemy=Probe->Enemy.Get();
   Probe->Check(Enemy && FMath::IsNearlyEqual(Enemy->GetAbilitySystemComponent()->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute()),1038.75f,.01f),TEXT("actual_projectile_hit_11_25"));
   FFileHelper::SaveStringToFile(Probe->Pass ? TEXT("{\"pass\":true}") : TEXT("{\"pass\":false}"),*(FPaths::ProjectSavedDir()/TEXT("SkillSettingsSmoke.json")));
   UE_LOG(LogTemp,Display,TEXT("SKILL_SETTINGS_SMOKE_%s"),Probe->Pass ? TEXT("PASS") : TEXT("FAIL"));
   GEngine->Exec(W,TEXT("QUIT"));
  },2.f,false);
 },3.f,false);
}
FAutoConsoleCommandWithWorld SettingsSmoke(TEXT("CC.SkillSettingsSmokeTest"),TEXT("Dedicated standalone game smoke: Ink default, acquire/use Nature, 22-skill mural panel, key rebinding, elite scale and a real 15 * .75 particle hit."),FConsoleCommandWithWorldDelegate::CreateStatic(&RunSettingsSmoke));
}
#endif
