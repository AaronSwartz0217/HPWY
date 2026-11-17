#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CC_GameplayData.generated.h"

class UGameplayAbility;
class USkeletalMesh;
class UAnimInstance;
class UNiagaraSystem;
class UParticleSystem;
class UTexture2D;
class UMaterialInterface;

USTRUCT(BlueprintType)
struct FCC_CombatProfile
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName ProfileId;
	// Optional parent row. Elite rows inherit advanced values instead of duplicating them.
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName ParentProfileId;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="-1")) int32 ExperienceReward = -1;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="-1")) int32 CopperReward = -1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<USkeletalMesh> Mesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.01")) float HealthMultiplier = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float AttackMultiplier = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bElite = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) float EliteMultiplier = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.1")) float EliteScale = 1.2f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<class UCC_SkillLoadout> Loadout;
};

// Shared by evolution stages and enemies: change the baseline or a row in one place.
UCLASS(BlueprintType)
class CRASHCOURSE_API UCC_CombatBalance : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) float BaseHealth = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float BaseAttack = 15.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FCC_CombatProfile> Profiles;
	int32 ResolveExperienceReward(FName Id, const USkeletalMesh* Mesh = nullptr) const;
 int32 ResolveCopperReward(FName Id,const USkeletalMesh* Mesh=nullptr) const;
 const FCC_CombatProfile* FindProfile(FName Id, const USkeletalMesh* Mesh = nullptr) const;
	bool Resolve(FName Id, float& Health, float& Attack, const USkeletalMesh* Mesh = nullptr) const;
};

UENUM(BlueprintType)
enum class ECC_SkillPowerMode : uint8 { Flat, AttackMultiplier };

UENUM(BlueprintType)
enum class ECC_AttackPose : uint8 { Jab, CrossJab, Uppercut, None, Cast, Guard };

UENUM(BlueprintType)
enum class ECC_SkillBehavior : uint8
{
	Melee, Combo, Sweep, Dash, Projectile, Spread, Homing, Piercing,
	Burst, GroundStrike, DamageZone, LineStrike, Heal, AreaHeal,
	Shield, DefenseBuff, AttackBuff, SpeedBuff, Slow, Knockback
};

UENUM(BlueprintType)
enum class ECC_DesignMechanic : uint8 { None, HealPercent, Summon, StatBoost, Charge, Guard, Invulnerable, Gravity, PulseBurst, CleanseShield, Stealth, Regrowth, Lifesteal, RootArea };

UCLASS(BlueprintType)
class CRASHCOURSE_API UCC_SkillDefinition : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName SkillId;
 // Empty = general skill. Otherwise this power must be acquired, independent of active form.
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Unlock") FName RequiredFormId;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Design") ECC_DesignMechanic DesignMechanic=ECC_DesignMechanic::None;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Design") TArray<float> DamageSteps;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Design") float HealFraction=0.f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Design") float AttackFactor=1.f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Design") float DefenseFactor=1.f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Design") float SpeedFactor=1.f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Design") float ControlDuration=0.f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Design") float ShieldHealthFraction=0.f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Design") float ShieldDefenseFactor=0.f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Design") int32 MaxTargets=0;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Design") float MaxChargeSeconds=2.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(MultiLine="true")) FText Description;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Balance") ECC_SkillPowerMode PowerMode = ECC_SkillPowerMode::Flat;
	UFUNCTION(BlueprintPure, Category="Skills") float CalculatePower(float Attack, int32 Level = 1) const;
	UFUNCTION(BlueprintPure, Category="Skills") FText GetNumericDescription(float Attack, int32 Level = 1) const;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) ECC_SkillBehavior Behavior = ECC_SkillBehavior::Melee;
	// Optional custom GA. Empty uses the built-in configurable ability.
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TSubclassOf<UGameplayAbility> AbilityClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") ECC_AttackPose AttackPose = ECC_AttackPose::Jab;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") bool bAlternateAttackPose = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation", meta=(ClampMin="0.1")) float AnimationPlayRate = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation", meta=(ClampMin="0", ClampMax="3")) float Windup = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TObjectPtr<UParticleSystem> CastParticles;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TObjectPtr<UParticleSystem> ImpactParticles;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TObjectPtr<UNiagaraSystem> CastNiagara;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TObjectPtr<UNiagaraSystem> ImpactNiagara;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TObjectPtr<UNiagaraSystem> ProjectileNiagara;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Presentation") TObjectPtr<UMaterialInterface> ProjectileCoreMaterial;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Presentation") TObjectPtr<UMaterialInterface> ShieldVisualMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation", meta=(ClampMin="0.01")) float EffectScale = 0.35f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation", meta=(ClampMin="0.1", ClampMax="10")) float EffectLifetime = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float Power = 20.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float PowerPerLevel = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float ManaCost = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.1")) float Cooldown = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) float Range = 300.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) float Radius = 150.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float Duration = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.05")) float Interval = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1", ClampMax="32")) int32 Count = 3;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="180")) float Angle = 90.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) float ProjectileSpeed = 1200.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float KnockbackForce = 600.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="1")) float SlowFraction = 0.4f;
};

