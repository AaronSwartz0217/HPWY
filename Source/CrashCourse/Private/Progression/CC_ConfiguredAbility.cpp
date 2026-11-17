#include "Progression/CC_ConfiguredAbility.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "GameFramework/PlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Progression/CC_ConfiguredAbility.h"
#include "Progression/CC_GameplayData.h"
#include "Progression/CC_SkillComponent.h"
#include "Progression/CC_SkillProjectile.h"
#include "Characters/CC_BaseCharacter.h"
#include "Characters/CC_LocomotionComponent.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "GameplayTags/CCTags.h"
#include "NativeGameplayTags.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Player/CC_PlayerState.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/CharacterMovementComponent.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_IndirectDamage, "CCTags.Skill.IndirectDamage");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SkillValue, "CCTags.Skill.Value");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SkillDuration, "CCTags.Skill.Duration");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Cooldown1, "CCTags.Cooldown.Slot1");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Cooldown2, "CCTags.Cooldown.Slot2");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Cooldown3, "CCTags.Cooldown.Slot3");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Cooldown4, "CCTags.Cooldown.Slot4");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_BasicCooldown, "CCTags.Cooldown.BasicAttack");

static void ConfigureEffect(UGameplayEffect* Effect, FGameplayAttribute Attribute, bool bTimed)
{
	Effect->DurationPolicy = bTimed ? EGameplayEffectDurationType::HasDuration : EGameplayEffectDurationType::Instant;
	FSetByCallerFloat Value; Value.DataTag = TAG_SkillValue;
	FGameplayModifierInfo Modifier; Modifier.Attribute = Attribute;
	Modifier.ModifierOp = EGameplayModOp::Additive; Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(Value);
	Effect->Modifiers.Add(Modifier);
	if (bTimed) { FSetByCallerFloat Duration; Duration.DataTag = TAG_SkillDuration; Effect->DurationMagnitude = FGameplayEffectModifierMagnitude(Duration); }
}
UCC_DamageEffect::UCC_DamageEffect() { ConfigureEffect(this, UCC_AttributeSet::GetIncomingDamageAttribute(), false); }
UCC_InitialAttributesEffect::UCC_InitialAttributesEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	const FGameplayAttribute Attributes[] = { UCC_AttributeSet::GetMaxHealthAttribute(), UCC_AttributeSet::GetMaxManaAttribute(), UCC_AttributeSet::GetHealthAttribute(), UCC_AttributeSet::GetManaAttribute(), UCC_AttributeSet::GetAttackAttribute(), UCC_AttributeSet::GetDefenseAttribute(), UCC_AttributeSet::GetShieldAttribute(), UCC_AttributeSet::GetMoveSpeedAttribute() };
	const float Values[] = { 100.f, 100.f, 100.f, 100.f, 15.f, 0.f, 0.f, 500.f };
	for (int32 I = 0; I < UE_ARRAY_COUNT(Attributes); ++I)
	{
		FGameplayModifierInfo Modifier;
		Modifier.Attribute = Attributes[I]; Modifier.ModifierOp = EGameplayModOp::Override;
		Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(Values[I]);
		Modifiers.Add(Modifier);
	}
}
UCC_HealthEffect::UCC_HealthEffect() { ConfigureEffect(this, UCC_AttributeSet::GetHealthAttribute(), false); }
UCC_ManaEffect::UCC_ManaEffect() { ConfigureEffect(this, UCC_AttributeSet::GetManaAttribute(), false); }
UCC_ShieldEffect::UCC_ShieldEffect() { ConfigureEffect(this, UCC_AttributeSet::GetShieldAttribute(), false); Modifiers[0].ModifierOp = EGameplayModOp::Override; }
UCC_DefenseEffect::UCC_DefenseEffect() { ConfigureEffect(this, UCC_AttributeSet::GetDefenseAttribute(), true); }
UCC_AttackEffect::UCC_AttackEffect() { ConfigureEffect(this, UCC_AttributeSet::GetAttackAttribute(), true); }
UCC_SpeedEffect::UCC_SpeedEffect() { ConfigureEffect(this, UCC_AttributeSet::GetMoveSpeedAttribute(), true); }
UCC_CooldownEffect::UCC_CooldownEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	FSetByCallerFloat Duration; Duration.DataTag = TAG_SkillDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(Duration);
}
UCC_ConfiguredAbility::UCC_ConfiguredAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}
FGameplayTag UCC_ConfiguredAbility::GetSlotCooldownTag(int32 Slot)
{
	switch (Slot) { case 1: return TAG_Cooldown2; case 2: return TAG_Cooldown3; case 3: return TAG_Cooldown4; case 4: return TAG_BasicCooldown; default: return TAG_Cooldown1; }
}
bool UCC_ConfiguredAbility::AreAllies(const AActor* A, const AActor* B)
{
	return A && B && A->ActorHasTag(CrashTags::Player) == B->ActorHasTag(CrashTags::Player);
}
void UCC_ConfiguredAbility::ApplyEffect(AActor* Source, AActor* Target, TSubclassOf<UGameplayEffect> Effect, float Value, float Duration, bool bDirectDamage)
{
	if (!IsValid(Source) || !Source->HasAuthority() || !IsValid(Target) || !Effect) return;
	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Source);
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!SourceASC || !TargetASC) return;
	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext(); Context.AddSourceObject(Source);
	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(Effect, 1.f, Context);
	if (!Spec.IsValid()) return;
 if (!bDirectDamage) Spec.Data->AddDynamicAssetTag(TAG_IndirectDamage);
 if(Effect==UCC_SpeedEffect::StaticClass() && Value<0.f) Spec.Data->DynamicGrantedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("CCTags.Debuff.Slow")));
	Spec.Data->SetSetByCallerMagnitude(TAG_SkillValue, Value);
	Spec.Data->SetSetByCallerMagnitude(TAG_SkillDuration, FMath::Max(0.05f, Duration));
	SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
 if(Effect==UCC_DamageEffect::StaticClass()&&Value>0&&!AreAllies(Source,Target))
 {
  if(bDirectDamage)if(auto* C=Cast<ACC_BaseCharacter>(Source))C->Skills->RememberCombatTarget(Target);
  if(auto* C=Cast<ACC_BaseCharacter>(Target))C->Skills->RememberCombatTarget(Source,false);
 }
	if (Effect == UCC_ShieldEffect::StaticClass())
	{
		if (ACC_BaseCharacter* Character = Cast<ACC_BaseCharacter>(Target))
		{
			const uint32 Generation = ++Character->Skills->ShieldGeneration;
			TWeakObjectPtr<ACC_BaseCharacter> WeakCharacter = Character;
			FTimerHandle Timer;
			Character->GetWorldTimerManager().SetTimer(Timer, [WeakCharacter, Generation]()
			{
				if (WeakCharacter.IsValid() && WeakCharacter->Skills->ShieldGeneration == Generation)
					if (UAbilitySystemComponent* ASC = WeakCharacter->GetAbilitySystemComponent()) ASC->SetNumericAttributeBase(UCC_AttributeSet::GetShieldAttribute(), 0.f);
			}, FMath::Max(0.05f, Duration), false);
		}
	}
}
bool UCC_ConfiguredAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags) || !ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid()) return false;
	const ACC_BaseCharacter* Character = Cast<ACC_BaseCharacter>(ActorInfo->AvatarActor.Get());
	const FGameplayAbilitySpec* Spec = ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle);
	const UCC_SkillDefinition* Skill = Spec ? Cast<UCC_SkillDefinition>(Spec->SourceObject.Get()) : nullptr;
	if (const auto* PS=Character ? Character->GetPlayerState<ACC_PlayerState>() : nullptr; PS && !PS->Progression->IsSkillUnlocked(Skill)) return false;
 return Character && Character->IsAlive() && (!Character->Skills->IsStunned()||(Skill&&Skill->DesignMechanic==ECC_DesignMechanic::Invulnerable)) && !Character->Skills->HasBlockingSkillAction() && Skill && !Spec->IsActive() && !(Skill->Behavior==ECC_SkillBehavior::Dash&&Character->Skills->IsRooted()) && FMath::IsFinite(Skill->ManaCost) && Skill->ManaCost >= 0.f && Spec->InputID >= 0 && Spec->InputID <= 4 && !ActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(GetSlotCooldownTag(Spec->InputID)) && ActorInfo->AbilitySystemComponent->GetNumericAttribute(UCC_AttributeSet::GetManaAttribute()) >= Skill->ManaCost;
}
void UCC_ConfiguredAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	Definition = Cast<UCC_SkillDefinition>(GetCurrentSourceObject());
	ACC_BaseCharacter* Character = Cast<ACC_BaseCharacter>(GetAvatarActorFromActorInfo());
	if (!Definition || !Character || !Character->HasAuthority()) { EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return; }
	bBlocksActionInput=true;
 bCharging=Definition->DesignMechanic==ECC_DesignMechanic::Charge;
 if(bCharging){ChargeStarted=GetWorld()->GetTimeSeconds();bChargeReleaseRequested=false;}
 Character->Skills->LastExecutedSkill=Definition;Character->Skills->LastExecutedSlot=GetCurrentAbilitySpec()->InputID;
 AttackAtCast=Character->GetAbilitySystemComponent()->GetNumericAttribute(UCC_AttributeSet::GetAttackAttribute());
 Power = Definition->CalculatePower(AttackAtCast, GetAbilityLevel());
 const bool bDamage=Definition->PowerMode==ECC_SkillPowerMode::AttackMultiplier || !Definition->DamageSteps.IsEmpty();
 if(bDamage) Character->Skills->BreakStealth();
	ApplyEffect(Character, Character, UCC_ManaEffect::StaticClass(), -Definition->ManaCost);
 if(Definition->DesignMechanic!=ECC_DesignMechanic::Charge) StartCooldown(Definition->Cooldown);
	TargetActor = Character->Skills->RequestedTarget;
	RefreshAim();
	OnSkillExecuted(Definition, TargetPoint);
	Character->Locomotion->PresentSkill(Definition, TargetPoint, bCharging ? ECC_SkillPresentationPhase::ChargeStart : ECC_SkillPresentationPhase::Cast);
	if (Definition->Windup > 0.f&&Definition->DesignMechanic!=ECC_DesignMechanic::Invulnerable&&Definition->DesignMechanic!=ECC_DesignMechanic::Guard)
	{
		UAbilityTask_WaitDelay* Task = UAbilityTask_WaitDelay::WaitDelay(this, FMath::Clamp(Definition->Windup, 0.f, 3.f));
		Task->OnFinish.AddDynamic(this, &ThisClass::ExecuteSkill); Task->ReadyForActivation();
	}
	else ExecuteSkill();
}
void UCC_ConfiguredAbility::RefreshAim()
{
	auto* Character=Cast<ACC_BaseCharacter>(GetAvatarActorFromActorInfo());if(!Character||!Definition)return;
	const float Range=FMath::Max(1.f,Definition->Range);
	if(const auto* T=Cast<ACC_BaseCharacter>(TargetActor.Get());T&&!T->IsAlive())TargetActor.Reset();
	if (TargetActor.IsValid() && FVector::Dist(Character->GetActorLocation(), TargetActor->GetActorLocation()) > Range) TargetActor.Reset();
	if (TargetActor.IsValid())
	{
		FHitResult Obstruction; FCollisionQueryParams Visibility; Visibility.AddIgnoredActor(Character);
		if (GetWorld()->LineTraceSingleByChannel(Obstruction, Character->GetPawnViewLocation(), TargetActor->GetActorLocation(), ECC_Visibility, Visibility) && Obstruction.GetActor() != TargetActor.Get()) TargetActor.Reset();
	}
	AimDirection = Character->GetBaseAimRotation().Vector();
	if (TargetActor.IsValid()) AimDirection = (TargetActor->GetActorLocation() - Character->GetActorLocation()).GetSafeNormal();
	FHitResult Hit; FCollisionQueryParams Params; Params.AddIgnoredActor(Character);
 FVector Start = Character->GetPawnViewLocation();
 const bool bProjectile=Definition->Behavior==ECC_SkillBehavior::Projectile||Definition->Behavior==ECC_SkillBehavior::Spread||Definition->Behavior==ECC_SkillBehavior::Homing||Definition->Behavior==ECC_SkillBehavior::Piercing;
 if(bProjectile&&!TargetActor.IsValid())if(auto* PC=Cast<APlayerController>(Character->GetController()))
 {
  FRotator ViewRotation;PC->GetPlayerViewPoint(Start,ViewRotation);AimDirection=ViewRotation.Vector();
  // Trace the actual camera ray from the character's depth, not through objects
  // behind the shoulder camera. The muzzle sweep still prevents shooting through walls.
  Start+=AimDirection*FMath::Max(0.f,FVector::DotProduct(Character->GetActorLocation()-Start,AimDirection));
 }
 TArray<AActor*> Attachments;Character->GetAttachedActors(Attachments,true,true);Params.AddIgnoredActors(Attachments);
 GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + AimDirection * Range, ECC_Visibility, Params);
	TargetPoint = TargetActor.IsValid() ? TargetActor->GetActorLocation() : Hit.bBlockingHit ? Hit.ImpactPoint : Start + AimDirection * Range;
 if(Definition->DesignMechanic==ECC_DesignMechanic::Gravity||Definition->DesignMechanic==ECC_DesignMechanic::RootArea)
 {
  FCollisionObjectQueryParams Ground;Ground.AddObjectTypesToQuery(ECC_WorldStatic);Ground.AddObjectTypesToQuery(ECC_WorldDynamic);
  FHitResult Floor;FCollisionQueryParams Query;Query.AddIgnoredActor(Character);
  if(GetWorld()->LineTraceSingleByObjectType(Floor,TargetPoint+FVector(0,0,60),TargetPoint-FVector(0,0,1200),Ground,Query)&&Floor.ImpactNormal.Z>.5f)TargetPoint=Floor.ImpactPoint+FVector(0,0,2);
 }
}
void UCC_ConfiguredAbility::ExecuteSkill()
{
	if(!IsActive())return;
	ACC_BaseCharacter* Character = Cast<ACC_BaseCharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !Character->IsAlive() || !Definition) { EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true); return; }
	if(Definition->Behavior==ECC_SkillBehavior::Dash&&Character->Skills->IsRooted()){EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,true,true);return;}
	RefreshAim();
	const auto B=Definition->Behavior;
	bBlocksActionInput=Definition->DesignMechanic==ECC_DesignMechanic::Charge||(Definition->DesignMechanic==ECC_DesignMechanic::None&&(B==ECC_SkillBehavior::Melee||B==ECC_SkillBehavior::Combo||B==ECC_SkillBehavior::Sweep||B==ECC_SkillBehavior::Dash||B==ECC_SkillBehavior::LineStrike||B==ECC_SkillBehavior::Knockback));
	const float Range = FMath::Max(1.f, Definition->Range);
	if(ExecuteDesignSkill()) return;
 const ECC_SkillBehavior Behavior = Definition->Behavior;
	if (Behavior == ECC_SkillBehavior::Projectile || Behavior == ECC_SkillBehavior::Spread || Behavior == ECC_SkillBehavior::Homing || Behavior == ECC_SkillBehavior::Piercing)
	{
		SpawnProjectiles(); EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false); return;
	}
	if (Behavior == ECC_SkillBehavior::Dash)
	{
		FHitResult DashHit;
		Character->SetActorLocation(Character->GetActorLocation() + AimDirection.GetSafeNormal2D() * Range, true, &DashHit);
	}
	PulsesRemaining = Behavior == ECC_SkillBehavior::Combo ? FMath::Clamp(Definition->Count, 1, 32) : Behavior == ECC_SkillBehavior::DamageZone ? FMath::Clamp(FMath::CeilToInt(Definition->Duration / FMath::Max(0.05f, Definition->Interval)), 1, 64) : 1;
	if (Behavior == ECC_SkillBehavior::GroundStrike) WaitForPulse(FMath::Max(0.05f, Definition->Interval));
	else Pulse();
}
void UCC_ConfiguredAbility::WaitForPulse(float Delay)
{
	UAbilityTask_WaitDelay* Task = UAbilityTask_WaitDelay::WaitDelay(this, Delay);
	Task->OnFinish.AddDynamic(this, &ThisClass::Pulse); Task->ReadyForActivation();
}
void UCC_ConfiguredAbility::SpawnProjectiles()
{
	AActor* Source = GetAvatarActorFromActorInfo();
	const int32 Count = Definition->Behavior == ECC_SkillBehavior::Spread ? FMath::Clamp(Definition->Count, 1, 32) : 1;
	const FVector Barrel=Source->GetActorLocation()+FVector(0,0,40);
	const auto* Character=Cast<ACC_BaseCharacter>(Source);
 const float Clearance=Character?FMath::Max(80.f,Character->GetCapsuleComponent()->GetScaledCapsuleRadius()+20.f):80.f;
 FVector Muzzle=Barrel+AimDirection*Clearance;
	FHitResult Block;FCollisionQueryParams Params;Params.AddIgnoredActor(Source);
 TArray<AActor*> Attachments;Source->GetAttachedActors(Attachments,true,true);Params.AddIgnoredActors(Attachments);
	if(GetWorld()->SweepSingleByChannel(Block,Barrel,Muzzle,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(16.f),Params))
	{
		if(FParse::Param(FCommandLine::Get(),TEXT("ParticleDiagnostics")))UE_LOG(LogTemp,Display,TEXT("PLAYER_PARTICLE_MUZZLE_BLOCK actor=%s component=%s initialOverlap=%d barrel=%s muzzle=%s point=%s"),*GetNameSafe(Block.GetActor()),*GetNameSafe(Block.GetComponent()),Block.bStartPenetrating,*Barrel.ToString(),*Muzzle.ToString(),*Block.ImpactPoint.ToString());
		if(!Cast<ACC_BaseCharacter>(Block.GetActor()))
		{
			if(auto* C=Cast<ACC_BaseCharacter>(Source))C->Locomotion->PresentImpact(Definition,Block.ImpactPoint);
			return;
		}
		Muzzle=Barrel;
	}
	const FVector ShotDirection=(TargetPoint-Muzzle).GetSafeNormal(SMALL_NUMBER,AimDirection);
	for (int32 I = 0; I < Count; ++I)
	{
		const float Angle = Count == 1 ? 0.f : FMath::Lerp(-Definition->Angle * 0.5f, Definition->Angle * 0.5f, static_cast<float>(I) / (Count - 1));
		const FRotator Rotation = ShotDirection.RotateAngleAxis(Angle, FVector::UpVector).Rotation();
		const FTransform Transform(Rotation, Muzzle);
		ACC_SkillProjectile* Projectile = GetWorld()->SpawnActorDeferred<ACC_SkillProjectile>(ACC_SkillProjectile::StaticClass(), Transform, Source, Cast<APawn>(Source), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (Projectile) { Projectile->Initialize(Definition, Power, TargetActor.Get()); Projectile->FinishSpawning(Transform); }
	}
}
void UCC_ConfiguredAbility::Pulse()
{
	if(!IsActive())return;
	ACC_BaseCharacter* Source = Cast<ACC_BaseCharacter>(GetAvatarActorFromActorInfo());
	if (!Source || !Source->IsAlive() || !Definition) { EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true); return; }
	const ECC_SkillBehavior B = Definition->Behavior;
	if(B==ECC_SkillBehavior::Melee||B==ECC_SkillBehavior::Combo||B==ECC_SkillBehavior::Sweep||B==ECC_SkillBehavior::Dash||B==ECC_SkillBehavior::LineStrike||B==ECC_SkillBehavior::Knockback)RefreshAim();
	if(B==ECC_SkillBehavior::Combo && Definition->bAlternateAttackPose && PulsesRemaining<FMath::Clamp(Definition->Count,1,32))
		Source->Locomotion->PresentSkill(Definition,TargetPoint,ECC_SkillPresentationPhase::AttackFollowThrough);
	if (B == ECC_SkillBehavior::Heal || B == ECC_SkillBehavior::Shield || B == ECC_SkillBehavior::DefenseBuff || B == ECC_SkillBehavior::AttackBuff || B == ECC_SkillBehavior::SpeedBuff)
	{
		TSubclassOf<UGameplayEffect> Effect = UCC_HealthEffect::StaticClass();
		if (B == ECC_SkillBehavior::Shield) Effect = UCC_ShieldEffect::StaticClass();
		if (B == ECC_SkillBehavior::DefenseBuff) Effect = UCC_DefenseEffect::StaticClass();
		if (B == ECC_SkillBehavior::AttackBuff) Effect = UCC_AttackEffect::StaticClass();
		if (B == ECC_SkillBehavior::SpeedBuff) Effect = UCC_SpeedEffect::StaticClass();
		ApplyEffect(Source, Source, Effect, Power, Definition->Duration);
		Source->Locomotion->PresentImpact(Definition, Source->GetActorLocation());
	}
	else
	{
		const bool bGround = B == ECC_SkillBehavior::GroundStrike || B == ECC_SkillBehavior::DamageZone;
		const FVector Origin = bGround ? TargetPoint : Source->GetActorLocation();
		const float Radius = FMath::Max(1.f, Definition->Radius);
		const bool bMelee = B == ECC_SkillBehavior::Melee || B == ECC_SkillBehavior::Combo || B == ECC_SkillBehavior::Sweep || B == ECC_SkillBehavior::Dash || B == ECC_SkillBehavior::LineStrike;
		TArray<FOverlapResult> Hits;
		FCollisionObjectQueryParams Objects(ECC_Pawn); FCollisionQueryParams Params;
		GetWorld()->OverlapMultiByObjectType(Hits, Origin, FQuat::Identity, Objects, FCollisionShape::MakeSphere(bMelee ? Definition->Range : Radius), Params);
		Hits.Sort([Origin](const FOverlapResult& A,const FOverlapResult& B){return A.GetActor() && B.GetActor() ? FVector::DistSquared(A.GetActor()->GetActorLocation(),Origin)<FVector::DistSquared(B.GetActor()->GetActorLocation(),Origin) : A.GetActor()!=nullptr;});
  int32 Affected=0;
  TSet<AActor*> Seen;
		for (const FOverlapResult& Hit : Hits)
		{
			ACC_BaseCharacter* Target = Cast<ACC_BaseCharacter>(Hit.GetActor());
			if (!Target || !Target->IsAlive() || !Target->GetAbilitySystemComponent() || Seen.Contains(Target)) continue;
			Seen.Add(Target);
			const bool bHeal = B == ECC_SkillBehavior::AreaHeal;
			if (AreAllies(Source, Target) != bHeal) continue;
			const FVector ToTarget = Target->GetActorLocation() - Origin;
			if (bMelee && ToTarget.GetSafeNormal2D().Dot(AimDirection.GetSafeNormal2D()) < FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(Definition->Angle, 1.f, 180.f) * 0.5f))) continue;
			if (B == ECC_SkillBehavior::LineStrike && FVector::CrossProduct(ToTarget, AimDirection).Size() > Radius) continue;
			FHitResult Obstruction; FCollisionQueryParams Visibility; Visibility.AddIgnoredActor(Source);
			if (GetWorld()->LineTraceSingleByChannel(Obstruction, Origin + FVector(0, 0, 40), Target->GetActorLocation(), ECC_Visibility, Visibility) && Obstruction.GetActor() != Target) continue;
			if (B == ECC_SkillBehavior::Slow)
			{
				const float Speed = Target->GetAbilitySystemComponent()->GetNumericAttribute(UCC_AttributeSet::GetMoveSpeedAttribute());
				ApplyEffect(Source, Target, UCC_SpeedEffect::StaticClass(), -Speed * FMath::Clamp(Definition->SlowFraction, 0.f, 1.f), Definition->Duration);
			}
			else
			{
				const int32 HitCount=B==ECC_SkillBehavior::Combo?FMath::Clamp(Definition->Count,1,32):Definition->Count;
    const int32 Step=HitCount-PulsesRemaining;
    const float HitPower=Definition->DamageSteps.IsValidIndex(Step) ? AttackAtCast*Definition->DamageSteps[Step] : Power;
    ApplyEffect(Source, Target, bHeal ? UCC_HealthEffect::StaticClass() : UCC_DamageEffect::StaticClass(), HitPower,0.f,B!=ECC_SkillBehavior::DamageZone);
    const bool StaggerLast=B!=ECC_SkillBehavior::Combo || PulsesRemaining==1;
    const bool FullCharge=Definition->DesignMechanic!=ECC_DesignMechanic::Charge || bFullyCharged;
    if(!bHeal && StaggerLast && FullCharge) Target->Skills->ApplyCrowdControl(Definition->ControlDuration,false);
				if (B == ECC_SkillBehavior::Knockback && Target->IsAlive() && !Target->Skills->BlocksKnockback(Source)) Target->LaunchCharacter(ToTarget.GetSafeNormal2D() * Definition->KnockbackForce + FVector(0, 0, Definition->KnockbackForce * 0.5f), true, true);
			}
			Target->Locomotion->PresentImpact(Definition, Target->GetActorLocation() + FVector(0, 0, 25));
   if(Definition->MaxTargets>0 && ++Affected>=Definition->MaxTargets) break;
		}
	}
	if (--PulsesRemaining > 0) WaitForPulse(FMath::Max(0.05f, Definition->Interval));
	else EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
