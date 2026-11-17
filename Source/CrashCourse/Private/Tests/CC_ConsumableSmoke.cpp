#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/IConsoleManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Player/CC_PlayerController.h"
#include "Player/CC_PlayerState.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Characters/CC_PatrolEnemyCharacter.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Progression/CC_ProgressionHUD.h"
#include "Progression/CC_CoinPickup.h"
#include "World/CC_CoinSpawnDirector.h"
#include "World/CC_AmbientSpawnDirector.h"
#include "World/CC_ShanheChunkWorld.h"
#include "UI/CC_MuralMenuWidget.h"
#include "UI/CC_PlayerHealthHUDWidget.h"
#include "Components/ComboBoxString.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/InputComponent.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "UObject/UnrealType.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Kismet/GameplayStatics.h"
#include "UnrealClient.h"

namespace
{
template<class T>T* ItemProperty(UObject* Object,const TCHAR* Name)
{
 const auto* Property=Object?FindFProperty<FObjectPropertyBase>(Object->GetClass(),Name):nullptr;
 return Property?Cast<T>(Property->GetObjectPropertyValue_InContainer(Object)):nullptr;
}
struct FItemSmoke: TSharedFromThis<FItemSmoke>
{
 TWeakObjectPtr<UWorld> World;TWeakObjectPtr<ACC_PlayerController> Controller;TWeakObjectPtr<UCC_ProgressionComponent> Progression;TWeakObjectPtr<UCC_MuralMenuWidget> Menu;
 bool Passed=true;TSharedRef<FJsonObject> Checks=MakeShared<FJsonObject>();
 void Check(bool Good,const TCHAR* Name){Passed&=Good;Checks->SetBoolField(Name,Good);UE_LOG(LogTemp,Display,TEXT("ITEM_CHECK %s: %d"),Name,Good);}
 void Later(float Delay,TFunction<void()> Function){FTimerHandle Timer;World->GetTimerManager().SetTimer(Timer,MoveTemp(Function),Delay,false);}
 void Capture(const TCHAR* Name){if(FParse::Param(FCommandLine::Get(),TEXT("ItemCapture")))FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString(Name),true,false);}
 bool Click(const TCHAR* Name){auto* Button=ItemProperty<UButton>(Menu.Get(),Name);if(!Button||!Button->GetIsEnabled()){Check(false,Name);return false;}Button->OnClicked.Broadcast();return true;}
 void Finish(){Checks->SetBoolField(TEXT("passed"),Passed);FString Output;FJsonSerializer::Serialize(Checks,TJsonWriterFactory<>::Create(&Output));FFileHelper::SaveStringToFile(Output,*(FPaths::ProjectSavedDir()/FString::Printf(TEXT("ConsumableSmoke_%s.json"),*UGameplayStatics::GetCurrentLevelName(World.Get(),true))));GEngine->Exec(World.Get(),TEXT("QUIT"));}
 void Run()
 {
  auto* W=World.Get();Controller=Cast<ACC_PlayerController>(W->GetFirstPlayerController());auto* PC=Controller.Get();auto* PS=PC?PC->GetPlayerState<ACC_PlayerState>():nullptr;Progression=PS?PS->Progression.Get():nullptr;
  if(!Progression.IsValid()){Check(false,TEXT("player_ready"));Finish();return;}
  auto* P=Progression.Get();P->ApplyDeveloperPreset(false);P->AddCopperCoins(200);P->bRemoteShopUnlocked=true;
  for(TActorIterator<ACC_AmbientSpawnDirector> It(W);It;++It)It->bEnabled=false;
  for(TActorIterator<ACC_PatrolEnemyCharacter> It(W);It;++It){It->StopPatrol();It->bDetectPlayers=false;}
  PC->OpenProgressionMenu();Menu=Cast<UCC_MuralMenuWidget>(PC->GetProgressionMenuWidget());
  if(!Menu.IsValid()){Check(false,TEXT("menu_ready"));Finish();return;}
  Menu->ShowPage(ECC_MuralMenuPage::Trade);Check(P->ItemCatalog.Num()>=7,TEXT("potions_and_two_scrolls_configured"));
  auto* Choice=ItemProperty<UComboBoxString>(Menu.Get(),TEXT("ShopItemChoice"));Check(Choice&&Choice->GetOptionCount()==3,TEXT("inn_lists_three_experience_products"));
  Click(TEXT("BuyItemButton"));Click(TEXT("BuyItemButton"));Check(P->GetItemCount(P->ItemCatalog[0])==2&&P->CopperCoins==190,TEXT("buy_buttons_charge_coins_and_add_stock"));
  auto Self=AsShared();
  Later(1,[Self]()
  {
   auto* Image=ItemProperty<UImage>(Self->Menu.Get(),TEXT("ShopItemIcon"));auto* Texture=Image?Cast<UTexture2D>(Image->GetBrush().GetResourceObject()):nullptr;
   Self->Check(Texture&&Texture->GetSizeX()==128&&Texture->GetSizeY()==128,TEXT("shop_displays_128_pixel_icon"));
   // Loading a Texture2D can leave its render platform data compiling for a frame.
   for(const auto& Item:Self->Progression->ItemCatalog)Item->Icon.LoadSynchronous();
   Self->Capture(TEXT("ConsumableShop.png"));
  });
  Later(2,[Self]()
  {
   auto* P=Self->Progression.Get();auto* Category=ItemProperty<UComboBoxString>(Self->Menu.Get(),TEXT("ItemCategoryChoice"));auto* Choice=ItemProperty<UComboBoxString>(Self->Menu.Get(),TEXT("ShopItemChoice"));
   if(!Category||!Choice){Self->Check(false,TEXT("categories_present"));Self->Finish();return;}
   auto* Store=ItemProperty<UComboBoxString>(Self->Menu.Get(),TEXT("TownShopChoice"));if(!Store){Self->Finish();return;}
   Store->SetSelectedIndex(int32(ECC_TownShopId::Ceramic));Category->SetSelectedIndex(2);Self->Check(Choice->GetOptionCount()==1,TEXT("ceramic_lists_reset_potion"));Choice->SetSelectedIndex(0);Self->Click(TEXT("BuyItemButton"));Self->Click(TEXT("EquipShopItemButton"));Self->Check(P->EquippedItem==P->ItemCatalog[4],TEXT("shop_configures_slot_five"));
   Self->Menu->ShowPage(ECC_MuralMenuPage::Inventory);auto* Bag=ItemProperty<UComboBoxString>(Self->Menu.Get(),TEXT("BagItemChoice"));if(Bag)Bag->SetSelectedIndex(0);Self->Click(TEXT("EquipBagItemButton"));Self->Check(P->EquippedItem==P->ItemCatalog[0],TEXT("bag_configures_slot_five"));
   Self->Later(1,[Self]()
   {
    auto* PC=Self->Controller.Get();auto* P=Self->Progression.Get();PC->CloseProgressionMenu();bool Bound=false;
    for(auto& Binding:PC->InputComponent->KeyBindings)if(Binding.Chord.Key==EKeys::Five&&Binding.KeyEvent==IE_Pressed){Binding.KeyDelegate.Execute(EKeys::Five);Binding.KeyDelegate.Execute(EKeys::Five);Bound=true;break;}
    Self->Check(Bound&&P->Experience==50&&P->GetItemCount(P->ItemCatalog[0])==1,TEXT("digit_five_uses_once_and_rejects_spam"));Self->Check(P->GetItemCooldownRemaining()>14.8f,TEXT("slot_has_fifteen_second_cooldown"));
    auto* HUD=PC->GetHUD<ACC_ProgressionHUD>();if(HUD&&HUD->PlayerHealthHUD){HUD->PlayerHealthHUD->SetExpanded(true);HUD->PlayerHealthHUD->CycleSkill(1);HUD->PlayerHealthHUD->PressSelectedSkill();}
    Self->Later(1,[Self]()
    {
     auto* PC=Self->Controller.Get();auto* HUD=PC->GetHUD<ACC_ProgressionHUD>();auto* Widget=HUD?HUD->PlayerHealthHUD.Get():nullptr;
     Self->Check(Widget&&Widget->GetDisplayedCooldown(4)>0.f,TEXT("hud_tracks_item_cooldown"));Self->Capture(TEXT("ConsumableHUD.png"));
     Self->Later(15,[Self]()
     {
      auto* W=Self->World.Get();int32 Count=0,Cap=0;bool Values=true;
      for(TActorIterator<ACC_CoinSpawnDirector> It(W);It;++It)Cap=It->PopulationCap;
      for(TActorIterator<ACC_CoinPickup> It(W);It;++It){++Count;Values&=It->CopperAmount==5;}
      Self->Check(Cap>0&&Count>0&&Count<=Cap&&Values,TEXT("region_generates_bounded_five_coin_population"));Self->Checks->SetNumberField(TEXT("coinCount"),Count);Self->Checks->SetNumberField(TEXT("coinCap"),Cap);
      Self->Check(Self->Progression->GetItemCooldownRemaining()==0,TEXT("item_cooldown_expires_in_game"));
      bool Icons=true;for(const auto& Item:Self->Progression->ItemCatalog)
      {
       auto* Icon=Item->Icon.LoadSynchronous();const bool Good=Icon&&Icon->GetSizeX()==128&&Icon->GetSizeY()==128;Icons&=Good;
       if(!Good)UE_LOG(LogTemp,Error,TEXT("ITEM_ICON %s: %s %d x %d"),*Item->ItemId.ToString(),*Item->Icon.ToString(),Icon?Icon->GetSizeX():0,Icon?Icon->GetSizeY():0);
      }
      Self->Check(Icons,TEXT("five_low_resolution_icons_bound"));
      for(TActorIterator<ACC_ShanheChunkWorld> It(W);It;++It)Self->Check(It->GetLoadedChunkCount()==9,TEXT("shanhe_keeps_nine_chunks"));
      Self->Finish();
     });
    });
   });
  });
 }
};
void RunItemSmoke(UWorld* W)
{
 if(!W||!W->IsGameWorld()||W->GetNetMode()!=NM_Standalone)return;auto Probe=MakeShared<FItemSmoke>();Probe->World=W;Probe->Later(5,[Probe](){Probe->Run();});
}
}
static FAutoConsoleCommandWithWorld ItemSmoke(TEXT("CC.ConsumableSmoke"),TEXT("Check shop, slot 5 and regional coin spawning without saving player progress."),FConsoleCommandWithWorldDelegate::CreateStatic(&RunItemSmoke));
#endif
