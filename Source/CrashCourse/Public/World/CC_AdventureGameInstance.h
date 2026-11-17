#pragma once
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Containers/Ticker.h"
#include "UObject/UObjectGlobals.h"
#include "CC_AdventureGameInstance.generated.h"

class UCC_ProgressionSave;
class UCC_ProgressionComponent;
class ACC_MapPortal;
class ACC_PlayerCharacter;
class USoundBase;
class UAudioComponent;
class UTexture2D;
class SWidget;
class UNiagaraSystem;
struct FCC_LoadingPresentation;

USTRUCT(BlueprintType)
struct FCC_TravelRegion
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere,BlueprintReadOnly) FName Id;
	UPROPERTY(EditAnywhere,BlueprintReadOnly) FText DisplayName;
	UPROPERTY(EditAnywhere,BlueprintReadOnly) TSoftObjectPtr<UWorld> Map;
	UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bInitiallyUnlocked=false;
};

// Single-player travel authority, persistent progression handoff and visited-region ledger.
UCLASS(Config=Game)
class CRASHCOURSE_API UCC_AdventureGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:
	UCC_AdventureGameInstance();
	virtual void Init() override;
	virtual void Shutdown() override;
	virtual void LoadComplete(const float LoadTime,const FString& MapName) override;
	virtual TSubclassOf<AGameModeBase> OverrideGameModeClass(TSubclassOf<AGameModeBase> GameModeClass,const FString& MapName,const FString& Options,const FString& Portal) const override;
	UPROPERTY(Config,EditDefaultsOnly,BlueprintReadOnly,Category="Travel") TArray<FCC_TravelRegion> Regions;
 UPROPERTY(Config,EditDefaultsOnly,BlueprintReadOnly,Category="MainMenu") TSoftObjectPtr<UWorld> MainMenuMap;
 UFUNCTION(BlueprintPure,Category="MainMenu") bool IsMainMenuWorld() const;
 UFUNCTION(BlueprintCallable,Category="MainMenu") bool StartGameFromHome();
 UFUNCTION(BlueprintCallable,Category="MainMenu") bool ConsumeStartupLogo();
 UPROPERTY(Config,EditDefaultsOnly,BlueprintReadOnly,Category="Presentation") bool bShowRuntimeDebugMessages=false;
 UPROPERTY(Config,EditDefaultsOnly,BlueprintReadOnly,Category="Audio") TSoftObjectPtr<USoundBase> BackgroundMusic;
 UPROPERTY(Config,EditDefaultsOnly,BlueprintReadOnly,Category="Audio",meta=(ClampMin="0",ClampMax="1")) float BackgroundMusicVolume=.25f;
 UPROPERTY(Config,EditDefaultsOnly,BlueprintReadOnly,Category="Audio",meta=(ClampMin="0")) float MusicFadeInSeconds=1.5f;
 UFUNCTION(BlueprintCallable,Category="Audio") void EnsureBackgroundMusic();
 UFUNCTION(BlueprintCallable,Category="Audio") void SetBackgroundMusicVolume(float Volume);
	UPROPERTY(BlueprintReadOnly,Category="Travel") TArray<FName> VisitedRegions;
	UPROPERTY(BlueprintReadOnly,Category="Travel") FName CurrentRegion;
	UPROPERTY(BlueprintReadOnly,Category="Travel") bool bTravelling=false;
	UPROPERTY(BlueprintReadOnly,Category="Travel") bool bMinimapVisible=true;
	UPROPERTY(BlueprintReadOnly,Category="Travel") FText LastTravelError;
	UFUNCTION(BlueprintPure,Category="Travel") bool IsRegionUnlocked(FName Id) const;
	UFUNCTION(BlueprintPure,Category="Travel") float GetLoadingProgress() const;
	UFUNCTION(BlueprintPure,Category="Travel") bool IsOpeningWorld() const { return bOpeningWorld; }
	UFUNCTION(BlueprintPure,Category="Travel") FText GetLoadingTitle() const;
 UPROPERTY(Config,EditDefaultsOnly,BlueprintReadOnly,Category="Travel|Appearance") TSoftObjectPtr<UTexture2D> LoadingLogo;
 TSharedRef<SWidget> CreateLoadingScreen(FText FixedTitle=FText::GetEmpty());
	UFUNCTION(BlueprintCallable,Category="Travel") bool RequestFastTravel(FName Id);
 UFUNCTION(BlueprintPure,Category="Travel") float GetTravelCooldownRemaining() const;
 UFUNCTION(BlueprintPure,Category="Travel") bool IsPortalDeparturePending() const;
 UFUNCTION(BlueprintPure,Category="Travel") bool HasTravelScroll(FName Region) const;
 UPROPERTY(Config,EditDefaultsOnly,BlueprintReadOnly,Category="Travel") TSoftObjectPtr<UNiagaraSystem> TravelBallEffect;
 bool ArmPortalDeparture(ACC_MapPortal* Portal,ACC_PlayerCharacter* Character);
	const FCC_TravelRegion* FindRegion(FName Id) const;
	FName RegionForMap(const FString& MapName) const;
	void HandleRegionEntered(UWorld* World);
	bool RestoreSessionProgress(UCC_ProgressionComponent* Progression);
	void AdoptProgressSnapshot(const UCC_ProgressionSave* Save);
	void MarkDeveloperSession() {bSuppressAutomaticSaves=true;}
 void UnlockDeveloperTravel();
 void ResetDeveloperTravel();
	void ResumeAutomaticSaves() {bSuppressAutomaticSaves=false;}
 bool SaveAfterShopPurchase(){return SaveCurrentProgress();}
	bool RequestPortalTravel(ACC_MapPortal* Portal,class ACC_PlayerCharacter* Character);
	bool RespawnAtRest(class APlayerController* Controller);
