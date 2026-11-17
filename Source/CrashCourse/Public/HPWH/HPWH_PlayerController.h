#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "HPWH/HPWH_Types.h"
#include "HPWH_PlayerController.generated.h"

class UHPWH_HUDWidget;

UCLASS()
class CRASHCOURSE_API AHPWH_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AHPWH_PlayerController();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupInputComponent() override;

	void Interact();
	AActor* FindNearestInteractable() const;
	bool GetAttackAimPoint(FVector& OutPoint) const;
	bool HasManualAim() const { return bManualAim; }
	FString GetInteractPrompt() const { return CachedPrompt; }
	FVector2D GetJoystickVisual() const { return JoystickInput; }
	/** 攻击键按下时的 4px 微动偏移。 */
	FVector2D GetAttackVisualOffset() const { return AttackVisualOffset; }

protected:
	void TouchPressed(ETouchIndex::Type FingerIndex, FVector Location);
	void TouchMoved(ETouchIndex::Type FingerIndex, FVector Location);
	void TouchReleased(ETouchIndex::Type FingerIndex, FVector Location);

	void ForwardPressed();
	void ForwardReleased();
	void BackPressed();
	void BackReleased();
	void RightPressed();
	void RightReleased();
	void LeftPressed();
	void LeftReleased();
	void AttackPressed();
	void AttackReleased();
	void LookPressed();
	void LookReleased();
	void InteractPressed();

	void UpdateInteractPrompt();
	bool TryInteractNearby();
	void HandleMenuPoint(const FVector2D& Point);
	void HandleInGameMenuPoint(const FVector2D& Point);
	FVector2D GetViewportSize2D() const;

private:
	UPROPERTY() TObjectPtr<UHPWH_HUDWidget> HUDWidget;

	bool bForward = false;
	bool bBack = false;
	bool bRight = false;
	bool bLeft = false;
	bool bMouseAttack = false;
	bool bTouchAttack = false;
	bool bLookHeld = false;
	bool bManualAim = false;

	int32 JoystickFinger = -1;
	int32 AttackFinger = -1;
	int32 LookFinger = -1;
	FVector2D JoystickOrigin = FVector2D::ZeroVector;
	FVector2D JoystickInput = FVector2D::ZeroVector;
	FVector2D LookLast = FVector2D::ZeroVector;
	FVector2D TouchAimScreen = FVector2D::ZeroVector;
	FVector2D AttackOrigin = FVector2D::ZeroVector;
	FVector2D AttackVisualOffset = FVector2D::ZeroVector;

	FString CachedPrompt;
	float NextPromptUpdate = 0.f;
};
