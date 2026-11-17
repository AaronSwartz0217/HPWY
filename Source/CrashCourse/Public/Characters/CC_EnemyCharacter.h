// Copyright Druid Mechanics

#pragma once

#include "CoreMinimal.h"
#include "CC_BaseCharacter.h"
#include "CC_EnemyCharacter.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;
class UNiagaraSystem;
class UCC_CombatBalance;

UCLASS()
class CRASHCOURSE_API ACC_EnemyCharacter : public ACC_BaseCharacter
{
	GENERATED_BODY()

public:
	ACC_EnemyCharacter();
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Balance") TObjectPtr<UCC_CombatBalance> CombatBalance;
	// Empty selects by mesh. An explicit ID lets elites share a normal mesh.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Balance") FName StatProfileId;
	UPROPERTY(ReplicatedUsing=OnRep_ProfileScale) FVector ProfileMeshScale = FVector::ZeroVector;
	UFUNCTION() void OnRep_ProfileScale();
	FVector OriginalProfileMeshScale = FVector::ZeroVector;
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Balance") bool ApplyCombatProfile();
	virtual bool UsesLegacySearchAbility() const { return true; }
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Progression", meta=(ClampMin="0"))
	int32 ExperienceReward = 10;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Progression",meta=(ClampMin="0")) int32 CopperReward=0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Progression") TObjectPtr<UNiagaraSystem> ExperienceParticleSystem;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crash|AI")
	float AcceptanceRadius{500.f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crash|AI")
	float MinAttackDelay{.1f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crash|AI")
	float MaxAttackDelay{.5f};

	UFUNCTION(BlueprintImplementableEvent)
	float GetTimelineLength();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
	bool bIsBeingLaunched{false};

	void StopMovementUntilLanded();
protected:
	virtual void BeginPlay() override;
	virtual void HandleDeath() override;
private:

	UFUNCTION()
	void EnableMovementOnLanded(const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UAttributeSet> AttributeSet;
	
};
