#include "Progression/CC_SkillComponent.h"
#include "Progression/CC_ConfiguredAbility.h"
#include "Progression/CC_SummonedSoldier.h"
#include "Characters/CC_BaseCharacter.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "AIController.h"
#include "TimerManager.h"
void UCC_SkillComponent::ReleaseSlot(int32 Slot) {
 if(Slot<0 || Slot>3) return;
 if(!GetOwner()->HasAuthority()) { ServerReleaseSlot(Slot); return; }
 HeldSlots.Remove(Slot);
 auto* C=Cast<ACC_BaseCharacter>(GetOwner());auto* ASC=C?C->GetAbilitySystemComponent():nullptr;
 auto* Spec=ASC?ASC->FindAbilitySpecFromHandle(FindSkillHandle(Slot)):nullptr;
 if(Spec)for(auto* Instance:Spec->GetAbilityInstances())if(auto* A=Cast<UCC_ConfiguredAbility>(Instance))A->ReleaseChargeInput();
 if(QueuedSlot==Slot&&QueuedDefinition.IsValid()&&QueuedDefinition->DesignMechanic==ECC_DesignMechanic::Charge)ClearQueuedSkill();
}
void UCC_SkillComponent::ServerReleaseSlot_Implementation(int32 Slot) {ReleaseSlot(Slot);}
bool UCC_SkillComponent::IsInvulnerable() const {return GetWorld() && GetWorld()->GetTimeSeconds()<InvulnerableUntil;}
bool UCC_SkillComponent::IsStunned() const {return !IsInvulnerable() && GetWorld() && GetWorld()->GetTimeSeconds()<StunUntil;}
bool UCC_SkillComponent::IsRooted() const {return IsStunned() || (!IsInvulnerable() && GetWorld() && GetWorld()->GetTimeSeconds()<RootUntil);}
float UCC_SkillComponent::GuardDefense(const AActor* Source,float Defense) const {
 const auto* C=Cast<ACC_BaseCharacter>(GetOwner());
 if(!Source || !C || GetWorld()->GetTimeSeconds()>=GuardUntil) return Defense;
 const FVector D=(Source->GetActorLocation()-C->GetActorLocation()).GetSafeNormal2D();
 if(FVector::DotProduct(C->GetActorForwardVector(),D)<FMath::Cos(FMath::DegreesToRadians(GuardArc*.5f))) return Defense;
 auto* ASC=C->GetAbilitySystemComponent();
 return ASC ? FMath::Max(Defense,ASC->GetNumericAttributeBase(UCC_AttributeSet::GetDefenseAttribute())*GuardFactor) : Defense;
}
bool UCC_SkillComponent::BlocksKnockback(const AActor* Source) const {
 if(IsInvulnerable() || bControlImmune) return true;
 const auto* C=Cast<ACC_BaseCharacter>(GetOwner());
 return C && Source && GetWorld()->GetTimeSeconds()<GuardUntil && FVector::DotProduct(C->GetActorForwardVector(),(Source->GetActorLocation()-C->GetActorLocation()).GetSafeNormal2D())>=FMath::Cos(FMath::DegreesToRadians(GuardArc*.5f));
}
void UCC_SkillComponent::ApplyCrowdControl(float Seconds,bool RootOnly) {
 if(!GetOwner()->HasAuthority() || bControlImmune || IsInvulnerable() || Seconds<=0.f) return;
 const float Until=GetWorld()->GetTimeSeconds()+Seconds;
 if(RootOnly) RootUntil=FMath::Max(RootUntil,Until);
 else { StunUntil=FMath::Max(StunUntil,Until); CancelActiveSkills(); }
 if(auto* C=Cast<ACC_BaseCharacter>(GetOwner())) {
  C->GetCharacterMovement()->Velocity.X=0.f; C->GetCharacterMovement()->Velocity.Y=0.f;
  if(auto* AI=Cast<AAIController>(C->GetController())) AI->StopMovement();
 }
}
void UCC_SkillComponent::TickComponent(float Delta,ELevelTick TickType,FActorComponentTickFunction* Fn) {
 Super::TickComponent(Delta,TickType,Fn);
 if(!GetOwner()->HasAuthority()) return;
 auto* C=Cast<ACC_BaseCharacter>(GetOwner()); if(!C) return;
 if(!C->IsAlive()) {ClearDesignStates(); return;}
 TickCooldownHaste(Delta);
 if(IsRooted()) {C->GetCharacterMovement()->Velocity.X=0.f; C->GetCharacterMovement()->Velocity.Y=0.f;}
 DrainQueuedSkill();
}
void UCC_SkillComponent::OnRep_Stealth() {if(auto* C=Cast<ACC_BaseCharacter>(GetOwner())) C->GetMesh()->SetVisibility(!bStealthed,true);}
void UCC_SkillComponent::BreakStealth() {
 if(!GetOwner()->HasAuthority() || !bStealthed) return;
 bStealthed=false; ++StealthGeneration; OnRep_Stealth();
 if(auto* C=Cast<ACC_BaseCharacter>(GetOwner())) if(auto* ASC=C->GetAbilitySystemComponent())
  if(auto* H=DesignEffects.Find(TEXT("StealthSpeed"))) {ASC->RemoveActiveGameplayEffect(*H);DesignEffects.Remove(TEXT("StealthSpeed"));}
 GetOwner()->ForceNetUpdate();
}
void UCC_SkillComponent::ActivateDesignStatus(UCC_SkillDefinition* S) {
 auto* C=Cast<ACC_BaseCharacter>(GetOwner()); auto* ASC=C ? C->GetAbilitySystemComponent() : nullptr;
 if(!S || !ASC || !GetOwner()->HasAuthority()) return;
 const float Now=GetWorld()->GetTimeSeconds();
 auto Buff=[&](FName Key,TSubclassOf<UGameplayEffect> Effect,float Value) {
  if(auto* Old=DesignEffects.Find(Key)) ASC->RemoveActiveGameplayEffect(*Old);
  auto Spec=ASC->MakeOutgoingSpec(Effect,1,ASC->MakeEffectContext()); if(!Spec.IsValid()) return;
  Spec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(TEXT("CCTags.Skill.Value")),Value);
  Spec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(TEXT("CCTags.Skill.Duration")),FMath::Max(.05f,S->Duration));
  DesignEffects.Add(Key,ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data));
 };
 switch(S->DesignMechanic) {
 case ECC_DesignMechanic::StatBoost:
  Buff(TEXT("SchoolAttack"),UCC_AttackEffect::StaticClass(),ASC->GetNumericAttributeBase(UCC_AttributeSet::GetAttackAttribute())*(S->AttackFactor-1.f));
  Buff(TEXT("SchoolDefense"),UCC_DefenseEffect::StaticClass(),ASC->GetNumericAttributeBase(UCC_AttributeSet::GetDefenseAttribute())*(S->DefenseFactor-1.f)); break;
 case ECC_DesignMechanic::Guard: GuardUntil=Now+S->Duration;GuardFactor=S->DefenseFactor;GuardArc=S->Angle;break;
 case ECC_DesignMechanic::Invulnerable: InvulnerableUntil=Now+S->Duration;break;
 case ECC_DesignMechanic::Stealth: {
  bStealthed=true;OnRep_Stealth();GetOwner()->ForceNetUpdate();
  Buff(TEXT("StealthSpeed"),UCC_SpeedEffect::StaticClass(),ASC->GetNumericAttributeBase(UCC_AttributeSet::GetMoveSpeedAttribute())*(S->SpeedFactor-1.f));
  const uint32 Generation=++StealthGeneration;TWeakObjectPtr<UCC_SkillComponent> Weak=this;FTimerHandle Timer;
  GetWorld()->GetTimerManager().SetTimer(Timer,[Weak,Generation](){if(Weak.IsValid() && Weak->StealthGeneration==Generation) Weak->BreakStealth();},FMath::Max(.05f,S->Duration),false);break;
 }
 case ECC_DesignMechanic::Lifesteal: LifestealUntil=Now+S->Duration;LifestealFraction=S->HealFraction;LifestealWindowStart=-1.f;LifestealHealed=0.f;break;
 default:break;
 }
}
void UCC_SkillComponent::OnDamageDealt(float HealthDamage,bool Direct) {
 auto* C=Cast<ACC_BaseCharacter>(GetOwner());
 if(!GetOwner()->HasAuthority() || !C || !C->IsAlive() || !Direct || HealthDamage<=0.f || GetWorld()->GetTimeSeconds()>=LifestealUntil) return;
 const float Now=GetWorld()->GetTimeSeconds();if(Now-LifestealWindowStart>=1.f) {LifestealWindowStart=Now;LifestealHealed=0.f;}
 auto* ASC=C->GetAbilitySystemComponent();
 const float MaxHealth=ASC->GetNumericAttribute(UCC_AttributeSet::GetMaxHealthAttribute());
 const float Heal=FMath::Min3(HealthDamage*LifestealFraction,FMath::Max(0.f,MaxHealth*.08f-LifestealHealed),FMath::Max(0.f,MaxHealth-ASC->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute())));
 LifestealHealed+=Heal;UCC_ConfiguredAbility::ApplyEffect(C,C,UCC_HealthEffect::StaticClass(),Heal);
}
void UCC_SkillComponent::ClearDesignStates() {
 if(!GetOwner()->HasAuthority()) return;
 ClearCooldownHaste();
 if(auto* C=Cast<ACC_BaseCharacter>(GetOwner())) if(auto* ASC=C->GetAbilitySystemComponent()) for(const auto& It:DesignEffects) ASC->RemoveActiveGameplayEffect(It.Value);
 CombatFocus.Reset();CombatFocusUntil=0;
 DesignEffects.Empty();StunUntil=RootUntil=InvulnerableUntil=GuardUntil=LifestealUntil=0.f;
 BreakStealth();HeldSlots.Empty();ClearQueuedSkill();RequestedTarget.Reset();
 for(auto& Soldier:SummonedSoldiers) if(Soldier.IsValid()) Soldier->Destroy();SummonedSoldiers.Empty();
}
void UCC_SkillComponent::SummonSoldiers(UCC_SkillDefinition* S) {
 auto* C=Cast<ACC_BaseCharacter>(GetOwner());if(!C || !C->HasAuthority() || !S) return;
 for(auto& Soldier:SummonedSoldiers) if(Soldier.IsValid()) Soldier->Destroy();SummonedSoldiers.Empty();
 const TSubclassOf<ACC_SummonedSoldier> SoldierClass=SummonedSoldierClass ? SummonedSoldierClass.Get() : ACC_SummonedSoldier::StaticClass();
 for(int32 I=0;I<FMath::Clamp(S->Count,1,3);++I) {
  FTransform Transform;bool Found=false;
  // Try the formation, then alternatives; never force-spawn inside geometry.
  for(int32 Attempt=0;Attempt<9 && !Found;++Attempt) {
   const float Angle=Attempt==0 ? (I-1)*40.f : I*120.f+Attempt*45.f;
   const float Radius=Attempt<5 ? 180.f : 280.f;
   const FVector Offset=C->GetActorForwardVector().RotateAngleAxis(Angle,FVector::UpVector)*Radius;
   Found=ACC_SummonedSoldier::FindGroundedSpawn(C,C->GetActorLocation()+Offset,Transform,SoldierClass);
  }
  if(!Found) continue;
  FActorSpawnParameters Params;Params.Owner=C;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
  auto* Soldier=GetWorld()->SpawnActor<ACC_SummonedSoldier>(SoldierClass,Transform,Params);
  if(Soldier) {Soldier->InitializeSummon(C,S);SummonedSoldiers.Add(Soldier);}
 }
}

void UCC_SkillComponent::RememberCombatTarget(AActor* Target,bool bOverride)
{
 auto* C=Cast<ACC_BaseCharacter>(GetOwner());auto* Enemy=Cast<ACC_BaseCharacter>(Target);
 if(!C||!C->HasAuthority()||!Enemy||!Enemy->IsAlive()||UCC_ConfiguredAbility::AreAllies(C,Enemy)||(!bOverride&&GetCombatFocus()))return;
 CombatFocus=Enemy;CombatFocusUntil=GetWorld()->GetTimeSeconds()+6.f;
}
ACC_BaseCharacter* UCC_SkillComponent::GetCombatFocus() const
{
 return CombatFocus.IsValid()&&CombatFocus->IsAlive()&&GetWorld()->GetTimeSeconds()<CombatFocusUntil?CombatFocus.Get():nullptr;
}
