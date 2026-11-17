#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HPWH/HPWH_POI.h"
#include "HPWH_Companion.generated.h"

class AHPWH_WorldDirector;
class AHPWH_PlayerCharacter;
class UPointLightComponent;

UENUM(BlueprintType)
enum class EHPWH_CompanionState : uint8
{
	Waiting UMETA(DisplayName = "等待结伴"),
	Following UMETA(DisplayName = "跟随"),
	Hostile UMETA(DisplayName = "模拟人"),
	Down UMETA(DisplayName = "倒下")
};

/** 随机生成的特殊角色：可结伴、跟随、协同射击；玩家受伤后有 50% 概率叛变为“模拟人”。 */
UCLASS()
class CRASHCOURSE_API AHPWH_Companion : public ACharacter, public IHPWH_Interactable
{
	GENERATED_BODY()

public:
	AHPWH_Companion();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void InitCompanion(AHPWH_WorldDirector* InDirector, const FIntPoint& InCell);
	void ResetForRespawn();

	/** 玩家受伤回调：仅跟随状态参与 50% 叛变判定。 */
	void NotifyPlayerHurt();
	/** 调试用：立即叛变。 */
	void ForceBetray();
	/** 玩家子弹命中：仅敌对状态生效。 */
	void ReceiveHPWHDamage(float Damage);

	UFUNCTION(BlueprintPure, Category = "HPWH|Companion") bool IsHostile() const { return State == EHPWH_CompanionState::Hostile; }
	UFUNCTION(BlueprintPure, Category = "HPWH|Companion") bool IsFollowing() const { return State == EHPWH_CompanionState::Following; }
	UFUNCTION(BlueprintPure, Category = "HPWH|Companion") EHPWH_CompanionState GetState() const { return State; }
	UFUNCTION(BlueprintPure, Category = "HPWH|Companion") FVector GetRespawnLocation() const { return SpawnLocation; }

	virtual FString GetHPWHInteractPrompt(APawn* Interactor) const override;
	virtual float GetHPWHInteractRange() const override { return 300.f; }
	virtual bool CanHPWHInteract(APawn* Interactor) const override;
	virtual void HPWHInteract(APawn* Interactor) override;

	UPROPERTY(VisibleAnywhere, Category = "HPWH|Companion") TObjectPtr<UPointLightComponent> CompanionLight;

protected:
	void JoinParty();
	void Betray();
	void TickFollow(float DeltaSeconds, AHPWH_PlayerCharacter* Player, float Distance);
	void TickHostile(float DeltaSeconds, AHPWH_PlayerCharacter* Player, float Distance);
	void TryFireAt(AActor* Target, bool bHostileShot);
	AActor* FindNearestEnemy() const;
	bool HasLineOfSightTo(const AActor* Target) const;
	void UpdatePresentation();
	void Die();

	UPROPERTY() TObjectPtr<AHPWH_WorldDirector> Director;
	UPROPERTY() FIntPoint SourceCell = FIntPoint::ZeroValue;
	UPROPERTY(EditDefaultsOnly, Category = "HPWH|Companion") float MaxHealth = 60.f;
	UPROPERTY(EditDefaultsOnly, Category = "HPWH|Companion") float FollowSpeed = 520.f;
	UPROPERTY(EditDefaultsOnly, Category = "HPWH|Companion") float FriendlyShotDamage = 6.f;
	UPROPERTY(EditDefaultsOnly, Category = "HPWH|Companion") float ShotInterval = 1.8f;
	UPROPERTY(EditDefaultsOnly, Category = "HPWH|Companion") float ShotRange = 1000.f;

	float Health = 60.f;
	EHPWH_CompanionState State = EHPWH_CompanionState::Waiting;
	FVector SpawnLocation = FVector::ZeroVector;
	float NextShotTime = 0.f;
	float NextMeleeTime = 0.f;
	float NextBetrayalRollTime = 0.f;
	bool bGreeted = false;
	bool bDead = false;
};
