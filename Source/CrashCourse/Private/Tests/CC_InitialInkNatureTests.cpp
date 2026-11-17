#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Player/CC_PlayerState.h"
#include "Progression/CC_ArtifactComponent.h"
#include "Progression/CC_ConfiguredAbility.h"
#include "Progression/CC_GameplayData.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Progression/CC_ProgressionSave.h"
#include "Progression/CC_SkillComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Progression/CC_FormPowerOrb.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCC_InitialInkNatureTest, "CrashCourse.Progression.InitialInkAndNature", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCC_InitialInkNatureTest::RunTest(const FString& Parameters)
{
    auto* Ink = LoadObject<UCC_FormDefinition>(nullptr, TEXT("/Game/HPWH/Forms/DA_Form_InitialInk.DA_Form_InitialInk"));
    auto* Nature = LoadObject<UCC_FormDefinition>(nullptr, TEXT("/Game/HPWH/Forms/DA_Form_Native.DA_Form_Native"));
    auto* Bronze = LoadObject<UCC_FormDefinition>(nullptr, TEXT("/Game/HPWH/Forms/DA_Form_Artifact_01.DA_Form_Artifact_01"));
    if (!TestNotNull(TEXT("Initial Ink form configured"), Ink) || !TestNotNull(TEXT("Nature route retained"), Nature) || !TestNotNull(TEXT("Bronze route retained"), Bronze)) return false;
    TestEqual(TEXT("Ink has its own stable ID"), Ink->FormId, FName(TEXT("InitialInk")));
    TestEqual(TEXT("Nature keeps the existing stable ID"), Nature->FormId, FName(TEXT("Native")));
    TestFalse(TEXT("Initial Ink does not sell evolution stages"), Ink->bCanEvolve);
    TestTrue(TEXT("Initial Ink remains a complete form definition"), Ink->IsComplete());
    TestTrue(TEXT("Nature still offers evolution"), Nature->bCanEvolve);
    UClass* ConfiguredState = LoadClass<ACC_PlayerState>(nullptr, TEXT("/Game/HPWH/Blueprints/BP_ProgressionPlayerState.BP_ProgressionPlayerState_C"));
    if (!TestNotNull(TEXT("PlayerState configuration exists"), ConfiguredState)) return false;
    const auto* ConfiguredProgression = ConfiguredState->GetDefaultObject<ACC_PlayerState>()->Progression.Get();
    TestEqual(TEXT("Game starts as Ink"), ConfiguredProgression->NativeForm.Get(), Ink);
    TestTrue(TEXT("Nature requires the artifact catalog"), ConfiguredProgression->ArtifactForms.Contains(Nature));
    TestEqual(TEXT("Bronze shortcut order retained"), ConfiguredProgression->ArtifactForms[0].Get(), Bronze);
    UClass* WhiteClass = LoadClass<ACC_FormArtifact>(nullptr, TEXT("/Game/HPWH/Blueprints/BP_Artifact_Nature.BP_Artifact_Nature_C"));
    if (!TestNotNull(TEXT("White Nature artifact blueprint"), WhiteClass)) return false;
    TestEqual(TEXT("White cube points to Nature"), WhiteClass->GetDefaultObject<ACC_FormArtifact>()->Artifact->Form.Get(), Nature);

    UWorld::InitializationValues Values;
    Values.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("InitialInkNatureTestWorld"), nullptr, true, ERHIFeatureLevel::Num, &Values);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    auto* PS = World->SpawnActor<ACC_PlayerState>();
    auto* Player = World->SpawnActor<ACC_PlayerCharacter>();
    Player->SetPlayerState(PS);
    auto* ASC = Player->GetAbilitySystemComponent();
    ASC->InitAbilityActorInfo(PS, Player);
    ASC->AddAttributeSetSubobject(Cast<UCC_AttributeSet>(Player->GetAttributeSet()));
    ASC->ApplyGameplayEffectToSelf(GetDefault<UCC_InitialAttributesEffect>(), 1.f, ASC->MakeEffectContext());
    auto* P = PS->Progression.Get();
    P->NativeForm = Ink;
    P->ArtifactForms = ConfiguredProgression->ArtifactForms;
    P->SkillCatalog = ConfiguredProgression->SkillCatalog;
    P->InitializeAvatar();
    TestEqual(TEXT("Initial maximum health is 75% baseline"), ASC->GetNumericAttribute(UCC_AttributeSet::GetMaxHealthAttribute()), 75.f);
    TestEqual(TEXT("Initial attack rounds the 75% baseline"), ASC->GetNumericAttribute(UCC_AttributeSet::GetAttackAttribute()), 11.f);
    TestEqual(TEXT("Initial Ink does not expose an evolution cost"), P->GetEvolutionCost(), -1);
    TestFalse(TEXT("Menu can query that Ink cannot evolve"), P->CanEvolveCurrentForm());
    P->AddExperience(1100);
    P->Evolve();
    TestEqual(TEXT("Attempting Ink evolution cannot spend experience"), P->Experience, 1100);
    TestEqual(TEXT("Attempting Ink evolution cannot change tier"), P->CurrentTier, 0);
    P->SwitchForm(Nature);
    TestNull(TEXT("Nature cannot be selected before interaction"), P->CurrentForm.Get());
    auto* Artifact = World->SpawnActor<ACC_FormArtifact>();
    Artifact->Artifact->Form = Nature;
    Artifact->SetActorLocation(FVector(100, 0, 0));
    P->Interact(Artifact);
    TestEqual(TEXT("Interaction does not unlock before contact"),P->Forms.Num(),0);
    ACC_FormPowerOrb* PowerOrb=nullptr;
    for (TActorIterator<ACC_FormPowerOrb> It(World);It;++It) if (It->Recipient==PS && It->RewardForm==Nature) {PowerOrb=*It;break;}
    TestNotNull(TEXT("Interaction releases Nature token"),PowerOrb);
    if (PowerOrb) {
        PowerOrb->BeginSeeking();PowerOrb->SetActorLocation(Player->GetActorLocation());
        TestTrue(TEXT("Nature token grants on physical contact"),PowerOrb->Phase==ECC_ExperienceOrbPhase::Collected || PowerOrb->TryCollect(Player));
    }
    TestEqual(TEXT("White artifact adds Nature to inventory"), P->Forms.Num(), 1);
    TestEqual(TEXT("Acquisition keeps Ink active until inventory selection"), P->GetActiveForm(), Ink);
    TestEqual(TEXT("Token experience strengthens active Ink"), ASC->GetNumericAttribute(UCC_AttributeSet::GetMaxHealthAttribute()), 938.f);
    P->SwitchForm(Nature);
    TestEqual(TEXT("Inventory use activates unlocked Nature"), P->GetActiveForm(), Nature);
    TestEqual(TEXT("Nature starts with 100 health"), ASC->GetNumericAttribute(UCC_AttributeSet::GetMaxHealthAttribute()), 100.f);
    TestEqual(TEXT("Nature starts with 15 attack"), ASC->GetNumericAttribute(UCC_AttributeSet::GetAttackAttribute()), 15.f);
    TestEqual(TEXT("Nature middle evolution costs 100"), P->GetEvolutionCost(), 100);
    P->Evolve();
    TestEqual(TEXT("Nature evolves to middle"), P->CurrentTier, 1);
    TestEqual(TEXT("Middle Nature health follows its existing profile"), ASC->GetNumericAttribute(UCC_AttributeSet::GetMaxHealthAttribute()), 250.f);
    P->SwitchForm(nullptr);
    TestEqual(TEXT("Returning to initial Ink resets only its own tier"), P->CurrentTier, 0);
    TestEqual(TEXT("Returning to Ink uses the remaining 1050 experience"), ASC->GetNumericAttribute(UCC_AttributeSet::GetAttackAttribute()), 129.f);
    P->Interact(Artifact);
    TestEqual(TEXT("Revisiting acquired artifact still keeps Ink active"), P->GetActiveForm(), Ink);
    TestEqual(TEXT("Acquiring the same power does not duplicate inventory"), P->Forms.Num(), 1);
    P->SwitchForm(Nature);
    TestEqual(TEXT("Inventory use preserves Nature tier"), P->CurrentTier, 1);
    TestEqual(TEXT("Nature advanced evolution costs 1000"), P->GetEvolutionCost(), 1000);
    P->Evolve();
    TestEqual(TEXT("Nature reaches advanced"), P->CurrentTier, 2);
    TestEqual(TEXT("Nature advanced health retained"), ASC->GetNumericAttribute(UCC_AttributeSet::GetMaxHealthAttribute()), 450.f);
    TestEqual(TEXT("Nature advanced attack retained"), ASC->GetNumericAttribute(UCC_AttributeSet::GetAttackAttribute()), 67.5f);
    TestEqual(TEXT("Two Nature prices paid and token's 50 experience retained"), P->Experience, 50);

    const FString SaveSlot = TEXT("Codex_InitialInkNature_TestOnly");
    TestTrue(TEXT("Version 2 save succeeds"), P->SaveProgress(SaveSlot));
    auto* CurrentSave = Cast<UCC_ProgressionSave>(UGameplayStatics::LoadGameFromSlot(SaveSlot, 0));
    TestNotNull(TEXT("Version 2 record is readable"), CurrentSave);
    if (CurrentSave)
    {
        TestEqual(TEXT("Current format records version 4"), CurrentSave->Version, 4);
        TestEqual(TEXT("Current format records native identity"), CurrentSave->NativeFormId, FName(TEXT("InitialInk")));
    }
    P->SwitchForm(nullptr);
    TestTrue(TEXT("Version 2 save reloads"), P->LoadProgress(SaveSlot));
    TestEqual(TEXT("Version 2 preserves selected Nature"), P->CurrentForm.Get(), Nature);
    TestEqual(TEXT("Version 2 preserves advanced Nature"), P->CurrentTier, 2);
    P->SwitchForm(nullptr);
    TestTrue(TEXT("Saving selected Ink succeeds"), P->SaveProgress(SaveSlot));
    P->SwitchForm(Nature);
    TestTrue(TEXT("Reloading selected Ink succeeds"), P->LoadProgress(SaveSlot));
    TestNull(TEXT("Version 2 keeps Ink selected when no artifact is selected"), P->CurrentForm.Get());
    TestEqual(TEXT("Version 2 does not invent an Ink evolution tier"), P->NativeTier, 0);

    auto* Legacy = Cast<UCC_ProgressionSave>(UGameplayStatics::CreateSaveGameObject(UCC_ProgressionSave::StaticClass()));
    Legacy->Version = 1; Legacy->Experience = 456; Legacy->NativeTier = 1;
    Legacy->FormTiers.Add(Bronze->FormId, 2);
    TestTrue(TEXT("Legacy fixture writes to a test-only slot"), UGameplayStatics::SaveGameToSlot(Legacy, SaveSlot, 0));
    TestTrue(TEXT("Legacy version 1 migrates"), P->LoadProgress(SaveSlot));
    TestEqual(TEXT("Migration preserves wallet"), P->Experience, 456);
    TestEqual(TEXT("Legacy selected native now selects Nature"), P->CurrentForm.Get(), Nature);
    TestEqual(TEXT("Legacy native tier is restored on Nature"), P->CurrentTier, 1);
    TestEqual(TEXT("Legacy native tier is not applied to Ink"), P->NativeTier, 0);
    Legacy->SelectedForm = Bronze->FormId;
    UGameplayStatics::SaveGameToSlot(Legacy, SaveSlot, 0);
    TestTrue(TEXT("Legacy selected artifact migrates"), P->LoadProgress(SaveSlot));
    TestEqual(TEXT("Legacy selected artifact stays selected"), P->CurrentForm.Get(), Bronze);
    TestEqual(TEXT("Legacy artifact level stays selected"), P->CurrentTier, 2);
    P->SwitchForm(Nature);
    TestEqual(TEXT("Legacy Nature still unlocks when another artifact was selected"), P->CurrentTier, 1);
    P->ArtifactForms.Remove(Nature);
    P->RestoreExperienceFromBackend(22);
    TestFalse(TEXT("Incomplete migration refuses loading"), P->LoadProgress(SaveSlot));
    TestEqual(TEXT("Refused migration leaves current wallet intact"), P->Experience, 22);
    P->ArtifactForms.Add(Nature);
    UGameplayStatics::DeleteGameInSlot(SaveSlot, 0);
    World->DestroyWorld(false);
    GEngine->DestroyWorldContext(World);
    return true;
}
#endif
