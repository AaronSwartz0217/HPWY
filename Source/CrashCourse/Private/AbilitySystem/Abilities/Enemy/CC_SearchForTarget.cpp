// Copyright Druid Mechanics


#include "AbilitySystem/Abilities/Enemy/CC_SearchForTarget.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Abilities/Async/AbilityAsync_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystem/AbilityTasks/CC_WaitGameplayEvent.h"
#include "Characters/CC_EnemyCharacter.h"
#include "GameplayTags/CCTags.h"
#include "Tasks/AITask_MoveTo.h"
#include "Utils/CC_BlueprintLibrary.h"
#include "Progression/CC_SkillComponent.h"
#include "Progression/CC_GameplayData.h"

UCC_SearchForTarget::UCC_SearchForTarget()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	FGameplayTagContainer Tags;
	Tags.AddTag(CCTags::CCAbilities::ActivateOnGiven);
	SetAssetTags(Tags);
}

void UCC_SearchForTarget::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	OwningEnemy = Cast<ACC_EnemyCharacter>(GetAvatarActorFromActorInfo());
	if (OwningEnemy.IsValid() && !OwningEnemy->UsesLegacySearchAbility())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	check(OwningEnemy.IsValid());
	OwningAIController = Cast<AAIController>(OwningEnemy->GetController());
	check(OwningAIController.IsValid());

	StartSearch();

	WaitGameplayEventTask = UCC_WaitGameplayEvent::WaitGameplayEventToActorProxy(GetAvatarActorFromActorInfo(), CCTags::Events::Enemy::EndAttack);
	WaitGameplayEventTask->EventReceived.AddDynamic(this, &ThisClass::EndAttackEventReceived);
	WaitGameplayEventTask->StartActivation();
}

void UCC_SearchForTarget::StartSearch()
{
	if (bDrawDebugs) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, FString::Printf(TEXT("UCC_SearchForTarget::StartSearch")));
	if (!OwningEnemy.IsValid()) return;
	
	const float SearchDelay = FMath::RandRange(OwningEnemy->MinAttackDelay, OwningEnemy->MaxAttackDelay);
	SearchDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, SearchDelay);
	SearchDelayTask->OnFinish.AddDynamic(this, &ThisClass::Search);
	SearchDelayTask->Activate();
}

void UCC_SearchForTarget::EndAttackEventReceived(FGameplayEventData Payload)
{
	if (OwningEnemy.IsValid() && !OwningEnemy->bIsBeingLaunched)
	{
		StartSearch();
	}
}
	

void UCC_SearchForTarget::Search()
{
	const FVector SearchOrigin = GetAvatarActorFromActorInfo()->GetActorLocation();
	if (!OwningEnemy.IsValid()) return;
	FClosestActorWithTagResult ClosestActorResult = UCC_BlueprintLibrary::FindClosestActorWithTag(GetAvatarActorFromActorInfo(), SearchOrigin, CrashTags::Player, OwningEnemy->SearchRange);

	TargetBaseCharacter = Cast<ACC_BaseCharacter>(ClosestActorResult.Actor);

	if (!TargetBaseCharacter.IsValid())
	{
		StartSearch();
		return;
	}
	if (TargetBaseCharacter->IsAlive())
	{
		MoveToTargetAndAttack();
	}
	else
	{
		StartSearch();
	}
}

void UCC_SearchForTarget::MoveToTargetAndAttack()
{
	if (!OwningEnemy.IsValid() || !OwningAIController.IsValid() || !TargetBaseCharacter.IsValid()) return;
	if (!OwningEnemy->IsAlive())
	{
		StartSearch();
		return;
	}

	MoveToLocationOrActorTask = UAITask_MoveTo::AIMoveTo(
		OwningAIController.Get(),
		FVector(),
		TargetBaseCharacter.Get(),
		OwningEnemy->AcceptanceRadius);

	MoveToLocationOrActorTask->OnMoveTaskFinished.AddUObject(this, &ThisClass::AttackTarget);
	MoveToLocationOrActorTask->ConditionalPerformMove();
	
}

void UCC_SearchForTarget::AttackTarget(TEnumAsByte<EPathFollowingResult::Type> Result, AAIController* AIController)
{
	if (Result != EPathFollowingResult::Success)
	{
		// Ranged skills can still attack when no navigation path is available.
		bool bInSkillRange = false;
		if (OwningEnemy.IsValid() && TargetBaseCharacter.IsValid() && OwningEnemy->Skills->CurrentLoadout)
			for (const FCC_SkillSlot& Slot : OwningEnemy->Skills->CurrentSlots)
				if (Slot.Skill && FVector::Dist(OwningEnemy->GetActorLocation(), TargetBaseCharacter->GetActorLocation()) <= Slot.Skill->Range) bInSkillRange = true;
		if (!bInSkillRange) { StartSearch(); return; }
	}
	if (!OwningEnemy.IsValid() || !TargetBaseCharacter.IsValid() || !OwningEnemy->IsAlive()) return;
	OwningEnemy->RotateToTarget(TargetBaseCharacter.Get());
	
	AttackDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, OwningEnemy->GetTimelineLength());
	AttackDelayTask->OnFinish.AddDynamic(this, &ThisClass::Attack);
	AttackDelayTask->Activate();
}

void UCC_SearchForTarget::Attack()
{
	if (!OwningEnemy.IsValid() || !OwningEnemy->IsAlive() || !TargetBaseCharacter.IsValid()) return;
	if (OwningEnemy->Skills->CurrentLoadout)
	{
		const int32 StartSlot = FMath::RandRange(0, 3);
		for (int32 Offset = 0; Offset < 4; ++Offset)
		{
			const int32 Slot = (StartSlot + Offset) % 4;
			const UCC_SkillDefinition* Skill = OwningEnemy->Skills->CurrentSlots[Slot].Skill;
			if (Skill && FVector::Dist(OwningEnemy->GetActorLocation(), TargetBaseCharacter->GetActorLocation()) <= Skill->Range && OwningEnemy->Skills->ActivateSlot(Slot, TargetBaseCharacter.Get())) return;
		}
		StartSearch();
		return;
	}
	const FGameplayTag AttackTag = CCTags::CCAbilities::Enemy::Attack;
	GetAbilitySystemComponentFromActorInfo()->TryActivateAbilitiesByTag(AttackTag.GetSingleTagContainer());
}
