#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HPWH/HPWH_Types.h"
#include "HPWH/HPWH_Upgrades.h"
#include "HPWH/HPWH_Menu.h"
#include "HPWH_GameMode.generated.h"

class AHPWH_WorldDirector;
class AHPWH_PlayerCharacter;
class AHPWH_SoulAvatar;
class AHPWH_Crawler;
class AHPWH_Companion;
class AHPWH_NextFloorPortal;
class USoundBase;
class UAudioComponent;

/** 统一游戏状态协调器：楼层、灵魂、标记、概率与消息。 */
UCLASS()
class CRASHCOURSE_API AHPWH_GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AHPWH_GameMode();

	virtual void StartPlay() override;
	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditDefaultsOnly, Category = "HPWH|Run") int32 InitialRunSeed = 20261117;
	UPROPERTY(EditDefaultsOnly, Category = "HPWH|Run") float PlayerSpawnHeight = 120.f;

	UPROPERTY(BlueprintReadOnly, Category = "HPWH|Run") int32 RunSeed = 20261117;
	UPROPERTY(BlueprintReadOnly, Category = "HPWH|Run") int32 LayerIndex = 1;
	UPROPERTY(BlueprintReadOnly, Category = "HPWH|Run") int32 DeathCount = 0;
	UPROPERTY(BlueprintReadOnly, Category = "HPWH|Run") float RawChance = 5.f;
	UPROPERTY(BlueprintReadOnly, Category = "HPWH|Run") float RollChance = 5.f;
	UPROPERTY(BlueprintReadOnly, Category = "HPWH|Run") float ChancePenalty = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "HPWH|Run") bool bRunImpossible = false;
	UPROPERTY(BlueprintReadOnly, Category = "HPWH|Run") int32 CrawlerSpeedLevel = 0;
	UPROPERTY(BlueprintReadOnly, Category = "HPWH|Run") int32 PuzzleFailCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "HPWH|State") EHPWH_FloorState FloorState = EHPWH_FloorState::Exploring;
	UPROPERTY(BlueprintReadOnly, Category = "HPWH|State") EHPWH_SoulState SoulState = EHPWH_SoulState::Embodied;
	UPROPERTY(BlueprintReadOnly, Category = "HPWH|State") EHPWH_MarkState MarkState = EHPWH_MarkState::Unmarked;
	UPROPERTY(BlueprintReadOnly, Category = "HPWH|State") bool bCoreCollected = false;

	UFUNCTION(BlueprintPure, Category = "HPWH") AHPWH_WorldDirector* GetDirector();
	UFUNCTION(BlueprintPure, Category = "HPWH") AHPWH_PlayerCharacter* GetPlayerBody();
	UFUNCTION(BlueprintPure, Category = "HPWH") AHPWH_SoulAvatar* GetSoulAvatar() const { return SoulAvatar; }
	UFUNCTION(BlueprintPure, Category = "HPWH") AHPWH_Crawler* GetCrawler();
	UFUNCTION(BlueprintPure, Category = "HPWH") AHPWH_Companion* GetCompanion();
	/** 玩家受伤：把 50% 叛变判定交给同伴。 */
	void NotifyCompanionPlayerHurt();

	UFUNCTION(BlueprintPure, Category = "HPWH") int32 GetRunSeed() const { return RunSeed; }
	UFUNCTION(BlueprintPure, Category = "HPWH") int32 GetLayerIndex() const { return LayerIndex; }
	UFUNCTION(BlueprintPure, Category = "HPWH|Run") int32 GetPlayerLevel() const { return PlayerLevel; }
	UFUNCTION(BlueprintPure, Category = "HPWH|Run") int32 GetKillCount() const { return KillCount; }
	UFUNCTION(BlueprintPure, Category = "HPWH|Run") int32 GetExperience() const { return Experience; }
	UFUNCTION(BlueprintPure, Category = "HPWH|Run") int32 GetExperienceToNextLevel() const { return ExperienceToNextLevel; }
	UFUNCTION(BlueprintPure, Category = "HPWH|Run") int32 GetPendingUpgradeChoices() const { return PendingUpgradeChoices; }
	UFUNCTION(BlueprintPure, Category = "HPWH|Audio") bool HasSfx(EHPWH_Sfx Id) const;
	void PlaySfx2D(EHPWH_Sfx Id, float Volume = 1.f, float Pitch = 1.f);
	void PlaySfxAt(EHPWH_Sfx Id, const FVector& Location, float Volume = 1.f, float Pitch = 1.f);
	const FHPWH_RunStats& GetRunStats() const { return RunStats; }
	const TArray<FHPWH_OwnedUpgrade>& GetOwnedUpgrades() const { return OwnedUpgrades; }
	const TArray<FHPWH_UpgradeOption>& GetUpgradeOptions() const { return UpgradeOptions; }
	bool IsUpgradeSelectionActive() const { return bUpgradeSelectionActive; }
	bool CanRerollUpgrade() const { return bUpgradeSelectionActive && !bUpgradeRerollUsed; }
	void RerollUpgrade();
	float GetPuzzleTimeBonus() const { return RunStats.PuzzleTimeBonus; }
	UFUNCTION(BlueprintPure, Category = "HPWH") int32 GetCrawlerSpeedLevel() const { return CrawlerSpeedLevel; }
	UFUNCTION(BlueprintPure, Category = "HPWH") EHPWH_SoulState GetSoulState() const { return SoulState; }
	UFUNCTION(BlueprintPure, Category = "HPWH") EHPWH_MarkState GetMarkState() const { return MarkState; }
	UFUNCTION(BlueprintPure, Category = "HPWH") bool IsCoreCollected() const { return bCoreCollected; }
	UFUNCTION(BlueprintPure, Category = "HPWH") int32 GetDisplayPercent() const { return FMath::RoundToInt(RawChance); }

	void SetSoulAvatar(AHPWH_SoulAvatar* InAvatar) { SoulAvatar = InAvatar; }
	void SetSoulState(EHPWH_SoulState InState) { SoulState = InState; }
	void SetMarkState(EHPWH_MarkState InState) { MarkState = InState; }
	void SetFloorState(EHPWH_FloorState InState) { FloorState = InState; }

	void RecomputeChance();
	void ApplyChancePenalty(float Percent);
	void PushMessage(const FString& Text, float Duration = 3.f, bool bPriority = false);
	FString GetActiveMessage() const;
	bool IsPriorityMessageActive() const;

	void TriggerSoulOut();
	void HandleCrawlerCaughtBody();
	void HandleSoulKilled();
	void OnEnemyKilled();
	void AddExperience(int32 Amount);
	void NotifyPlayerAttacked();
	void OnSoulReturnedToBody();
	void RequestUpgradeChoice();
	void ChooseUpgrade(int32 Index);
	void CollectCore();
	void OnPuzzleSolved(EHPWH_TerrainType Type);
	void OnPuzzleFailed();
	void HandlePlayerDeath();
	void TryExit(APawn* Interactor);

	// 异层传送门
	bool IsPortalChoiceActive() const { return bPortalChoiceActive; }
	bool IsPortalChoiceSuccess() const { return bPortalChoiceSuccess; }
	int32 GetSolvedPuzzleTypeCount() const { return SolvedPuzzleTypes.Num(); }
	void OpenFloorPortalChoice();
	void ConfirmFloorPortalChoice();

	// 主界面
	bool IsMainMenuActive() const { return bMainMenuActive; }
	EHPWH_MenuPage GetMenuPage() const { return MenuPage; }
	EHPWH_GraphicsMode GetGraphicsMode() const { return GraphicsMode; }
	void HandleMenuSelection(int32 Index);
	void MenuBack();
	void StartGameFromMenu();
	bool HasSaveGame() const;
	bool LoadProgress();
	void SetGraphicsMode(EHPWH_GraphicsMode Mode);

	// 游戏内菜单
	bool IsInGameMenuOpen() const { return bInGameMenuOpen; }
	bool IsInGameSettingsOpen() const { return bInGameSettingsOpen; }
	void ToggleInGameMenu();
	void CloseInGameMenu();
	void HandleInGameMenuSelection(int32 Index);
	void SaveProgress();
	void ReturnToMainMenu();

