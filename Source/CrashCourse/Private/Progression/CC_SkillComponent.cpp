#include "Progression/CC_SkillComponent.h"
#include "Progression/CC_GameplayData.h"
#include "Progression/CC_ConfiguredAbility.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Characters/CC_BaseCharacter.h"
#include "Characters/CC_LocomotionComponent.h"
#include "Player/CC_PlayerState.h"
#include "AbilitySystemComponent.h"
#include "GameplayTags/CCTags.h"
#include "Net/UnrealNetwork.h"

UCC_SkillComponent::UCC_SkillComponent() { SetIsReplicatedByDefault(true); PrimaryComponentTick.bCanEverTick=true; }
void UCC_SkillComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, CurrentLoadout);
 DOREPLIFETIME(ThisClass,bStealthed);
	DOREPLIFETIME(ThisClass, CurrentSlots);
 DOREPLIFETIME_CONDITION(ThisClass,CooldownHasteEndsAt,COND_OwnerOnly);
 DOREPLIFETIME_CONDITION(ThisClass,CooldownHasteMultiplier,COND_OwnerOnly);
}
void UCC_SkillComponent::Initialize() { if (DefaultLoadout) ApplyLoadout(DefaultLoadout); }

bool UCC_SkillComponent::ApplyLoadout(UCC_SkillLoadout* Loadout)
{
	ACC_BaseCharacter* Character = Cast<ACC_BaseCharacter>(GetOwner());
	UAbilitySystemComponent* ASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
	if (!ASC || !Character->HasAuthority() || (Loadout && !Loadout->IsComplete())) return false;
	HeldSlots.Empty();ClearQueuedSkill();RequestedTarget.Reset();
	ASC->CancelAbilityHandle(BasicAttackHandle);
	if (BasicAttack && !ASC->FindAbilitySpecFromHandle(BasicAttackHandle))
	{
		const TSubclassOf<UGameplayAbility> Class = BasicAttack->AbilityClass ? BasicAttack->AbilityClass.Get() : UCC_ConfiguredAbility::StaticClass();
		BasicAttackHandle = ASC->GiveAbility(FGameplayAbilitySpec(Class, 1, 4, BasicAttack));
	}
	if (!bCapturedOriginal)
	{
		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			if (!Spec.Ability) continue;
			const FGameplayTagContainer& Tags = Spec.Ability->GetAssetTags();
			if (Tags.HasTagExact(CCTags::CCAbilities::Primary) || Tags.HasTagExact(CCTags::CCAbilities::Secondary) || Tags.HasTagExact(CCTags::CCAbilities::Tertiary) || Tags.HasTagExact(CCTags::CCAbilities::Enemy::Attack))
			{
				OriginalAbilities.Add(Spec);
				GrantedHandles.Add(Spec.Handle);
			}
		}
		bCapturedOriginal = true;
	}
	for (const FGameplayAbilitySpecHandle Handle : GrantedHandles)
	{
		ASC->CancelAbilityHandle(Handle);
		ASC->ClearAbility(Handle);
	}
	Character->Locomotion->CancelPresentation();
	GrantedHandles.Empty();
	CurrentLoadout = Loadout;
	CurrentSlots = Loadout ? Loadout->Slots : TArray<FCC_SkillSlot>();
	if (ACC_PlayerState* PS = Character->GetPlayerState<ACC_PlayerState>(); PS && PS->Progression->ManualSkills.Num() == 4 && Loadout && !PS->Progression->ManualSkills.ContainsByPredicate([PS](const UCC_SkillDefinition* S){return !PS->Progression->IsSkillUnlocked(S);}))
		for (int32 I=0; I<4; ++I) { CurrentSlots[I].Skill = PS->Progression->ManualSkills[I]; CurrentSlots[I].Level = 1; }
	if (Loadout)
	{
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const FCC_SkillSlot& Slot = CurrentSlots[Index];
			const TSubclassOf<UGameplayAbility> Class = Slot.Skill->AbilityClass ? Slot.Skill->AbilityClass.Get() : UCC_ConfiguredAbility::StaticClass();
			FGameplayAbilitySpec Spec(Class, Slot.Level, Index, Slot.Skill);
			GrantedHandles.Add(ASC->GiveAbility(Spec));
		}
		RefreshLevels();
	}
	else
	{
		for (const FGameplayAbilitySpec& Old : OriginalAbilities)
		{
			GrantedHandles.Add(ASC->GiveAbility(FGameplayAbilitySpec(Old.Ability->GetClass(), Old.Level, Old.InputID, Old.SourceObject.Get())));
		}
	}
	OnLoadoutChanged.Broadcast();
	return true;
}

void UCC_SkillComponent::OnRep_CurrentSlots() { OnLoadoutChanged.Broadcast(); }

