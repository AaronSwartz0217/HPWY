// Copyright Druid Mechanics

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CC_PlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
struct FGameplayTag;
class UUserWidget;
class UCC_LoreReader;
class ACC_LoreStand;
class UCC_PlayerScanComponent;
class UCC_WorldNavigationWidget;
class ACC_MinimapCapture;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCC_MenuVisibilityChanged, bool, bOpen);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCC_CombatKeysChanged);

UCLASS()
class CRASHCOURSE_API ACC_PlayerController : public APlayerController
{
	GENERATED_BODY()
public:
 ACC_PlayerController();
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Scan") TObjectPtr<UCC_PlayerScanComponent> Scanner;
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	UFUNCTION(BlueprintCallable,Category="Crash|Travel") void ToggleMinimap();
	UFUNCTION(BlueprintCallable,Category="Crash|Travel") void ToggleTravelMenu();
	UFUNCTION(BlueprintCallable,Category="Crash|Travel") void SetTravelMenuOpen(bool bOpen);
	UFUNCTION(BlueprintPure,Category="Crash|Input") bool IsGameplayInputBlocked() const;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Crash|Travel") TSubclassOf<ACC_MinimapCapture> MinimapClass;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Crash|Travel") TSubclassOf<UCC_WorldNavigationWidget> WorldNavigationClass;
	UFUNCTION(BlueprintPure,Category="Crash|Travel") ACC_MinimapCapture* GetMinimapCamera() const {return MinimapCamera;}
	UFUNCTION(BlueprintPure,Category="Crash|Travel") UCC_WorldNavigationWidget* GetWorldNavigationWidget() const {return WorldNavigation;}
	UFUNCTION(BlueprintCallable, Category="Crash|Menu") void ToggleProgressionMenu();
	UFUNCTION(BlueprintCallable, Category="Crash|Interaction") void RequestInteraction(AActor* Target);
	UFUNCTION(BlueprintPure, Category="Crash|Interaction") AActor* FindInteractionTarget() const;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Crash|Interaction", meta=(ClampMin="1")) float NearbyInteractionSearchRadius = 450.f;
	UFUNCTION(BlueprintPure, Category="Crash|Menu") bool IsProgressionMenuOpen() const;
 UFUNCTION(BlueprintPure, Category="Crash|Menu") UUserWidget* GetProgressionMenuWidget() const { return ProgressionMenu; }
 UFUNCTION(BlueprintCallable, Category="Crash|Menu") void OpenProgressionMenu();
 UFUNCTION(BlueprintCallable, Category="Crash|Menu") void CloseProgressionMenu();
 void CloseLoreReader();
 UCC_LoreReader* GetLoreReader() const { return LoreReader; }
 // Optional designer UMG. Empty keeps the native text menu.
 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Crash|Menu") TSubclassOf<UUserWidget> ProgressionMenuClass;
 UPROPERTY(BlueprintAssignable, Category="Crash|Menu") FCC_MenuVisibilityChanged OnMenuVisibilityChanged;
 UPROPERTY(BlueprintAssignable, Category="Crash|Input") FCC_CombatKeysChanged OnCombatKeysChanged;
 UFUNCTION(BlueprintPure, Category="Crash|Input") FKey GetCombatKey(int32 Action) const;
 UFUNCTION(BlueprintCallable, Category="Crash|Input") bool SetCombatKey(int32 Action, FKey Key);
 UFUNCTION(BlueprintCallable, Category="Crash|Input") void ResetCombatKeys();
	UPROPERTY(EditDefaultsOnly, Category="Crash|Progression") bool bShowProgressionPanel = false;
protected:
	virtual void SetupInputComponent() override;

