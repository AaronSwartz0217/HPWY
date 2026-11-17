#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Player/CC_PlayerState.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "Progression/CC_ConfiguredAbility.h"
#include "Progression/CC_GameplayData.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Progression/CC_ProgressionSave.h"
#include "Progression/CC_FormPowerOrb.h"
#include "Components/SkeletalMeshComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCC_ChromaticInkTest,"CrashCourse.Progression.ChromaticInk",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCC_ChromaticInkTest::RunTest(const FString& Parameters)
{
 UClass* StateClass=LoadClass<ACC_PlayerState>(nullptr,TEXT("/Game/HPWH/Blueprints/BP_ProgressionPlayerState.BP_ProgressionPlayerState_C"));
 UClass* CharacterClass=LoadClass<ACC_PlayerCharacter>(nullptr,TEXT("/Game/HPWH/Blueprints/BP_ProgressionCharacter.BP_ProgressionCharacter_C"));
 if(!TestNotNull(TEXT("Configured player state"),StateClass)||!TestNotNull(TEXT("Configured player character"),CharacterClass))return false;
 UWorld::InitializationValues Values;
 Values.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,TEXT("ChromaticInkTest"),nullptr,true,ERHIFeatureLevel::Num,&Values);
 GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
 auto* PS=World->SpawnActor<ACC_PlayerState>(StateClass);
 auto* Player=World->SpawnActor<ACC_PlayerCharacter>(CharacterClass);
 Player->SetPlayerState(PS);
 auto* ASC=Player->GetAbilitySystemComponent();
 ASC->InitAbilityActorInfo(PS,Player);
 ASC->AddAttributeSetSubobject(Cast<UCC_AttributeSet>(Player->GetAttributeSet()));
 ASC->ApplyGameplayEffectToSelf(GetDefault<UCC_InitialAttributesEffect>(),1.f,ASC->MakeEffectContext());
 auto* P=PS->Progression.Get();P->InitializeAvatar();
 const auto OriginalOverrides=Player->GetMesh()->OverrideMaterials;
 auto* OriginalOverlay=Player->GetMesh()->GetOverlayMaterial();
 const float InitialHP=ASC->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute());
 const float InitialAttack=ASC->GetNumericAttribute(UCC_AttributeSet::GetAttackAttribute());
 TestFalse(TEXT("Fresh Ink is not unlocked"),P->HasChromaticUnlock());
 P->ToggleChromaticAppearance();
 TestTrue(TEXT("Q previews before acquisition"),P->IsChromaticAppearanceEnabled());
 TestTrue(TEXT("Preview applies body and outline"),Player->IsChromaticMaterialApplied());
 TestTrue(TEXT("Preview also previews rainbow health"),P->HasRainbowHealth());
 TestEqual(TEXT("Preview cannot heal"),ASC->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute()),InitialHP);
 TestEqual(TEXT("Preview cannot change attack"),ASC->GetNumericAttribute(UCC_AttributeSet::GetAttackAttribute()),InitialAttack);
 P->ToggleChromaticAppearance();
 TestFalse(TEXT("Second Q clears preview"),Player->IsChromaticMaterialApplied());
 TestTrue(TEXT("Exact original material overrides restored"),OriginalOverrides==Player->GetMesh()->OverrideMaterials);
 TestEqual(TEXT("Exact original overlay restored"),Player->GetMesh()->GetOverlayMaterial(),OriginalOverlay);
 const FName IDs[]={TEXT("Native"),TEXT("Artifact_01"),TEXT("Artifact_02")};
 for(int32 Index=0;Index<3;++Index)
 {
  auto Found=P->ArtifactForms.FindByPredicate([&](const UCC_FormDefinition* F){return F&&F->FormId==IDs[Index];});
  if(!TestTrue(TEXT("Required element is in catalog"),Found!=nullptr))break;
  auto* Orb=World->SpawnActor<ACC_FormPowerOrb>();Orb->Recipient=PS;Orb->RewardForm=*Found;Orb->ExperienceValue=0;
  Orb->SetActorLocation(Player->GetActorLocation());Orb->BeginSeeking();
  TestTrue(TEXT("Element acquired by actual orb contact"),Orb->Phase==ECC_ExperienceOrbPhase::Collected||Orb->TryCollect(Player));
  TestEqual(TEXT("Only all three elements unlock chromatic"),P->HasChromaticUnlock(),Index==2);
 }
 TestTrue(TEXT("Third element automatically enables persistent outline"),Player->IsChromaticMaterialApplied());
 TestEqual(TEXT("Unlock keeps current form"),P->GetActiveForm(),P->NativeForm.Get());
 P->ToggleChromaticAppearance();
 TestFalse(TEXT("Q suppresses unlocked outline"),P->IsChromaticAppearanceEnabled());
 TestTrue(TEXT("Rainbow health stays unlocked when outline is hidden"),P->HasRainbowHealth());
 auto* Save=P->CreateSnapshot();
 TestTrue(TEXT("Preference saves"),Save&&Save->bChromaticSuppressed);
 P->ToggleChromaticAppearance();
 TestTrue(TEXT("Q re-enables unlocked outline"),Player->IsChromaticMaterialApplied());
 TestTrue(TEXT("Snapshot restores"),P->RestoreSnapshot(Save));
 TestFalse(TEXT("Saved suppression survives restore"),Player->IsChromaticMaterialApplied());
 TestTrue(TEXT("Collection survives restore"),P->HasChromaticUnlock());
 P->ToggleChromaticAppearance();
 const auto* BronzeEntry=P->ArtifactForms.FindByPredicate([](const UCC_FormDefinition* F){return F&&F->FormId==TEXT("Artifact_01");});
 UCC_FormDefinition* Bronze=BronzeEntry?BronzeEntry->Get():nullptr;
 P->SwitchForm(Bronze);
 TestTrue(TEXT("Overlay follows a changed mesh"),Player->IsChromaticMaterialApplied());
 P->ToggleChromaticAppearance();
 TestTrue(TEXT("Changed form restores its own mesh materials"),Player->GetMesh()->OverrideMaterials.IsEmpty());
 TestEqual(TEXT("Q never changes form selection"),P->GetActiveForm(),Bronze);
 World->DestroyWorld(false);GEngine->DestroyWorldContext(World);
 return true;
}
#endif
