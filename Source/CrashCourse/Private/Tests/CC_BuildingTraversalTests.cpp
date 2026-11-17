#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/WorldSettings.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "PhysicsEngine/BodySetup.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCC_BuildingTraversalTest,"CrashCourse.PCG.BuildingTraversal",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCC_BuildingTraversalTest::RunTest(const FString&)
{
    const FString Roots[]={TEXT("/SpiritWFC/Settlement/Meshes/"),TEXT("/Game/LanTuRebuild/Maps/万象天宫/Meshes/")};
    for (const FString& Root:Roots) for (int32 Turn=0;Turn<4;++Turn) for (int32 Kind=0;Kind<4;++Kind)
    {
        UWorld::InitializationValues Values;Values.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
        auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        const FTransform Rotation(FRotator(0,Turn*90,0));
        auto* Geometry=World->SpawnActor<AActor>();
        auto AddMesh=[&](UStaticMesh* Mesh,const FTransform& Transform)
        {
            auto* Component=NewObject<UHierarchicalInstancedStaticMeshComponent>(Geometry);
            Geometry->AddInstanceComponent(Component);Mesh->GetBodySetup()->CreatePhysicsMeshes();Component->SetStaticMesh(Mesh);Component->SetWalkableSlopeOverride(Mesh->GetBodySetup()->WalkableSlopeOverride);
            Component->SetMobility(EComponentMobility::Static);Component->SetCollisionProfileName(TEXT("BlockAll"));
            Component->RegisterComponent();Component->AddInstance(Transform);Component->BuildTreeIfOutdated(false,true);Component->RecreatePhysicsState();
        };
        auto* Base=LoadObject<UStaticMesh>(nullptr,*(Root+(Kind==1 ? TEXT("SM_Settlement_House_0") : Kind==2 ? TEXT("SM_Settlement_CompoundBase_0") : TEXT("SM_Settlement_Base_0"))));
        auto* Top=LoadObject<UStaticMesh>(nullptr,*(Root+TEXT("SM_Settlement_Top_0")));
        if (!TestNotNull(Root+TEXT(" base mesh"),Base) || !TestNotNull(Root+TEXT(" top mesh"),Top)) {GEngine->DestroyWorldContext(World);World->DestroyWorld(false);continue;}
        AddMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")),FTransform(FRotator::ZeroRotator,FVector(0,0,-50),FVector(20,20,1)));
        AddMesh(Base,Rotation);if (Kind!=1) AddMesh(Top,FTransform(FRotator(0,Turn*90,0),FVector(0,0,Kind==3 ? 720 : 360)));
        if (Kind==3) AddMesh(LoadObject<UStaticMesh>(nullptr,*(Root+TEXT("SM_Settlement_Middle_0"))),FTransform(FRotator(0,Turn*90,0),FVector(0,0,360)));
        if (Kind==2) AddMesh(LoadObject<UStaticMesh>(nullptr,*(Root+TEXT("SM_Settlement_CompoundWing_0"))),FTransform(FRotator(0,Turn*90,0),Rotation.TransformPosition(FVector(600,0,0))));
        FHitResult GroundProbe;FCollisionQueryParams ProbeParams;ProbeParams.bTraceComplex=true;
        World->LineTraceSingleByChannel(GroundProbe,Rotation.TransformPosition(FVector(-145,-175,500)),Rotation.TransformPosition(FVector(-145,-175,-50)),ECC_Visibility,ProbeParams);
        AddInfo(Root+FString::Printf(TEXT(" Stair trace at yaw %d: hit=%d z=%.1f, bounds=%s"),Turn,GroundProbe.bBlockingHit,GroundProbe.ImpactPoint.Z,*Base->GetBounds().BoxExtent.ToString()));
        auto* Character=World->SpawnActor<ACharacter>();
        Character->GetCapsuleComponent()->InitCapsuleSize(42,96);
        auto* Movement=Character->GetCharacterMovement();Movement->bRunPhysicsWithNoController=true;
        Movement->MaxWalkSpeed=180;Movement->MaxStepHeight=45;Movement->SetWalkableFloorAngle(44.0f);
        Character->SetActorLocation(Rotation.TransformPosition(FVector(130,-330,98)),false,nullptr,ETeleportType::TeleportPhysics);
        World->GetWorldSettings()->NotifyBeginPlay();Movement->SetMovementMode(MOVE_Walking);
        for (int32 I=0;I<15;++I) Movement->TickComponent(1.f/60,LEVELTICK_All,nullptr);
        // Enter, cross beneath the upper flight, climb/turn/exit onto the upper floor, then return.
        TArray<FVector> Waypoints={{130,-145,18},{-145,-145,54},{-145,160,180},{5,160,180},{5,-145,360},{135,-145,360},{135,100,360},
                                  {135,-145,360},{5,-145,360},{5,160,180},{-145,160,180},{-145,-145,54},{130,-145,18},{130,-330,0}};
        if (Kind==1) Waypoints={{130,-120,0},{130,100,0},{-130,100,0},{-130,-100,0},{130,-120,0},{130,-330,0}};
        if (Kind==2) {TArray<FVector> Entrance={{130,-145,18},{130,0,0},{600,0,0},{700,140,0},{600,0,0},{130,0,0}};Entrance.Append(Waypoints);Waypoints=MoveTemp(Entrance);}
        if (Kind==3) {TArray<FVector> Route;for (int32 I=0;I<7;++I)Route.Add(Waypoints[I]);for (int32 I=0;I<Waypoints.Num()-1;++I)Route.Add(Waypoints[I]+FVector(0,0,360));for (int32 I=7;I<Waypoints.Num();++I)Route.Add(Waypoints[I]);Waypoints=MoveTemp(Route);}
        bool Passed=true;
        for (int32 Point=0;Point<Waypoints.Num();++Point)
        {
            const FVector Target=Rotation.TransformPosition(Waypoints[Point]+FVector(0,0,98));
            bool Reached=false;
            for (int32 Tick=0;Tick<480;++Tick)
            {
                FVector Direction=Target-Character->GetActorLocation();Direction.Z=0;
                if (Direction.Size2D()<12 && FMath::Abs(Target.Z-Character->GetActorLocation().Z)<35) {Reached=true;break;}
                Movement->AddInputVector(Direction.GetSafeNormal());Movement->TickComponent(1.f/60,LEVELTICK_All,nullptr);
            }
            if (!Reached) {AddError(FString::Printf(TEXT("%s turn %d kind %d waypoint %d blocked at %s (target %s)"),*Root,Turn,Kind,Point,*Rotation.InverseTransformPosition(Character->GetActorLocation()).ToString(),*Waypoints[Point].ToString()));Passed=false;break;}
        }
        TestTrue(Root+FString::Printf(TEXT(" yaw %d kind %d completes the accessible route"),Turn*90,Kind),Passed);
        GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    }
    return true;
}
#endif