void UCC_SkillComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ACC_BaseCharacter* Character = Cast<ACC_BaseCharacter>(GetOwner()); Character && Character->HasAuthority())
	{
		if (UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent())
		{
			ASC->CancelAbilityHandle(BasicAttackHandle); ASC->ClearAbility(BasicAttackHandle);
			for (const FGameplayAbilitySpecHandle Handle : GrantedHandles) { ASC->CancelAbilityHandle(Handle); ASC->ClearAbility(Handle); }
		}
	}
	ClearDesignStates();
 Super::EndPlay(EndPlayReason);
}

void UCC_SkillComponent::RefreshLevels()
{
	ACC_BaseCharacter* Character = Cast<ACC_BaseCharacter>(GetOwner());
	if (!Character || !Character->HasAuthority() || !CurrentLoadout) return;
	UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
	ACC_PlayerState* PS = Character->GetPlayerState<ACC_PlayerState>();
	if (BasicAttack)
		if (FGameplayAbilitySpec* BasicSpec = ASC->FindAbilitySpecFromHandle(BasicAttackHandle))
		{
			BasicSpec->Level = 1 + (PS ? PS->Progression->GetSkillBonus(BasicAttack->SkillId) : 0);
			ASC->MarkAbilitySpecDirty(*BasicSpec);
		}
	for (const FGameplayAbilitySpecHandle Handle : GrantedHandles)
	{
		FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle);
		if (!Spec || !CurrentSlots.IsValidIndex(Spec->InputID)) continue;
		const FCC_SkillSlot& Slot = CurrentSlots[Spec->InputID];
		Spec->Level = Slot.Level + (PS ? PS->Progression->GetSkillBonus(Slot.Skill->SkillId) : 0);
		ASC->MarkAbilitySpecDirty(*Spec);
	}
}

bool UCC_SkillComponent::ActivateSlot(int32 Slot, AActor* Target)
{
	if (Slot < 0 || Slot >= 4) return false;
	if (!GetOwner()->HasAuthority()) { ServerActivateSlot(Slot, Target); return true; }
	ACC_BaseCharacter* Character = Cast<ACC_BaseCharacter>(GetOwner());
	if (!Character || !Character->IsAlive() || !CurrentLoadout) return false;
	const bool bWasHeld = HeldSlots.Contains(Slot);
	HeldSlots.Add(Slot);
	const bool Accepted=TrySkillInput(Slot,Target,true);
	if (!Accepted && !bWasHeld) HeldSlots.Remove(Slot);
	return Accepted;
}
void UCC_SkillComponent::ServerActivateSlot_Implementation(int32 Slot, AActor* Target) { ActivateSlot(Slot, Target); }

bool UCC_SkillComponent::ActivateBasicAttack()
{
	if (!GetOwner()->HasAuthority()) { ServerBasicAttack(); return true; }
	ACC_BaseCharacter* Character = Cast<ACC_BaseCharacter>(GetOwner());
	if (!Character || !Character->IsAlive() || !Character->GetAbilitySystemComponent()) return false;
	return TrySkillInput(4,nullptr,true);
}
void UCC_SkillComponent::ServerBasicAttack_Implementation() { ActivateBasicAttack(); }

void UCC_SkillComponent::CancelActiveSkills()
{
	ACC_BaseCharacter* Character = Cast<ACC_BaseCharacter>(GetOwner());
	if (!Character || !Character->HasAuthority()) return;
	HeldSlots.Empty();ClearQueuedSkill();RequestedTarget.Reset();
 Character->Locomotion->CancelPresentation();
	if (UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent())
	{
		ASC->CancelAbilityHandle(BasicAttackHandle);
		const TArray<FGameplayAbilitySpecHandle> Handles = GrantedHandles;
		for (const FGameplayAbilitySpecHandle Handle : Handles) ASC->CancelAbilityHandle(Handle);
	}
}

float UCC_SkillComponent::GetCooldownRemaining(int32 Slot) const
{
	float Duration;
	return GetCooldownState(Slot, Duration);
}

float UCC_SkillComponent::GetCooldownState(int32 Slot, float& OutDuration) const
{
	OutDuration = 0.f;
	const ACC_BaseCharacter* Character = Cast<ACC_BaseCharacter>(GetOwner());
	if (!Character || !Character->GetAbilitySystemComponent() || Slot < 0 || Slot > 4) return 0.f;
	const FGameplayTag Tag = UCC_ConfiguredAbility::GetSlotCooldownTag(Slot);
	const auto Times = Character->GetAbilitySystemComponent()->GetActiveEffectsTimeRemainingAndDuration(FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(Tag.GetSingleTagContainer()));
	float Remaining = 0.f;
	for (const auto& Time : Times)
	{
		if (Time.Key > Remaining)
		{
			Remaining = Time.Key;
			OutDuration = Time.Value;
		}
	}
	return Remaining;
}