private:
 bool bStartupLogoShown=false;
 bool bScreenMessagePolicyApplied=false;
 UPROPERTY(Transient) TObjectPtr<UAudioComponent> MusicComponent;
 UPROPERTY(Transient) TObjectPtr<UTexture2D> LoadedLoadingLogo;
	UPROPERTY(Transient) TObjectPtr<UCC_ProgressionSave> SessionProgress;
	UPROPERTY(Transient) TObjectPtr<UPackage> PendingMapPackage;
 // Holding the package alone does not keep its contained world alive through travel GC.
 UPROPERTY(Transient) TObjectPtr<UWorld> PendingLoadedWorld;
	UPROPERTY(Transient) TWeakObjectPtr<UCC_ProgressionComponent> RestoredProgression;
 UPROPERTY(Transient) TObjectPtr<UCC_ProgressionSave> BeforeMenuStart;
 TArray<FName> BeforeMenuVisited;
 bool bStartingFromMenu=false;
 bool bBeforeRestoreFailed=false;
 bool bBeforeSuppressSaves=false;
	FName PendingRegion;
	FString PendingPackage;
	bool bOpeningWorld=false;
	bool bRestoreFailed=false;
	bool bSuppressAutomaticSaves=false;
	bool bPausedForTravel=false;
 double TravelCooldownEndsAt=0;
 TWeakObjectPtr<ACC_MapPortal> ActiveDeparture;
 FName PendingTravelScrollRegion;
 bool ValidateTravel(FName Id,bool bAllowLocked,bool bRespawn);
	FTSTicker::FDelegateHandle LoadTicker;
	TSharedPtr<FCC_LoadingPresentation,ESPMode::ThreadSafe> LoadingPresentation;
	bool BeginTravel(FName Id,bool bAllowLocked,bool bRespawn);
 bool IsMainMenuMap(const FString& MapName) const;
 void StartPackageLoad(FName RegionId,const FString& Package);
	bool SaveCurrentProgress();
	bool UpdateLoading(float DeltaSeconds);
	void MapAssetsReady(const FName& PackageName,UPackage* Package,EAsyncLoadingResult::Type Result);
	void PrepareLoadingScreen(const FWorldContext& Context,const FString& MapName);
	void TravelFailed(UWorld* World,ETravelFailure::Type FailureType,const FString& Error);
	void FailTravel(const FText& Error);
};
