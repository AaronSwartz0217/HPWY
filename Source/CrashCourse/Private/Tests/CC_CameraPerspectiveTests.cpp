#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/World.h"
#include "Engine/Blueprint.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraphUtilities.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCC_CameraPerspectiveTest,"CrashCourse.Camera.FirstAndThirdPerson",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCC_CameraPerspectiveTest::RunTest(const FString&)
{
    // Preserve the original Blueprint graph as concrete migration evidence.
    if (auto* BP=LoadObject<UBlueprint>(nullptr,TEXT("/Game/LanTuRebuild/Blueprints/WPQT/功能性蓝图/BP_ThirdPersonWPQT.BP_ThirdPersonWPQT")))
    {
        FString GraphText;
        for (UEdGraph* Graph:BP->UbergraphPages) {TSet<UObject*> Nodes;for(UEdGraphNode* N:Graph->Nodes)Nodes.Add(N);FString Text;FEdGraphUtilities::ExportNodesToText(Nodes,Text);GraphText+=Text;}
        if(BP->SimpleConstructionScript)for(auto* N:BP->SimpleConstructionScript->GetAllNodes())if(auto* C=Cast<UCameraComponent>(N->ComponentTemplate))GraphText+=FString::Printf(TEXT("\nLEGACY_CAMERA %s location=%s pawnControl=%d active=%d\n"),*N->GetVariableName().ToString(),*C->GetRelativeLocation().ToString(),C->bUsePawnControlRotation,C->IsActive());
        const FString Directory=FPaths::ProjectSavedDir()/TEXT("FirstPerson");IFileManager::Get().MakeDirectory(*Directory,true);
        FFileHelper::SaveStringToFile(GraphText,*(Directory/TEXT("LegacyCharacterCameraGraph.txt")));
    }
    auto* W=UWorld::CreateWorld(EWorldType::EditorPreview,false);
    auto* C=W->SpawnActor<ACC_PlayerCharacter>();
    if(!TestNotNull(TEXT("Character"),C)){W->DestroyWorld(false);return false;}

    TestTrue(TEXT("Camera defaults to and is locked in top-down"),C->IsTopDown());
    TestTrue(TEXT("Top-down camera is the only active camera by default"),C->GetTopDownCamera()->IsActive()&&!C->GetFollowCamera()->IsActive()&&!C->GetFirstPersonCamera()->IsActive());
    TestTrue(TEXT("Top-down keeps the body visible"),!C->GetMesh()->bOwnerNoSee);
    TestTrue(TEXT("Top-down boom is raised above the character"),C->GetTopDownBoom()->TargetArmLength>=300.f);
    TestTrue(TEXT("Top-down faces movement"),C->GetCharacterMovement()->bOrientRotationToMovement);

    for(int32 I=0;I<3;++I)
    {
        // Damage is the only supported way to reach first person.
        C->SetFirstPerson(true);
        TestTrue(TEXT("First-person camera is the only active camera"),C->GetFirstPersonCamera()->IsActive()&&!C->GetFollowCamera()->IsActive()&&!C->GetTopDownCamera()->IsActive());
        TestTrue(TEXT("Owner geometry cannot occlude first-person view"),C->GetMesh()->bOwnerNoSee);
        TestTrue(TEXT("First person follows view yaw"),C->bUseControllerRotationYaw&&!C->GetCharacterMovement()->bOrientRotationToMovement);
        TestTrue(TEXT("Eye is capsule-attached, independent of animation bones"),C->GetFirstPersonCamera()->GetAttachParent()==C->GetCapsuleComponent());

        C->SetFirstPerson(false);
        TestTrue(TEXT("Leaving first person returns to top-down"),C->IsTopDown()&&C->GetTopDownCamera()->IsActive()&&!C->GetFirstPersonCamera()->IsActive()&&!C->GetMesh()->bOwnerNoSee);

        C->SetCameraMode(ECC_CameraMode::ThirdPerson);
        TestTrue(TEXT("Third person is unavailable and coerces to top-down"),C->IsTopDown()&&C->GetTopDownCamera()->IsActive()&&!C->GetFollowCamera()->IsActive());
    }
    W->DestroyWorld(false);return true;
}
#endif
