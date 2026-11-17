#pragma once

#include "CoreMinimal.h"
#include "HPWH/HPWH_Enemies.h"
#include "HPWH_SoulSpider.generated.h"

class USceneComponent;
class UStaticMeshComponent;

UENUM()
enum class EHPWH_SpiderState : uint8
{
	PlayingDead,
	FlipToUpright,
	SlowChase
};

/** 摄魂蜘蛛：唯一能让玩家灵魂离体的怪物。 */
UCLASS()
class CRASHCOURSE_API AHPWH_SoulSpider : public AHPWH_EnemyBase
{
	GENERATED_BODY()

public:
	AHPWH_SoulSpider();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void ReceiveHPWHDamage(float Damage) override;

	UFUNCTION(BlueprintPure, Category = "HPWH|Spider") bool IsPlayingDead() const { return State == EHPWH_SpiderState::PlayingDead; }

protected:
	virtual void TickBehavior(float DeltaSeconds, AHPWH_PlayerCharacter* Target) override;
	virtual FLinearColor GetTintColor() const override { return FLinearColor(0.13f, 0.13f, 0.15f); }

	void ActivateSpider();
	void ApplyPose(bool bDormant, float FlipAlpha);

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> VisualRoot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> BodyBox;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> ImportedVisual;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Legs;

	EHPWH_SpiderState State = EHPWH_SpiderState::PlayingDead;
	float FlipEnd = 0.f;
};
