#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Characters/CC_LocomotionComponent.h"
#include "Characters/CC_CharacterAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimBlueprint.h"
#include "AnimationRuntime.h"
#include "AnimGraphNode_LayeredBoneBlend.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCC_UpperBodyAttackTest, "CrashCourse.Character.UpperBodyAttack", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCC_UpperBodyAttackTest::RunTest(const FString& Parameters)
{
    auto* Library = LoadObject<UCC_LocomotionLibrary>(nullptr, TEXT("/Game/HPWH/Locomotion/DA_NPCLocomotion.DA_NPCLocomotion"));
    if (!TestNotNull(TEXT("Locomotion library exists"), Library)) return false;
    UWorld::InitializationValues Values;
    Values.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    auto* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("UpperBodyAttackTest"), nullptr, true, ERHIFeatureLevel::Num, &Values);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    auto* Character = World->SpawnActor<ACC_PlayerCharacter>();
    Character->Locomotion->Library = Library;
    auto* AttackMesh = Character->GetMesh();
    auto* ControlMesh = NewObject<USkeletalMeshComponent>(Character);
    ControlMesh->RegisterComponentWithWorld(World);
    AttackMesh->bEnableUpdateRateOptimizations = ControlMesh->bEnableUpdateRateOptimizations = false;
    AttackMesh->VisibilityBasedAnimTickOption = ControlMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

    for (const auto& Entry : Library->Entries)
    {
        if (!Entry.Mesh || !Entry.AnimClass) continue;
        const FString Label = Entry.Mesh->GetName();
        auto* Blueprint = Cast<UAnimBlueprint>(Entry.AnimClass->ClassGeneratedBy);
        UAnimGraphNode_LayeredBoneBlend* Layer = nullptr;
        TArray<UEdGraph*> Graphs; Blueprint->GetAllGraphs(Graphs);
        for (UEdGraph* Graph : Graphs) for (UEdGraphNode* Node : Graph->Nodes)
            if (auto* Candidate = Cast<UAnimGraphNode_LayeredBoneBlend>(Node)) Layer = Candidate;
        if (!TestNotNull(Label + TEXT(" upper-body blend exists"), Layer)) continue;
        TArray<FPerBoneBlendWeight> Weights;
        FAnimationRuntime::CreateMaskWeights(Weights, Layer->Node.LayerSetup, Entry.Mesh->GetSkeleton());
        const auto& Skeleton = Entry.Mesh->GetRefSkeleton();
        TArray<int32> LowerBones, UpperBones;
        for (int32 I = 0; I < Skeleton.GetNum(); ++I)
        {
            const FString Bone = Skeleton.GetBoneName(I).ToString();
            if (Bone.Contains(TEXT("thigh")) || Bone.Contains(TEXT("calf")) || Bone.Contains(TEXT("foot")) || Bone.Contains(TEXT("toe")) || Bone.Contains(TEXT("pelvis")) || Bone.Contains(TEXT("Hips")) || Bone.Contains(TEXT("Leg")))
            {
                LowerBones.Add(I);
                TestEqual(Label + TEXT(" lower-body mask ") + Bone, Weights[I].BlendWeight, 0.f);
            }
            if (Bone.Contains(TEXT("hand")) || Bone.Contains(TEXT("Hand"))) UpperBones.Add(I);
        }
        TestTrue(Label + TEXT(" has lower and upper bones to compare"), !LowerBones.IsEmpty() && !UpperBones.IsEmpty());
        AttackMesh->SetSkeletalMesh(Entry.Mesh); ControlMesh->SetSkeletalMesh(Entry.Mesh);
        AttackMesh->SetAnimInstanceClass(Entry.AnimClass); ControlMesh->SetAnimInstanceClass(Entry.AnimClass);
        auto TickPoses = [&]()
        {
            AttackMesh->TickAnimation(1.f/60.f, false); AttackMesh->RefreshBoneTransforms();
            ControlMesh->TickAnimation(1.f/60.f, false); ControlMesh->RefreshBoneTransforms();
        };
        const UAnimSequence* Clips[] = { Entry.Jab, Entry.CrossJab, Entry.Uppercut };
        for (int32 Mode = 0; Mode < 3; ++Mode) for (const UAnimSequence* Clip : Clips)
        {
            Character->GetCharacterMovement()->SetMovementMode(Mode == 2 ? MOVE_Falling : MOVE_Walking);
            Character->GetCharacterMovement()->Velocity = Mode == 0 ? FVector::ZeroVector : Mode == 1 ? FVector(400,0,0) : FVector(100,0,-200);
            AttackMesh->InitAnim(true); ControlMesh->InitAnim(true);
            for (int32 I = 0; I < 30; ++I) TickPoses();
            auto* Anim = AttackMesh->GetAnimInstance();
            Anim->PlaySlotAnimationAsDynamicMontage(const_cast<UAnimSequence*>(Clip), TEXT("DefaultSlot"), .12f, .18f);
            double LowerError = 0., UpperDifference = 0.;
            for (int32 I = 0; I < 18; ++I)
            {
                TickPoses();
                const auto& Attack = AttackMesh->GetComponentSpaceTransforms();
                const auto& Control = ControlMesh->GetComponentSpaceTransforms();
                for (int32 Bone : LowerBones)
                    LowerError = FMath::Max(LowerError, FVector::Distance(Attack[Bone].GetLocation(), Control[Bone].GetLocation()) + Attack[Bone].GetRotation().AngularDistance(Control[Bone].GetRotation()));
                for (int32 Bone : UpperBones)
                    UpperDifference = FMath::Max(UpperDifference, FVector::Distance(Attack[Bone].GetLocation(), Control[Bone].GetLocation()) + Attack[Bone].GetRotation().AngularDistance(Control[Bone].GetRotation()));
            }
            const FString Case = FString::Printf(TEXT("%s %s mode %d"), *Label, *Clip->GetName(), Mode);
            TestTrue(Case + FString::Printf(TEXT(" lower body follows locomotion (error %.6f)"), LowerError), LowerError < .01);
            TestTrue(Case + TEXT(" attack visibly changes upper body"), UpperDifference > .1);
        }
    }
    ControlMesh->DestroyComponent();
    GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
    return true;
}
#endif
