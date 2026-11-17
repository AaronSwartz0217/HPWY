#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/IConsoleManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "World/CC_TownShop.h"
#include "World/CC_CoinSpawnDirector.h"
#include "Progression/CC_CoinPickup.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Progression/CC_ProgressionSave.h"
#include "Progression/CC_ProgressionHUD.h"
#include "Player/CC_PlayerController.h"
#include "Player/CC_PlayerState.h"
#include "Characters/CC_PlayerCharacter.h"
#include "UI/CC_MuralMenuWidget.h"
#include "UI/CC_FirstPersonScanWidget.h"
#include "Components/ComboBoxString.h"
#include "Components/ScrollBox.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "UObject/UObjectGlobals.h"
#include "Components/Button.h"
#include "Components/InputComponent.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "UObject/UnrealType.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

namespace
{
template<class T>T* ShopObject(UObject* Object,const TCHAR* Name){const auto* P=Object?FindFProperty<FObjectPropertyBase>(Object->GetClass(),Name):nullptr;return P?Cast<T>(P->GetObjectPropertyValue_InContainer(Object)):nullptr;}
struct FTownShopSmoke:TSharedFromThis<FTownShopSmoke>
{
 TWeakObjectPtr<UWorld> World;TWeakObjectPtr<ACC_PlayerController> PC;TWeakObjectPtr<ACC_PlayerCharacter> Player;TWeakObjectPtr<UCC_ProgressionComponent> P;TWeakObjectPtr<UCC_MuralMenuWidget> Menu;TWeakObjectPtr<ACC_TownShop> Inn;TWeakObjectPtr<ACameraActor> Camera;
 bool Passed=true;TSharedRef<FJsonObject> Checks=MakeShared<FJsonObject>();
 void Check(bool B,const FString& Name){Passed&=B;Checks->SetBoolField(Name,B);UE_LOG(LogTemp,Display,TEXT("TOWN_SHOP_CHECK %s: %d"),*Name,B);}
 void Later(float Delay,TFunction<void()> Fn){FTimerHandle Timer;World->GetTimerManager().SetTimer(Timer,MoveTemp(Fn),Delay,false);}
 void Capture(const TCHAR* Name){if(FParse::Param(FCommandLine::Get(),TEXT("TownShopCapture")))FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString(Name),true,false);}
 void Finish(){Checks->SetBoolField(TEXT("passed"),Passed);FString Text;FJsonSerializer::Serialize(Checks,TJsonWriterFactory<>::Create(&Text));FFileHelper::SaveStringToFile(Text,*(FPaths::ProjectSavedDir()/TEXT("TownShopSmoke.json")));GEngine->Exec(World.Get(),TEXT("QUIT"));}
 bool Click(const TCHAR* Name){auto* Button=ShopObject<UButton>(Menu.Get(),Name);const bool Good=Button&&Button->GetIsEnabled();Check(Good,FString(TEXT("button_"))+Name);if(Good)Button->OnClicked.Broadcast();return Good;}
 FVector Ground(FVector Location)
 {
  FHitResult Hit;FCollisionQueryParams Params;Params.AddIgnoredActor(Player.Get());
  if(World->LineTraceSingleByChannel(Hit,Location+FVector(0,0,1800),Location-FVector(0,0,2000),ECC_Visibility,Params))Location.Z=Hit.ImpactPoint.Z+Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+4;
  return Location;
 }
 void Run()
 {
  auto* W=World.Get();PC=Cast<ACC_PlayerController>(W->GetFirstPlayerController());Player=PC.IsValid()?Cast<ACC_PlayerCharacter>(PC->GetPawn()):nullptr;auto* State=Player.IsValid()?Player->GetPlayerState<ACC_PlayerState>():nullptr;P=State?State->Progression.Get():nullptr;
  if(!P.IsValid()){Check(false,TEXT("player_ready"));Finish();return;}
  P->ApplyDeveloperPreset(false);for(TActorIterator<ACC_CoinSpawnDirector> It(W);It;++It)It->Destroy();for(TActorIterator<ACC_CoinPickup> It(W);It;++It)It->Destroy();P->AddCopperCoins(200);P->AddExperience(500);
  TSet<ECC_TownShopId> Shops;
  for(TActorIterator<ACC_TownShop> It(W);It;++It)
  {
   Shops.Add(It->ShopId);if(It->ShopId==ECC_TownShopId::Inn)Inn=*It;
   const FVector Position=Ground(It->GetActorLocation());Player->SetActorLocation(Position,false,nullptr,ETeleportType::TeleportPhysics);
   FCollisionQueryParams Params;Params.AddIgnoredActor(Player.Get());Params.AddIgnoredActor(*It);
   const bool Free=!W->OverlapBlockingTestByChannel(Position,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(Player->GetCapsuleComponent()->GetScaledCapsuleRadius(),Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()),Params);
   Check(Free&&It->CanInteract(Player.Get())&&PC->FindInteractionTarget()==*It,CCTownShops::Name(It->ShopId).ToString()+TEXT("_doorway_is_reachable_and_interactive"));
  }
  Check(Shops.Num()==5&&Inn.IsValid(),TEXT("five_distinct_large_building_shops"));if(!Inn.IsValid()){Finish();return;}
  Player->SetActorLocation(Ground(FVector(300,-1000,200)));auto* Small=P->ItemCatalog[0].Get();P->BuyItem(Small,1);P->PurchaseRemoteShop();Check(!P->bRemoteShopUnlocked&&P->GetItemCount(Small)==0&&P->CopperCoins==200,TEXT("remote_purchases_locked_before_visiting_inn"));
  Camera=W->SpawnActor<ACameraActor>();Camera->SetActorLocation({0,-7600,6500});Camera->SetActorRotation(UKismetMathLibrary::FindLookAtRotation(Camera->GetActorLocation(),{0,0,200}));Camera->GetCameraComponent()->FieldOfView=65;PC->SetViewTarget(Camera.Get());
  auto Self=AsShared();Later(1,[Self](){Self->Capture(TEXT("TownShopOverview.png"));});
  Later(3,[Self]()
  {
   Self->Player->SetActorLocation(Self->Ground(Self->Inn->GetActorLocation()),false,nullptr,ETeleportType::TeleportPhysics);
   Self->Camera->SetActorLocation(Self->Inn->GetActorLocation()+FVector(0,-650,320));Self->Camera->SetActorRotation(UKismetMathLibrary::FindLookAtRotation(Self->Camera->GetActorLocation(),Self->Inn->GetActorLocation()+FVector(0,0,170)));
   Self->Later(1,[Self]()
   {
    auto* HUD=Self->PC->GetHUD<ACC_ProgressionHUD>();Self->Check(HUD&&HUD->FirstPersonScanHUD&&HUD->FirstPersonScanHUD->GetDisplayedTarget()==Self->Inn.Get()&&HUD->FirstPersonScanHUD->IsInteractionReady(),TEXT("doorway_shows_shop_interaction_prompt"));Self->Capture(TEXT("TownInnEntrance.png"));
    Self->Later(1,[Self]()
    {
     bool Key=false;const auto Interact=Self->PC->GetCombatKey(5);for(auto& Binding:Self->PC->InputComponent->KeyBindings)if(Binding.Chord.Key==Interact&&Binding.KeyEvent==IE_Pressed){Binding.KeyDelegate.Execute(Interact);Key=true;break;}
     Self->Menu=Cast<UCC_MuralMenuWidget>(Self->PC->GetProgressionMenuWidget());Self->Check(Key&&Self->Menu.IsValid()&&Self->Menu->GetBrowsingShop()==ECC_TownShopId::Inn,TEXT("interact_key_opens_inn_shop"));if(!Self->Menu.IsValid()){Self->Finish();return;}
     auto* Items=ShopObject<UComboBoxString>(Self->Menu.Get(),TEXT("ShopItemChoice"));Self->Check(Items&&Items->GetOptionCount()==3,TEXT("inn_sells_three_experience_potions"));
     Self->Later(1,[Self](){Self->Capture(TEXT("TownInnShop.png"));});
     Self->Later(2,[Self]()
     {
      Self->Click(TEXT("RemoteShopPurchaseButton"));Self->Check(Self->P->bRemoteShopUnlocked&&Self->P->CopperCoins==100,TEXT("inn_permit_unlocks_once_for_one_hundred_copper"));Self->P->PurchaseRemoteShop();Self->Check(Self->P->CopperCoins==100,TEXT("duplicate_permit_is_not_charged"));
      auto* Snapshot=Self->P->CreateSnapshot();Self->Check(Snapshot&&Snapshot->bRemoteShopUnlocked,TEXT("permit_is_in_saved_progress"));
      Self->PC->CloseProgressionMenu();Self->Player->SetActorLocation(Self->Ground({300,-1000,200}),false,nullptr,ETeleportType::TeleportPhysics);Self->Check(Self->P->GetLocalTownShop()==ECC_TownShopId::None,TEXT("remote_test_is_outside_all_stores"));
      Self->PC->OpenProgressionMenu();Self->Menu=Cast<UCC_MuralMenuWidget>(Self->PC->GetProgressionMenuWidget());Self->Menu->ShowPage(ECC_MuralMenuPage::Inventory);Self->Click(TEXT("RemoteShopShortcut"));auto* Choice=ShopObject<UComboBoxString>(Self->Menu.Get(),TEXT("TownShopChoice"));
      if(!Choice){Self->Check(false,TEXT("remote_store_selector"));Self->Finish();return;}
      auto* AllItems=ShopObject<UComboBoxString>(Self->Menu.Get(),TEXT("ShopItemChoice"));Self->Check(Choice->GetSelectedIndex()==0&&AllItems&&AllItems->GetOptionCount()==7,TEXT("remote_defaults_to_all_seven_items"));
      Self->Click(TEXT("BuyItemButton"));Self->Check(Self->P->GetItemCount(Self->P->ItemCatalog[0])==1,TEXT("remote_all_shops_view_can_purchase"));
      auto* Filter=ShopObject<UComboBoxString>(Self->Menu.Get(),TEXT("ItemCategoryChoice"));if(Filter)Filter->SetSelectedIndex(2);
      Choice->SetSelectedIndex(int32(ECC_TownShopId::Books));Self->Check(Filter&&Filter->GetSelectedIndex()==0&&AllItems&&AllItems->GetOptionCount()==2,TEXT("changing_shop_clears_stale_category_filter"));
      bool Categories=true;for(int32 Id=1;Id<=5;++Id){Choice->SetSelectedIndex(Id);Categories&=int32(Self->Menu->GetBrowsingShop())==Id&&Self->P->CanAccessTownShop(static_cast<ECC_TownShopId>(Id));}
      Self->Check(Categories,TEXT("backpack_remote_shop_opens_all_five_merchants"));Choice->SetSelectedIndex(int32(ECC_TownShopId::Bamboo));Self->Click(TEXT("BuyItemButton"));
      Self->Check(Self->P->GetItemCount(Self->P->ItemCatalog[3])==1,TEXT("remote_bamboo_purchase_grants_haste_potion"));Choice->SetSelectedIndex(int32(ECC_TownShopId::Ceramic));Self->Click(TEXT("BuyItemButton"));Self->Check(Self->P->GetItemCount(Self->P->ItemCatalog[4])==1,TEXT("remote_ceramic_purchase_grants_reset_potion"));
      Choice->SetSelectedIndex(int32(ECC_TownShopId::Bronze));auto* Upgrade=ShopObject<UComboBoxString>(Self->Menu.Get(),TEXT("UpgradeChoice"));Self->Check(Upgrade&&Upgrade->GetOptionCount()==2,TEXT("bronze_lists_attack_and_defense_training"));Self->Click(TEXT("PurchaseButton"));
      Self->Later(1,[Self](){Self->Capture(TEXT("TownRemoteShop.png"));Self->Later(1,[Self](){Self->Finish();});});
     });
    });
   });
  });
 }
};
void TownShopSmoke(UWorld* W){if(!W||!W->IsGameWorld()||W->GetNetMode()!=NM_Standalone)return;auto Probe=MakeShared<FTownShopSmoke>();Probe->World=W;Probe->Later(5,[Probe](){Probe->Run();});}
void ClickShopDropdown(UComboBoxString& Choice)
{
 auto& Slate = FSlateApplication::Get();
 const auto Window = Slate.FindWidgetWindow(Choice.TakeWidget());
 const FGeometry& Geometry = Choice.GetCachedGeometry();
 const FVector2D Point = Geometry.LocalToAbsolute(Geometry.GetLocalSize() * .5);
 const TSet<FKey> Pressed{EKeys::LeftMouseButton};
 const TSet<FKey> Released;
 const FModifierKeysState Modifiers;
 const auto Pointer = FSlateApplication::CursorPointerIndex;
 Slate.ProcessMouseMoveEvent(FPointerEvent(Pointer, Point, Point, Released, EKeys::Invalid, 0, Modifiers), true);
 Slate.ProcessMouseButtonDownEvent(Window ? Window->GetNativeWindow() : nullptr,
  FPointerEvent(Pointer, Point, Point, Pressed, EKeys::LeftMouseButton, 0, Modifiers));
 Slate.ProcessMouseButtonUpEvent(FPointerEvent(Pointer, Point, Point, Released, EKeys::LeftMouseButton, 0, Modifiers));
}

