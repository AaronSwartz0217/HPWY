// Copyright Druid Mechanics

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"

#include "CC_BaseCharacter.generated.h"

namespace CrashTags
{
	extern CRASHCOURSE_API const FName Player;
}

struct FOnAttributeChangeData;
class UAttributeSet;
class UGameplayAbility;
class UGameplayEffect;
class UCC_SkillComponent;
class UCC_WidgetComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FASCInitialized, UAbilitySystemComponent*, ASC, UAttributeSet*, AS);

UCLASS(Abstract)
class CRASHCOURSE_API ACC_BaseCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACC_BaseCharacter();
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Crash|Animation")
	TObjectPtr<class UCC_LocomotionComponent> Locomotion;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Skills")
	TObjectPtr<UCC_SkillComponent> Skills;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Crash|UI")
	TObjectPtr<UCC_WidgetComponent> HealthDisplay;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const { return nullptr; }
	bool IsAlive() const { return bAlive; }
	// A lightweight state override used by setup/tests. Gameplay deaths use HandleDeath.
	void SetAlive(bool bAliveStatus) { bAlive = bAliveStatus; }
	UFUNCTION(BlueprintPure, Category = "Crash|Death")
	bool IsRagdollActive() const { return bRagdollActive; }

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Death")
	bool bEnableDeathRagdoll = true;

	/** Give the Death state time to collapse before handing the pose to physics. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Death", meta = (ClampMin = "0", ClampMax = "5", Units = "s"))
	float DeathToRagdollDelay = 0.65f;

	/** Enemy subclasses are removed this many seconds after death; zero keeps the corpse. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Death", meta = (ClampMin = "0", Units = "s"))
	float CorpseLifetime = 10.f;

	UPROPERTY(BlueprintAssignable)
	FASCInitialized OnASCInitialized;

	UFUNCTION(BlueprintCallable, Category = "Crash|Death")
	virtual void HandleRespawn();

	UFUNCTION(BlueprintCallable, Category = "Crash|Attributes")
	void ResetAttributes();

	UFUNCTION(BlueprintImplementableEvent)
	void RotateToTarget(AActor* RotateTarget);

	UPROPERTY(EditAnywhere, Category = "Crash|AI")
	float SearchRange{1000.f};

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Crash|Damage")
	float DamageNumberVerticalOffset{200.f};
protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void SetDefaultAttributeEffects(TSubclassOf<UGameplayEffect> Effect) { InitializeAttributesEffect = Effect; ResetAttributesEffect = Effect; }
	void GiveStartupAbilities();
	void InitializeAttributes() const;

	void OnHealthChanged(const FOnAttributeChangeData& AttributeChangeData);
	virtual void HandleDeath();
private:

	UPROPERTY(EditDefaultsOnly, Category = "Crash|Abilities")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;

	UPROPERTY(EditDefaultsOnly, Category = "Crash|Effects")
	TSubclassOf<UGameplayEffect> InitializeAttributesEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Crash|Effects")
	TSubclassOf<UGameplayEffect> ResetAttributesEffect;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), ReplicatedUsing = OnRep_Alive)
	bool bAlive = true;

	UFUNCTION()
	void OnRep_Alive();
	void BeginDeathPresentation();
	void EnterRagdoll();
	void RestoreLivingPresentation();

	FTimerHandle RagdollTimer;
	FTimerHandle CorpseTimer;
	bool bDeathPresentationActive = false;
	bool bRagdollActive = false;
	FTransform LivingMeshRelativeTransform;
	FVector DeathVelocity = FVector::ZeroVector;
	FName LivingMeshCollisionProfile;
	FCollisionResponseContainer LivingMeshCollisionResponses;
	TEnumAsByte<ECollisionChannel> LivingMeshObjectType = ECC_Pawn;
	TEnumAsByte<ECollisionEnabled::Type> LivingMeshCollision = ECollisionEnabled::NoCollision;
	TEnumAsByte<ECollisionEnabled::Type> LivingCapsuleCollision = ECollisionEnabled::QueryAndPhysics;
	uint8 LivingMovementMode = MOVE_Walking;
	uint8 LivingCustomMovementMode = 0;
	bool bLivingMeshPauseAnims = false;
	bool bLivingMeshGravity = true;
	bool bLivingMeshBlendPhysics = false;
};
