// Copyright Druid Mechanics

#pragma once

#include "CoreMinimal.h"
#include "CC_BaseCharacter.h"
#include "ActiveGameplayEffectHandle.h"
#include "CC_PlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UMaterialInterface;

USTRUCT()
struct FCC_SprintState
{
 GENERATED_BODY()
 UPROPERTY() bool bActive=false;
 UPROPERTY() float ActiveUntil=0.f;
 UPROPERTY() float CooldownUntil=0.f;
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCC_SprintStateChanged,bool,bActive);

UENUM(BlueprintType)
enum class ECC_CameraMode : uint8
{
	FirstPerson UMETA(DisplayName="第一视角"),
	ThirdPerson UMETA(DisplayName="第三视角"),
	TopDown UMETA(DisplayName="俯视角")
};

UCLASS()
class CRASHCOURSE_API ACC_PlayerCharacter : public ACC_BaseCharacter
{
	GENERATED_BODY()

public:
	ACC_PlayerCharacter();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
 void RefreshChromaticAppearance();
 void ClearChromaticAppearance();
 UFUNCTION(BlueprintPure, Category="Appearance|Chromatic") bool IsChromaticMaterialApplied() const {return bChromaticMaterialApplied;}
 UPROPERTY(EditDefaultsOnly, Category="Appearance|Chromatic") TSoftObjectPtr<UMaterialInterface> ChromaticBodyMaterial;
 UPROPERTY(EditDefaultsOnly, Category="Appearance|Chromatic") TSoftObjectPtr<UMaterialInterface> ChromaticOutlineMaterial;
	UFUNCTION(BlueprintCallable, Category="Camera") void SetFirstPerson(bool bEnabled);
	UFUNCTION(BlueprintPure, Category="Camera") bool IsFirstPerson() const {return CameraMode==ECC_CameraMode::FirstPerson;}
	UFUNCTION(BlueprintCallable, Category="Camera") void SetCameraMode(ECC_CameraMode NewMode);
	UFUNCTION(BlueprintPure, Category="Camera") ECC_CameraMode GetCameraMode() const {return CameraMode;}
	UFUNCTION(BlueprintPure, Category="Camera") bool IsTopDown() const {return CameraMode==ECC_CameraMode::TopDown;}
	UFUNCTION(BlueprintPure, Category="Camera") UCameraComponent* GetFirstPersonCamera() const {return FirstPersonCamera;}
	UFUNCTION(BlueprintPure, Category="Camera") UCameraComponent* GetTopDownCamera() const {return TopDownCamera;}
	UFUNCTION(BlueprintPure, Category="Camera") USpringArmComponent* GetTopDownBoom() const {return TopDownBoom;}
	UPROPERTY(EditDefaultsOnly, Category="Camera") FVector FirstPersonEyeOffset=FVector(0,0,65);
	UPROPERTY(EditDefaultsOnly, Category="Camera|TopDown", meta=(ClampMin="300", Units="cm")) float TopDownArmLength=1500.f;
	UPROPERTY(EditDefaultsOnly, Category="Camera|TopDown", meta=(ClampMin="-89", ClampMax="-20")) float TopDownPitch=-70.f;
	UPROPERTY(EditDefaultsOnly, Category="Camera|TopDown", meta=(Units="cm")) float TopDownTargetHeight=0.f;
	UFUNCTION(BlueprintPure, Category="Camera") USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	UFUNCTION(BlueprintPure, Category="Camera") UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void HandleRespawn() override;
	virtual void UnPossessed() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Skills|Sprint",meta=(ClampMin="1.01")) float SprintSpeedMultiplier=1.5f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Skills|Sprint",meta=(ClampMin="0.01",Units="s")) float SprintMaxDuration=5.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Skills|Sprint",meta=(ClampMin="0",Units="s")) float SprintCooldown=5.f;
 UPROPERTY(EditDefaultsOnly,Category="Skills|Dodge",meta=(ClampMin="10",Units="cm")) float DodgeDistance=300.f;
 UPROPERTY(EditDefaultsOnly,Category="Skills|Dodge",meta=(ClampMin="0.1",Units="s")) float DodgeCooldown=1.f;
 UPROPERTY(EditDefaultsOnly,Category="Skills|Dodge") TSoftObjectPtr<UMaterialInterface> DodgeAfterimageMaterial{FSoftObjectPath(TEXT("/Game/HPWH/Effects/Dodge/M_DodgeAfterimage.M_DodgeAfterimage"))};
 void RequestDodge(FVector Direction);
	UPROPERTY(BlueprintAssignable,Category="Skills|Sprint") FCC_SprintStateChanged OnSprintStateChanged;
	UFUNCTION(BlueprintCallable,Category="Skills|Sprint") void SetSprintHeld(bool bHeld);
	UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Skills|Sprint") void ResetSprintState();
	UFUNCTION(BlueprintPure,Category="Skills|Sprint") bool IsSprinting() const {return SprintState.bActive;}
	UFUNCTION(BlueprintPure,Category="Skills|Sprint") bool CanSprint() const;
	UFUNCTION(BlueprintPure,Category="Skills|Sprint") float GetSprintTimeRemaining() const;
	UFUNCTION(BlueprintPure,Category="Skills|Sprint") float GetSprintCooldownRemaining() const;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Travel",meta=(ClampMin="0.1")) float RestRespawnDelay=5.f;
