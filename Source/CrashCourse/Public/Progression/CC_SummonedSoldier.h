#pragma once
#include "CoreMinimal.h"
#include "Characters/CC_EnemyCharacter.h"
#include "CC_SummonedSoldier.generated.h"
class UCC_FormDefinition;
class UCC_SkillDefinition;
struct FAIMoveRequest;

// Allied combat pawn with its own ASC, without player inventory or save state.
UCLASS(Blueprintable)
class CRASHCOURSE_API ACC_SummonedSoldier : public ACC_EnemyCharacter {
 GENERATED_BODY()
public:
 ACC_SummonedSoldier();
 void InitializeSummon(ACC_BaseCharacter* Source,UCC_SkillDefinition* Skill);
 static bool FindGroundedSpawn(ACC_BaseCharacter* Source,FVector PreferredLocation,FTransform& OutTransform,TSubclassOf<ACC_SummonedSoldier> SoldierClass=nullptr);
 virtual void Tick(float DeltaSeconds) override;
 virtual void PossessedBy(AController* NewController) override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
 virtual bool UsesLegacySearchAbility() const override { return false; }
 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Companion|Appearance") TObjectPtr<UCC_FormDefinition> SoldierForm;
 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Companion|Combat") TObjectPtr<UCC_SkillDefinition> MeleePresentation;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Companion|Combat", meta=(ClampMin="1")) float DetectionRadius=1400.f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Companion|Combat", meta=(ClampMin="100")) float AttackRange=190.f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Companion|Combat", meta=(ClampMin="0.1")) float AttackInterval=2.f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Companion|Combat", meta=(ClampMin="0",ClampMax="1")) float AttackWindup=.15f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Companion|Combat", meta=(ClampMin="0")) float AttackFraction=.25f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Companion|Combat", meta=(ClampMin="0.01")) float HealthFraction=.5f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Companion|Movement", meta=(ClampMin="1")) float FollowDistance=200.f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Companion|Movement") float MeshGroundOffset=0.f;
 UPROPERTY(BlueprintReadOnly, Replicated, Category="Companion") TObjectPtr<ACC_BaseCharacter> CurrentTarget;
 UFUNCTION(BlueprintPure, Category="Companion") ACC_BaseCharacter* GetSummoner() const { return Summoner; }
protected:
 virtual void BeginPlay() override;
 virtual void HandleDeath() override;
 virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
 UPROPERTY(ReplicatedUsing=OnRep_Summoner) TObjectPtr<ACC_BaseCharacter> Summoner;
 UFUNCTION() void OnRep_Summoner();
 void ApplyAppearance();
 void UpdateBrain();
 bool HasClearSight(const ACC_BaseCharacter* Target) const;
 ACC_BaseCharacter* FindEnemy() const;
 bool CanHit(const ACC_BaseCharacter* Target) const;
 FAIMoveRequest MakeMoveRequest(ACC_BaseCharacter* Goal,float Acceptance) const;
 void MoveToward(ACC_BaseCharacter* Goal,float Acceptance);
 void StopMoving();
 void PerformMeleeHit();
 FTimerHandle HitTimer;
 TWeakObjectPtr<ACC_BaseCharacter> PendingTarget;
 TWeakObjectPtr<ACC_BaseCharacter> DirectMoveGoal;
 TWeakObjectPtr<ACC_BaseCharacter> LastMoveGoal;
 float DirectAcceptance=0.f;
 float BrainElapsed=.2f;
 float NextMoveRequest=0.f;
 float NextAttack=0.f;
};
