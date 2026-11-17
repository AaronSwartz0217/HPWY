#if WITH_DEV_AUTOMATION_TESTS
#include "World/CC_AdventureGameInstance.h"
#include "World/CC_MapPortal.h"
#include "World/CC_TownShop.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Player/CC_PlayerController.h"
#include "Player/CC_PlayerState.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Progression/CC_ProgressionSave.h"
#include "Progression/CC_SkillComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Containers/Ticker.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Misc/CommandLine.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

namespace
{
struct FTravelBallProbe
{
 TWeakObjectPtr<UCC_AdventureGameInstance> Instance;
 TWeakObjectPtr<ACC_MapPortal> Ball;
 TSharedPtr<FJsonObject> Checks=MakeShared<FJsonObject>();
 bool Passed=true,bShopCaptured=false;int32 Phase=0;
 double Started=FPlatformTime::Seconds(),Next=0,SecondCastAt=0;
 float CastWorldTime=0;FVector StartPosition;
 void Check(bool Value,const TCHAR* Name){Passed&=Value;Checks->SetBoolField(Name,Value);UE_LOG(LogTemp,Display,TEXT("TRAVEL_BALL_CHECK %s: %d"),Name,Value);}
 void Capture(const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/TravelBall")/(FString(Name)+TEXT(".png")),true,false);}
 bool Finish(){auto Report=MakeShared<FJsonObject>();Report->SetBoolField(TEXT("passed"),Passed);Report->SetObjectField(TEXT("checks"),Checks);FString Json;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("TravelBallSmoke.json")));UE_LOG(LogTemp,Display,TEXT("TRAVEL_BALL_SMOKE_%s"),Passed?TEXT("PASS"):TEXT("FAIL"));FPlatformMisc::RequestExit(false);return false;}
 bool Tick(float)
 {
  const double Now=FPlatformTime::Seconds();auto* GI=Instance.Get();if(!GI)return Finish();
  if(Now-Started>180){Check(false,TEXT("timeout"));return Finish();}if(Now<Next)return true;
  auto* World=GI->GetWorld();if(!World||!World->HasBegunPlay())return true;
  auto* PC=Cast<ACC_PlayerController>(World->GetFirstPlayerController());auto* C=PC?Cast<ACC_PlayerCharacter>(PC->GetPawn()):nullptr;
  auto* PS=PC?PC->GetPlayerState<ACC_PlayerState>():nullptr;auto* P=PS?PS->Progression.Get():nullptr;if(!C||!P)return true;
  const FName Destination=FParse::Param(FCommandLine::Get(),TEXT("PalaceTravelSmoke")) ? TEXT("Palace") : TEXT("Heaven");
  auto* Heaven=P->FindTravelScroll(Destination);auto* Arena=P->FindTravelScroll(TEXT("Arena"));
  switch(Phase)
  {
   case 0:
   {
    P->ApplyDeveloperPreset(false);Check(GI->CurrentRegion==TEXT("Town"),TEXT("real_town_map"));
    Check(Heaven&&Arena&&!Heaven->IconMaterial.IsNull()&&!Arena->IconMaterial.IsNull(),TEXT("two_scroll_assets_and_procedural_icon"));
    if(!Heaven||!Arena)return Finish();
    Check(!GI->RequestFastTravel(Destination)&&!GI->IsPortalDeparturePending(),TEXT("missing_scroll_cannot_spawn_ball"));
    ACC_TownShop* Books=nullptr;for(TActorIterator<ACC_TownShop> It(World);It;++It)if(It->ShopId==ECC_TownShopId::Books)Books=*It;
    Check(Books!=nullptr,TEXT("paper_shop_exists"));if(!Books)return Finish();
    C->SetActorLocation(Books->GetActorLocation());C->GetCharacterMovement()->StopMovementImmediately();P->AddCopperCoins(200);P->EnterTownShop(Books);
    Check(P->GetLocalTownShop()==ECC_TownShopId::Books,TEXT("physical_paper_shop_access"));
    const int32 Money=P->CopperCoins;P->BuyItem(Heaven,2);P->BuyItem(Arena,2);
    Check(P->GetItemCount(Heaven)==2&&P->GetItemCount(Arena)==2&&P->CopperCoins==Money-100,TEXT("paper_shop_sells_both_scrolls_at_30_and_20"));
    Check(P->EquippedItem==Heaven,TEXT("scroll_equips_in_fifth_slot"));
    ++Phase;Next=Now+2.5;break;
   }
   case 1:
    if(!bShopCaptured){Capture(TEXT("PaperShop"));bShopCaptured=true;Next=Now+.3;return true;}
    PC->CloseProgressionMenu();StartPosition=C->GetActorLocation();P->UseEquippedItem();
    for(TActorIterator<ACC_MapPortal> It(World);It;++It)if(It->bTemporaryPortal)Ball=*It;
    Check(Ball.IsValid()&&Ball->IsDeparturePending()&&Ball->DepartureEffect!=nullptr,TEXT("fifth_slot_spawns_original_ball_effect"));
    Check(!GI->bTravelling&&GI->CurrentRegion==TEXT("Town")&&P->GetItemCount(Heaven)==2,TEXT("cast_waits_before_loading_and_does_not_consume"));
    Check(GI->GetTravelCooldownRemaining()>29&&GI->GetTravelCooldownRemaining()<=30,TEXT("cast_starts_thirty_second_shared_cooldown"));
    Check(!GI->RequestFastTravel(TEXT("Arena")),TEXT("cannot_spawn_second_destination_ball"));
    C->Skills->RefreshSkillCooldowns();Check(GI->GetTravelCooldownRemaining()>29,TEXT("skill_cooldown_reset_cannot_reset_travel"));
    CastWorldTime=World->GetTimeSeconds();++Phase;Next=Now+2.5;break;
   case 2:
    Capture(TEXT("DepartureBall"));C->SetActorLocation(StartPosition+FVector(2000,0,0));C->GetCharacterMovement()->StopMovementImmediately();++Phase;break;
   case 3:
    if(World->GetTimeSeconds()-CastWorldTime<8.2f)return true;
    Check(!Ball.IsValid()&&!GI->IsPortalDeparturePending()&&!GI->bTravelling&&GI->CurrentRegion==TEXT("Town"),TEXT("walking_away_removes_ball_at_eight_seconds_and_cancels"));
    Check(P->GetItemCount(Heaven)==2,TEXT("cancelled_cast_consumes_no_scroll"));
    Check(GI->GetTravelCooldownRemaining()>0&&!GI->RequestFastTravel(Destination),TEXT("cancelled_ball_still_respects_shared_cooldown"));
    if(FParse::Param(FCommandLine::Get(),TEXT("TravelBallPreviewOnly")))return Finish();
    ++Phase;break;
   case 4:
    if(GI->GetTravelCooldownRemaining()>0)return true;
    C->SetActorLocation(StartPosition);C->GetCharacterMovement()->StopMovementImmediately();
    Check(GI->RequestFastTravel(Destination),TEXT("new_ball_allowed_after_thirty_seconds"));
    SecondCastAt=Now;CastWorldTime=World->GetTimeSeconds();++Phase;break;
   case 5:
    if(GI->bTravelling||GI->CurrentRegion==Destination){Check(Now-SecondCastAt>=7.9,TEXT("travel_never_begins_before_eight_seconds"));++Phase;}
    break;
   case 6:
    if(GI->bTravelling)return true;
    Check(GI->CurrentRegion==Destination,TEXT("staying_near_ball_arrives_in_heaven"));
    Check(P->GetItemCount(Heaven)==1&&P->GetItemCount(Arena)==2,TEXT("arrival_consumes_exactly_one_matching_scroll"));
    Check(FMath::Abs(GI->GetTravelCooldownRemaining()-FMath::Max(0.,30.-(Now-SecondCastAt)))<.3,TEXT("cooldown_continues_across_map_change"));
    {const auto* Save=P->CreateSnapshot();Check(Save&&Save->ConsumableCounts.FindRef(FName(*(TEXT("Travel_")+Destination.ToString())))==1,TEXT("consumed_scroll_is_in_progress_snapshot"));}
    Capture(TEXT("Arrived"));++Phase;Next=Now+.5;break;
   default:return Finish();
  }
  return true;
 }
};
static FAutoConsoleCommandWithWorld Command(TEXT("CC.TravelBallSmoke"),TEXT("Isolated rendered paper-shop, cancellation, cooldown and successful scroll travel check."),FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World){
 if(!World||World->GetNetMode()!=NM_Standalone||!FPaths::ProjectSavedDir().Contains(TEXT("TravelBallSmokeUser")))return;
 auto* GI=World->GetGameInstance<UCC_AdventureGameInstance>();if(!GI)return;
 auto Probe=MakeShared<FTravelBallProbe>();Probe->Instance=GI;
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Probe](float Delta){return Probe->Tick(Delta);}),.05f);
}));
}
#endif
