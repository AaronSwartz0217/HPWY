#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Progression/CC_GameplayData.h"
#include "Progression/CC_SkillComponent.h"
#include "Progression/CC_ConfiguredAbility.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Characters/CC_LocomotionComponent.h"
#include "Characters/CC_PatrolEnemyCharacter.h"
#include "Player/CC_PlayerState.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCC_CombatBalanceTest,"CrashCourse.Combat.ProfilesAndEquipment",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCC_CombatBalanceTest::RunTest(const FString& Parameters)
{
 auto* Balance=LoadObject<UCC_CombatBalance>(nullptr,TEXT("/Game/HPWH/Balance/DA_CombatBalance.DA_CombatBalance"));
 if (!TestNotNull(TEXT("Configured shared balance asset"),Balance)) return false;
 const TCHAR* Ids[]={TEXT("Nature_T1"),TEXT("Nature_T2"),TEXT("Nature_T3"),TEXT("Bronze_T1"),TEXT("Bronze_T2"),TEXT("Bronze_T3"),TEXT("Ceramic_T1"),TEXT("Ceramic_T2"),TEXT("Ceramic_T3"),TEXT("Bronze_Elite"),TEXT("Ceramic_Elite")};
 const float Healths[]={100,250,450,140,300,500,160,350,700,750,1050};
 const float Attacks[]={15,37.5,67.5,21,45,75,15,37.5,67.5,112.5,101.25};
 for (int32 I=0;I<11;++I) { float HP=0,ATK=0; TestTrue(Ids[I],Balance->Resolve(Ids[I],HP,ATK)); TestEqual(FString(Ids[I])+TEXT(" HP"),HP,Healths[I]); TestEqual(FString(Ids[I])+TEXT(" ATK"),ATK,Attacks[I]); }
 auto* Particle=LoadObject<UCC_SkillDefinition>(nullptr,TEXT("/Game/HPWH/Skills/DA_Skill_01.DA_Skill_01"));
 if (!TestNotNull(TEXT("Particle skill #01"),Particle)) return false;
 TestTrue(TEXT("First skill uses projectile behavior"),Particle->Behavior==ECC_SkillBehavior::Projectile);
 TestEqual(TEXT("Whiteboard projectile 75%"),Particle->CalculatePower(15),11.25f);
 TestEqual(TEXT("Bronze elite projectile 75%"),Particle->CalculatePower(112.5f),84.375f);
 TestEqual(TEXT("Ceramic elite projectile 75%"),Particle->CalculatePower(101.25f),75.9375f);
 TestEqual(TEXT("Skill upgrades preserve specified 75% ratio"),Particle->CalculatePower(30,6),22.5f);
 TestTrue(TEXT("Numeric description reports attack coefficient"),Particle->GetNumericDescription(15).ToString().Contains(TEXT("75.0%")));
 auto* Broken=NewObject<UCC_CombatBalance>(); FCC_CombatProfile Loop; Loop.ProfileId=TEXT("Loop"); Loop.ParentProfileId=TEXT("Loop"); Broken->Profiles.Add(Loop);
 float H=0,A=0; TestFalse(TEXT("Cyclic profiles rejected"),Broken->Resolve(TEXT("Loop"),H,A));

 UWorld::InitializationValues Values; Values.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,TEXT("CombatBalanceTests"),nullptr,true,ERHIFeatureLevel::Num,&Values);
 GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
 auto* PS=World->SpawnActor<ACC_PlayerState>(); auto* Player=World->SpawnActor<ACC_PlayerCharacter>(); Player->SetPlayerState(PS);
 auto* ASC=Player->GetAbilitySystemComponent(); ASC->InitAbilityActorInfo(PS,Player); ASC->AddAttributeSetSubobject(Cast<UCC_AttributeSet>(Player->GetAttributeSet()));
 ASC->ApplyGameplayEffectToSelf(GetDefault<UCC_InitialAttributesEffect>(),1,ASC->MakeEffectContext());
 auto* P=PS->Progression.Get(); P->NativeForm=LoadObject<UCC_FormDefinition>(nullptr,TEXT("/Game/HPWH/Forms/DA_Form_Native.DA_Form_Native"));
 for (int32 I=1;I<=22;++I) {
  auto* S=LoadObject<UCC_SkillDefinition>(nullptr,*FString::Printf(TEXT("/Game/HPWH/Skills/DA_Skill_%02d.DA_Skill_%02d"),I,I));
  TestNotNull(FString::Printf(TEXT("Skill %02d exists"),I),S); if (S) { P->SkillCatalog.Add(S); TestFalse(TEXT("Skill has effect description"),S->Description.IsEmpty()); }
 }
 auto* Alternating=P->SkillCatalog[20].Get();
 TestTrue(TEXT("Legacy primary alternates attack pose"),Alternating->bAlternateAttackPose);
 Player->Locomotion->PresentSkill(Alternating,FVector::ZeroVector); TestTrue(TEXT("First punch uses jab"),Player->Locomotion->LastAttackPose==ECC_AttackPose::Jab);
 Player->Locomotion->PresentSkill(Alternating,FVector::ZeroVector); TestTrue(TEXT("Second punch uses cross jab"),Player->Locomotion->LastAttackPose==ECC_AttackPose::CrossJab);
 auto* Sprint=P->SkillCatalog[21].Get(); TestEqual(TEXT("Legacy sprint duration"),Sprint->Duration,1.f);
 const float BeforeSpeed=ASC->GetNumericAttribute(UCC_AttributeSet::GetMoveSpeedAttribute());
 UCC_ConfiguredAbility::ApplyEffect(Player,Player,UCC_SpeedEffect::StaticClass(),Sprint->CalculatePower(15),Sprint->Duration);
 TestEqual(TEXT("Legacy sprint applies configurable bonus"),ASC->GetNumericAttribute(UCC_AttributeSet::GetMoveSpeedAttribute()),BeforeSpeed+500.f);
 ASC->RemoveActiveEffects(FGameplayEffectQuery()); TestEqual(TEXT("Sprint removal restores movement"),ASC->GetNumericAttribute(UCC_AttributeSet::GetMoveSpeedAttribute()),BeforeSpeed);
 for (const auto& Stage:P->NativeForm->Stages) if(Stage.Loadout) for(const auto& Slot:Stage.Loadout->Slots) if(!P->SkillCatalog.Contains(Slot.Skill)) P->SkillCatalog.Add(Slot.Skill);
 P->InitializeAvatar(); TestEqual(TEXT("Initial player attack"),ASC->GetNumericAttribute(UCC_AttributeSet::GetAttackAttribute()),15.f);
 TestTrue(TEXT("Default loadout valid"),P->NativeForm->Stages[0].Loadout->IsComplete());
 TestFalse(TEXT("Particle skill not automatically equipped"),Player->Skills->CurrentSlots.ContainsByPredicate([Particle](const FCC_SkillSlot& S) { return S.Skill==Particle; }));
 auto* DefaultSkill=Player->Skills->CurrentSlots[0].Skill.Get(); P->EquipSkill(0,Particle);
 TestEqual(TEXT("Manual particle equip"),Player->Skills->CurrentSlots[0].Skill.Get(),Particle);
 TestEqual(TEXT("Shared form asset not mutated"),P->NativeForm->Stages[0].Loadout->Slots[0].Skill.Get(),DefaultSkill);
 P->EquipSkill(2,Particle); TestEqual(TEXT("Duplicate equip swaps slots"),Player->Skills->CurrentSlots[2].Skill.Get(),Particle);
 TestEqual(TEXT("Four skills only"),ASC->GetActivatableAbilities().Num(),4);
 auto* Unlisted=NewObject<UCC_SkillDefinition>(); Unlisted->SkillId=TEXT("Forged"); P->EquipSkill(2,Unlisted);
 TestEqual(TEXT("Unlisted skills rejected"),Player->Skills->CurrentSlots[2].Skill.Get(),Particle);
 P->EquipSkill(-1,Particle); P->EquipSkill(4,Particle); TestEqual(TEXT("Bad slot indices rejected"),Player->Skills->CurrentSlots.Num(),4);
 TestTrue(TEXT("Particle activates"),Player->Skills->ActivateSlot(2)); P->ResetSkills(); TestEqual(TEXT("Cooldown blocks skill reconfiguration"),P->ManualSkills.Num(),4);
 Player->Skills->CancelActiveSkills(); ASC->RemoveActiveEffects(FGameplayEffectQuery());
 P->AddExperience(100); P->Evolve(); TestEqual(TEXT("Evolution uses shared profile health"),ASC->GetNumericAttribute(UCC_AttributeSet::GetMaxHealthAttribute()),250.f);
 TestEqual(TEXT("Evolution preserves manual particle slot"),Player->Skills->CurrentSlots[2].Skill.Get(),Particle);
 const FString SaveSlot=TEXT("Codex_CombatBalance_TestOnly"); TestTrue(TEXT("Save manual skills"),P->SaveProgress(SaveSlot)); P->ResetSkills(); TestEqual(TEXT("Reset restores form defaults"),P->ManualSkills.Num(),0);
 TestTrue(TEXT("Load equipped skills"),P->LoadProgress(SaveSlot)); TestEqual(TEXT("Loaded skills restored by stable IDs"),Player->Skills->CurrentSlots[2].Skill.Get(),Particle); UGameplayStatics::DeleteGameInSlot(SaveSlot,0);
 auto* Enemy=World->SpawnActor<ACC_PatrolEnemyCharacter>(); Enemy->CombatBalance=Balance; Enemy->StatProfileId=TEXT("Bronze_Elite");
 auto* EASC=Enemy->GetAbilitySystemComponent(); EASC->InitAbilityActorInfo(Enemy,Enemy); EASC->AddAttributeSetSubobject(Cast<UCC_AttributeSet>(Enemy->GetAttributeSet()));
 Enemy->GetMesh()->SetRelativeScale3D(FVector(2)); TestTrue(TEXT("Apply elite profile"),Enemy->ApplyCombatProfile()); TestTrue(TEXT("Elite scale is 1.2 times starting model"),Enemy->GetMesh()->GetRelativeScale3D().Equals(FVector(2.4f)));
 Enemy->ApplyCombatProfile(); TestTrue(TEXT("Applying profile twice does not stack size"),Enemy->GetMesh()->GetRelativeScale3D().Equals(FVector(2.4f)));
 ASC->SetNumericAttributeBase(UCC_AttributeSet::GetDefenseAttribute(),0); ASC->SetNumericAttributeBase(UCC_AttributeSet::GetHealthAttribute(),250);
 UCC_ConfiguredAbility::ApplyEffect(Enemy,Player,UCC_DamageEffect::StaticClass(),Particle->CalculatePower(112.5f));
 TestEqual(TEXT("Attack not multiplied a second time during damage resolution"),ASC->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute()),165.625f);
 World->DestroyWorld(false); GEngine->DestroyWorldContext(World); return true;
}
#endif
