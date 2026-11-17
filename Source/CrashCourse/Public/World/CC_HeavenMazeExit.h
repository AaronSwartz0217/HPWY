#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CC_HeavenMazeExit.generated.h"

/** Authority-only overlap trigger for the current generated maze. */
UCLASS(NotPlaceable)
class ACC_HeavenMazeExit : public AActor
{
    GENERATED_BODY()

public:
    ACC_HeavenMazeExit();
    void NotifyActorBeginOverlap(AActor* OtherActor) override;
    int32 Revision = 0;
};
