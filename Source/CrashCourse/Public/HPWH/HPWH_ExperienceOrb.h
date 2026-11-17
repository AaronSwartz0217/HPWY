#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HPWH_ExperienceOrb.generated.h"

class USphereComponent;
class UNiagaraComponent;
class UPointLightComponent;
class AHPWH_PlayerCharacter;

/**
 * 经验球：沿用 GAS 项目的散射 → 追踪 → 接触拾取逻辑与 NS_ExperienceOrb 特效。
 * 区别是奖励直接进入 HPWH 的 GameMode 经验池，而不是 GAS 属性。
 */
UCLASS()
class CRASHCOURSE_API AHPWH_ExperienceOrb : public AActor
{
	GENERATED_BODY()

public:
	AHPWH_ExperienceOrb();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 在指定位置生成一颗经验球，自动给一个散射初速度。 */
	static void SpawnOrb(UObject* WorldContextObject, const FVector& Origin, int32 ExperienceAmount);

	UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Collision;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UNiagaraComponent> Particles;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Glow;

private:
	void BeginSeeking();
	bool TryCollect(AHPWH_PlayerCharacter* Body);

	int32 ExperienceValue = 5;
	float Age = 0.f;
	float ScatterDuration = 0.35f;
	float SeekSpeed = 320.f;
	float MaximumSeekSpeed = 1700.f;
	float SeekAcceleration = 1500.f;
	float MaximumLifetime = 60.f;
	float CollectRadius = 78.f;
	bool bSeeking = false;
	FVector LaunchVelocity = FVector::ZeroVector;
};