USTRUCT(BlueprintType)
struct FCC_SkillSlot
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UCC_SkillDefinition> Skill;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) int32 Level = 1;
};

UCLASS(BlueprintType)
class CRASHCOURSE_API UCC_SkillLoadout : public UDataAsset
{
	GENERATED_BODY()
public:
	UCC_SkillLoadout() { Slots.SetNum(4); }
	UPROPERTY(EditAnywhere, BlueprintReadOnly, EditFixedSize) TArray<FCC_SkillSlot> Slots;
	bool IsComplete() const;
};

USTRUCT(BlueprintType)
struct FCC_FormStage
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) int32 Cost = 100;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) float MaxHealth = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float MaxMana = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float Attack = 15.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Balance") TObjectPtr<UCC_CombatBalance> CombatBalance;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Balance") FName StatProfileId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float Defense = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) float MoveSpeed = 500.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UCC_SkillLoadout> Loadout;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<USkeletalMesh> Mesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TSubclassOf<UAnimInstance> AnimClass;
	// Absolute component scale for an explicit Mesh; multiplier of the original mesh otherwise.
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector MeshScale = FVector::OneVector;
 // Menu artwork follows the actual form and tier. Empty uses the menu's fallback art.
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Menu") TSoftObjectPtr<UTexture2D> MenuPortrait;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Menu") TSoftObjectPtr<UTexture2D> MenuTitle;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Menu") FText MenuDisplayName;
};

UCLASS(BlueprintType)
class CRASHCOURSE_API UCC_FormDefinition : public UDataAsset
{
	GENERATED_BODY()
public:
	UCC_FormDefinition() { Stages.SetNum(3); }
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName FormId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bAvailable = true;
	// Initial ink grows with experience instead of selling three-stage evolution.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Progression") bool bCanEvolve = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory") TSoftObjectPtr<UTexture2D> InventoryIcon;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory") FText InventoryName;
 UFUNCTION(BlueprintPure, Category="Inventory") FText GetInventoryName() const;
	// Indices: 0 initial, 1 intermediate, 2 advanced.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, EditFixedSize) TArray<FCC_FormStage> Stages;
	bool IsComplete() const;
};

UENUM(BlueprintType)
enum class ECC_UpgradeType : uint8 { MaxHealth, MaxMana, Attack, Defense, MoveSpeed, SkillLevel };

UCLASS(BlueprintType)
class CRASHCOURSE_API UCC_UpgradeDefinition : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName UpgradeId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) ECC_UpgradeType Type = ECC_UpgradeType::MaxHealth;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) TArray<int32> Prices;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float BonusPerLevel = 10.f;
	// Used only for SkillLevel; purchased levels are integer increments of one.
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName SkillId;
};
