#pragma once
#include "CoreMinimal.h"
class AWFCMapGenerator;
namespace CCHeavenScene {
 bool IsHeaven(UWorld* World);
 AWFCMapGenerator* FindGenerator(UWorld* World);
 bool Enter(UWorld* World);
 bool Rebuild(UWorld* World, bool bToggleMaze = false);
}