private:
	void AdvanceLayer();
	void PlacePlayerAtOrigin();
	void EnsureRuntimeLighting();
	void BeginUpgradeSelection();
	void EndUpgradeSelection();
	void GenerateUpgradeOptions();
	void ApplyUpgrade(const FHPWH_UpgradeOption& Option);
	/** 合并重复类型、丢弃越界类型，保证强化列表按类型唯一。 */
	void ConsolidateOwnedUpgrades();
	void CancelUpgradeSelection();

	// 音乐与音效
	void LoadMusic();
	void LoadSfx();
	void PlayMusic(USoundBase* Sound);
	void RefreshMusicState();
	void ApplyGraphicsMode();
	void SetGameplayPaused(bool bPaused);
	void SpawnFloorPortal();
	void DestroyFloorPortal();

	UPROPERTY() TObjectPtr<AHPWH_WorldDirector> WorldDirector;
	UPROPERTY() TObjectPtr<AHPWH_PlayerCharacter> PlayerBody;
	UPROPERTY() TObjectPtr<AHPWH_SoulAvatar> SoulAvatar;
	UPROPERTY() TObjectPtr<AHPWH_NextFloorPortal> FloorPortal;

	UPROPERTY() TObjectPtr<USoundBase> BgmCalm;
	UPROPERTY() TObjectPtr<USoundBase> BgmAlert;
	UPROPERTY() TObjectPtr<USoundBase> BgmCrawler;
	UPROPERTY() TObjectPtr<UAudioComponent> MusicComponent;
	UPROPERTY() TObjectPtr<USoundBase> CurrentMusic;
	UPROPERTY() TArray<TObjectPtr<USoundBase>> SfxById;

	FRandomStream EscapeStream;
	FRandomStream UpgradeStream;

	FString NormalMessage;
	float NormalMessageEnd = 0.f;
	FString PriorityMessage;
	float PriorityMessageEnd = 0.f;
	bool bDirectorInitialized = false;
	bool bBodyCaughtPenalized = false;

	FHPWH_RunStats RunStats;
	TArray<FHPWH_OwnedUpgrade> OwnedUpgrades;
	TArray<FHPWH_UpgradeOption> UpgradeOptions;
	int32 PlayerLevel = 1;
	int32 KillCount = 0;
	int32 Experience = 0;
	int32 ExperienceToNextLevel = 10;
	int32 PendingSoulHits = 0;
	int32 PendingUpgradeChoices = 0;
	bool bUpgradeSelectionActive = false;
	bool bUpgradeRerollUsed = false;
	bool bAutoPickDebug = false;
	bool bDebugAutoReroll = false;
	double UpgradeShownRealTime = 0.0;
	double ScreenshotAtRealTime = 0.0;
	bool bMainMenuActive = true;
	bool bAlertMusic = false;
	bool bInGameMenuOpen = false;
	bool bInGameSettingsOpen = false;
	bool bFloorPortalSpawned = false;
	bool bPortalChoiceActive = false;
	bool bPortalChoiceSuccess = false;
	double PortalChoiceRealTime = 0.0;
	double NextDebugSfxTime = 0.0;
	int32 DebugSfxIndex = 0;
	bool bDebugSfx = false;
	TSet<EHPWH_TerrainType> SolvedPuzzleTypes;
	EHPWH_MenuPage MenuPage = EHPWH_MenuPage::Main;
	EHPWH_GraphicsMode GraphicsMode = EHPWH_GraphicsMode::TSR;
};
