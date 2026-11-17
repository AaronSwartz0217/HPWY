#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Player/CC_PlayerController.h"
#include "Player/CC_PlayerState.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Characters/CC_PatrolEnemyCharacter.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Progression/CC_ProgressionSave.h"
#include "World/CC_AdventureGameInstance.h"
#include "Progression/CC_SkillComponent.h"
#include "Progression/CC_ExperienceOrb.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
namespace {
void RunDeveloperKeysSmoke(UWorld* W) {
 if (!W || !W->IsGameWorld() || W->GetNetMode()!=NM_Standalone) return;
 FTimerHandle Start; W->GetTimerManager().SetTimer(Start,[W]() {
  bool Passed=true; auto Results=MakeShared<FJsonObject>();
  auto Check=[&](bool V,const TCHAR* Label) { Passed &= V; Results->SetBoolField(Label,V); UE_LOG(LogTemp,Display,TEXT("DEVKEY_%s: %d"),Label,V); };
  auto* PC=Cast<ACC_PlayerController>(W->GetFirstPlayerController());
  auto* C=PC ? Cast<ACC_PlayerCharacter>(PC->GetPawn()) : nullptr;
  auto* PS=PC ? PC->GetPlayerState<ACC_PlayerState>() : nullptr;
  Check(PC && C && PS,TEXT("player_ready"));
  const FString SavePath=FPaths::ProjectSavedDir()/TEXT("SaveGames/SpiritTales.sav"); TArray<uint8> Before,After; const bool HadSave=FFileHelper::LoadFileToArray(Before,*SavePath);
  if (PC && C && PS) {
   for (TActorIterator<ACC_PatrolEnemyCharacter> It(W);It;++It) It->StopPatrol();
   auto Dispatch=[PC](FKey Key) { for (const auto& B:PC->InputComponent->KeyBindings) if (B.Chord.Key==Key && B.KeyEvent==IE_Pressed) { B.KeyDelegate.Execute(Key); return true; } return false; };
   auto* P=PS->Progression.Get(); auto* ASC=C->GetAbilitySystemComponent(); auto* AS=Cast<UCC_AttributeSet>(C->GetAttributeSet());
   Check(!PC->SetCombatKey(0,EKeys::Zero) && !PC->SetCombatKey(0,EKeys::Nine),TEXT("developer_keys_reserved"));
   Check(Dispatch(EKeys::Nine),TEXT("nine_key_bound_and_dispatched_initially"));
   int32 OpenRoutes=0; bool MaxRoutes=true;
   for (const auto& F:P->ArtifactForms) if (F && F->bAvailable && F->IsComplete()) { ++OpenRoutes; MaxRoutes &= P->Forms.ContainsByPredicate([F](const FCC_FormProgress& R) { return R.Form==F && R.Tier==(F->bCanEvolve ? 2 : 0); }); }
   Check(P->Forms.Num()==OpenRoutes && MaxRoutes,TEXT("all_open_routes_maxed"));
   bool MaxUpgrades=true; for (const auto& U:P->UpgradeCatalog) if (U) MaxUpgrades &= P->GetUpgradeLevel(U)==U->Prices.Num();
   Check(MaxUpgrades && P->Experience==1000000,TEXT("all_upgrades_and_test_currency"));
   Check(AS->GetHealth()==AS->GetMaxHealth() && AS->GetMana()==AS->GetMaxMana(),TEXT("full_health_and_mana"));
   bool EquipAll=P->SkillCatalog.Num()>=22; for (const auto& Skill:P->SkillCatalog) { P->EquipSkill(0,Skill); EquipAll &= C->Skills->CurrentSlots[0].Skill==Skill; }
   Check(EquipAll && C->Skills->CurrentSlots.Num()==4,TEXT("all_22_skills_equippable_four_slots"));
   const int32 NumUpgrades=P->Upgrades.Num(); const float Attack=AS->GetAttack(); Dispatch(EKeys::Nine);
   Check(P->Upgrades.Num()==NumUpgrades && AS->GetAttack()==Attack && P->Experience==1000000,TEXT("repeated_nine_does_not_stack"));
   if (!P->Forms.IsEmpty()) P->SwitchForm(P->Forms[0].Form);
   ACC_ExperienceOrb* Orb=W->SpawnActor<ACC_ExperienceOrb>(); Orb->Recipient=PS;
   C->DeathToRagdollDelay=0.f; ASC->SetNumericAttributeBase(AS->GetHealthAttribute(),0.f);
   Check(!C->IsAlive(),TEXT("death_fixture_ready"));
   Check(Dispatch(EKeys::Nine),TEXT("nine_key_bound_and_dispatched"));
   Check(C->IsAlive() && !C->IsRagdollActive(),TEXT("nine_restores_living_character"));
   auto AllItems999=[P](){bool All=P->ItemCatalog.Num()>=7;for(const auto& Item:P->ItemCatalog)if(Item&&Item->IsValidItem())All&=P->GetItemCount(Item)==999&&P->GetItemStackLimit(Item)==999;return All;};
   Check(P->bDeveloperInventory&&P->bRemoteShopUnlocked&&AllItems999(),TEXT("nine_unlocks_remote_shop_and_every_item_999"));
   auto* Adventure=W->GetGameInstance<UCC_AdventureGameInstance>();bool AllRegions=Adventure!=nullptr;
   if(Adventure)for(const auto& Region:Adventure->Regions)AllRegions&=Adventure->IsRegionUnlocked(Region.Id);
   Check(AllRegions&&Adventure->HasTravelScroll(TEXT("Arena"))&&Adventure->HasTravelScroll(TEXT("Heaven")),TEXT("nine_unlocks_all_regions_and_scroll_access"));
   Check(P->HasChromaticUnlock()&&P->IsChromaticAppearanceEnabled(),TEXT("nine_enables_chromatic_form"));
   auto* TestSnapshot=P->CreateSnapshot();Check(TestSnapshot&&TestSnapshot->bDeveloperInventory&&P->RestoreSnapshot(TestSnapshot)&&AllItems999(),TEXT("travel_snapshot_preserves_999_instead_of_clamping_to_99"));
   Check(Adventure&&Adventure->RequestFastTravel(TEXT("Heaven"))&&Adventure->IsPortalDeparturePending()&&Adventure->GetTravelCooldownRemaining()>0,TEXT("pending_travel_reset_fixture"));
   Check(Dispatch(EKeys::Zero),TEXT("zero_key_resets_current_progress"));
   Check(!P->bDeveloperInventory&&!P->bRemoteShopUnlocked&&P->ItemStacks.IsEmpty()&&!P->EquippedItem&&P->CopperCoins==0&&!P->HasChromaticUnlock(),TEXT("zero_clears_inventory_currency_permission_and_chromatic_form"));
   Check(Adventure&&Adventure->VisitedRegions.IsEmpty()&&Adventure->GetTravelCooldownRemaining()==0&&!Adventure->IsPortalDeparturePending(),TEXT("zero_resets_travel_progress_and_cooldown"));
   Check(P->GetActiveForm()==P->NativeForm && P->NativeForm->FormId==TEXT("InitialInk") && P->CurrentTier==0 && P->NativeTier==0,TEXT("initial_ink_restored"));
   Check(P->Experience==0 && P->Forms.IsEmpty() && P->Upgrades.IsEmpty() && P->ManualSkills.IsEmpty(),TEXT("all_progression_cleared"));
   FCC_FormStage InitialStage;const bool HasInitialStage=P->GetStagePreview(P->NativeForm,0,InitialStage);
   Check(HasInitialStage&&FMath::IsNearlyEqual(AS->GetHealth(),InitialStage.MaxHealth)&&FMath::IsNearlyEqual(AS->GetMaxHealth(),InitialStage.MaxHealth)&&FMath::IsNearlyEqual(AS->GetAttack(),InitialStage.Attack)&&AS->GetMana()==AS->GetMaxMana()&&AS->GetShield()==0.f,TEXT("initial_attributes_restored_from_current_profile"));
   const auto* Loadout=P->NativeForm->Stages[0].Loadout.Get(); bool DefaultSkills=C->Skills->CurrentSlots.Num()==4;
   for (int32 I=0;I<4 && DefaultSkills;++I) DefaultSkills &= C->Skills->CurrentSlots[I].Skill==Loadout->Slots[I].Skill;
   Check(DefaultSkills,TEXT("default_four_skills_restored"));
   bool NoCooldown=true; for (int32 I=0;I<5;++I) NoCooldown &= C->Skills->GetCooldownRemaining(I)==0.f;
   Check(NoCooldown && ASC->GetActiveEffects(FGameplayEffectQuery()).IsEmpty(),TEXT("effects_and_cooldowns_cleared"));
   Check(Orb->IsActorBeingDestroyed(),TEXT("pending_experience_removed"));
   Dispatch(EKeys::Zero);Check(P->Experience==0&&P->Upgrades.IsEmpty(),TEXT("repeated_zero_stays_initial"));
   Dispatch(EKeys::Nine);Dispatch(EKeys::Nine);Check(P->Experience==1000000&&AllItems999()&&P->ItemStacks.Num()==P->ItemCatalog.Num(),TEXT("repeated_nine_refills_999_without_duplicate_stacks"));
  }
  Check(FFileHelper::LoadFileToArray(After,*SavePath)==HadSave && After==Before,TEXT("user_progress_save_unchanged"));
  auto Root=MakeShared<FJsonObject>(); Root->SetBoolField(TEXT("passed"),Passed); Root->SetObjectField(TEXT("checks"),Results);
  FString Json; FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Json)); FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("DeveloperKeysSmoke.json")));
  UE_LOG(LogTemp,Display,TEXT("DEVELOPER_KEYS_SMOKE_%s"),Passed ? TEXT("PASS") : TEXT("FAIL"));
  GEngine->Exec(W,TEXT("QUIT"));
 },3.f,false);
}
FAutoConsoleCommandWithWorld DevKeysSmoke(TEXT("CC.DeveloperKeysSmokeTest"),TEXT("Standalone probe of full sandbox unlock, 999 inventory and snapshot persistence; no saves changed."),FConsoleCommandWithWorldDelegate::CreateStatic(&RunDeveloperKeysSmoke));
}
#endif