private:
	bool bSprintInputHeld=false;
 bool bSprintThresholdReached=false;
 double SprintPressedAt=0.0;
 FTimerHandle SprintHoldTimer;
 UPROPERTY(EditDefaultsOnly,Category="Crash|Input|Movement",meta=(ClampMin="0.05",Units="s")) float SprintHoldThreshold=.20f;
	TWeakObjectPtr<class ACC_PlayerCharacter> SprintInputCharacter;
	void StartSprintInput();
 void BeginHeldSprint();
	void ReleaseSprintInput();
	void StopSprintInput();
	UPROPERTY(Transient) TObjectPtr<UCC_WorldNavigationWidget> WorldNavigation;
	UPROPERTY(Transient) TObjectPtr<ACC_MinimapCapture> MinimapCamera;
	bool bTravelInputLocked=false;
	UPROPERTY(Transient) TObjectPtr<UUserWidget> ProgressionMenu;
 bool bMenuInputLocked = false;
 UPROPERTY(Transient) TObjectPtr<UCC_LoreReader> LoreReader;
 void OpenLoreReader(const ACC_LoreStand* Stand);
 UPROPERTY(Transient) TArray<TObjectPtr<UInputMappingContext>> RuntimeContexts;
 UPROPERTY(Transient) TArray<FKey> CombatKeys;
 TArray<FKey> BoundCombatKeys;
 TSet<FKey> ReservedInputKeys;
 void BindCombatKeys();
 void ReleaseRuntimeInputContexts();
 bool IsAllowedCombatKey(FKey Key) const;
 void SaveCombatKeys();

	UPROPERTY(EditDefaultsOnly, Category = "Crash|Input")
	TArray<TObjectPtr<UInputMappingContext>> InputMappingContexts;

	UPROPERTY(EditDefaultsOnly, Category = "Crash|Input|Movement")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, Category = "Crash|Input|Movement")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Crash|Input|Movement")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Crash|Input|Abilities")
	TObjectPtr<UInputAction> PrimaryAction;

	UPROPERTY(EditDefaultsOnly, Category = "Crash|Input|Abilities")
	TObjectPtr<UInputAction> SecondaryAction;

	UPROPERTY(EditDefaultsOnly, Category = "Crash|Input|Abilities")
	TObjectPtr<UInputAction> TertiaryAction;
	UPROPERTY(EditDefaultsOnly, Category="Crash|Input|Abilities") TObjectPtr<UInputAction> FourthAction;
	UPROPERTY(EditDefaultsOnly, Category="Crash|Input|Abilities") TObjectPtr<UInputAction> BasicAttackAction;
	UPROPERTY(EditDefaultsOnly, Category="Crash|Input") TObjectPtr<UInputAction> InteractAction;
	UPROPERTY(EditDefaultsOnly, Category="Crash|Input", meta=(ClampMin="1")) float InteractionTraceDistance = 1500.f;
	void Fourth();
	void BasicAttack();
	void Interact();
	UFUNCTION(Server,Reliable) void ServerReassemblePlatform(class ACC_LevitationPlatform* Platform);
	UFUNCTION(Server, Reliable) void ServerInteractExperiencePack(class ACC_ExperiencePack* Pack);
	void DebugSlot(int32 Slot);
	void DebugForm(int32 Index);
	void DebugPurchase(int32 Index);
 void DeveloperUnlockAll();
 void DeveloperReset();
 void ToggleChromaticTest();
 void UseItemSlot();
 void PressPrimaryMouse();
 void ReleasePrimaryMouse();
	void DebugEvolve();

public:
 UFUNCTION(BlueprintCallable,Category="Crash|Combat HUD") void ToggleCombatHUD();
 AActor* GetHeavenSceneTarget()const;
 UFUNCTION(Server,Reliable) void ServerRegenerateHeavenScene();
 UFUNCTION(Server,Reliable) void ServerToggleHeavenMaze();
 UFUNCTION(Client,Reliable) void ClientHeavenMazeCompleted(int32 Revision);
 void CompleteHeavenMaze(int32 Revision);
 bool IsHeavenCityUnlocked() const { return CompletedHeavenMazeRevision > 0; }
 bool HasCompletedHeavenMaze(int32 Revision) const { return Revision > 0 && CompletedHeavenMazeRevision == Revision; }
private:
 int32 CompletedHeavenMazeRevision = 0;
 void NextHUDSkill();
 void PreviousHUDSkill();
 void PressHUDSkill();
 void ReleaseHUDSkill();
	void Jump();
	void StopJumping();
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Primary();
	void Secondary();
	void Tertiary();
	void ActivateAbility(const FGameplayTag& AbilityTag) const;

	bool IsAlive() const;
};
