#include "Progression/CC_ConfiguredAbility.h"
#include "Progression/CC_GameplayData.h"
#include "Progression/CC_SkillComponent.h"
#include "Characters/CC_BaseCharacter.h"
#include "Characters/CC_LocomotionComponent.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NativeGameplayTags.h"
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_CleansePoison,"CCTags.Debuff.Poison");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_CleanseBurn,"CCTags.Debuff.Burn");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_CleanseSlow,"CCTags.Debuff.Slow");
bool UCC_ConfiguredAbility::ExecuteDesignSkill() {
 auto* C=Cast<ACC_BaseCharacter>(GetAvatarActorFromActorInfo());if(!C || !Definition) return false;
 auto* ASC=C->GetAbilitySystemComponent();const auto M=Definition->DesignMechanic;
 if(M==ECC_DesignMechanic::None) return false;
 switch(M) {
 case ECC_DesignMechanic::HealPercent:
  ApplyEffect(C,C,UCC_HealthEffect::StaticClass(),ASC->GetNumericAttribute(UCC_AttributeSet::GetMaxHealthAttribute())*Definition->HealFraction);break;
 case ECC_DesignMechanic::Summon:C->Skills->SummonSoldiers(Definition);break;
 case ECC_DesignMechanic::StatBoost:case ECC_DesignMechanic::Guard:case ECC_DesignMechanic::Invulnerable:case ECC_DesignMechanic::Stealth:case ECC_DesignMechanic::Lifesteal:
  C->Skills->ActivateDesignStatus(Definition);break;
 case ECC_DesignMechanic::CleanseShield: {
  FGameplayTagContainer Tags;Tags.AddTag(TAG_CleansePoison);Tags.AddTag(TAG_CleanseBurn);Tags.AddTag(TAG_CleanseSlow);
  ASC->RemoveActiveEffects(FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(Tags));
  ApplyEffect(C,C,UCC_ShieldEffect::StaticClass(),ASC->GetNumericAttribute(UCC_AttributeSet::GetMaxHealthAttribute())*Definition->ShieldHealthFraction+ASC->GetNumericAttributeBase(UCC_AttributeSet::GetDefenseAttribute())*Definition->ShieldDefenseFactor,Definition->Duration);break;
 }
 case ECC_DesignMechanic::Charge:
  ChargeTick();return true;
 case ECC_DesignMechanic::Regrowth:case ECC_DesignMechanic::PulseBurst:case ECC_DesignMechanic::Gravity:case ECC_DesignMechanic::RootArea: {
  DesignPulsesTotal=M==ECC_DesignMechanic::Gravity ? FMath::Max(2,FMath::RoundToInt(Definition->Duration/.1f)+1) : M==ECC_DesignMechanic::RootArea ? 1 : FMath::Max(1,Definition->Count);
  PulsesRemaining=DesignPulsesTotal;DesignStarted=GetWorld()->GetTimeSeconds();LastDesignPulseTime=DesignStarted-(M==ECC_DesignMechanic::Gravity?.1f:0.f);
  if(M==ECC_DesignMechanic::Regrowth || M==ECC_DesignMechanic::PulseBurst) {
   auto* T=UAbilityTask_WaitDelay::WaitDelay(this,FMath::Max(.05f,Definition->Interval));T->OnFinish.AddDynamic(this,&ThisClass::DesignPulse);T->ReadyForActivation();
  } else DesignPulse();
  return true;
 }
 default:break;
 }
 C->Locomotion->PresentImpact(Definition,C->GetActorLocation());EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,true,false);return true;
}
void UCC_ConfiguredAbility::ChargeTick() {
 if(!IsActive())return;
 auto* C=Cast<ACC_BaseCharacter>(GetAvatarActorFromActorInfo());
 if(!C || !C->IsAlive() || !Definition) {EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,true,true);return;}
 const float Elapsed=(bChargeReleaseRequested?ChargeReleasedAt:GetWorld()->GetTimeSeconds())-ChargeStarted;
 if(bChargeReleaseRequested || !C->Skills->IsSlotHeld(GetCurrentAbilitySpec()->InputID) || Elapsed>=Definition->MaxChargeSeconds) {
  bCharging=false;
  const float Minimum=FMath::Clamp(Definition->Windup,0.f,Definition->MaxChargeSeconds);
  const float Alpha=FMath::Clamp((Elapsed-Minimum)/FMath::Max(.05f,Definition->MaxChargeSeconds-Minimum),0.f,1.f);
  bFullyCharged=Alpha>=1.f-KINDA_SMALL_NUMBER;Power=AttackAtCast*FMath::Lerp(2.f,4.f,Alpha);RefreshAim();
  StartCooldown(Definition->Cooldown);PulsesRemaining=1;C->Locomotion->PresentSkill(Definition,TargetPoint,ECC_SkillPresentationPhase::ChargeRelease);Pulse();return;
 }
 auto* T=UAbilityTask_WaitDelay::WaitDelay(this,.03f);T->OnFinish.AddDynamic(this,&ThisClass::ChargeTick);T->ReadyForActivation();
}
void UCC_ConfiguredAbility::DesignPulse() {
 if(!IsActive())return;
 auto* C=Cast<ACC_BaseCharacter>(GetAvatarActorFromActorInfo());
 if(!C || !C->IsAlive() || !Definition) {EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,true,true);return;}
 const auto M=Definition->DesignMechanic;const float Now=GetWorld()->GetTimeSeconds();
 // Gravity has only start/end damage; skip stale pull samples to finish on time even during hitches.
 if(M==ECC_DesignMechanic::Gravity){const float Interval=FMath::Max(.001f,Definition->Duration/FMath::Max(1,DesignPulsesTotal-1));const int32 Due=FMath::Clamp(FMath::FloorToInt((Now-DesignStarted+.0001f)/Interval),0,DesignPulsesTotal-1);PulsesRemaining=FMath::Min(PulsesRemaining,DesignPulsesTotal-Due);}
 const int32 Index=DesignPulsesTotal-PulsesRemaining;
 if(M==ECC_DesignMechanic::Regrowth) ApplyEffect(C,C,UCC_HealthEffect::StaticClass(),C->GetAbilitySystemComponent()->GetNumericAttribute(UCC_AttributeSet::GetMaxHealthAttribute())*Definition->HealFraction);
 else {
  const FVector Origin=M==ECC_DesignMechanic::PulseBurst ? C->GetActorLocation() : TargetPoint;
  TArray<FOverlapResult> Hits;FCollisionObjectQueryParams Objects(ECC_Pawn);FCollisionQueryParams Params;TSet<AActor*> Seen;
  GetWorld()->OverlapMultiByObjectType(Hits,Origin,FQuat::Identity,Objects,FCollisionShape::MakeSphere(Definition->Radius),Params);
  FCollisionQueryParams Visibility;Visibility.AddIgnoredActor(C);
  for(const auto& H:Hits)if(auto* Pawn=Cast<APawn>(H.GetActor()))Visibility.AddIgnoredActor(Pawn);
  for(const auto& Hit:Hits) {
   auto* T=Cast<ACC_BaseCharacter>(Hit.GetActor());if(!T || !T->IsAlive() || AreAllies(C,T) || Seen.Contains(T)) continue;Seen.Add(T);
   FHitResult Block;
   if(GetWorld()->LineTraceSingleByChannel(Block,Origin+FVector(0,0,40),T->GetActorLocation(),ECC_Visibility,Visibility) && Block.GetActor()!=T) continue;
   float Damage=0.f;
   if(M==ECC_DesignMechanic::Gravity) {
    if(Index==0) Damage=.4f*AttackAtCast;
    if(PulsesRemaining==1) Damage=.8f*AttackAtCast;
    if(!T->Skills->bControlImmune && !T->Skills->IsInvulnerable()) {
     FVector Delta=(Origin-T->GetActorLocation()).GetSafeNormal2D()*(400.f*FMath::Clamp(Now-LastDesignPulseTime,0.f,.4f));FHitResult Sweep;
     T->SetActorLocation(T->GetActorLocation()+Delta,true,&Sweep);
     if(PulsesRemaining==1) {T->Skills->ApplyCrowdControl(Definition->ControlDuration,false);T->LaunchCharacter(FVector(0,0,450),false,true);}
    }
   } else {
    Damage=Definition->CalculatePower(AttackAtCast,GetAbilityLevel());
    if(M==ECC_DesignMechanic::RootArea) T->Skills->ApplyCrowdControl(Definition->ControlDuration,true);
   }
   if(Damage>0.f) ApplyEffect(C,T,UCC_DamageEffect::StaticClass(),Damage,0.f,M==ECC_DesignMechanic::RootArea);
   if(Damage>0.f) T->Locomotion->PresentImpact(Definition,T->GetActorLocation());
  }
  C->Locomotion->PresentImpact(Definition,Origin);
 }
 LastDesignPulseTime=Now;
 if(--PulsesRemaining<=0) {EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,true,false);return;}
 const float Interval=M==ECC_DesignMechanic::Gravity?Definition->Duration/FMath::Max(1,DesignPulsesTotal-1):FMath::Max(.05f,Definition->Interval);
 const int32 NextIndex=DesignPulsesTotal-PulsesRemaining+(M==ECC_DesignMechanic::Gravity?0:1);
 const float Delay=FMath::Max(.001f,DesignStarted+Interval*NextIndex-GetWorld()->GetTimeSeconds());
 auto* T=UAbilityTask_WaitDelay::WaitDelay(this,Delay);T->OnFinish.AddDynamic(this,&ThisClass::DesignPulse);T->ReadyForActivation();
}

void UCC_ConfiguredAbility::ReleaseChargeInput()
{
 if(IsActive()&&bCharging&&!bChargeReleaseRequested){bChargeReleaseRequested=true;ChargeReleasedAt=GetWorld()->GetTimeSeconds();}
}