protected:
	virtual void HandleDeath() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
 bool bChromaticMaterialApplied=false;
 UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInterface>> PreChromaticOverrides;
 UPROPERTY(Transient) TObjectPtr<UMaterialInterface> PreChromaticOverlay;
	UPROPERTY(ReplicatedUsing=OnRep_SprintState) FCC_SprintState SprintState;
	TWeakObjectPtr<UAbilitySystemComponent> SprintASC;
	FActiveGameplayEffectHandle SprintEffect;
	bool bSprintHeld=false;
	float GetSprintClock() const;
	void HandleSprintInput(bool bHeld);
	void FinishSprint();
	void RemoveSprintEffect();
	UFUNCTION() void OnRep_SprintState();
	UFUNCTION(Server,Reliable) void ServerSetSprintHeld(bool bHeld);
 UFUNCTION(Server,Reliable) void ServerRequestDodge(FVector_NetQuantizeNormal Direction);
 UFUNCTION(NetMulticast,Unreliable) void MulticastDodgeTrail(FVector_NetQuantize From, FVector_NetQuantize To);
 void ExecuteDodge(FVector Direction);
 float DodgeReadyAt=0.f;
	bool bStartupGrantedOnThisAvatar = false;
	FTimerHandle RestReturnTimer;
	bool bCameraHidesOwner = false;
	UPROPERTY(ReplicatedUsing=OnRep_CameraMode) ECC_CameraMode CameraMode=ECC_CameraMode::TopDown;
	UPROPERTY(EditDefaultsOnly, Category="Camera|Damage", meta=(ClampMin="0.2", Units="s")) float DamageFirstPersonDuration=3.f;
	FTimerHandle DamageFirstPersonTimer;
	float LastObservedHealth=-1.f;
	void CheckDamageCamera();
	UFUNCTION() void ReturnToTopDown();
	UPROPERTY(VisibleAnywhere, Category="Camera") TObjectPtr<UCameraComponent> FirstPersonCamera;
	UPROPERTY(VisibleAnywhere, Category="Camera") TObjectPtr<USpringArmComponent> TopDownBoom;
	UPROPERTY(VisibleAnywhere, Category="Camera") TObjectPtr<UCameraComponent> TopDownCamera;
	UFUNCTION() void OnRep_CameraMode();
	UFUNCTION(Server,Reliable) void ServerSetCameraMode(ECC_CameraMode NewMode);
	void ApplyCameraMode();
	void UpdateTopDownRotation();

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;
};
