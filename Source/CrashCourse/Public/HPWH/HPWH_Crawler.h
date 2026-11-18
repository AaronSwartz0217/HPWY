#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HPWH/HPWH_Types.h"
#include "HPWH_Crawler.generated.h"

class AHPWH_WorldDirector;
class UStaticMeshComponent;
class UPointLightComponent;

/** 匍匐者：不可被常规杀死的巨型追逐者。 */
UCLASS()
class CRASHCOURSE_API AHPWH_Crawler : public ACharacter
{
	GENERATED_BODY()

public:
	AHPWH_Crawler();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void SetDirector(AHPWH_WorldDirector* InDirector) { Director = InDirector; }
	void Activate(bool bFromSoulMark);
	void ResetDormant();
	void Disrupt(float Duration);
	void DebugPlaceNearPlayer(float DistanceCm);

	UFUNCTION(BlueprintPure, Category = "HPWH|Crawler") EHPWH_CrawlerState GetCrawlerState() const { return State; }
	UFUNCTION(BlueprintPure, Category = "HPWH|Crawler") float GetCurrentSpeed() const { return CurrentSpeed; }

	float GetAnimationSpeed() const { return AnimationSpeed; }

	UPROPERTY(VisibleAnywhere, Category = "HPWH|Crawler") TObjectPtr<UStaticMeshComponent> EyeLeft;
	UPROPERTY(VisibleAnywhere, Category = "HPWH|Crawler") TObjectPtr<UStaticMeshComponent> EyeRight;
	UPROPERTY(VisibleAnywhere, Category = "HPWH|Crawler") TObjectPtr<UPointLightComponent> EyeLight;

private:
	AActor* ResolveTarget() const;
	void AttackTarget(AActor* Target);
	void RelocateNearPlayer();
	void UpdatePresentation();
	float ComputeCurrentSpeed(const APawn* PlayerBody) const;

	UPROPERTY() TObjectPtr<AHPWH_WorldDirector> Director;
	UPROPERTY() EHPWH_CrawlerState State = EHPWH_CrawlerState::Dormant;

	FVector Anchor = FVector(5000.f, 5000.f, 300.f);
	/** 逻辑位置，常驻计算，精度 0.1 米；只有进入可见格才显示实体。 */
	FVector LogicalLocation = FVector(5000.f, 5000.f, 300.f);
	float NextAttackTime = 0.f;
	float DisruptEnd = 0.f;
	float LastRelocateTime = -100.f;
	float CurrentSpeed = 0.f;
	float AnimationSpeed = 0.f;
	bool bUsingImportedVisual = false;
	float LastLoggedSpeed = -1.f;
	bool bPresentationVisible = false;
};
