#pragma once

#include "Components/PoseableMeshComponent.h"
#include "CC_DodgeAfterimageComponent.generated.h"

UCLASS()
class UCC_DodgeAfterimageComponent : public UPoseableMeshComponent
{
 GENERATED_BODY()
protected:
 // This effect can expire before a delayed render proxy is created.
 virtual bool UsePSOPrecacheRenderProxyDelay() const override { return false; }
};
