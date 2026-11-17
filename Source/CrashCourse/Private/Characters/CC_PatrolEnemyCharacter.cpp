#include "Characters/CC_PatrolEnemyCharacter.h"
#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Progression/CC_SkillComponent.h"
#include "Progression/CC_GameplayData.h"
#include "Progression/CC_ConfiguredAbility.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "Net/UnrealNetwork.h"

ACC_PatrolEnemyCharacter::ACC_PatrolEnemyCharacter()
{
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
	SetDefaultAttributeEffects(UCC_InitialAttributesEffect::StaticClass());
}
void ACC_PatrolEnemyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, PatrolOrigin);
	DOREPLIFETIME(ThisClass, PatrolDestination);
	DOREPLIFETIME(ThisClass, PatrolState);
	DOREPLIFETIME(ThisClass, CurrentTarget);
}
void ACC_PatrolEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority()) return;
	PatrolOrigin = GetActorLocation(); bHasOrigin = true;
	if (bStartPatrolling) StartPatrol();
}
void ACC_PatrolEnemyCharacter::SetPatrolState(ECC_PatrolState NewState)
{
	if (PatrolState == NewState) return;
	const ECC_PatrolState Previous = PatrolState;
	PatrolState = NewState;
	ForceNetUpdate();
	OnPatrolStateChanged.Broadcast(Previous, NewState);
}
void ACC_PatrolEnemyCharacter::OnRep_PatrolState(ECC_PatrolState PreviousState) { OnPatrolStateChanged.Broadcast(PreviousState, PatrolState); }
bool ACC_PatrolEnemyCharacter::IsInsidePatrolArea(FVector Location) const
{
	return FVector::DistSquared2D(PatrolOrigin, Location) <= FMath::Square(FMath::Max(0.f, PatrolRadius));
}
void ACC_PatrolEnemyCharacter::SetMovementSpeed(float Speed)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent()) ASC->SetNumericAttributeBase(UCC_AttributeSet::GetMoveSpeedAttribute(), FMath::Max(1.f, Speed));
}
void ACC_PatrolEnemyCharacter::StartPatrol(bool bResetOrigin)
{
	if (!HasAuthority() || !IsAlive()) return;
	if (!bHasOrigin || bResetOrigin) { PatrolOrigin = GetActorLocation(); bHasOrigin = true; }
	if (!GetController()) SpawnDefaultController();
	CurrentTarget = nullptr;
	Skills->CancelActiveSkills();
	if (AAIController* AI = GetController<AAIController>()) AI->StopMovement();
	SetMovementSpeed(PatrolSpeed);
	WaitForNextPoint(0.f);
	GetWorldTimerManager().SetTimer(BrainTimer, this, &ThisClass::UpdateBrain, FMath::Max(0.1f, BrainInterval), true);
}
void ACC_PatrolEnemyCharacter::StopPatrol()
{
	if (!HasAuthority()) return;
	GetWorldTimerManager().ClearTimer(BrainTimer);
	CurrentTarget = nullptr;
	Skills->CancelActiveSkills();
	if (AAIController* AI = GetController<AAIController>()) AI->StopMovement();
	SetPatrolState(IsAlive() ? ECC_PatrolState::Disabled : ECC_PatrolState::Dead);
}
void ACC_PatrolEnemyCharacter::ReturnToPatrolOrigin()
{
	if (!HasAuthority() || !IsAlive() || !bHasOrigin) return;
	CurrentTarget = nullptr;
	Skills->CancelActiveSkills();
	if (AAIController* AI = GetController<AAIController>()) AI->StopMovement();
	LastMoveTime = -100.f;
	SetMovementSpeed(PatrolSpeed);
	SetPatrolState(ECC_PatrolState::Returning);
	if (!GetWorldTimerManager().IsTimerActive(BrainTimer)) GetWorldTimerManager().SetTimer(BrainTimer, this, &ThisClass::UpdateBrain, FMath::Max(0.1f, BrainInterval), true);
}
void ACC_PatrolEnemyCharacter::WaitForNextPoint(float Delay)
{
	WaitUntil = GetWorld()->GetTimeSeconds() + FMath::Max(0.f, Delay);
	SetPatrolState(ECC_PatrolState::Waiting);
}
bool ACC_PatrolEnemyCharacter::CanSeeTarget(const ACC_BaseCharacter* Target) const
{
	if (!IsValid(Target) || !Target->IsAlive() || Target->Skills->IsStealthed()) return false;
	if (!bRequireLineOfSight) return true;
	FCollisionQueryParams Params; Params.AddIgnoredActor(this);
	FHitResult Hit;
	return !GetWorld()->LineTraceSingleByChannel(Hit, GetPawnViewLocation(), Target->GetPawnViewLocation(), ECC_Visibility, Params) || Hit.GetActor() == Target;
}
ACC_BaseCharacter* ACC_PatrolEnemyCharacter::FindVisiblePlayer() const
{
	TArray<AActor*> Players;
	UGameplayStatics::GetAllActorsWithTag(this, CrashTags::Player, Players);
	ACC_BaseCharacter* Closest = nullptr;
	float BestDistance = FMath::Square(FMath::Max(0.f, DetectionRadius));
	const float Leash = FMath::Max(PatrolRadius, MaxChaseDistance);
	for (AActor* Actor : Players)
	{
		ACC_BaseCharacter* Candidate = Cast<ACC_BaseCharacter>(Actor);
		if (!Candidate || !Candidate->IsAlive() || FVector::DistSquared2D(Candidate->GetActorLocation(), PatrolOrigin) > FMath::Square(Leash)) continue;
		const float Distance = FVector::DistSquared(GetActorLocation(), Candidate->GetActorLocation());
		if (Distance <= BestDistance && CanSeeTarget(Candidate)) { Closest = Candidate; BestDistance = Distance; }
	}
	return Closest;
}
bool ACC_PatrolEnemyCharacter::FindPatrolDestination(FVector& OutDestination) const
{
	if (!bUseNavMeshForPatrol)
	{
		const float Angle = FMath::FRandRange(0.f, 2.f * UE_PI);
		OutDestination = PatrolOrigin + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0) * PatrolRadius * .9f;
		return true;
	}
	UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!Nav || PatrolRadius <= 1.f) return false;
	for (int32 Attempt = 0; Attempt < 6; ++Attempt)
	{
		FNavLocation Point;
		if (Nav->GetRandomReachablePointInRadius(PatrolOrigin, PatrolRadius, Point) && IsInsidePatrolArea(Point.Location)
			&& FVector::DistSquared2D(Point.Location, GetActorLocation()) > FMath::Square(PatrolAcceptanceRadius * 2.f))
		{ OutDestination = Point.Location; return true; }
	}
	return false;
}
bool ACC_PatrolEnemyCharacter::MoveToPoint(FVector Destination, float Acceptance)
{
	AAIController* AI = GetController<AAIController>();
	if (!AI || Skills->IsRooted()) return false;
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastMoveTime < FMath::Max(0.2f, MoveRetryDelay)) return AI->GetMoveStatus() == EPathFollowingStatus::Moving;
	if (AI->GetMoveStatus() == EPathFollowingStatus::Moving && FVector::DistSquared2D(LastMoveGoal, Destination) < FMath::Square(100.f)) return true;
	LastMoveTime = Now; LastMoveGoal = Destination;
	// Use the same explicit radius as the state transitions. Overlap adds the
	// path follower's agent-radius multiplier and can finish outside our radius.
	return AI->MoveToLocation(Destination, FMath::Max(1.f, Acceptance), false,
		bUseNavMeshForPatrol, bUseNavMeshForPatrol, false, nullptr, false) != EPathFollowingRequestResult::Failed;
}
bool ACC_PatrolEnemyCharacter::IsCastingSkill() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return false;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities()) if (Spec.IsActive() && Cast<UCC_SkillDefinition>(Spec.SourceObject.Get())) return true;
	return false;
}
bool ACC_PatrolEnemyCharacter::TryCombatSkill()
{
	if (Skills->IsStunned() || !Skills->CurrentLoadout || !IsValid(CurrentTarget) || !CanSeeTarget(CurrentTarget)) return false;
	const float Distance = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());
	const int32 FirstSlot = FMath::RandRange(0, 3);
	for (int32 Offset = 0; Offset < 4; ++Offset)
	{
		const int32 Slot = (FirstSlot + Offset) % 4;
		const UCC_SkillDefinition* Skill = Skills->CurrentSlots[Slot].Skill;
		if (Skill && Distance <= Skill->Range && Skills->ActivateSlot(Slot, CurrentTarget)) return true;
	}
	return false;
}
void ACC_PatrolEnemyCharacter::UpdateBrain()
{
	if (!HasAuthority()) return;
	if (!IsAlive()) { StopPatrol(); return; }
	AAIController* AI = GetController<AAIController>();
	if (!AI) return;
	if (Skills->IsStunned() || bIsBeingLaunched || GetCharacterMovement()->IsFalling()) { AI->StopMovement(); return; }
	const float Now = GetWorld()->GetTimeSeconds();
	if (PatrolState == ECC_PatrolState::Returning)
	{
		if (FVector::DistSquared2D(GetActorLocation(), PatrolOrigin) <= FMath::Square(FMath::Max(1.f, PatrolAcceptanceRadius)))
		{ AI->StopMovement(); WaitForNextPoint(MinPatrolWait); }
		else MoveToPoint(PatrolOrigin, PatrolAcceptanceRadius);
		return;
	}
	if (!IsValid(CurrentTarget) && bDetectPlayers)
	{
		CurrentTarget = FindVisiblePlayer();
		if (CurrentTarget) { LastSeenTime = Now; LastMoveTime = -100.f; SetMovementSpeed(ChaseSpeed); SetPatrolState(ECC_PatrolState::Chasing); }
	}
	if (IsValid(CurrentTarget))
	{
		const float Leash = FMath::Max(PatrolRadius, MaxChaseDistance);
		const bool bVisible = !CurrentTarget->Skills->IsStealthed() && CanSeeTarget(CurrentTarget);
		if (bVisible) LastSeenTime = Now;
		if (!CurrentTarget->IsAlive() || FVector::DistSquared2D(GetActorLocation(), PatrolOrigin) > FMath::Square(Leash)
			|| FVector::DistSquared2D(CurrentTarget->GetActorLocation(), PatrolOrigin) > FMath::Square(Leash)
			|| (!bVisible && Now - LastSeenTime >= FMath::Max(0.f, LoseSightDelay)))
		{ ReturnToPatrolOrigin(); return; }
		if (IsCastingSkill()) { AI->StopMovement(); SetPatrolState(ECC_PatrolState::Attacking); return; }
		if (bVisible)
		{
			FRotator Facing = (CurrentTarget->GetActorLocation() - GetActorLocation()).Rotation(); Facing.Pitch = Facing.Roll = 0.f;
			SetActorRotation(Facing);
			if (Now >= NextAttackTime && TryCombatSkill())
			{
				AI->StopMovement();
				NextAttackTime = Now + FMath::RandRange(FMath::Max(0.1f, MinAttackDelay), FMath::Max(FMath::Max(0.1f, MinAttackDelay), MaxAttackDelay));
				SetPatrolState(ECC_PatrolState::Attacking); return;
			}
		}
		SetPatrolState(ECC_PatrolState::Chasing);
		if (FVector::DistSquared2D(GetActorLocation(), CurrentTarget->GetActorLocation()) > FMath::Square(FMath::Max(1.f, CombatDistance))) MoveToPoint(CurrentTarget->GetActorLocation(), CombatDistance);
		else AI->StopMovement();
		return;
	}
	if (PatrolState == ECC_PatrolState::Chasing || PatrolState == ECC_PatrolState::Attacking) { ReturnToPatrolOrigin(); return; }
	if (PatrolState == ECC_PatrolState::Patrolling)
	{
		if (!IsInsidePatrolArea(GetActorLocation())) { ReturnToPatrolOrigin(); return; }
		if (FVector::DistSquared2D(GetActorLocation(), PatrolDestination) <= FMath::Square(FMath::Max(1.f, PatrolAcceptanceRadius)))
		{ AI->StopMovement(); WaitForNextPoint(FMath::RandRange(FMath::Max(0.f, MinPatrolWait), FMath::Max(MinPatrolWait, MaxPatrolWait))); }
		else if (AI->GetMoveStatus() != EPathFollowingStatus::Moving) WaitForNextPoint(MoveRetryDelay);
		return;
	}
	if (PatrolState == ECC_PatrolState::Waiting && Now >= WaitUntil)
	{
		FVector Point;
		if (FindPatrolDestination(Point) && MoveToPoint(Point, PatrolAcceptanceRadius))
		{ PatrolDestination = Point; SetPatrolState(ECC_PatrolState::Patrolling); }
		else WaitForNextPoint(MoveRetryDelay);
	}
}
void ACC_PatrolEnemyCharacter::HandleDeath() { Super::HandleDeath(); StopPatrol(); }
void ACC_PatrolEnemyCharacter::HandleRespawn() { Super::HandleRespawn(); if (HasAuthority() && bStartPatrolling) StartPatrol(); }
void ACC_PatrolEnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(BrainTimer);
	Super::EndPlay(EndPlayReason);
}
