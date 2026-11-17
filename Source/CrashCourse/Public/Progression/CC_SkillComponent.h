#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpec.h"
#include "Progression/CC_GameplayData.h"
#include "CC_SkillComponent.generated.h"

class UCC_SkillLoadout;
class UCC_SkillDefinition;
class ACC_BaseCharacter;
class ACC_SummonedSoldier;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCC_LoadoutChanged);

UCLASS(ClassGroup=(Crash), meta=(BlueprintSpawnableComponent))
class CRASHCOURSE_API UCC_SkillComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UCC_SkillComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	uint32 ShieldGeneration = 0;
 UPROPERTY(BlueprintReadOnly,Category="Skills") TObjectPtr<UCC_SkillDefinition> LastExecutedSkill;
 UPROPERTY(BlueprintReadOnly,Category="Skills") int32 LastExecutedSlot=INDEX_NONE;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skills") TObjectPtr<UCC_SkillLoadout> DefaultLoadout;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skills") TObjectPtr<UCC_SkillDefinition> BasicAttack;
 // Empty uses the native soldier; assign a derived blueprint to customize allies.
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skills|Summon") TSubclassOf<ACC_SummonedSoldier> SummonedSoldierClass;
	UPROPERTY(BlueprintReadOnly, Replicated) TObjectPtr<UCC_SkillLoadout> CurrentLoadout;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_CurrentSlots) TArray<FCC_SkillSlot> CurrentSlots;
 UPROPERTY(BlueprintAssignable, Category="Skills") FCC_LoadoutChanged OnLoadoutChanged;
	UFUNCTION(BlueprintCallable, Category="Skills") bool ActivateSlot(int32 Slot, AActor* Target = nullptr);
	UFUNCTION(BlueprintCallable, Category="Skills") bool ActivateBasicAttack();
 UFUNCTION(BlueprintCallable, Category="Skills") void ReleaseSlot(int32 Slot);
 bool IsSlotHeld(int32 Slot) const {return HeldSlots.Contains(Slot);}
 UFUNCTION(BlueprintCallable,Category="Skills|Input") void CancelPendingInput();
 UFUNCTION(BlueprintPure,Category="Skills|Input") bool HasBlockingSkillAction() const;
 UFUNCTION(BlueprintPure,Category="Skills|Input") int32 GetQueuedSkillSlot() const {return QueuedSlot;}
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Skills|Input",meta=(ClampMin="0",ClampMax="0.35")) float InputBufferSeconds=.18f;
 bool IsStunned() const; bool IsRooted() const; bool IsInvulnerable() const;
 bool IsStealthed() const {return bStealthed;}
 bool BlocksKnockback(const AActor* Source) const;
 float GuardDefense(const AActor* Source,float NormalDefense) const;
 void ApplyCrowdControl(float Seconds,bool RootOnly);
 void ActivateDesignStatus(UCC_SkillDefinition* Skill);
 void BreakStealth();
 void RememberCombatTarget(AActor* Target,bool bOverride=true);
 ACC_BaseCharacter* GetCombatFocus() const;
 void OnDamageDealt(float HealthDamage,bool DirectDamage);
 void ClearDesignStates();
 void SummonSoldiers(UCC_SkillDefinition* Skill);
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Skills|Control") bool bControlImmune=false;
 UPROPERTY(BlueprintReadOnly,ReplicatedUsing=OnRep_Stealth) bool bStealthed=false;
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Skills") void CancelActiveSkills();
	UFUNCTION(BlueprintPure, Category="Skills") float GetCooldownRemaining(int32 Slot) const;
 float GetCooldownState(int32 Slot, float& OutDuration) const;
 UFUNCTION(BlueprintPure,Category="Skills|Items") float GetCooldownHasteRemaining() const;
 void ActivateCooldownHaste(float Duration,float Multiplier);
 void RefreshSkillCooldowns();
 void ClearCooldownHaste();
	bool ApplyLoadout(UCC_SkillLoadout* Loadout);
	void Initialize();
	void RefreshLevels();
	TWeakObjectPtr<AActor> RequestedTarget;
private:
 UPROPERTY(Replicated) double CooldownHasteEndsAt=0;
 UPROPERTY(Replicated) float CooldownHasteMultiplier=1.f;
 double CooldownHasteStartsAt=0;
 double CooldownClock() const;
 void TickCooldownHaste(float Delta);
 virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction) override;
 UFUNCTION() void OnRep_Stealth();
 UFUNCTION(Server,Reliable) void ServerReleaseSlot(int32 Slot);
 TSet<int32> HeldSlots;
 int32 QueuedSlot=INDEX_NONE;
 float QueuedUntil=0;
 TWeakObjectPtr<ACC_BaseCharacter> CombatFocus;
 float CombatFocusUntil=0;
 TWeakObjectPtr<AActor> QueuedTarget;
 TWeakObjectPtr<UCC_SkillDefinition> QueuedDefinition;
 FGameplayAbilitySpecHandle FindSkillHandle(int32 Slot) const;
 bool TrySkillInput(int32 Slot,AActor* Target,bool bAllowBuffer);
 void ClearQueuedSkill();
 void DrainQueuedSkill();
 UFUNCTION(Server,Reliable) void ServerCancelPendingInput();
 float StunUntil=0.f,RootUntil=0.f,InvulnerableUntil=0.f,GuardUntil=0.f,GuardArc=120.f,GuardFactor=2.f,LifestealUntil=0.f,LifestealFraction=.2f;
 float LifestealWindowStart=-1.f,LifestealHealed=0.f;
 uint32 StealthGeneration=0;
 TMap<FName,FActiveGameplayEffectHandle> DesignEffects;
 TArray<TWeakObjectPtr<class ACC_SummonedSoldier>> SummonedSoldiers;
 UFUNCTION() void OnRep_CurrentSlots();
	UFUNCTION(Server, Reliable) void ServerActivateSlot(int32 Slot, AActor* Target);
	UFUNCTION(Server, Reliable) void ServerBasicAttack();
	FGameplayAbilitySpecHandle BasicAttackHandle;
	UPROPERTY() TArray<FGameplayAbilitySpec> OriginalAbilities;
	TArray<FGameplayAbilitySpecHandle> GrantedHandles;
	bool bCapturedOriginal = false;
};