void UCC_ConfiguredAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	bBlocksActionInput=false;
	if(bWasCancelled && bCharging) { bCharging=false; StartCooldown(3.f); }
 if (bWasCancelled && ActorInfo)
		if (ACC_BaseCharacter* Character = Cast<ACC_BaseCharacter>(ActorInfo->AvatarActor.Get())) Character->Locomotion->CancelPresentation();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	// Next-tick dispatch avoids replacing this ability while its end callback is still unwinding.
	TWeakObjectPtr<AActor> Avatar = ActorInfo ? ActorInfo->AvatarActor : nullptr;
	if (Avatar.IsValid() && !Avatar->ActorHasTag(CrashTags::Player))
		GetWorld()->GetTimerManager().SetTimerForNextTick([Avatar]() { if (Avatar.IsValid()) UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Avatar.Get(), CCTags::Events::Enemy::EndAttack, FGameplayEventData()); });
}

void UCC_ConfiguredAbility::StartCooldown(float Seconds) {
 auto* ASC=GetAbilitySystemComponentFromActorInfo(); if(!ASC || !GetCurrentAbilitySpec()) return;
 auto Spec=ASC->MakeOutgoingSpec(UCC_CooldownEffect::StaticClass(),1.f,ASC->MakeEffectContext());if(!Spec.IsValid()) return;
 Spec.Data->SetSetByCallerMagnitude(TAG_SkillDuration,FMath::Max(.1f,Seconds));Spec.Data->DynamicGrantedTags.AddTag(GetSlotCooldownTag(GetCurrentAbilitySpec()->InputID));ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
}
