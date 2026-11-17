#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Player/CC_PlayerController.h"
#include "Player/CC_PlayerState.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Characters/CC_PatrolEnemyCharacter.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Progression/CC_SkillComponent.h"
#include "Progression/CC_ArtifactComponent.h"
#include "Progression/CC_FormPowerOrb.h"
#include "Progression/CC_ConfiguredAbility.h"
#include "Progression/CC_CoinPickup.h"
#include "Progression/CC_SummonedSoldier.h"
#include "Progression/CC_ProgressionSave.h"
#include "UI/CC_MuralMenuWidget.h"
#include "UI/CC_PlayerHealthHUDWidget.h"
#include "UI/CC_WidgetComponent.h"
#include "Progression/CC_ProgressionHUD.h"
#include "UI/CC_SkillGalleryWidget.h"
#include "UI/CC_SkillGalleryData.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"
namespace {
struct FRealSkillProbe {
 bool Pass=true;
 TWeakObjectPtr<ACC_PlayerController> PC;TWeakObjectPtr<ACC_PlayerCharacter> Player;TWeakObjectPtr<ACC_EnemyCharacter> Dummy;
 TSharedPtr<FJsonObject> Checks=MakeShared<FJsonObject>();
 float BeforeHP=0.f,AttackAtProbe=0.f;int32 SavedCoins=0;
 void Check(bool B,const TCHAR* Name){Pass&=B;Checks->SetBoolField(Name,B);UE_LOG(LogTemp,Display,TEXT("REALSKILL_%s: %d"),Name,B);
  if(!B&&Player.IsValid())UE_LOG(LogTemp,Display,TEXT("REALSKILL_VALUES attack=%.4f baseAttack=%.4f hp=%.4f maxHP=%.4f shield=%.4f lastSkill=%s"),ASC()->GetNumericAttribute(UCC_AttributeSet::GetAttackAttribute()),ASC()->GetNumericAttributeBase(UCC_AttributeSet::GetAttackAttribute()),ASC()->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute()),ASC()->GetNumericAttribute(UCC_AttributeSet::GetMaxHealthAttribute()),ASC()->GetNumericAttribute(UCC_AttributeSet::GetShieldAttribute()),*GetNameSafe(Player->Skills->LastExecutedSkill));
 }
 UCC_ProgressionComponent* P(){return PC->GetPlayerState<ACC_PlayerState>()->Progression;}
 UAbilitySystemComponent* ASC(){return Player->GetAbilitySystemComponent();}
 UCC_SkillDefinition* Skill(const TCHAR* Id){
  auto* Gallery=LoadObject<UCC_SkillGalleryData>(nullptr,TEXT("/Game/UI/MuralMenu/DA_SkillGallery.DA_SkillGallery"));
  const auto* E=Gallery->Entries.FindByPredicate([Id](const FCC_SkillGalleryEntry& Entry){return Entry.DesignId==Id;});return E?E->ExecutableSkill.Get():nullptr;
 }
 void Clear(){Player->Skills->CancelActiveSkills();Player->Skills->ClearDesignStates();ASC()->RemoveActiveEffects(FGameplayEffectQuery());}
 bool Key(int32 Slot,EInputEvent Event=IE_Pressed){FKey K=PC->GetCombatKey(Slot);for(const auto& B:PC->InputComponent->KeyBindings) if(B.Chord.Key==K && B.KeyEvent==Event){B.KeyDelegate.Execute(K);return true;}return false;}
 void Equip(const TCHAR* Id,int32 Slot=0){Clear();P()->EquipSkill(Slot,Skill(Id));Check(Player->Skills->CurrentSlots[Slot].Skill==Skill(Id),*FString::Printf(TEXT("equip_%s_slot%d"),Id,Slot));}
};
// Each phase waits relative to completion of the preceding phase. Absolute timers
// can validate a new windup before it executes when a rendered frame hitches.
struct FRealSkillSequence:public TSharedFromThis<FRealSkillSequence> {
 UWorld* World=nullptr;
 TArray<TPair<float,TFunction<void()>>> Steps;
 int32 Index=0;
 FTimerHandle Timer;
 void Next() {
  if(Index>=Steps.Num())return;
  const float Delay=Index==0?Steps[Index].Key:Steps[Index].Key-Steps[Index-1].Key;
  World->GetTimerManager().SetTimer(Timer,[Self=AsShared()]{Self->Steps[Self->Index].Value();++Self->Index;Self->Next();},FMath::Max(.01f,Delay),false);
 }
};
void RunRealSkillsSmoke(UWorld* W) {
 if(!W || !W->IsGameWorld() || W->GetNetMode()!=NM_Standalone) return;
 auto Q=MakeShared<FRealSkillProbe>();
 auto Later=[W](float Delay,TFunction<void()> Fn){FTimerHandle T;W->GetTimerManager().SetTimer(T,MoveTemp(Fn),Delay,false);};
 Later(3.f,[W,Q](){
  auto Sequence=MakeShared<FRealSkillSequence>();Sequence->World=W;
  auto Later=[Sequence](float Delay,TFunction<void()> Fn){Sequence->Steps.Emplace(Delay,MoveTemp(Fn));};
  Q->PC=Cast<ACC_PlayerController>(W->GetFirstPlayerController());Q->Player=Q->PC.IsValid()?Cast<ACC_PlayerCharacter>(Q->PC->GetPawn()):nullptr;
  if(!Q->PC.IsValid() || !Q->Player.IsValid()){GEngine->Exec(W,TEXT("QUIT"));return;}
  for(TActorIterator<ACC_PatrolEnemyCharacter> It(W);It;++It) It->StopPatrol();
  auto* HUD=Cast<ACC_ProgressionHUD>(Q->PC->GetHUD());
  Q->Check(HUD&&HUD->PlayerHealthHUD&&HUD->PlayerHealthHUD->IsInViewport(),TEXT("health_hud_exists_in_gameplay"));
  Q->Check(!Q->Player->HealthDisplay->IsVisible(),TEXT("own_overhead_health_label_hidden"));
  Q->P()->ApplyDeveloperPreset(false);Q->Player->SetActorLocation(FVector(-2400,-2400,120));Q->Player->SetActorRotation(FRotator::ZeroRotator);Q->PC->SetControlRotation(FRotator::ZeroRotator);
  auto* Balance=LoadObject<UCC_CombatBalance>(nullptr,TEXT("/Game/HPWH/Balance/DA_CombatBalance.DA_CombatBalance"));
  bool Rewards=true;for(const auto& Row:Balance->Profiles){const FString Id=Row.ProfileId.ToString();if(Id.EndsWith(TEXT("_T1")))Rewards&=Balance->ResolveExperienceReward(Row.ProfileId)==10;if(Id.EndsWith(TEXT("_T2")))Rewards&=Balance->ResolveExperienceReward(Row.ProfileId)==40;if(Id.EndsWith(TEXT("_T3")))Rewards&=Balance->ResolveExperienceReward(Row.ProfileId)==200;if(Row.bElite)Rewards&=Balance->ResolveExperienceReward(Row.ProfileId)==500;}
  Q->Check(Rewards && Balance->ResolveExperienceReward(TEXT("Nature_PotHead"))==200,TEXT("profile_rewards_10_40_200_500_and_inheritance"));
  bool EnemyRewards=true;for(TActorIterator<ACC_PatrolEnemyCharacter> It(W);It;++It) if(It->CombatBalance) EnemyRewards&=It->ExperienceReward==It->CombatBalance->ResolveExperienceReward(It->StatProfileId,It->GetMesh()->GetSkeletalMeshAsset());
  Q->Check(EnemyRewards,TEXT("actual_map_enemy_rewards_follow_model_profiles"));
  UClass* CoinClass=LoadClass<ACC_CoinPickup>(nullptr,TEXT("/Game/HPWH/Coins/BP_CopperCoin.BP_CopperCoin_C"));
  auto* Coin=W->SpawnActor<ACC_CoinPickup>(CoinClass,Q->Player->GetActorLocation()+FVector(700,0,0),FRotator::ZeroRotator);
  Q->Check(Coin && Coin->CoinMesh->GetSkeletalMeshAsset() && Coin->PickupSound,TEXT("migrated_coin_mesh_audio_loaded"));
  Q->Check(Coin && !Coin->TryCollect(Q->Player.Get()),TEXT("remote_coin_request_rejected"));
  const int32 ExpectedCoins=Q->P()->CopperCoins+(Coin?Coin->CopperAmount:0);
  const int32 ExpectedExperience=Q->P()->Experience+(Coin?Coin->ExperienceAmount:0);
  if(Coin) Coin->SetActorLocation(Q->Player->GetActorLocation());
  Q->SavedCoins=Q->P()->CopperCoins;
  Later(.2f,[W,Q,ExpectedCoins,ExpectedExperience,bSpawned=Coin!=nullptr](){
   Q->Check(bSpawned && Q->P()->CopperCoins==ExpectedCoins && Q->P()->Experience==ExpectedExperience,TEXT("physical_coin_overlap_awards_configured_reward_once"));
   const FString Slot=TEXT("RealSkillsSmokeTemp");Q->Check(Q->P()->SaveProgress(Slot),TEXT("currency_save_v3"));Q->P()->AddCopperCoins(9);Q->Check(Q->P()->LoadProgress(Slot) && Q->P()->CopperCoins==ExpectedCoins,TEXT("currency_restored_v3"));UGameplayStatics::DeleteGameInSlot(Slot,0);
   auto* Old=NewObject<UCC_ProgressionSave>();Old->Version=2;Old->NativeFormId=TEXT("InitialInk");Old->CopperCoins=99;UGameplayStatics::SaveGameToSlot(Old,Slot,0);Q->Check(Q->P()->LoadProgress(Slot) && Q->P()->CopperCoins==0,TEXT("legacy_v2_currency_defaults_zero"));UGameplayStatics::DeleteGameInSlot(Slot,0);
   Q->PC->OpenProgressionMenu();auto* M=Cast<UCC_MuralMenuWidget>(Q->PC->GetProgressionMenuWidget());auto* G=M->GetSkillGallery();M->ShowPage(ECC_MuralMenuPage::Skills);
   G->SelectCategory(1);G->SelectPreviewSlot(0);G->PreviewEquip();Q->Check(!Q->P()->IsSkillUnlocked(Q->Skill(TEXT("bronze_summon"))) && Q->P()->ManualSkills.IsEmpty(),TEXT("locked_school_ui_cannot_equip"));
   Q->P()->EquipSkill(0,Q->Skill(TEXT("bronze_summon")));Q->Check(Q->P()->ManualSkills.IsEmpty(),TEXT("locked_school_server_rpc_rejects"));
   auto* S=Q->Skill(TEXT("bronze_summon"));FGameplayAbilitySpecHandle H=Q->ASC()->GiveAbility(FGameplayAbilitySpec(UCC_ConfiguredAbility::StaticClass(),1,0,S));Q->Check(!Q->ASC()->TryActivateAbility(H),TEXT("locked_school_direct_gas_activation_rejects"));Q->ASC()->ClearAbility(H);
   G->SelectCategory(0);G->SelectEntry(4);G->SelectPreviewSlot(1);G->PreviewEquip();Q->Check(Q->Player->Skills->CurrentSlots[1].Skill==Q->Skill(TEXT("basic_particle")) && G->GetPreviewEntry(1)==4,TEXT("menu_equipment_changes_real_slot_and_icon"));
   Q->PC->CloseProgressionMenu();
   Q->Equip(TEXT("basic_straight_punch"),0);Q->Equip(TEXT("basic_particle"),1);Q->Equip(TEXT("basic_heavy_punch"),2);Q->Equip(TEXT("basic_heal"),3);
   auto* Class=LoadClass<ACC_EnemyCharacter>(nullptr,TEXT("/Game/HPWH/Blueprints/BP_TestDummy.BP_TestDummy_C"));
   Q->Dummy=W->SpawnActor<ACC_EnemyCharacter>(Class,Q->Player->GetActorLocation()+FVector(155,0,0),FRotator(0,180,0));if(auto* Patrol=Cast<ACC_PatrolEnemyCharacter>(Q->Dummy.Get())) Patrol->StopPatrol();
   auto* DA=Q->Dummy->GetAbilitySystemComponent();DA->SetNumericAttributeBase(UCC_AttributeSet::GetMaxHealthAttribute(),10000.f);DA->SetNumericAttributeBase(UCC_AttributeSet::GetHealthAttribute(),10000.f);DA->SetNumericAttributeBase(UCC_AttributeSet::GetDefenseAttribute(),0.f);
   Q->BeforeHP=10000.f;Q->Check(Q->Key(0),TEXT("key1_dispatched"));
  });
  Later(.65f,[Q](){Q->Check(Q->Player->Skills->LastExecutedSkill==Q->Skill(TEXT("basic_straight_punch")) && Q->Dummy->GetAbilitySystemComponent()->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute())<Q->BeforeHP,TEXT("key1_executes_straight_punch_and_real_damage"));Q->BeforeHP=Q->Dummy->GetAbilitySystemComponent()->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute());Q->Key(1);});
  Later(1.1f,[Q](){Q->Check(Q->Player->Skills->LastExecutedSkill==Q->Skill(TEXT("basic_particle")) && Q->Dummy->GetAbilitySystemComponent()->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute())<Q->BeforeHP,TEXT("key2_executes_projectile_and_real_hit"));Q->BeforeHP=Q->Dummy->GetAbilitySystemComponent()->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute());Q->Key(2);});
  Later(1.6f,[Q](){Q->Check(Q->Player->Skills->LastExecutedSkill==Q->Skill(TEXT("basic_heavy_punch")) && Q->Dummy->GetAbilitySystemComponent()->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute())<Q->BeforeHP,TEXT("key3_executes_distinct_heavy_damage"));Q->ASC()->SetNumericAttributeBase(UCC_AttributeSet::GetHealthAttribute(),20.f);Q->Key(3);});
  Later(2.f,[Q](){Q->Check(Q->Player->Skills->LastExecutedSkill==Q->Skill(TEXT("basic_heal")) && Q->ASC()->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute())>20.f,TEXT("key4_heals_instead_of_attacking"));
   Q->Clear();const auto Old0=Q->Player->Skills->CurrentSlots[0].Skill;Q->P()->EquipSkill(0,Q->Skill(TEXT("basic_particle")));Q->Check(Q->Player->Skills->CurrentSlots[1].Skill==Old0 && Q->Player->Skills->CurrentSlots[0].Skill==Q->Skill(TEXT("basic_particle")),TEXT("duplicate_skill_moves_by_swapping_real_slots"));
   for(TActorIterator<ACC_FormArtifact> It(Q->Player->GetWorld());It;++It) if(It->Artifact->Form && It->Artifact->Form->bAvailable && It->Artifact->bEnabled){Q->Player->SetActorLocation(It->GetActorLocation()+FVector(-130,0,120));Q->P()->Interact(*It);
    // The current acquisition flow releases a physical orb; interaction itself grants nothing.
    for(TActorIterator<ACC_FormPowerOrb> Orb(Q->Player->GetWorld());Orb;++Orb)if(Orb->Recipient==Q->PC->GetPlayerState<ACC_PlayerState>()&&Orb->RewardForm==It->Artifact->Form&&!Orb->IsActorBeingDestroyed()){
     Orb->BeginSeeking();Q->Player->SetActorLocation(Orb->GetActorLocation());if(!Orb->IsActorBeingDestroyed())Orb->TryCollect(Q->Player.Get());break;
    }
   }
   Q->Check(Q->P()->IsSkillUnlocked(Q->Skill(TEXT("bronze_summon"))) && Q->P()->IsSkillUnlocked(Q->Skill(TEXT("temple_golden_body"))) && Q->P()->IsSkillUnlocked(Q->Skill(TEXT("field_root_prison"))),TEXT("acquiring_three_powers_unlocks_corresponding_skills"));
   Q->Player->SetActorLocation(FVector(-2400,-2400,120));Q->Equip(TEXT("bronze_summon"));Q->Player->Skills->ActivateSlot(0);
  });
  Later(2.4f,[Q](){int32 Count=0;for(TActorIterator<ACC_SummonedSoldier> It(Q->Player->GetWorld());It;++It) if(It->GetOwner()==Q->Player.Get())++Count;Q->Check(Count==3,TEXT("summon_creates_three_real_allied_soldiers"));
   Q->Equip(TEXT("bronze_evolve"));Q->Player->Skills->ActivateSlot(0);
  });
  Later(2.7f,[Q](){const float Base=Q->ASC()->GetNumericAttributeBase(UCC_AttributeSet::GetAttackAttribute());Q->Check(FMath::IsNearlyEqual(Q->ASC()->GetNumericAttribute(UCC_AttributeSet::GetAttackAttribute()),Base*1.4f,.01f),TEXT("temporary_evolution_changes_actual_attack"));
   Q->Equip(TEXT("temple_golden_body"));Q->Player->Skills->ActivateSlot(0);
  });
  Later(3.f,[Q](){float HP=Q->ASC()->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute());UCC_ConfiguredAbility::ApplyEffect(Q->Dummy.Get(),Q->Player.Get(),UCC_DamageEffect::StaticClass(),30.f);Q->Check(Q->Player->Skills->IsInvulnerable() && Q->ASC()->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute())==HP,TEXT("golden_body_blocks_real_incoming_damage"));
   Q->Equip(TEXT("field_stealth"));Q->Player->Skills->ActivateSlot(0);
  });
  Later(3.3f,[Q](){Q->Check(Q->Player->Skills->IsStealthed() && !Q->Player->GetMesh()->IsVisible(),TEXT("stealth_hides_character"));Q->Player->Skills->ActivateBasicAttack();Q->Check(!Q->Player->Skills->IsStealthed(),TEXT("attack_breaks_stealth"));
   Q->Equip(TEXT("field_regrowth"));Q->ASC()->SetNumericAttributeBase(UCC_AttributeSet::GetHealthAttribute(),20.f);Q->Player->Skills->ActivateSlot(0);
  });
  Later(4.7f,[Q](){Q->Check(Q->ASC()->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute())>20.f,TEXT("regrowth_has_real_delayed_healing_tick"));Q->Equip(TEXT("bronze_charged_hit"));Q->Player->Skills->ActivateSlot(0,Q->Dummy.Get());Q->Check(Q->Player->Skills->GetCooldownRemaining(0)==0.f,TEXT("charge_cooldown_waits_for_release"));Q->Player->Skills->ReleaseSlot(0);});
  Later(5.1f,[Q](){Q->Check(Q->Player->Skills->GetCooldownRemaining(0)>0.f,TEXT("releasing_charge_starts_actual_cooldown"));Q->Equip(TEXT("field_lifesteal"));Q->ASC()->SetNumericAttributeBase(UCC_AttributeSet::GetHealthAttribute(),20.f);Q->Player->Skills->ActivateSlot(0);});
  Later(5.4f,[Q](){Q->Dummy->SetActorLocation(Q->Player->GetActorLocation()+FVector(155,0,0));UCC_ConfiguredAbility::ApplyEffect(Q->Player.Get(),Q->Dummy.Get(),UCC_DamageEffect::StaticClass(),50.f);Q->Check(FMath::IsNearlyEqual(Q->ASC()->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute()),20.f+FMath::Min(10.f,Q->ASC()->GetNumericAttribute(UCC_AttributeSet::GetMaxHealthAttribute())*.08f),.02f),TEXT("lifesteal_uses_real_health_damage_and_per_second_cap"));Q->Equip(TEXT("bronze_guard"));Q->ASC()->SetNumericAttributeBase(UCC_AttributeSet::GetDefenseAttribute(),50.f);Q->ASC()->SetNumericAttributeBase(UCC_AttributeSet::GetHealthAttribute(),75.f);Q->Player->Skills->ActivateSlot(0);});
  Later(5.7f,[Q](){Q->Player->SetActorRotation(FRotator::ZeroRotator);Q->Dummy->SetActorLocation(Q->Player->GetActorLocation()+FVector(155,0,0));UCC_ConfiguredAbility::ApplyEffect(Q->Dummy.Get(),Q->Player.Get(),UCC_DamageEffect::StaticClass(),30.f);Q->Check(FMath::IsNearlyEqual(Q->ASC()->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute()),60.f,.05f),TEXT("guard_doubles_defense_against_frontal_hits"));Q->Dummy->SetActorLocation(Q->Player->GetActorLocation()-FVector(155,0,0));UCC_ConfiguredAbility::ApplyEffect(Q->Dummy.Get(),Q->Player.Get(),UCC_DamageEffect::StaticClass(),30.f);Q->Check(FMath::IsNearlyEqual(Q->ASC()->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute()),40.f,.05f),TEXT("guard_does_not_block_rear_hits"));
   auto* HUD=Cast<ACC_ProgressionHUD>(Q->PC->GetHUD());Q->Check(HUD&&HUD->PlayerHealthHUD&&FMath::IsNearlyEqual(HUD->PlayerHealthHUD->GetDisplayedHealth(),Q->ASC()->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute()),.01f)&&FMath::IsNearlyEqual(HUD->PlayerHealthHUD->GetDisplayedMaxHealth(),Q->ASC()->GetNumericAttribute(UCC_AttributeSet::GetMaxHealthAttribute()),.01f),TEXT("health_hud_updates_after_damage_and_growth"));
   FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/FactionHealthHUD/CombatHealth.png"),true,false);Q->Equip(TEXT("temple_cleanse"));Q->ASC()->SetNumericAttributeBase(UCC_AttributeSet::GetDefenseAttribute(),0.f);Q->Player->Skills->ActivateSlot(0);});
  Later(6.f,[Q](){Q->Check(FMath::IsNearlyEqual(Q->ASC()->GetNumericAttribute(UCC_AttributeSet::GetShieldAttribute()),Q->ASC()->GetNumericAttribute(UCC_AttributeSet::GetMaxHealthAttribute())*.1f,.05f),TEXT("cleanse_grants_percent_health_shield"));Q->Equip(TEXT("field_root_prison"));Q->Dummy->SetActorLocation(Q->Player->GetActorLocation()+FVector(155,0,0));Q->BeforeHP=Q->Dummy->GetAbilitySystemComponent()->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute());Q->Player->Skills->ActivateSlot(0,Q->Dummy.Get());});
  Later(6.4f,[Q](){Q->Check(Q->Dummy->Skills->IsRooted() && !Q->Dummy->Skills->IsStunned() && Q->Dummy->GetAbilitySystemComponent()->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute())<Q->BeforeHP,TEXT("root_area_damages_and_roots_without_stunning_attacks"));Q->Equip(TEXT("temple_gravity"));Q->AttackAtProbe=Q->ASC()->GetNumericAttribute(UCC_AttributeSet::GetAttackAttribute());Q->BeforeHP=Q->Dummy->GetAbilitySystemComponent()->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute());Q->Player->Skills->ActivateSlot(0,Q->Dummy.Get());});
  Later(9.9f,[Q](){Q->Check(Q->Dummy->GetAbilitySystemComponent()->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute())<=Q->BeforeHP-Q->AttackAtProbe*1.2f+.05f,TEXT("gravity_delivers_initial_and_final_damage"));Q->Equip(TEXT("temple_lamps"));Q->BeforeHP=Q->Dummy->GetAbilitySystemComponent()->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute());Q->Player->Skills->ActivateSlot(0);});
  Later(13.5f,[Q](){Q->Check(Q->Dummy->GetAbilitySystemComponent()->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute())<=Q->BeforeHP-33.7f,TEXT("lamps_delivers_six_distinct_periodic_damage_pulses"));Q->Equip(TEXT("basic_combo"));Q->Dummy->SetActorLocation(Q->Player->GetActorLocation()+FVector(155,0,0));Q->BeforeHP=Q->Dummy->GetAbilitySystemComponent()->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute());Q->Player->Skills->ActivateSlot(0,Q->Dummy.Get());});
  Later(15.2f,[Q](){Q->Check(Q->Dummy->GetAbilitySystemComponent()->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute())<=Q->BeforeHP-28.f,TEXT("combo_uses_distinct_three_hit_damage_coefficients"));Q->Clear();Q->Equip(TEXT("bronze_charged_hit"));Q->Player->Skills->ActivateSlot(0,Q->Dummy.Get());Q->PC->OpenProgressionMenu();
   Q->Check(!Q->Player->Skills->HasBlockingSkillAction()&&!Q->Player->Skills->IsSlotHeld(0)&&Q->Player->Skills->GetCooldownRemaining(0)>2.9f,TEXT("menu_open_cancels_charge_without_stuck_input"));Q->PC->CloseProgressionMenu();
   Q->Clear();Q->P()->ApplyDeveloperPreset(false);Q->Check(Q->P()->CopperCoins==0 && Q->P()->Forms.IsEmpty() && !Q->P()->IsSkillUnlocked(Q->Skill(TEXT("bronze_summon"))),TEXT("reset_clears_coin_wallet_and_school_access"));Q->PC->OpenProgressionMenu();auto* M=Cast<UCC_MuralMenuWidget>(Q->PC->GetProgressionMenuWidget());M->ShowPage(ECC_MuralMenuPage::Skills);});
  Later(15.9f,[W,Q](){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/RealSkills/LockedSchool.png"),true,false);});
  Later(16.4f,[W,Q](){
   Q->PC->CloseProgressionMenu();
   for(TActorIterator<ACC_CoinPickup> It(W);It;++It) {
    const FVector Coin=It->GetActorLocation();Q->Player->SetActorLocation(Coin+FVector(-250,-350,20));
    Q->PC->SetControlRotation((Coin-(Q->Player->GetActorLocation()+FVector(0,0,60))).Rotation());break;
   }
  });
  Later(17.f,[W,Q](){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/RealSkills/CoinArena.png"),true,false);});
  Later(17.6f,[W,Q](){Q->PC->CloseProgressionMenu();auto Root=MakeShared<FJsonObject>();Root->SetBoolField(TEXT("passed"),Q->Pass);Root->SetObjectField(TEXT("checks"),Q->Checks);FString Json;FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Json));FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("RealSkillsSmoke.json")));UE_LOG(LogTemp,Display,TEXT("REAL_SKILLS_SMOKE_%s"),Q->Pass?TEXT("PASS"):TEXT("FAIL"));GEngine->Exec(W,TEXT("QUIT"));});
  Sequence->Next();
 });
}
FAutoConsoleCommandWithWorld RealSkillsSmokeCommand(TEXT("CC.RealSkillsSmokeTest"),TEXT("Real gallery equipment, 1-4 execution, school authority, XP tiers and physical coin pickup."),FConsoleCommandWithWorldDelegate::CreateStatic(&RunRealSkillsSmoke));
}
#endif
