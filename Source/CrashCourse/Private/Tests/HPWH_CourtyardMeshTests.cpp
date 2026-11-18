#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/BodySetup.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHPWHCourtyardMeshTest, "CrashCourse.HPWH.CourtyardMesh",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHPWHCourtyardMeshTest::RunTest(const FString&)
{
    UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/HPWH/Assets/Models/四合院模型/StaticMeshes/courtyard_model.courtyard_model"));
    if (!TestNotNull(TEXT("Courtyard mesh"), Mesh) || !TestNotNull(TEXT("Body setup"), Mesh->GetBodySetup())) return false;
    TestEqual(TEXT("Uses triangles for movement sweeps"), Mesh->GetBodySetup()->CollisionTraceFlag.GetValue(), CTF_UseComplexAsSimple);
    UWorld::InitializationValues Values;
    Values.CreatePhysicsScene(true).AllowAudioPlayback(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
    if (!TestNotNull(TEXT("Physics test world"), World)) return false;
    UStaticMeshComponent* Comp = World->SpawnActor<AStaticMeshActor>(FVector(0,0,74),FRotator::ZeroRotator)->GetStaticMeshComponent();
    Comp->SetMobility(EComponentMobility::Movable);
    Comp->SetWorldScale3D(FVector(.21f));
    Comp->SetStaticMesh(Mesh);
    Comp->SetCollisionProfileName(TEXT("BlockAll"));
    Comp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Comp->RecreatePhysicsState();

    int32 Clear = 0;
    int32 WallHits = 0;
    for (const FVector Direction : {FVector::ForwardVector,-FVector::ForwardVector,FVector::RightVector,-FVector::RightVector})
    {
        const FVector Tangent = FVector::CrossProduct(FVector::UpVector,Direction);
        for (int32 Offset = -220; Offset <= 220; Offset += 10)
        {
            const FVector Base = FVector(0,0,90) + Offset*Tangent;
            const float RadiusEntry = FMath::Sqrt(315.f * 315.f - Offset * Offset);
            FHitResult Hit;
            if (!World->SweepSingleByChannel(Hit, Base+450*Direction, Base+RadiusEntry*Direction, FQuat::Identity,
                ECC_Pawn, FCollisionShape::MakeCapsule(40,90))) ++Clear;
            if (World->SweepSingleByChannel(Hit, Base+450*Direction, Base, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(40,90))) ++WallHits;
        }
    }
    AddInfo(FString::Printf(TEXT("Clear approach routes: %d/180"), Clear));
    TestTrue(TEXT("Courtyard mesh blocks some approaches"), WallHits > 0);
    TestTrue(TEXT("Enemy capsule can enter within the original 320 cm puzzle radius"), Clear > 0);
    World->DestroyWorld(false);
    return true;
}
#endif
