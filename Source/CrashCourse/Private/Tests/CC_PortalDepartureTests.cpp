#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "World/CC_MapPortal.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_CallFunction.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCC_PortalDepartureTimingTest,"CrashCourse.Travel.PortalDepartureTiming",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCC_PortalDepartureTimingTest::RunTest(const FString& Parameters)
{
    UWorld::InitializationValues Values;
    Values.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,TEXT("PortalDepartureTest"),nullptr,true,ERHIFeatureLevel::Num,&Values);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    World->InitializeActorsForPlay(FURL());
    auto* Portal=World->SpawnActor<ACC_MapPortal>();
    auto* Character=World->SpawnActor<ACC_PlayerCharacter>();
    Portal->DispatchBeginPlay();
    Portal->TravelDelay=1.f;
    Portal->StartDeparture(Character);
    TestTrue(TEXT("Touch begins a visible waiting phase instead of immediate completion"),Portal->IsDeparturePending());
    TestTrue(TEXT("Departure timer is active"),World->GetTimerManager().IsTimerActive(Portal->DepartureTimer));
    auto Advance=[World](float Seconds){++GFrameCounter;World->GetTimerManager().Tick(Seconds);};
    Advance(0.f); // Activate timers queued before the test world's first tick.
    Advance(.3f);
    TestTrue(TEXT("Still waiting before configured delay"),Portal->IsDeparturePending());
    const float Remaining=World->GetTimerManager().GetTimerRemaining(Portal->DepartureTimer);
    Portal->StartDeparture(Character);
    TestEqual(TEXT("Repeated overlap does not restart countdown"),World->GetTimerManager().GetTimerRemaining(Portal->DepartureTimer),Remaining);
    Advance(.8f);
    TestFalse(TEXT("Timer finishes after configured delay even if travel validation cannot succeed in this isolated world"),Portal->IsDeparturePending());
    Portal->TravelDelay=0.f;Portal->StartDeparture(Character);
    TestFalse(TEXT("Zero-delay custom portals retain immediate behavior"),Portal->IsDeparturePending());
    Portal->TravelDelay=1.f;Portal->StartDeparture(Character);
    const FTimerHandle Handle=Portal->DepartureTimer;
    Portal->Destroy();
    TestFalse(TEXT("Destroyed portal clears pending departure timer"),World->GetTimerManager().IsTimerActive(Handle));
    GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCC_LegacyPortalPresentationTest,"CrashCourse.Travel.LegacyPortalPresentation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCC_LegacyPortalPresentationTest::RunTest(const FString& Parameters)
{
    auto* Blueprint=LoadObject<UBlueprint>(nullptr,TEXT("/Game/LanTuRebuild/Blueprints/运河传送门/BP_传送门判断.BP_传送门判断"));
    if (!TestNotNull(TEXT("Original portal blueprint exists"),Blueprint)) return false;
    bool FoundDelay=false;
    for (UEdGraph* Graph:Blueprint->UbergraphPages)
        for (UEdGraphNode* Node:Graph->Nodes)
            if (auto* Call=Cast<UK2Node_CallFunction>(Node))
                if (Call->FunctionReference.GetMemberName()==TEXT("Delay"))
                    if (const UEdGraphPin* Pin=Call->FindPin(TEXT("Duration")))
                    {
                        FoundDelay=true;
                        AddInfo(FString::Printf(TEXT("Legacy departure Delay: %s"),*Pin->DefaultValue));
                        TestEqual(TEXT("Adapter matches the original blueprint's departure duration"),FCString::Atof(*Pin->DefaultValue),8.f);
                    }
    TestTrue(TEXT("Legacy delay node was inspected"),FoundDelay);
    TestNotNull(TEXT("Original Niagara departure system exists"),LoadObject<UNiagaraSystem>(nullptr,TEXT("/Game/LanTuRebuild/Effects/运河传送门/粒子传送门.粒子传送门")));
    return true;
}
#endif
