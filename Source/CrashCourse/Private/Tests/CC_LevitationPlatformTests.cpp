#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Characters/CC_PlayerCharacter.h"
#include "World/CC_LevitationPlatform.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/CharacterMovementComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCC_CarriedPlatformTest,"CrashCourse.World.CarriedPlatformAndReassembly",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCC_CarriedPlatformTest::RunTest(const FString&)
{
    UWorld::InitializationValues V;V.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    auto* W=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&V);GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(W);
    auto* Ground=W->SpawnActor<AActor>();auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
    auto* Floor=NewObject<UStaticMeshComponent>(Ground);Ground->AddInstanceComponent(Floor);Ground->SetRootComponent(Floor);Floor->SetCanEverAffectNavigation(false);Floor->SetStaticMesh(Cube);Floor->SetCollisionProfileName(TEXT("BlockAll"));Floor->SetWorldScale3D(FVector(40,40,1));Floor->SetWorldLocation(FVector(0,0,-50));Floor->RegisterComponent();
    auto* P=W->SpawnActor<ACC_LevitationPlatform>();auto* C=W->SpawnActor<ACC_PlayerCharacter>();auto* M=C->GetCharacterMovement();
    W->GetWorldSettings()->NotifyBeginPlay();for(int32 Frame=0;Frame<120;++Frame)W->Tick(LEVELTICK_All,1.f/60);M->bRunPhysicsWithNoController=true;M->SetMovementMode(MOVE_Walking);
    TestTrue(TEXT("Test world has passed the initial editor falling grace period"),W->HasBegunPlay()&&W->GetTimeSeconds()>=1.f);
    C->SetActorLocation(FVector(0,0,98),false,nullptr,ETeleportType::TeleportPhysics);
    for(int32 I=0;I<15;++I)M->TickComponent(1.f/60,LEVELTICK_All,nullptr);
    TestTrue(TEXT("Standing character uses the physical deck as its movement base"),P->IsCarrying(C));
    for(int32 I=0;I<120;++I){P->Tick(1.f/60);M->TickComponent(1.f/60,LEVELTICK_All,nullptr);}
    TestTrue(TEXT("Occupied platform rises"),P->CurrentLift>300);
    TestTrue(FString::Printf(TEXT("Actual character is carried with the deck (Z=%.1f, lift=%.1f)"),C->GetActorLocation().Z,P->CurrentLift),FMath::Abs(C->GetActorLocation().Z-P->CurrentLift-98)<10);
    TestEqual(TEXT("Character stays grounded, never free-flying"),M->MovementMode.GetValue(),MOVE_Walking);
    const int32 Seed=P->LayoutSeed;FTransform Before;P->Tiles->GetInstanceTransform(0,Before,false);
    TestFalse(TEXT("No scene generator means no fake stone-only rebuild"),P->Reassemble(C));TestEqual(TEXT("Missing scene generator preserves seed"),P->LayoutSeed,Seed);
    TestFalse(TEXT("Missing scene generator keeps rebuild unavailable"),P->Reassemble(C));
    for(int32 I=0;I<120;++I){P->Tick(1.f/60);M->TickComponent(1.f/60,LEVELTICK_All,nullptr);}
    FTransform After;P->Tiles->GetInstanceTransform(0,After,false);
    TestTrue(TEXT("Platform decoration remains fixed"),Before.GetLocation().Equals(After.GetLocation(),1));
    TestTrue(TEXT("Reassembly retains its passenger"),P->IsCarrying(C));TestEqual(TEXT("Nine stones retained"),P->Tiles->GetInstanceCount(),9);
    TestEqual(TEXT("Lift stops at configured height"),P->CurrentLift,P->LiftHeight);
    const float BeforeDrop=C->GetActorLocation().Z;
    for(int32 I=0;I<90;++I){M->AddInputVector(FVector::ForwardVector);M->TickComponent(1.f/60,LEVELTICK_All,nullptr);}
    AddInfo(FString::Printf(TEXT("Walk off: pos=%s velocity=%s acceleration=%s speed=%.1f deckextent=%s deckscale=%s"),*C->GetActorLocation().ToString(),*M->Velocity.ToString(),*M->GetCurrentAcceleration().ToString(),M->MaxWalkSpeed,*P->Deck->Bounds.BoxExtent.ToString(),*P->Deck->GetComponentScale().ToString()));
    TestFalse(TEXT("Outside character is no longer a passenger"),P->IsCarrying(C));TestTrue(TEXT("Gravity physically drops the character after leaving"),C->GetActorLocation().Z<BeforeDrop-50);
    for(int32 I=0;I<360;++I)P->Tick(1.f/60);
    TestEqual(TEXT("Empty platform returns for the next visitor"),P->CurrentLift,0.f);
    W->Tick(LEVELTICK_All,3.f);
    C->SetActorLocation(FVector(2500,2500,100),false,nullptr,ETeleportType::TeleportPhysics);
    const int32 RemoteSeed=P->LayoutSeed;TestFalse(TEXT("Remote interaction is rejected"),P->Reassemble(C));TestEqual(TEXT("Rejected interaction preserves layout"),P->LayoutSeed,RemoteSeed);
    C->SetAlive(false);M->DisableMovement();P->Tick(1.f/60);
    TestEqual(TEXT("Platform never re-enables dead character movement"),M->MovementMode.GetValue(),MOVE_None);
    GEngine->DestroyWorldContext(W);W->DestroyWorld(false);return true;
}
#endif
