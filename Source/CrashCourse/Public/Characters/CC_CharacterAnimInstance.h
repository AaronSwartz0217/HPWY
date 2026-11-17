#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "CC_CharacterAnimInstance.generated.h"

// Shared values consumed by generated, editable locomotion state machines.
UCLASS(Blueprintable, BlueprintType)
class CRASHCOURSE_API UCC_CharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
    virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
    UPROPERTY(EditDefaultsOnly,Category="Movement|Smoothing",meta=(ClampMin="0")) float SpeedInterpolation=10.f;
    UPROPERTY(EditDefaultsOnly,Category="Movement|Smoothing",meta=(ClampMin="0")) float PlayRateInterpolation=8.f;
    UPROPERTY(EditDefaultsOnly,Category="Movement|Smoothing",meta=(ClampMin="0")) float LandingHoldSeconds=.24f;
	UPROPERTY(BlueprintReadOnly, Category="Movement") float Speed = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Movement") float RunPlayRate = 1.f;
	UPROPERTY(BlueprintReadOnly, Category="Movement") bool bIdle = true;
	UPROPERTY(BlueprintReadOnly, Category="Movement") bool bRun = false;
	UPROPERTY(BlueprintReadOnly, Category="Movement") bool bJump = false;
	UPROPERTY(BlueprintReadOnly, Category="Movement") bool bFall = false;
	UPROPERTY(BlueprintReadOnly, Category="Movement") bool bLand = false;
	UPROPERTY(BlueprintReadOnly, Category="Movement") bool bDead = false;
private:
    friend class FCC_AnimationContinuityTest;
    void UpdateMovementState(float DeltaSeconds,float RawSpeed,float VerticalSpeed,bool bFalling,bool bAlive,float ReferenceSpeed);
	bool bWasFalling = false;
	float LandingTime = 0.f;
};
