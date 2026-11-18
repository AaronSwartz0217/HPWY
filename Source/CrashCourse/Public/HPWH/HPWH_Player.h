#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HPWH_Player.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class USphereComponent;
class UProjectileMovementComponent;
class AHPWH_SoulAvatar;

/** 初墨：俯视角肉身，被匍匐者命中后短暂强制第一人称。 */
UCLASS()
class CRASHCOURSE_API AHPWH_PlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AHPWH_PlayerCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "HPWH|Camera") bool IsFirstPerson() const { return false; }
	UFUNCTION(BlueprintPure, Category = "HPWH|Camera") bool IsFrightened() const { return bFrightened; }
	void SetFrightened(float Duration);
	void ExitFrightened();

	bool TryAttack();
	void TakeHPWHDamage(float Damage, bool bTriggerFrightened);
	void RestoreFullHealth();
	void AddMaxHealth(float Amount);
	void HealHPWH(float Amount);
	void SetHealthForLoad(float NewHealth);
	void ResetMaxHealthForLoad();
	void RefreshMoveSpeed();

	UFUNCTION(BlueprintPure, Category = "HPWH|Health") float GetHealth() const { return Health; }
	UFUNCTION(BlueprintPure, Category = "HPWH|Health") float GetMaxHealth() const { return MaxHealth; }
	UFUNCTION(BlueprintPure, Category = "HPWH|Health") int32 GetHitCount() const { return HitCount; }
	UFUNCTION(BlueprintPure, Category = "HPWH|Combat") float GetBulletCooldownRemaining() const;
	UFUNCTION(BlueprintPure, Category = "HPWH|Combat") float GetBulletCooldownDuration() const;

	void EnterSoulOut();
	void ReturnToBody();
	void ForceReturnToBody();
	UFUNCTION(BlueprintPure, Category = "HPWH|Soul") bool IsSoulOut() const { return bSoulOut; }

	void ForceReset();
	void SetRunLocked(bool bLocked);
	UFUNCTION(BlueprintPure, Category = "HPWH|State") bool IsRunLocked() const { return bRunLocked; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "HPWH|Camera") TObjectPtr<USpringArmComponent> TopDownBoom;
	UPROPERTY(VisibleAnywhere, Category = "HPWH|Camera") TObjectPtr<UCameraComponent> TopDownCamera;
	UPROPERTY(VisibleAnywhere, Category = "HPWH|Light") TObjectPtr<UPointLightComponent> SoulLamp;
	UPROPERTY(VisibleAnywhere, Category = "HPWH|Light") TObjectPtr<UStaticMeshComponent> SoulLampVisual;

	UPROPERTY(EditDefaultsOnly, Category = "HPWH|Camera") float TopDownArmLength = 1350.f;
	UPROPERTY(EditDefaultsOnly, Category = "HPWH|Camera") float TopDownPitch = -72.f;

	UPROPERTY(EditDefaultsOnly, Category = "HPWH|Combat") float MaxHealth = 30.f;
	UPROPERTY(EditDefaultsOnly, Category = "HPWH|Combat") float AttackDamage = 10.f;
	UPROPERTY(EditDefaultsOnly, Category = "HPWH|Combat") float AttackRangeCm = 600.f;
	UPROPERTY(EditDefaultsOnly, Category = "HPWH|Combat") float BaseBulletCooldown = 2.f;
	UPROPERTY(EditDefaultsOnly, Category = "HPWH|Camera") float FrightenedDuration = 2.5f;

private:
	float Health = 100.f;
	int32 HitCount = 0;
	float NextAttackTime = 0.f;
	float ShotCooldownEnd = 0.f;
	float AttackFlashEnd = 0.f;
	float NextDamageTime = 0.f;
	float FootstepTimer = 0.f;
	bool bFrightened = false;
	float FrightenedEnd = 0.f;
	bool bSoulOut = false;
	bool bRunLocked = false;
	bool bDead = false;
};

/** 离体灵魂：只能移动和寻找肉身，无法攻击、拾取灯芯或使用出口。 */
UCLASS()
class CRASHCOURSE_API AHPWH_SoulAvatar : public ACharacter
{
	GENERATED_BODY()

public:
	AHPWH_SoulAvatar();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void TryReturnToBody();
	UFUNCTION(BlueprintPure, Category = "HPWH|Soul") bool CanReturnToBody() const;
	void RefreshMoveSpeed();

	UPROPERTY(VisibleAnywhere, Category = "HPWH|Camera") TObjectPtr<USpringArmComponent> TopDownBoom;
	UPROPERTY(VisibleAnywhere, Category = "HPWH|Camera") TObjectPtr<UCameraComponent> TopDownCamera;
	UPROPERTY(VisibleAnywhere, Category = "HPWH|Visual") TObjectPtr<UStaticMeshComponent> GhostBody;
	UPROPERTY(VisibleAnywhere, Category = "HPWH|Light") TObjectPtr<UPointLightComponent> SoulLight;

private:
	bool bContactDeathTriggered = false;
};

/** 魂灯弹与远程器物怪的弹丸。 */
UCLASS()
class CRASHCOURSE_API AHPWH_Projectile : public AActor
{
	GENERATED_BODY()

public:
	AHPWH_Projectile();

	virtual void Tick(float DeltaSeconds) override;
	void InitProjectile(const FVector& Direction, float Speed, float InDamage, bool bInHostile, AActor* InInstigator);

	UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Collision;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Visual;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UProjectileMovementComponent> Movement;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Glow;

private:
	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	float Damage = 14.f;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Trail;
	float TrailAge = 0.f;
	bool bHostile = false;
};
