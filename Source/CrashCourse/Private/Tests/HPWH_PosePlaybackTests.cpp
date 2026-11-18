#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "HPWH/HPWH_Player.h"
#include "HPWH/HPWH_Crawler.h"
#include "Characters/CC_CharacterAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHPWHPosePlaybackTest, "CrashCourse.HPWH.PosePlayback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHPWHPosePlaybackTest::RunTest(const FString&)
{
    UWorld::InitializationValues Values;
    Values.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
    if (!TestNotNull(TEXT("Test world"), World)) return false;
    auto* Player = World->SpawnActor<AHPWH_PlayerCharacter>();
    auto* Crawler = World->SpawnActor<AHPWH_Crawler>(FVector(1000,0,0),FRotator::ZeroRotator);
    auto CheckMotion = [&](ACharacter* Character, const TCHAR* Label, bool bRun)
    {
        USkeletalMeshComponent* Mesh = Character->GetMesh();
        if (!TestNotNull(FString(Label) + TEXT(" skeleton"), Mesh->GetSkeletalMeshAsset()->GetSkeleton())) return;
        auto* Anim = Cast<UCC_CharacterAnimInstance>(Mesh->GetAnimInstance());
        if (!TestNotNull(FString(Label) + TEXT(" live animation instance"), Anim)) return;
        Character->GetCharacterMovement()->Velocity = bRun ? FVector(400,0,0) : FVector::ZeroVector;
        auto Advance = [&]()
        {
            Mesh->TickAnimation(1.f/30.f, false);
            Mesh->RefreshBoneTransforms();
        };
        for (int32 I=0; I<12; ++I) Advance();
        const auto Initial = Mesh->GetComponentSpaceTransforms();
        bool bChanged = false;
        for (int32 Frame=0; Frame<24 && !bChanged; ++Frame)
        {
            Advance();
            const auto& Current = Mesh->GetComponentSpaceTransforms();
            for (int32 I=1; I<FMath::Min(Initial.Num(),Current.Num()); ++I)
                if (!Initial[I].Equals(Current[I],.002f)) { bChanged = true; break; }
        }
        TestTrue(FString(Label) + TEXT(" bone pose changes over time"), bChanged);
        TestEqual(FString(Label) + TEXT(" run state"), Anim->bRun, bRun);
    };
    TestEqual(TEXT("Uses the supplied player model"), Player->GetMesh()->GetSkeletalMeshAsset()->GetPathName(), FString(TEXT("/Game/zichan/model/主角/SkeletalMeshes/主角.主角")));
    TestEqual(TEXT("Uses the supplied crawler model"), Crawler->GetMesh()->GetSkeletalMeshAsset()->GetPathName(), FString(TEXT("/Game/zichan/model/徘徊者/SkeletalMeshes/徘徊者.徘徊者")));
    CheckMotion(Player,TEXT("Protagonist idle"),false);
    CheckMotion(Player,TEXT("Protagonist run"),true);
    CheckMotion(Crawler,TEXT("Crawler idle"),false);
    World->DestroyWorld(false);
    return true;
}
#endif
