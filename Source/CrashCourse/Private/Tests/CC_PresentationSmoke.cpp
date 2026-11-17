#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Containers/Ticker.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineGlobals.h"
#include "EngineUtils.h"
#include "World/CC_AdventureGameInstance.h"
#include "World/CC_AdventureGameMode.h"
#include "World/CC_MapPortal.h"
#include "Player/CC_MainMenuController.h"
#include "Player/CC_PlayerController.h"
#include "Player/CC_PlayerState.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Characters/CC_PlayerCharacter.h"
#include "UI/CC_MainMenuWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWave.h"
#include "UObject/UnrealType.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

namespace
{
struct FPresentationProbe : TSharedFromThis<FPresentationProbe>
{
 TWeakObjectPtr<UCC_AdventureGameInstance> Instance;
 TWeakObjectPtr<UAudioComponent> InitialMusic;
 TWeakObjectPtr<ACC_MapPortal> RoomPortal;
 TArray<TSharedPtr<FJsonValue>> Checks;
 int32 Phase=0;
 bool bPassed=true;
 double Started=FPlatformTime::Seconds(),Next=0.,HomeRevealedAt=0.;
 FString Root=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir())/TEXT("Saved");
 void Check(bool Value,const FString& Name) {
  auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("check"),Name);Row->SetBoolField(TEXT("passed"),Value);
  Checks.Add(MakeShared<FJsonValueObject>(Row));bPassed &= Value;
  UE_LOG(LogTemp,Display,TEXT("Presentation check %s: %s"),*Name,Value ? TEXT("PASS") : TEXT("FAIL"));
 }
 UAudioComponent* Music() const {
  auto* GI=Instance.Get();const auto* Property=GI ? FindFProperty<FObjectPropertyBase>(GI->GetClass(),TEXT("MusicComponent")) : nullptr;
  return Property ? Cast<UAudioComponent>(Property->GetObjectPropertyValue_InContainer(GI)) : nullptr;
 }
 void Capture(const TCHAR* Name) {
  const FString Directory=Root/TEXT("Screenshots/Presentation");IFileManager::Get().MakeDirectory(*Directory,true);
  FScreenshotRequest::RequestScreenshot(Directory/(FString(Name)+TEXT(".png")),true,false);
 }
 bool Finish() {
  auto Result=MakeShared<FJsonObject>();Result->SetBoolField(TEXT("passed"),bPassed);Result->SetBoolField(TEXT("homeOnly"),FParse::Param(FCommandLine::Get(),TEXT("HomePreviewOnly")));Result->SetArrayField(TEXT("checks"),Checks);
  Result->SetStringField(TEXT("isolatedSavedDir"),FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()));
  Result->SetNumberField(TEXT("elapsedSeconds"),FPlatformTime::Seconds()-Started);
  FString JSON;auto Writer=TJsonWriterFactory<>::Create(&JSON);FJsonSerializer::Serialize(Result,Writer);
  IFileManager::Get().MakeDirectory(*(Root/TEXT("UI")),true);FFileHelper::SaveStringToFile(JSON,*(Root/(FParse::Param(FCommandLine::Get(),TEXT("HomePreviewOnly")) ? TEXT("UI/HomeCenteredPreview.json") : TEXT("UI/PresentationSmoke.json"))));
  UE_LOG(LogTemp,Display,TEXT("PRESENTATION_SMOKE_FINISHED %s"),bPassed ? TEXT("PASS") : TEXT("FAIL"));
  FPlatformMisc::RequestExit(false);return false;
 }
 void CheckRegion(FName Region) {
  auto* GI=Instance.Get();UWorld* World=GI->GetWorld();
  Check(GI->CurrentRegion==Region,TEXT("region_")+Region.ToString());
  Check(World->GetAuthGameMode() && World->GetAuthGameMode()->IsA<ACC_AdventureGameMode>(),Region.ToString()+TEXT("_game_mode"));
  Check(Cast<ACC_PlayerCharacter>(World->GetFirstPlayerController()->GetPawn())!=nullptr,Region.ToString()+TEXT("_current_player"));
  Check(Music() && Music()==InitialMusic.Get() && Music()->IsPlaying(),Region.ToString()+TEXT("_same_music_keeps_playing"));
 }
 bool Tick(float) {
  auto* GI=Instance.Get();if (!GI) {Check(false,TEXT("instance_lifetime"));return Finish();}
  const double Now=FPlatformTime::Seconds();if (Now-Started>240.) {Check(false,FString::Printf(TEXT("timeout_phase_%d"),Phase));return Finish();}
  if (Now<Next) return true;
  UWorld* World=GI->GetWorld();if (!World || !World->HasBegunPlay()) return true;
  auto* HomePC=Cast<ACC_MainMenuController>(World->GetFirstPlayerController());auto* Menu=HomePC ? HomePC->GetMainMenu() : nullptr;
  switch (Phase) {
   case 0:
    if (!Menu) return true;
    Check(Menu->IsStartupSplashActive(),TEXT("startup_logo_immediately_visible"));
    Check(!GAreScreenMessagesEnabled,TEXT("runtime_debug_overlay_hidden"));
    if (!FParse::Param(FCommandLine::Get(),TEXT("HomePreviewOnly"))) Check(Music() && Music()->IsPlaying(),TEXT("music_playing_at_startup"));InitialMusic=Music();
    {auto* Wave=Cast<USoundWave>(GI->BackgroundMusic.LoadSynchronous());Check(Wave && Wave->bLooping && Wave->GetName()==TEXT("SW_ExhibitionTheme"),TEXT("requested_exhibition_track_loops"));}
    Capture(TEXT("01_StartupLogo"));++Phase;Next=Now+.3;break;
   case 1:
    if (!Menu) {Check(false,TEXT("home_menu_available"));return Finish();}
    if (Menu->IsStartupSplashActive()) {
     auto* Logo=Cast<UImage>(Menu->WidgetTree->FindWidget(TEXT("StartupLogoImage")));
     if (Logo && Logo->GetRenderOpacity()<.8f && Logo->GetRenderOpacity()>.1f) {Check(true,TEXT("logo_fades_after_hold"));Capture(TEXT("02_LogoFade"));++Phase;}
    } else {Check(false,TEXT("logo_fade_observed"));++Phase;}
    break;
   case 2:
    if (!Menu || Menu->IsStartupSplashActive()) return true;
    if (HomeRevealedAt==0.) HomeRevealedAt=Now;
    if (Now-HomeRevealedAt<.6) return true;
    Check(true,TEXT("home_revealed_after_intro"));Capture(TEXT("03_Home"));++Phase;Next=Now+.5;break;
   case 3:
    if (FParse::Param(FCommandLine::Get(),TEXT("HomePreviewOnly"))) return Finish();
    Check(GI->StartGameFromHome(),TEXT("start_game_requests_rest"));++Phase;break;
   case 4:
    if (GI->bTravelling || GI->CurrentRegion!=TEXT("Rest")) return true;
    CheckRegion(TEXT("Rest"));Capture(TEXT("04_Rest"));
    for (TActorIterator<ACC_MapPortal> It(World);It;++It) if (It->DestinationRegion==TEXT("Town")) {RoomPortal=*It;break;}
    Check(RoomPortal.IsValid(),TEXT("original_room_portal_adapted"));if (!RoomPortal.IsValid()) return Finish();
    World->GetFirstPlayerController()->GetPawn()->SetActorLocation(RoomPortal->GetActorLocation()+FVector(1000,0,0));
    ++Phase;Next=Now+1.5;break;
   case 5:
    World->GetFirstPlayerController()->GetPawn()->SetActorLocation(RoomPortal->GetActorLocation());++Phase;break;
   case 6:
    if (GI->bTravelling || GI->CurrentRegion!=TEXT("Town")) return true;
    CheckRegion(TEXT("Town"));Check(GI->IsRegionUnlocked(TEXT("Town")),TEXT("town_unlocked_after_real_portal_entry"));
    {int32 Count=0;for (TActorIterator<ACC_MapPortal> It(World);It;++It) ++Count;Check(Count==0,TEXT("town_has_no_return_interaction"));}
    Capture(TEXT("05_Town"));++Phase;Next=Now+1.;break;
   case 7:
    if(GI->GetTravelCooldownRemaining()>0)return true;
    {auto* State=World->GetFirstPlayerController()->GetPlayerState<ACC_PlayerState>();auto* Progress=State->Progression.Get();
     Progress->bRemoteShopUnlocked=true;Progress->AddCopperCoins(20);Progress->BuyItem(Progress->FindTravelScroll(TEXT("Arena")),1);}
    Check(GI->RequestFastTravel(TEXT("Arena")),TEXT("map_ui_travel_to_arena_accepted"));++Phase;break;
   case 8:
    if (GI->bTravelling || GI->CurrentRegion!=TEXT("Arena")) return true;
    CheckRegion(TEXT("Arena"));Capture(TEXT("06_Arena"));++Phase;Next=Now+1.;break;
   default:return Finish();
  }
  return true;
 }
};
void RunPresentationSmoke(UWorld* World)
{
 const FString SaveDir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
 if (!World || !World->IsGameWorld() || World->GetNetMode()!=NM_Standalone || !SaveDir.Contains(TEXT("PresentationSmokeUser"))) {
  UE_LOG(LogTemp,Error,TEXT("Presentation smoke requires a dedicated standalone process and isolated -UserDir containing PresentationSmokeUser."));return;
 }
 auto* GI=World->GetGameInstance<UCC_AdventureGameInstance>();if (!GI || !GI->IsMainMenuWorld()) return;
 auto Probe=MakeShared<FPresentationProbe>();Probe->Instance=GI;
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Probe](float Delta){return Probe->Tick(Delta);}),.05f);
}
FAutoConsoleCommandWithWorld PresentationSmokeCommand(TEXT("CC.PresentationSmoke"),TEXT("Isolated rendered home/intro/music/three-map travel verification."),FConsoleCommandWithWorldDelegate::CreateStatic(&RunPresentationSmoke));
}
#endif
