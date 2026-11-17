#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Player/CC_PlayerState.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Progression/CC_ConfiguredAbility.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCC_InkExperienceGrowth,"CrashCourse.Progression.InkExperienceAndInventoryEvolution",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCC_InkExperienceGrowth::RunTest(const FString& Parameters)
{
 UWorld::InitializationValues Values;
 Values.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,TEXT("InkExperienceTest"),nullptr,true,ERHIFeatureLevel::Num,&Values);
 GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
 auto* PS=World->SpawnActor<ACC_PlayerState>();auto* Player=World->SpawnActor<ACC_PlayerCharacter>();Player->SetPlayerState(PS);
 auto* ASC=Player->GetAbilitySystemComponent();auto* AS=Cast<UCC_AttributeSet>(Player->GetAttributeSet());
 ASC->InitAbilityActorInfo(PS,Player);ASC->AddAttributeSetSubobject(AS);
 ASC->ApplyGameplayEffectToSelf(GetDefault<UCC_InitialAttributesEffect>(),1,ASC->MakeEffectContext());
 auto* Loadout=NewObject<UCC_SkillLoadout>(PS);
 for (int32 I=0;I<4;++I) {Loadout->Slots[I].Skill=NewObject<UCC_SkillDefinition>(PS);Loadout->Slots[I].Skill->SkillId=FName(*FString::Printf(TEXT("InkFixture_%d"),I));}
 auto Form=[PS,Loadout](FName Id,float Health,float Attack) {
  auto* F=NewObject<UCC_FormDefinition>(PS);F->FormId=Id;
  for (int32 I=0;I<3;++I) {F->Stages[I].Loadout=Loadout;F->Stages[I].Cost=I==0 ? 0 : I==1 ? 100 : 1000;F->Stages[I].MaxHealth=Health;F->Stages[I].Attack=Attack;}
  return F;
 };
 auto* P=PS->Progression.Get();auto* Ink=Form(TEXT("InitialInk"),75,11.25f);Ink->bCanEvolve=false;P->NativeForm=Ink;
 P->bRemoteShopUnlocked=true;
 auto* Bronze=Form(TEXT("Artifact_01"),140,21);Bronze->Stages[1].MaxHealth=300;Bronze->Stages[1].Attack=45;P->ArtifactForms.Add(Bronze);
 auto* Locked=Form(TEXT("Locked"),160,15);P->ArtifactForms.Add(Locked);
 FCC_FormProgress Owned;Owned.Form=Bronze;P->Forms.Add(Owned);P->InitializeAvatar();
 TestEqual(TEXT("Ink is rounded at zero balance"),AS->GetAttack(),11.f);
 P->AddExperience(50);
 TestEqual(TEXT("Half health rounds upward"),AS->GetMaxHealth(),113.f);TestEqual(TEXT("Ink scales actual attack"),AS->GetAttack(),17.f);
 ASC->SetNumericAttributeBase(AS->GetHealthAttribute(),56.5f);P->AddExperience(50);
 TestEqual(TEXT("100 held experience doubles health"),AS->GetMaxHealth(),150.f);TestEqual(TEXT("22.5 attack rounds upward"),AS->GetAttack(),23.f);
 TestEqual(TEXT("Reward recalculation preserves health fraction"),AS->GetHealth(),75.f);
 const int32 AbilityCount=ASC->GetActivatableAbilities().Num();
 P->EvolveForm(Locked);TestEqual(TEXT("Locked inventory evolution does not spend"),P->Experience,100);
 P->EvolveForm(Bronze);
 TestEqual(TEXT("Inactive power pays once"),P->Experience,0);TestEqual(TEXT("Inactive power advances its own tier"),P->GetOwnedFormTier(Bronze),1);
 TestEqual(TEXT("Inactive evolution keeps Ink selected"),P->GetActiveForm(),Ink);TestEqual(TEXT("Ink tier remains initial"),P->CurrentTier,0);
 TestEqual(TEXT("Spending removes Ink experience bonus"),AS->GetMaxHealth(),75.f);TestEqual(TEXT("Spending preserves health fraction"),AS->GetHealth(),37.5f);
 TestEqual(TEXT("Passive recalculation does not duplicate abilities"),ASC->GetActivatableAbilities().Num(),AbilityCount);
 P->AddExperience(100);P->SwitchForm(Bronze);P->AddExperience(50);
 TestEqual(TEXT("Other forms ignore experience scaling"),AS->GetMaxHealth(),300.f);TestEqual(TEXT("Other form attack remains configured"),AS->GetAttack(),45.f);
 P->SwitchForm(nullptr);TestEqual(TEXT("Returning to Ink uses current balance and fresh baseline"),AS->GetMaxHealth(),188.f);
 P->RestoreExperienceFromBackend(1);TestEqual(TEXT("Backend restore recalculates Ink"),AS->GetMaxHealth(),76.f);
 P->RestoreExperienceFromBackend(1);TestEqual(TEXT("Repeated restore cannot stack growth"),AS->GetMaxHealth(),76.f);
 P->RestoreExperienceFromBackend(50);
 auto* Upgrade=NewObject<UCC_UpgradeDefinition>(PS);Upgrade->UpgradeId=TEXT("InkHealthFixture");Upgrade->Prices={25};Upgrade->BonusPerLevel=10;P->UpgradeCatalog.Add(Upgrade);P->Purchase(Upgrade);
 TestEqual(TEXT("Purchase spends balance before recalculation"),P->Experience,25);TestEqual(TEXT("Permanent bonus participates before rounding"),AS->GetMaxHealth(),106.f);
 FCC_FormStage Preview;TestTrue(TEXT("Ink preview is available"),P->GetStagePreview(Ink,0,Preview));TestEqual(TEXT("Preview excludes purchased flat bonus"),Preview.MaxHealth,94.f);
 P->EvolveForm(Ink);TestEqual(TEXT("Ink does not sell stage evolution"),P->Experience,25);
 World->DestroyWorld(false);GEngine->DestroyWorldContext(World);return true;
}
#endif