void TestTownShopPaintAfterGC(UWorld* World)
{
 if (!World || !World->IsGameWorld() || World->GetNetMode() != NM_Standalone)
  return;
 auto State = MakeShared<FTownShopSmoke>();
 State->World = World;
 State->Later(3.f, [State]
 {
  auto* Controller = Cast<ACC_PlayerController>(State->World->GetFirstPlayerController());
  auto* PlayerState = Controller ? Controller->GetPlayerState<ACC_PlayerState>() : nullptr;
  auto* Progression = PlayerState ? PlayerState->Progression.Get() : nullptr;
  State->Check(Progression && Controller->InputComponent, TEXT("player_ready"));
  if (!Progression || !Controller->InputComponent) { State->Finish(); return; }
  State->Check(Progression->ApplyDeveloperSandbox(), TEXT("test_permit_unlocked"));
  for (auto& Binding : Controller->InputComponent->KeyBindings)
  {
   if (Binding.Chord.Key == EKeys::Tab && Binding.KeyEvent == IE_Pressed)
   {
    Binding.KeyDelegate.Execute(EKeys::Tab);
    break;
   }
  }
  State->PC = Controller;
  State->Menu = Cast<UCC_MuralMenuWidget>(Controller->GetProgressionMenuWidget());
  State->Check(State->Menu.IsValid(), TEXT("tab_opens_progression_menu"));
  if (!State->Menu.IsValid()) { State->Finish(); return; }
  State->Menu->ShowPage(ECC_MuralMenuPage::Inventory);
  State->Click(TEXT("RemoteShopShortcut"));
  auto* Choice = ShopObject<UComboBoxString>(State->Menu.Get(), TEXT("ShopItemChoice"));
  auto* Scroll = ShopObject<UScrollBox>(State->Menu.Get(), TEXT("Scroll"));
  State->Check(Choice && Scroll && Choice->GetOptionCount() >= 5 &&
   State->Menu->GetMenuPage() == ECC_MuralMenuPage::Trade, TEXT("remote_shop_shows_items"));
  if (!Choice || !Scroll || Choice->GetOptionCount() == 0) { State->Finish(); return; }

  // A detached custom row has the same ownership as the selected item and popup rows.
  UWidget* Row = Choice->OnGenerateWidgetEvent.Execute(Choice->GetOptionAtIndex(0));
  State->Check(Row != nullptr, TEXT("generated_item_row"));
  if (!Row) { State->Finish(); return; }
  const TWeakObjectPtr<UWidget> WeakRow(Row);
  const TSharedRef<SWidget> SlateRow = Row->TakeWidget();
  CollectGarbage(RF_NoFlags);
  State->Check(WeakRow.IsValid(), TEXT("slate_keeps_generated_item_row_alive"));

  Scroll->ScrollWidgetIntoView(Choice, false, EDescendantScrollDestination::Center);
  State->Later(.5f, [State, Choice]
  {
   ClickShopDropdown(*Choice);
   State->Check(Choice->IsOpen(), TEXT("item_dropdown_open"));
   State->Later(.45f, [State, Choice]
   {
    CollectGarbage(RF_NoFlags);
    State->Later(.5f, [State, Choice]
    {
     State->Check(Choice->IsOpen(), TEXT("open_dropdown_renders_after_full_gc"));
     Choice->SetSelectedIndex(Choice->GetOptionCount() - 1);
     State->Later(.35f, [State, Choice]
     {
      State->Check(Choice->GetSelectedIndex() == Choice->GetOptionCount() - 1, TEXT("item_selection_still_works"));
      State->PC->CloseProgressionMenu();
      State->Finish();
     });
    });
   });
  });
 });
}

FAutoConsoleCommandWithWorld TownShopPaintCommand(TEXT("CC.TownShopPaintAfterGC"),
 TEXT("Open the remote shop through Tab, then render an open dropdown across a full object collection."),
 FConsoleCommandWithWorldDelegate::CreateStatic(&TestTownShopPaintAfterGC));

}
static FAutoConsoleCommandWithWorld TownShopCommand(TEXT("CC.TownShopSmoke"),TEXT("Test physical and backpack shops in the real town without saving normal progress."),FConsoleCommandWithWorldDelegate::CreateStatic(&TownShopSmoke));
#endif

