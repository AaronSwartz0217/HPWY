#include "Characters/CC_CharacterAnimInstance.h"
#include "Characters/CC_BaseCharacter.h"
#include "Characters/CC_LocomotionComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

void UCC_CharacterAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();
    Speed=0.f;RunPlayRate=1.f;LandingTime=0.f;bWasFalling=false;
    bIdle=true;bRun=bJump=bFall=bLand=bDead=false;
}
void UCC_CharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);
    const auto* C=Cast<ACC_BaseCharacter>(TryGetPawnOwner());
    if (!C || !C->Locomotion) return;
    const FCC_MeshLocomotion* Entry=C->Locomotion->GetCurrentEntry();
    UpdateMovementState(DeltaSeconds,C->GetVelocity().Size2D(),C->GetVelocity().Z,
        C->GetCharacterMovement()->IsFalling(),C->IsAlive(),Entry ? Entry->ReferenceSpeed : 400.f);
    if (bDead && IsAnyMontagePlaying()) Montage_Stop(.08f);
}
void UCC_CharacterAnimInstance::UpdateMovementState(float DeltaSeconds,float RawSpeed,float VerticalSpeed,bool bFalling,bool bAlive,float ReferenceSpeed)
{
    const float Dt=FMath::Max(0.f,DeltaSeconds);
    RawSpeed=FMath::Max(0.f,RawSpeed);
    Speed=FMath::FInterpTo(Speed,RawSpeed,Dt,SpeedInterpolation);
    const float TargetRate=RawSpeed>4.f ? FMath::Clamp(RawSpeed/FMath::Max(1.f,ReferenceSpeed),.5f,2.f) : 1.f;
    RunPlayRate=FMath::FInterpTo(RunPlayRate,TargetRate,Dt,PlayRateInterpolation);
    if (bWasFalling && !bFalling && bAlive) LandingTime=LandingHoldSeconds;
    bWasFalling=bFalling;
    bDead=!bAlive;
    bJump=!bDead && bFalling && VerticalSpeed>0.f;
    bFall=!bDead && bFalling && !bJump;
    bLand=!bDead && !bFalling && LandingTime>0.f;
    // Separate entry/exit thresholds prevent tiny velocity changes from chattering states.
    const bool bMoving=bRun ? RawSpeed>4.f : RawSpeed>12.f;
    bRun=!bDead && !bFalling && !bLand && bMoving;
    bIdle=!bDead && !bFalling && !bLand && !bRun;
    LandingTime=FMath::Max(0.f,LandingTime-Dt);
}
