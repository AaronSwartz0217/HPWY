#include "World/CC_AdventureGameInstance.h"
#include "World/CC_AdventureGameMode.h"
#include "World/CC_MainMenuGameMode.h"
#include "World/CC_MapPortal.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Progression/CC_ProgressionSave.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Player/CC_PlayerState.h"
#include "Player/CC_PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Engine/Engine.h"
#include "EngineGlobals.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "MoviePlayer.h"
#include "UI/SCC_LoadingScreen.h"
#include "Engine/Texture2D.h"
#include "Components/BoxComponent.h"
#include "NiagaraSystem.h"
#include "HAL/PlatformTime.h"

struct FCC_LoadingPresentation
{
	TAtomic<float> Progress{-1.f};
	TAtomic<bool> Opening{true};
};
namespace
{
int32 PresentationScreenPolicyUsers=0;
bool PreviousScreenMessageDisplay=true;
}

UCC_AdventureGameInstance::UCC_AdventureGameInstance()
{
 LoadingLogo=TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/UI/MuralMenu/Textures/Menu/T_Menu_Logo.T_Menu_Logo")));
 TravelBallEffect=TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/LanTuRebuild/Effects/运河传送门/粒子传送门.粒子传送门")));
 MainMenuMap=TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/UI/MainMenu/Maps/L_MainMenu.L_MainMenu")));
 BackgroundMusic=TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Audio/Music/SW_ExhibitionTheme.SW_ExhibitionTheme")));
	auto Add=[this](FName Id,const TCHAR* Name,const TCHAR* Path,bool Unlocked) {
		FCC_TravelRegion Region;Region.Id=Id;Region.DisplayName=FText::FromString(Name);
		Region.Map=TSoftObjectPtr<UWorld>(FSoftObjectPath(Path));Region.bInitiallyUnlocked=Unlocked;Regions.Add(Region);
	};
	Add(TEXT("Rest"),TEXT("休憩之地"),TEXT("/Game/LanTuRebuild/Maps/初始房间/场景优化.场景优化"),true);
	Add(TEXT("Town"),TEXT("小镇"),TEXT("/Game/LanTuRebuild/Maps/古风小镇/古风小镇.古风小镇"),false);
	Add(TEXT("Arena"),TEXT("山河社稷"),TEXT("/Game/HPWH/Maps/L_FlatTestArena.L_FlatTestArena"),true);
	Add(TEXT("Heaven"),TEXT("万象天宫"),TEXT("/Game/LanTuRebuild/Maps/万象天宫/万象天宫.万象天宫"),true);
	Add(TEXT("Palace"),TEXT("宫殿"),TEXT("/Game/LanTuRebuild/Maps/宫殿/宫殿.宫殿"),true);
}
void UCC_AdventureGameInstance::Init()
{
	Super::Init();
 if (!bShowRuntimeDebugMessages) {
  if (PresentationScreenPolicyUsers++==0) PreviousScreenMessageDisplay=GAreScreenMessagesEnabled;
  GAreScreenMessagesEnabled=false;bScreenMessagePolicyApplied=true;
 }
	LoadingPresentation=MakeShared<FCC_LoadingPresentation,ESPMode::ThreadSafe>();
 LoadedLoadingLogo=LoadingLogo.LoadSynchronous();
	if (UGameplayStatics::DoesSaveGameExist(TEXT("SpiritTales"),0))
	{
		auto* Save=Cast<UCC_ProgressionSave>(UGameplayStatics::LoadGameFromSlot(TEXT("SpiritTales"),0));
		if (Save && Save->Version>=1 && Save->Version<=4) AdoptProgressSnapshot(Save);
		else {bRestoreFailed=true;LastTravelError=FText::FromString(TEXT("角色存档无法读取，已保留原文件。"));}
	}
	FCoreUObjectDelegates::PreLoadMapWithContext.AddUObject(this,&ThisClass::PrepareLoadingScreen);
	if (GEngine) GEngine->OnTravelFailure().AddUObject(this,&ThisClass::TravelFailed);
}
void UCC_AdventureGameInstance::Shutdown()
{
 if (bScreenMessagePolicyApplied) {bScreenMessagePolicyApplied=false;if (--PresentationScreenPolicyUsers==0) GAreScreenMessagesEnabled=PreviousScreenMessageDisplay;}
 if (IsValid(MusicComponent)) {MusicComponent->Stop();MusicComponent->DestroyComponent();}MusicComponent=nullptr;
	bTravelling=false;bOpeningWorld=false;
	FTSTicker::GetCoreTicker().RemoveTicker(LoadTicker);
	FCoreUObjectDelegates::PreLoadMapWithContext.RemoveAll(this);
	if (GEngine) GEngine->OnTravelFailure().RemoveAll(this);
	Super::Shutdown();
}
void UCC_AdventureGameInstance::EnsureBackgroundMusic()
{
 UWorld* World=GetWorld();
 if (!World || !World->IsGameWorld() || World->IsNetMode(NM_DedicatedServer)) return;
 // The audio-device-owned component survives world destruction. Keep a single strong reference.
 if (IsValid(MusicComponent) && MusicComponent->IsPlaying()) return;
 USoundBase* Music=BackgroundMusic.LoadSynchronous();if (!Music) return;
 if (IsValid(MusicComponent)) MusicComponent->DestroyComponent();
 MusicComponent=UGameplayStatics::CreateSound2D(this,Music,FMath::Clamp(BackgroundMusicVolume,0.f,1.f),1.f,0.f,nullptr,true,false);
 if (MusicComponent) MusicComponent->FadeIn(FMath::Max(0.f,MusicFadeInSeconds),1.f);
}
void UCC_AdventureGameInstance::SetBackgroundMusicVolume(float Volume)
{
 BackgroundMusicVolume=FMath::Clamp(Volume,0.f,1.f);
 if (IsValid(MusicComponent)) MusicComponent->SetVolumeMultiplier(BackgroundMusicVolume);
}
const FCC_TravelRegion* UCC_AdventureGameInstance::FindRegion(FName Id) const
{
	return Regions.FindByPredicate([Id](const FCC_TravelRegion& Region){return Region.Id==Id;});
}
FName UCC_AdventureGameInstance::RegionForMap(const FString& MapName) const
{
	const FString ShortName=FPackageName::GetShortName(FPackageName::ObjectPathToPackageName(MapName));
	for (const auto& Region:Regions)
	{
		const FString Name=Region.Map.GetAssetName();
		if (ShortName==Name || ShortName.EndsWith(TEXT("_")+Name)) return Region.Id;
	}
	return NAME_None;
}
TSubclassOf<AGameModeBase> UCC_AdventureGameInstance::OverrideGameModeClass(TSubclassOf<AGameModeBase> GameModeClass,const FString& MapName,const FString& Options,const FString& Portal) const
{
 if (IsMainMenuMap(MapName)) return ACC_MainMenuGameMode::StaticClass();
	if (!RegionForMap(MapName).IsNone()) return ACC_AdventureGameMode::StaticClass();
	return Super::OverrideGameModeClass(GameModeClass,MapName,Options,Portal);
}
bool UCC_AdventureGameInstance::IsMainMenuMap(const FString& MapName) const
{
 const FString Short=FPackageName::GetShortName(FPackageName::ObjectPathToPackageName(MapName));
 const FString Menu=MainMenuMap.GetAssetName();return !Menu.IsEmpty() && (Short==Menu || Short.EndsWith(TEXT("_")+Menu));
}
bool UCC_AdventureGameInstance::IsMainMenuWorld() const
{
 return GetWorld() && IsMainMenuMap(UGameplayStatics::GetCurrentLevelName(GetWorld(),true));
}
bool UCC_AdventureGameInstance::ConsumeStartupLogo()
{
 if (bStartupLogoShown) return false;bStartupLogoShown=true;return true;
}
bool UCC_AdventureGameInstance::StartGameFromHome()
{
 if (!GetWorld() || !IsMainMenuWorld() || GetWorld()->GetNetMode()!=NM_Standalone || bTravelling) return false;
 LastTravelError=FText::GetEmpty();
 const auto* Rest=FindRegion(TEXT("Rest"));const FString Package=Rest ? Rest->Map.ToSoftObjectPath().GetLongPackageName() : FString();
 if (Package.IsEmpty() || !FPackageName::DoesPackageExist(Package)) {LastTravelError=FText::FromString(TEXT("休憩之地尚未就绪，请检查地图资源。"));return false;}
 const bool HasSave=UGameplayStatics::DoesSaveGameExist(TEXT("SpiritTales"),0);
 UCC_ProgressionSave* Snapshot=nullptr;
 if (!HasSave) {Snapshot=NewObject<UCC_ProgressionSave>(this);Snapshot->NativeFormId=TEXT("InitialInk");}
 else {
  Snapshot=Cast<UCC_ProgressionSave>(UGameplayStatics::LoadGameFromSlot(TEXT("SpiritTales"),0));
  if (!Snapshot || Snapshot->Version<1 || Snapshot->Version>4) {bRestoreFailed=true;LastTravelError=FText::FromString(TEXT("没有可读取的角色存档，原文件已保留。"));return false;}
 }
 BeforeMenuStart=SessionProgress;BeforeMenuVisited=VisitedRegions;bBeforeRestoreFailed=bRestoreFailed;bBeforeSuppressSaves=bSuppressAutomaticSaves;
 AdoptProgressSnapshot(Snapshot);RestoredProgression.Reset();CurrentRegion=NAME_None;bSuppressAutomaticSaves=false;bStartingFromMenu=true;
 // No disk overwrite here. The destination commits only after a valid character is ready.
 StartPackageLoad(TEXT("Rest"),Package);return true;
}
bool UCC_AdventureGameInstance::IsRegionUnlocked(FName Id) const
{
	const auto* Region=FindRegion(Id);
	return Region && (Region->bInitiallyUnlocked || VisitedRegions.Contains(Id));
}
void UCC_AdventureGameInstance::AdoptProgressSnapshot(const UCC_ProgressionSave* Save)
{
	if (!Save) return;
	SessionProgress=DuplicateObject<UCC_ProgressionSave>(Save,this);
	VisitedRegions.Empty();
	for (FName Id:Save->VisitedRegions) if (FindRegion(Id)) VisitedRegions.AddUnique(Id);
	bRestoreFailed=false;
}
bool UCC_AdventureGameInstance::RestoreSessionProgress(UCC_ProgressionComponent* Progression)
{
	if (!Progression || !Progression->GetOwner()->HasAuthority() || !GetWorld() || GetWorld()->GetNetMode()!=NM_Standalone || bRestoreFailed) return false;
	if (RegionForMap(UGameplayStatics::GetCurrentLevelName(GetWorld(),true)).IsNone()) return false;
	if (RestoredProgression.Get()==Progression) return true;
	if (SessionProgress && !Progression->RestoreSnapshot(SessionProgress))
	{
		bRestoreFailed=true;LastTravelError=FText::FromString(TEXT("进度与当前角色配置不兼容，已停止自动写档。"));return false;
	}
	RestoredProgression=Progression;
	return true;
}
bool UCC_AdventureGameInstance::SaveCurrentProgress()
{
	auto* PC=GetFirstLocalPlayerController();
	auto* PS=PC ? PC->GetPlayerState<ACC_PlayerState>() : nullptr;
	if (!PS || !PS->Progression || bRestoreFailed || RestoredProgression.Get()!=PS->Progression.Get()) return false;
	if (bSuppressAutomaticSaves)
	{
		auto* Snapshot=PS->Progression->CreateSnapshot();if (!Snapshot) return false;
		AdoptProgressSnapshot(Snapshot);return true;
	}
	return PS->Progression->SaveProgress();
}
void UCC_AdventureGameInstance::HandleRegionEntered(UWorld* World)
{
	if (!World || World!=GetWorld() || !World->HasBegunPlay() || World->GetNetMode()!=NM_Standalone) return;
 EnsureBackgroundMusic();
	const FName Id=RegionForMap(UGameplayStatics::GetCurrentLevelName(World,true));
	if (Id.IsNone()) return;
	CurrentRegion=Id;
	VisitedRegions.AddUnique(Id);
	if (auto* PC=World->GetFirstPlayerController()) if (auto* PS=PC->GetPlayerState<ACC_PlayerState>()) RestoreSessionProgress(PS->Progression);
	// Unlock is committed after actually entering the target world, never on button press.
 if(bTravelling&&PendingRegion==Id&&!PendingTravelScrollRegion.IsNone())
 {
  if(auto* PC=GetFirstLocalPlayerController())if(auto* PS=PC->GetPlayerState<ACC_PlayerState>())PS->Progression->ConsumeTravelScroll(PendingTravelScrollRegion);
  PendingTravelScrollRegion=NAME_None;
 }
	if (!SaveCurrentProgress() && !bRestoreFailed) LastTravelError=FText::FromString(TEXT("地区已抵达，本地进度保存失败。"));
	if (bTravelling && PendingRegion==Id)
	{
		bTravelling=false;bOpeningWorld=false;bPausedForTravel=false;PendingRegion=NAME_None;PendingMapPackage=nullptr;PendingLoadedWorld=nullptr;
		FTSTicker::GetCoreTicker().RemoveTicker(LoadTicker);LoadTicker.Reset();
		LoadingPresentation->Progress.Store(1.f);
		LoadingPresentation->Opening.Store(false);
  bStartingFromMenu=false;BeforeMenuStart=nullptr;BeforeMenuVisited.Empty();
	}
}
void UCC_AdventureGameInstance::LoadComplete(const float LoadTime,const FString& MapName)
{
	Super::LoadComplete(LoadTime,MapName);EnsureBackgroundMusic();HandleRegionEntered(GetWorld());
}
float UCC_AdventureGameInstance::GetLoadingProgress() const
{
	return LoadingPresentation ? LoadingPresentation->Progress.Load() : -1.f;
}
FText UCC_AdventureGameInstance::GetLoadingTitle() const
{
	const auto* Region=FindRegion(PendingRegion);
	return Region ? FText::Format(FText::FromString(TEXT("前往 · {0}")),Region->DisplayName) : FText::FromString(TEXT("正在载入场景"));
}
float UCC_AdventureGameInstance::GetTravelCooldownRemaining()const{return float(FMath::Max(0.,TravelCooldownEndsAt-FPlatformTime::Seconds()));}
void UCC_AdventureGameInstance::UnlockDeveloperTravel()
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
 if(!GetWorld()||GetWorld()->GetNetMode()!=NM_Standalone)return;
 MarkDeveloperSession();for(const auto& Region:Regions)VisitedRegions.AddUnique(Region.Id);
 TravelCooldownEndsAt=0;
