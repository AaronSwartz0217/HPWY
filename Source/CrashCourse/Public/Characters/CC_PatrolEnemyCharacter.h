#pragma once

#include "CoreMinimal.h"
#include "Characters/CC_EnemyCharacter.h"
#include "CC_PatrolEnemyCharacter.generated.h"

class AAIController;

UENUM(BlueprintType)
enum class ECC_PatrolState : uint8 { Disabled, Patrolling, Waiting, Chasing, Attacking, Returning, Dead };
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCC_PatrolStateChanged, ECC_PatrolState, PreviousState, ECC_PatrolState, NewState);

// Reusable server-authoritative enemy parent. Uses the existing four-skill component.
UCLASS(Blueprintable)
class CRASHCOURSE_API ACC_PatrolEnemyCharacter : public ACC_EnemyCharacter
{
	GENERATED_BODY()
public:
	ACC_PatrolEnemyCharacter();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool UsesLegacySearchAbility() const override { return false; }
	virtual void HandleRespawn() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Patrol") bool bStartPatrolling = true;
	bool bUseNavMeshForPatrol = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Patrol", meta=(ClampMin="0")) float PatrolRadius = 600.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Patrol", meta=(ClampMin="1")) float PatrolAcceptanceRadius = 60.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Patrol", meta=(ClampMin="0")) float MinPatrolWait = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Patrol", meta=(ClampMin="0")) float MaxPatrolWait = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Patrol", meta=(ClampMin="1")) float PatrolSpeed = 180.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Patrol", meta=(ClampMin="0.1")) float BrainInterval = 0.2f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Patrol", meta=(ClampMin="0.2")) float MoveRetryDelay = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Patrol|Combat") bool bDetectPlayers = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Patrol|Combat") bool bRequireLineOfSight = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Patrol|Combat", meta=(ClampMin="1")) float DetectionRadius = 900.f;
	// Hard leash measured from the original patrol center, not the current enemy location.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Patrol|Combat", meta=(ClampMin="1")) float MaxChaseDistance = 1800.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Patrol|Combat", meta=(ClampMin="0")) float LoseSightDelay = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Patrol|Combat", meta=(ClampMin="1")) float ChaseSpeed = 400.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Patrol|Combat", meta=(ClampMin="1")) float CombatDistance = 180.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category="Patrol") FVector PatrolOrigin = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Replicated, Category="Patrol") FVector PatrolDestination = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_PatrolState, Category="Patrol") ECC_PatrolState PatrolState = ECC_PatrolState::Disabled;
	UPROPERTY(BlueprintReadOnly, Replicated, Category="Patrol") TObjectPtr<ACC_BaseCharacter> CurrentTarget;
	UPROPERTY(BlueprintAssignable, Category="Patrol") FCC_PatrolStateChanged OnPatrolStateChanged;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Patrol") void StartPatrol(bool bResetOrigin = false);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Patrol") void StopPatrol();
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Patrol") void ReturnToPatrolOrigin();
	UFUNCTION(BlueprintPure, Category="Patrol") bool IsInsidePatrolArea(FVector Location) const;
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void HandleDeath() override;
private:
	friend class FCC_PatrolBehaviorTest;
	FTimerHandle BrainTimer;
	bool bHasOrigin = false;
	float WaitUntil = 0.f;
	float NextAttackTime = 0.f;
	float LastSeenTime = 0.f;
	float LastMoveTime = -100.f;
	FVector LastMoveGoal = FVector::ZeroVector;
	void UpdateBrain();
	void SetPatrolState(ECC_PatrolState NewState);
	void SetMovementSpeed(float Speed);
	void WaitForNextPoint(float Delay);
	bool FindPatrolDestination(FVector& OutDestination) const;
	ACC_BaseCharacter* FindVisiblePlayer() const;
	bool CanSeeTarget(const ACC_BaseCharacter* Target) const;
	bool IsCastingSkill() const;
	bool TryCombatSkill();
	bool MoveToPoint(FVector Destination, float Acceptance);
	UFUNCTION() void OnRep_PatrolState(ECC_PatrolState PreviousState);
};
