#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HPWH/HPWH_Types.h"
#include "HPWH_Enemies.generated.h"

class AHPWH_WorldDirector;
class AHPWH_PlayerCharacter;
class AHPWH_SoulAvatar;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UPointLightComponent;

/** 普通怪物基类：方块/通用人模型占位，只造成生命伤害。 */
UCLASS()
class CRASHCOURSE_API AHPWH_EnemyBase : public ACharacter
{
	GENERATED_BODY()

public:
	AHPWH_EnemyBase();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	void InitEnemy(AHPWH_WorldDirector* InDirector, const FIntPoint& InCell, bool bInCounted);
	virtual void ReceiveHPWHDamage(float Damage);
	UFUNCTION(BlueprintPure, Category = "HPWH|Enemy") bool IsDead() const { return bDead; }

protected:
	virtual void TickBehavior(float DeltaSeconds, AHPWH_PlayerCharacter* Target);
	void TickPatrol(float DeltaSeconds);
	virtual void Die();
	/** 灵魂离体时锁定灵魂，否则锁定肉身；肉身不会同时被普通怪锁定。 */
	AActor* FindTarget() const;
	/** 灵魂离体时接近灵魂；接触即杀死灵魂。 */
	void TickChaseSoul(float DeltaSeconds, AHPWH_SoulAvatar* Soul);
	virtual FLinearColor GetTintColor() const { return FLinearColor(0.55f, 0.45f, 0.42f); }

	UPROPERTY() TObjectPtr<AHPWH_WorldDirector> Director;
	UPROPERTY() FIntPoint SourceCell = FIntPoint::ZeroValue;
	UPROPERTY() bool bCountedEncounter = true;

	UPROPERTY(EditDefaultsOnly, Category = "HPWH|Enemy") float MaxHealth = 10.f;
	UPROPERTY(EditDefaultsOnly, Category = "HPWH|Enemy") float MoveSpeed = 300.f;
	/** 相对角色基础移速的倍率：近战 1.2、远程 0.8、其他按需配置。 */
	UPROPERTY(EditDefaultsOnly, Category = "HPWH|Enemy") float SpeedMultiplier = 1.f;
	UPROPERTY(EditDefaultsOnly, Category = "HPWH|Enemy") float AggroRadius = 1200.f;
	UPROPERTY(EditDefaultsOnly, Category = "HPWH|Enemy") float PatrolRadius = 420.f;

	float Health = 10.f;
	float NextAttackTime = 0.f;
	bool bDead = false;

	FVector SpawnLocation = FVector::ZeroVector;
	FVector PatrolTarget = FVector::ZeroVector;
	float NextPatrolTime = 0.f;
	float NextDebugLogTime = 0.f;
};

UCLASS()
class CRASHCOURSE_API AHPWH_MeleeMonster : public AHPWH_EnemyBase
{
	GENERATED_BODY()

public:
	AHPWH_MeleeMonster();

protected:
	virtual void TickBehavior(float DeltaSeconds, AHPWH_PlayerCharacter* Target) override;
	virtual FLinearColor GetTintColor() const override { return FLinearColor(0.46f, 0.20f, 0.18f); }
};

UCLASS()
class CRASHCOURSE_API AHPWH_RangedMonster : public AHPWH_EnemyBase
{
	GENERATED_BODY()

public:
	AHPWH_RangedMonster();

protected:
	virtual void TickBehavior(float DeltaSeconds, AHPWH_PlayerCharacter* Target) override;
	virtual FLinearColor GetTintColor() const override { return FLinearColor(0.22f, 0.30f, 0.52f); }
};

/** 五铢钱精怪：使用原动画蓝图状态机，可小范围巡逻；碰到只叫并给一次经验。 */
UCLASS()
class CRASHCOURSE_API AHPWH_CoinMonster : public ACharacter
{
	GENERATED_BODY()

public:
	AHPWH_CoinMonster();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<USkeletalMeshComponent> CoinMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> CoinLight;

private:
	float NextVoiceTime = 0.f;
	float BobTime = 0.f;
	bool bExperienceGranted = false;
	FVector SpawnLocation = FVector::ZeroVector;
	FVector PatrolTarget = FVector::ZeroVector;
	float NextPatrolTime = 0.f;
};