#endif
}
void UCC_AdventureGameInstance::ResetDeveloperTravel()
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
 if(!GetWorld()||GetWorld()->GetNetMode()!=NM_Standalone||bTravelling)return;
 if(ActiveDeparture.IsValid()) {
  auto* Portal=ActiveDeparture.Get();Portal->CancelDeparture();
  if(Portal->bTemporaryPortal)Portal->Destroy();
 }
 ActiveDeparture.Reset();PendingTravelScrollRegion=NAME_None;TravelCooldownEndsAt=0;
 VisitedRegions.Empty();SessionProgress=nullptr;LastTravelError=FText::GetEmpty();
#endif
}
bool UCC_AdventureGameInstance::IsPortalDeparturePending()const{return ActiveDeparture.IsValid()&&ActiveDeparture->IsDeparturePending();}
bool UCC_AdventureGameInstance::HasTravelScroll(FName Id)const
{
 if(Id!=TEXT("Arena")&&Id!=TEXT("Heaven")&&Id!=TEXT("Palace"))return true;
 const auto* PC=GetFirstLocalPlayerController();const auto* PS=PC?PC->GetPlayerState<ACC_PlayerState>():nullptr;
 const auto* P=PS?PS->Progression.Get():nullptr;
 return P&&P->GetItemCount(P->FindTravelScroll(Id))>0;
}
bool UCC_AdventureGameInstance::RequestFastTravel(FName Id)
{
 if(!ValidateTravel(Id,false,false))return false;
 if(IsPortalDeparturePending()){LastTravelError=FText::FromString(TEXT("传送球正在凝聚，请勿重复释放。"));return false;}
 if(GetTravelCooldownRemaining()>0){LastTravelError=FText::FromString(FString::Printf(TEXT("传送冷却还剩 %.0f 秒。"),FMath::CeilToFloat(GetTravelCooldownRemaining())));return false;}
 auto* Character=Cast<ACC_PlayerCharacter>(GetFirstLocalPlayerController()->GetPawn());
 const FVector Location=Character->GetActorLocation()+Character->GetActorForwardVector()*160.f;
 const FTransform Transform(FRotator::ZeroRotator,Location);
 auto* Portal=GetWorld()->SpawnActorDeferred<ACC_MapPortal>(ACC_MapPortal::StaticClass(),Transform,Character,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
 if(!Portal)return false;
 Portal->DestinationRegion=Id;Portal->bShowMarker=false;Portal->bTemporaryPortal=true;Portal->ArmDelay=0;Portal->TravelDelay=8.f;
 Portal->Trigger->SetBoxExtent(FVector(350,350,220));Portal->DepartureEffect=TravelBallEffect.LoadSynchronous();
 Portal->FinishSpawning(Transform);
 auto* PC=Cast<ACC_PlayerController>(Character->GetController());PC->CloseProgressionMenu();PC->SetTravelMenuOpen(false);
 if(!Portal->BeginSummonedDeparture(Character)){Portal->Destroy();return false;}
 return true;
}
bool UCC_AdventureGameInstance::ArmPortalDeparture(ACC_MapPortal* Portal,ACC_PlayerCharacter* Character)
{
 if(!IsValid(Portal)||!IsValid(Character)||Character->GetController()!=GetFirstLocalPlayerController())return false;
 if(IsPortalDeparturePending()||GetTravelCooldownRemaining()>0){LastTravelError=FText::FromString(TEXT("传送尚在冷却或已有传送球。"));return false;}
 const bool Discover=CurrentRegion==TEXT("Rest")&&Portal->DestinationRegion==TEXT("Town");
 if(!ValidateTravel(Portal->DestinationRegion,Discover,false))return false;
 ActiveDeparture=Portal;TravelCooldownEndsAt=FPlatformTime::Seconds()+30.;return true;
}
bool UCC_AdventureGameInstance::RequestPortalTravel(ACC_MapPortal* Portal,ACC_PlayerCharacter* Character)
{
	if (!IsValid(Portal)||ActiveDeparture.Get()!=Portal)return false;
 ActiveDeparture.Reset();
 if(!IsValid(Character)||Character->GetController()!=GetFirstLocalPlayerController()||!Portal->IsOverlappingActor(Character))
 {LastTravelError=FText::FromString(TEXT("已离开传送球范围，本次传送取消。"));return false;}
	const bool DiscoverTown=CurrentRegion==TEXT("Rest") && Portal->DestinationRegion==TEXT("Town");
	return BeginTravel(Portal->DestinationRegion,DiscoverTown,false);
}
bool UCC_AdventureGameInstance::RespawnAtRest(APlayerController* Controller)
{
	return Controller==GetFirstLocalPlayerController() && BeginTravel(TEXT("Rest"),false,true);
}
bool UCC_AdventureGameInstance::ValidateTravel(FName Id,bool bAllowLocked,bool bRespawn)
{
	LastTravelError=FText::GetEmpty();
	if (!GetWorld() || GetWorld()->GetNetMode()!=NM_Standalone || bTravelling || CurrentRegion.IsNone()) return false;
	const auto* Region=FindRegion(Id);
	if (!Region || (!IsRegionUnlocked(Id) && !bAllowLocked)) {LastTravelError=FText::FromString(TEXT("需要先抵达该地区。"));return false;}
	if (!bRespawn && Id==CurrentRegion) {LastTravelError=FText::FromString(TEXT("当前已在此地。"));return false;}
 if(!bRespawn&&!HasTravelScroll(Id)){LastTravelError=FText::FromString(TEXT("需要对应的传送卷轴，请到小镇旧纸摊购买。"));return false;}
	auto* PC=Cast<ACC_PlayerController>(GetFirstLocalPlayerController());
	auto* Character=PC ? Cast<ACC_PlayerCharacter>(PC->GetPawn()) : nullptr;
	auto* PS=PC ? PC->GetPlayerState<ACC_PlayerState>() : nullptr;
	if (!Character || !PS || !PS->Progression || (!bRespawn && (!Character->IsAlive() || !PS->Progression->CanConfigureSkills())))
	{LastTravelError=FText::FromString(TEXT("请在角色存活且技能结束、冷却完成后传送。"));return false;}
 if(!FPackageName::DoesPackageExist(Region->Map.ToSoftObjectPath().GetLongPackageName())){LastTravelError=FText::FromString(TEXT("目标地图资源不存在或尚未打包。"));return false;}
 return true;
}
bool UCC_AdventureGameInstance::BeginTravel(FName Id,bool bAllowLocked,bool bRespawn)
{
 if(!ValidateTravel(Id,bAllowLocked,bRespawn))return false;
 auto* PC=Cast<ACC_PlayerController>(GetFirstLocalPlayerController());auto* Character=Cast<ACC_PlayerCharacter>(PC->GetPawn());
 PendingPackage=FindRegion(Id)->Map.ToSoftObjectPath().GetLongPackageName();
	if (!SaveCurrentProgress()) {LastTravelError=FText::FromString(TEXT("角色进度保存失败，未开始传送。"));return false;}
 PendingTravelScrollRegion=!bRespawn&&(Id==TEXT("Arena")||Id==TEXT("Heaven")||Id==TEXT("Palace"))?Id:NAME_None;
	PC->CloseProgressionMenu();PC->SetTravelMenuOpen(false);
	Character->GetCharacterMovement()->StopMovementImmediately();Character->StopJumping();
	// Freeze rewards/combat after the snapshot while the engine ticker loads assets.
	bPausedForTravel=!UGameplayStatics::IsGamePaused(this) && PC->SetPause(true);
 StartPackageLoad(Id,PendingPackage);return true;
}
void UCC_AdventureGameInstance::StartPackageLoad(FName Id,const FString& Package)
{
 PendingRegion=Id;PendingPackage=Package;bTravelling=true;bOpeningWorld=false;
	LoadingPresentation->Opening.Store(false);LoadingPresentation->Progress.Store(-1.f);
	LoadTicker=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this,&ThisClass::UpdateLoading),.05f);
	LoadPackageAsync(PendingPackage,FLoadPackageAsyncDelegate::CreateUObject(this,&ThisClass::MapAssetsReady));
}
bool UCC_AdventureGameInstance::UpdateLoading(float DeltaSeconds)
{
	if (!bTravelling) return false;
	if (!bOpeningWorld)
	{
		const float Percent=GetAsyncLoadPercentage(FName(*PendingPackage));
		LoadingPresentation->Progress.Store(Percent<0 ? -1.f : FMath::Clamp(Percent/100.f,0.f,1.f));
	}
	return true;
}
void UCC_AdventureGameInstance::MapAssetsReady(const FName& PackageName,UPackage* Package,EAsyncLoadingResult::Type Result)
{
	if (!bTravelling || PackageName!=FName(*PendingPackage)) return;
	if (Result!=EAsyncLoadingResult::Succeeded || !Package) {FailTravel(FText::FromString(TEXT("地图读取失败，请重试。")));return;}
 PendingLoadedWorld=UWorld::FindWorldInPackage(Package);
 if(!PendingLoadedWorld)PendingLoadedWorld=UWorld::FollowWorldRedirectorInPackage(Package);
 if(!PendingLoadedWorld){FailTravel(FText::FromString(TEXT("目标资源不包含可进入的地图。")));return;}
	PendingMapPackage=Package;bOpeningWorld=true;
	LoadingPresentation->Opening.Store(true);
	// Defer one frame so the old world's UI can paint the initialization phase.
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this,[this](float){
		if (bTravelling) UGameplayStatics::OpenLevel(this,FName(*PendingPackage),true);
		return false;
	}));
}
TSharedRef<SWidget> UCC_AdventureGameInstance::CreateLoadingScreen(FText FixedTitle)
{
 const auto State=LoadingPresentation;
 TAttribute<FText> Title(FixedTitle);
 if(FixedTitle.IsEmpty())Title=TAttribute<FText>::CreateLambda([Weak=TWeakObjectPtr<UCC_AdventureGameInstance>(this)](){
  return Weak.IsValid()?Weak->GetLoadingTitle():FText::FromString(TEXT("正在载入场景"));
 });
 return SNew(SCC_LoadingScreen).Logo(LoadedLoadingLogo).Title(Title)
  .Progress_Lambda([State]()->TOptional<float>{const float P=State?State->Progress.Load():-1.f;return P<0?TOptional<float>():TOptional<float>(P);})
  .Opening_Lambda([State](){return State&&State->Opening.Load();});
}
void UCC_AdventureGameInstance::PrepareLoadingScreen(const FWorldContext& Context,const FString& MapName)
{
	if (Context.OwningGameInstance.Get()!=this || IsRunningDedicatedServer() || !IsMoviePlayerEnabled()) return;
	const auto* Region=FindRegion(RegionForMap(MapName));if (!Region) return;
	FLoadingScreenAttributes Screen;Screen.bAutoCompleteWhenLoadingCompletes=true;Screen.MinimumLoadingScreenDisplayTime=.2f;
 Screen.WidgetLoadingScreen=CreateLoadingScreen(FText::Format(FText::FromString(TEXT("前往 · {0}")),Region->DisplayName));
	GetMoviePlayer()->SetupLoadingScreen(Screen);
}
void UCC_AdventureGameInstance::TravelFailed(UWorld* World,ETravelFailure::Type FailureType,const FString& Error)
{
	if (bTravelling && (!World || World==GetWorld())) FailTravel(FText::FromString(TEXT("场景切换失败：")+Error));
}
void UCC_AdventureGameInstance::FailTravel(const FText& Error)
{
 if (bStartingFromMenu) {
  SessionProgress=BeforeMenuStart;VisitedRegions=BeforeMenuVisited;bRestoreFailed=bBeforeRestoreFailed;bSuppressAutomaticSaves=bBeforeSuppressSaves;
  RestoredProgression.Reset();CurrentRegion=NAME_None;BeforeMenuStart=nullptr;BeforeMenuVisited.Empty();bStartingFromMenu=false;
 }
	if (bPausedForTravel) if (auto* PC=GetFirstLocalPlayerController()) PC->SetPause(false);
	bPausedForTravel=false;
 PendingTravelScrollRegion=NAME_None;
	bTravelling=false;bOpeningWorld=false;PendingRegion=NAME_None;PendingMapPackage=nullptr;PendingLoadedWorld=nullptr;LastTravelError=Error;
	FTSTicker::GetCoreTicker().RemoveTicker(LoadTicker);LoadTicker.Reset();
	if (IsMoviePlayerEnabled()) GetMoviePlayer()->StopMovie();
}
