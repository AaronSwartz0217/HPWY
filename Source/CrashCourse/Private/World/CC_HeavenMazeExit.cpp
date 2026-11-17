#include "World/CC_HeavenMazeExit.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Player/CC_PlayerController.h"
#include "World/CC_HeavenSceneRebuild.h"
#include "WFCMapGenerator.h"

ACC_HeavenMazeExit::ACC_HeavenMazeExit()
{
    auto* Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("ExitTrigger"));
    SetRootComponent(Trigger);
    Trigger->InitBoxExtent(FVector(195, 195, 115));
    Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
    Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    Trigger->SetGenerateOverlapEvents(true);
}

void ACC_HeavenMazeExit::NotifyActorBeginOverlap(AActor* OtherActor)
{
    Super::NotifyActorBeginOverlap(OtherActor);
    const auto* Character = Cast<ACC_PlayerCharacter>(OtherActor);
    if (!HasAuthority() || !Character || !Character->IsAlive())
        return;
    const auto* Generator = CCHeavenScene::FindGenerator(GetWorld());
    if (Generator && Generator->RuntimeRevision == Revision && Generator->DisplayedSettings.LayoutPreset == EWFCLayoutPreset::Maze)
        if (auto* Controller = Cast<ACC_PlayerController>(Character->GetController()))
            Controller->CompleteHeavenMaze(Revision);
}
