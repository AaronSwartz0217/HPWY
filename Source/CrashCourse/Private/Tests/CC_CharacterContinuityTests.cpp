#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Characters/CC_CharacterAnimInstance.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Characters/CC_LocomotionComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCC_AnimationContinuityTest,"CrashCourse.Character.AnimationContinuity",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCC_AnimationContinuityTest::RunTest(const FString& Parameters)
{
    auto* OwnerMesh=NewObject<USkeletalMeshComponent>();
    auto* Anim=NewObject<UCC_CharacterAnimInstance>(OwnerMesh);
    Anim->UpdateMovementState(.016f,600.f,0.f,false,true,400.f);
    TestTrue(TEXT("Speed interpolates instead of jumping"),Anim->Speed>0.f && Anim->Speed<600.f);
    TestTrue(TEXT("Run play rate interpolates"),Anim->RunPlayRate>1.f && Anim->RunPlayRate<1.5f);
    TestTrue(TEXT("Run starts above entry threshold"),Anim->bRun);
    Anim->UpdateMovementState(.016f,8.f,0.f,false,true,400.f);
    TestTrue(TEXT("Run does not chatter near idle threshold"),Anim->bRun);
    Anim->UpdateMovementState(.016f,1.f,0.f,false,true,400.f);
    TestTrue(TEXT("Idle returns below exit threshold"),Anim->bIdle);
    Anim->UpdateMovementState(.016f,0.f,100.f,true,true,400.f);
    TestTrue(TEXT("Ascent enters Jump"),Anim->bJump);
    Anim->UpdateMovementState(.016f,0.f,-100.f,true,true,400.f);
    TestTrue(TEXT("Descent enters Fall"),Anim->bFall);
    Anim->UpdateMovementState(.016f,0.f,0.f,false,true,400.f);
    TestTrue(TEXT("Touchdown enters Land"),Anim->bLand);
    for (int32 I=0;I<10;++I) Anim->UpdateMovementState(.016f,0.f,0.f,false,true,400.f);
    TestTrue(TEXT("Landing pose has time to blend instead of exiting in 0.15 seconds"),Anim->bLand);
    for (int32 I=0;I<10;++I) Anim->UpdateMovementState(.016f,0.f,0.f,false,true,400.f);
    TestTrue(TEXT("Landing returns to Idle"),Anim->bIdle);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCC_CharacterGroundingTest,"CrashCourse.Character.MeshGrounding",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCC_CharacterGroundingTest::RunTest(const FString& Parameters)
{
    UWorld::InitializationValues Values;Values.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    auto* World=UWorld::CreateWorld(EWorldType::Game,false,TEXT("GroundingTest"),nullptr,true,ERHIFeatureLevel::Num,&Values);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    auto* Character=World->SpawnActor<ACC_PlayerCharacter>();
    auto* Library=LoadObject<UCC_LocomotionLibrary>(nullptr,TEXT("/Game/HPWH/Locomotion/DA_NPCLocomotion.DA_NPCLocomotion"));
    if (!TestNotNull(TEXT("Locomotion library exists"),Library)) {GEngine->DestroyWorldContext(World);World->DestroyWorld(false);return false;}
    Character->Locomotion->Library=Library;
    auto* Movement=Character->GetCharacterMovement();Movement->SetMovementMode(MOVE_Walking);
    Movement->CurrentFloor.bBlockingHit=true;Movement->CurrentFloor.bWalkableFloor=true;Movement->CurrentFloor.FloorDist=2.f;
    for (const auto& Entry:Library->Entries)
    {
        if (!Entry.Mesh) continue;
        auto* Mesh=Character->GetMesh();Mesh->SetSkeletalMesh(Entry.Mesh);
        const float Scale=Entry.Mesh->GetName().Contains(TEXT("InitialInk")) ? 1.f : 180.f/(Entry.Mesh->GetBounds().BoxExtent.Z*2.f);
        Mesh->SetRelativeScale3D(FVector(Scale));Mesh->SetRelativeRotation(FRotator(0,-90,0));Mesh->SetRelativeLocation(FVector(0,0,-90));
        Character->Locomotion->AlignMeshToGround();
        const FBox Visual=Entry.Mesh->GetBounds().GetBox().TransformBy(Mesh->GetRelativeTransform());
        const float Expected=-Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()-2.f;
        TestTrue(*FString::Printf(TEXT("%s feet align with capsule floor"),*Entry.Mesh->GetName()),FMath::IsNearlyEqual(Visual.Min.Z,Expected,.1f));
        const double GroundedZ=Mesh->GetRelativeLocation().Z;
        Movement->SetMovementMode(MOVE_Falling);Character->Locomotion->AlignMeshToGround();
        TestEqual(TEXT("Jump does not snap the visual root back up"),Mesh->GetRelativeLocation().Z,GroundedZ);
        Movement->SetMovementMode(MOVE_Walking);
    }
    GEngine->DestroyWorldContext(World);World->DestroyWorld(false);return true;
}
#endif
