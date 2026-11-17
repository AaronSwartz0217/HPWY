// Copyright Druid Mechanics


#include "AbilitySystem/CC_AttributeSet.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffectExtension.h"
#include "GameplayTags/CCTags.h"
#include "Net/UnrealNetwork.h"
#include "Characters/CC_EnemyCharacter.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Player/CC_PlayerState.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Progression/CC_ExperienceOrb.h"
#include "Progression/CC_SkillComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

void UCC_AttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Mana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MaxMana, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME(ThisClass, bAttributesInitialized);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Attack, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Defense, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MoveSpeed, COND_None, REPNOTIFY_Always);
}

void UCC_AttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
  auto* Victim=Cast<ACC_BaseCharacter>(Data.Target.GetAvatarActor());
  const auto* SourceASC=Data.EffectSpec.GetContext().GetOriginalInstigatorAbilitySystemComponent();
  auto* Source=SourceASC ? Cast<ACC_BaseCharacter>(SourceASC->GetAvatarActor()) : nullptr;
  const float Before=GetHealth();
  const float DefenseValue=Victim ? Victim->Skills->GuardDefense(Source,GetDefense()) : GetDefense();
  float Damage=Victim && Victim->Skills->IsInvulnerable() ? 0.f : FMath::Max(0.f,GetIncomingDamage())*100.f/(100.f+FMath::Max(0.f,DefenseValue));
  if(Damage>0.f && Victim) Victim->Skills->BreakStealth();
		SetIncomingDamage(0.f);
		const float Absorbed = FMath::Min(FMath::Max(0.f, GetShield()), Damage);
		SetShield(FMath::Max(0.f, GetShield() - Absorbed));
		Damage -= Absorbed;
		SetHealth(FMath::Clamp(GetHealth() - Damage, 0.f, GetMaxHealth()));
  FGameplayTagContainer AssetTags;Data.EffectSpec.GetAllAssetTags(AssetTags);
  if(Source) Source->Skills->OnDamageDealt(FMath::Max(0.f,Before-GetHealth()),!AssetTags.HasTagExact(FGameplayTag::RequestGameplayTag(TEXT("CCTags.Skill.IndirectDamage"))));
	}
	
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
	    SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
	}

	if (Data.EvaluatedData.Attribute == GetManaAttribute())
	{
		SetMana(FMath::Clamp(GetMana(), 0.f, GetMaxMana()));
	}

	if ((Data.EvaluatedData.Attribute == GetHealthAttribute() || Data.EvaluatedData.Attribute == GetIncomingDamageAttribute()) && GetHealth() <= 0.f && !bDeathRewarded)
	{
		bDeathRewarded = true;
		AActor* Killer = Data.EffectSpec.GetContext().GetOriginalInstigator();
		if (const UAbilitySystemComponent* Source = Data.EffectSpec.GetContext().GetOriginalInstigatorAbilitySystemComponent()) Killer = Source->GetAvatarActor();
		ACC_PlayerCharacter* Player = Cast<ACC_PlayerCharacter>(Killer);
  if(!Player && Killer) Player=Cast<ACC_PlayerCharacter>(Killer->GetOwner());
		ACC_EnemyCharacter* Enemy = Cast<ACC_EnemyCharacter>(Data.Target.GetAvatarActor());
		if (Enemy && Enemy->HasAuthority() && Player)
			if (ACC_PlayerState* PS = Player->GetPlayerState<ACC_PlayerState>())
   {
    ACC_ExperienceOrb::SpawnBurst(Enemy, PS, Enemy->GetActorLocation() + FVector(0.f, 0.f, 20.f), Enemy->ExperienceReward, Enemy->ExperienceParticleSystem);
    if(PS->Progression && Enemy->CopperReward>0)PS->Progression->AddCopperCoins(Enemy->CopperReward);
   }
		FGameplayEventData Payload;
		Payload.Instigator = Data.Target.GetAvatarActor();
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Killer, CCTags::Events::KillScored, Payload);
	}

	if (!bAttributesInitialized)
	{
		bAttributesInitialized = true;
		OnAttributesInitialized.Broadcast();
	}
}

void UCC_AttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
	if (Attribute == GetHealthAttribute() && NewValue > 0.f) bDeathRewarded = false;
	if (Attribute == GetMoveSpeedAttribute())
		if (ACharacter* Character = Cast<ACharacter>(GetOwningAbilitySystemComponent()->GetAvatarActor())) Character->GetCharacterMovement()->MaxWalkSpeed = FMath::Max(1.f, NewValue);
}
void UCC_AttributeSet::OnRep_Attack(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Attack, OldValue); }
void UCC_AttributeSet::OnRep_Defense(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Defense, OldValue); }
void UCC_AttributeSet::OnRep_Shield(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Shield, OldValue); }
void UCC_AttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MoveSpeed, OldValue); }

void UCC_AttributeSet::OnRep_AttributesInitialized()
{
	if (bAttributesInitialized)
	{
		OnAttributesInitialized.Broadcast();
	}
}

void UCC_AttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Health, OldValue);
}

void UCC_AttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MaxHealth, OldValue);
}

void UCC_AttributeSet::OnRep_Mana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Mana, OldValue);
}

void UCC_AttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MaxMana, OldValue);
}
