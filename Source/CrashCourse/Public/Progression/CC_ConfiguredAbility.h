#pragma once
#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "CC_ConfiguredAbility.generated.h"

class UCC_SkillDefinition;
class ACC_BaseCharacter;

// Explicit full initialization, independent of incomplete tutorial GE assets.
UCLASS() class CRASHCOURSE_API UCC_InitialAttributesEffect : public UGameplayEffect { GENERATED_BODY() public: UCC_InitialAttributesEffect(); };

UCLASS() class CRASHCOURSE_API UCC_DamageEffect : public UGameplayEffect { GENERATED_BODY() public: UCC_DamageEffect(); };
UCLASS() class CRASHCOURSE_API UCC_HealthEffect : public UGameplayEffect { GENERATED_BODY() public: UCC_HealthEffect(); };
UCLASS() class CRASHCOURSE_API UCC_ManaEffect : public UGameplayEffect { GENERATED_BODY() public: UCC_ManaEffect(); };
UCLASS() class CRASHCOURSE_API UCC_ShieldEffect : public UGameplayEffect { GENERATED_BODY() public: UCC_ShieldEffect(); };
UCLASS() class CRASHCOURSE_API UCC_DefenseEffect : public UGameplayEffect { GENERATED_BODY() public: UCC_DefenseEffect(); };
UCLASS() class CRASHCOURSE_API UCC_AttackEffect : public UGameplayEffect { GENERATED_BODY() public: UCC_AttackEffect(); };
UCLASS() class CRASHCOURSE_API UCC_SpeedEffect : public UGameplayEffect { GENERATED_BODY() public: UCC_SpeedEffect(); };
UCLASS() class CRASHCOURSE_API UCC_CooldownEffect : public UGameplayEffect { GENERATED_BODY() public: UCC_CooldownEffect(); };

// One server-authoritative execution implementation, configured by 20 separate assets.
UCLASS(Blueprintable)
class CRASHCOURSE_API UCC_ConfiguredAbility : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UCC_ConfiguredAbility();
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	static FGameplayTag GetSlotCooldownTag(int32 Slot);
	bool BlocksNewSkillInput() const {return IsActive()&&bBlocksActionInput;}
	void ReleaseChargeInput();
	static void ApplyEffect(AActor* Source, AActor* Target, TSubclassOf<UGameplayEffect> Effect, float Value, float Duration = 0.f, bool bDirectDamage = true);
	static bool AreAllies(const AActor* A, const AActor* B);
	UFUNCTION(BlueprintImplementableEvent, Category="Skills") void OnSkillExecuted(UCC_SkillDefinition* SkillData, FVector AimPoint);
private:
	UPROPERTY() TObjectPtr<UCC_SkillDefinition> Definition;
	FVector TargetPoint = FVector::ZeroVector;
	FVector AimDirection = FVector::ForwardVector;
	TWeakObjectPtr<AActor> TargetActor;
	int32 PulsesRemaining = 0;
	float Power = 0.f;
 float AttackAtCast=0.f,ChargeStarted=0.f;
 int32 DesignPulsesTotal=0;
 float DesignStarted=0.f,LastDesignPulseTime=0.f;
 bool bCharging=false;
 bool bBlocksActionInput=false,bChargeReleaseRequested=false,bFullyCharged=false;
 float ChargeReleasedAt=0.f;
 void RefreshAim();
 void StartCooldown(float Seconds);
 bool ExecuteDesignSkill();
 UFUNCTION() void DesignPulse();
 UFUNCTION() void ChargeTick();
	UFUNCTION() void Pulse();
	UFUNCTION() void ExecuteSkill();
	void WaitForPulse(float Delay);
	void SpawnProjectiles();
};
