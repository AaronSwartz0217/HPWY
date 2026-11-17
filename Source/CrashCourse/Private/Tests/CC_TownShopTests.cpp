#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "World/CC_TownShop.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Progression/CC_ProgressionSave.h"
#include "Progression/CC_ConfiguredAbility.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Player/CC_PlayerState.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Kismet/GameplayStatics.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCC_TownShopTest,"CrashCourse.Progression.TownShopsAndRemotePermission",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCC_TownShopTest::RunTest(const FString&)
{
 UWorld::InitializationValues Values;Values.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
 auto* World=UWorld::CreateWorld(EWorldType::Game,false,TEXT("TownShopTests"),nullptr,true,ERHIFeatureLevel::Num,&Values);GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
 auto* PS=World->SpawnActor<ACC_PlayerState>();auto* Player=World->SpawnActor<ACC_PlayerCharacter>();Player->SetPlayerState(PS);
 auto* ASC=Player->GetAbilitySystemComponent();ASC->InitAbilityActorInfo(PS,Player);ASC->AddAttributeSetSubobject(Cast<UCC_AttributeSet>(Player->GetAttributeSet()));ASC->ApplyGameplayEffectToSelf(GetDefault<UCC_InitialAttributesEffect>(),1,ASC->MakeEffectContext());
 auto* P=PS->Progression.Get();P->NativeForm=LoadObject<UCC_FormDefinition>(nullptr,TEXT("/Game/HPWH/Forms/DA_Form_Native.DA_Form_Native"));P->InitializeAvatar();
 auto Item=[&](const TCHAR* Id){auto* Result=LoadObject<UCC_ConsumableDefinition>(nullptr,*FString::Printf(TEXT("/Game/HPWH/Items/DA_Item_%s.DA_Item_%s"),Id,Id));if(Result)P->ItemCatalog.Add(Result);return Result;};
 auto* XP=Item(TEXT("XP_Small"));auto* Haste=Item(TEXT("CooldownHaste"));auto* Reset=Item(TEXT("CooldownReset"));
 if(!XP||!Haste||!Reset){World->DestroyWorld(false);GEngine->DestroyWorldContext(World);return false;}
 auto* Inn=World->SpawnActor<ACC_TownShop>();Inn->ShopId=ECC_TownShopId::Inn;
 auto* Bronze=World->SpawnActor<ACC_TownShop>();Bronze->ShopId=ECC_TownShopId::Bronze;Bronze->SetActorLocation({2000,0,0});
 auto* Attack=NewObject<UCC_UpgradeDefinition>(PS);Attack->UpgradeId=TEXT("TestAttack");Attack->Type=ECC_UpgradeType::Attack;Attack->Prices={10,20};Attack->BonusPerLevel=2;P->UpgradeCatalog.Add(Attack);
 P->AddCopperCoins(200);P->AddExperience(100);P->BuyItem(XP,1);P->Purchase(Attack);P->PurchaseRemoteShop();
 TestEqual(TEXT("No remote item purchase without permit"),P->GetItemCount(XP),0);TestEqual(TEXT("No remote upgrades without permit"),P->GetUpgradeLevel(Attack),0);TestFalse(TEXT("Permit cannot be purchased from outside a store"),P->bRemoteShopUnlocked);TestEqual(TEXT("Rejected purchases charge nothing"),P->CopperCoins,200);
 P->EnterTownShop(Bronze);TestNull(TEXT("Distant interaction rejected"),P->ActiveTownShop.Get());
 P->EnterTownShop(Inn);TestTrue(TEXT("Doorway interaction opens local inn"),P->GetLocalTownShop()==ECC_TownShopId::Inn);P->BuyItem(XP,1);P->BuyItem(Haste,1);P->BuyItem(Reset,1);P->Purchase(Attack);
 TestEqual(TEXT("Local inn sells XP"),P->GetItemCount(XP),1);TestEqual(TEXT("Inn cannot sell another store's potion"),P->GetItemCount(Haste)+P->GetItemCount(Reset),0);TestEqual(TEXT("Inn cannot sell bronze upgrades"),P->GetUpgradeLevel(Attack),0);
 Player->SetActorLocation({2000,0,0});P->EnterTownShop(Bronze);P->PurchaseRemoteShop();TestFalse(TEXT("Only representative inn sells permit"),P->bRemoteShopUnlocked);P->Purchase(Attack);TestEqual(TEXT("Bronze sells attack training"),P->GetUpgradeLevel(Attack),1);
 Player->SetActorLocation({0,0,0});P->EnterTownShop(Inn);P->CopperCoins=99;P->PurchaseRemoteShop();TestFalse(TEXT("Permit requires sufficient copper"),P->bRemoteShopUnlocked);TestEqual(TEXT("Failed permit preserves wallet"),P->CopperCoins,99);
 P->CopperCoins=200;P->PurchaseRemoteShop();TestTrue(TEXT("Inn unlocks remote shop"),P->bRemoteShopUnlocked);TestEqual(TEXT("Permit costs exactly 100 copper"),P->CopperCoins,100);P->PurchaseRemoteShop();TestEqual(TEXT("Duplicate permit cannot charge twice"),P->CopperCoins,100);
 Player->SetActorLocation({6000,0,0});P->BuyItem(Haste,1);P->BuyItem(Reset,1);P->Purchase(Attack);TestEqual(TEXT("Permit opens bamboo remotely"),P->GetItemCount(Haste),1);TestEqual(TEXT("Permit opens ceramic remotely"),P->GetItemCount(Reset),1);TestEqual(TEXT("Permit opens bronze remotely"),P->GetUpgradeLevel(Attack),2);
 const auto* Snapshot=P->CreateSnapshot();P->bRemoteShopUnlocked=false;TestTrue(TEXT("Permission snapshot restores"),P->RestoreSnapshot(Snapshot));TestTrue(TEXT("Permit survives restore"),P->bRemoteShopUnlocked);TestNull(TEXT("Physical session never persists"),P->ActiveTownShop.Get());
 const FString Slot=TEXT("Codex_TownShops_TestOnly");TestTrue(TEXT("Save permit to isolated slot"),P->SaveProgress(Slot));P->bRemoteShopUnlocked=false;TestTrue(TEXT("Load permit from isolated slot"),P->LoadProgress(Slot));TestTrue(TEXT("Permit persists on disk"),P->bRemoteShopUnlocked);UGameplayStatics::DeleteGameInSlot(Slot,0);
 auto* Legacy=NewObject<UCC_ProgressionSave>();Legacy->Version=4;TestTrue(TEXT("Old v4 save remains compatible"),P->RestoreSnapshot(Legacy));TestFalse(TEXT("Old saves start with permit locked"),P->bRemoteShopUnlocked);
 P->CopperCoins=200;Player->SetActorLocation({6000,0,0});P->BuyItem(XP,1);TestEqual(TEXT("Leaving store invalidates local purchase"),P->CopperCoins,200);
 Player->SetActorLocation({0,0,0});P->EnterTownShop(Inn);ASC->SetNumericAttributeBase(UCC_AttributeSet::GetHealthAttribute(),20);P->PurchaseInnRest();TestEqual(TEXT("Inn rest restores health"),ASC->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute()),ASC->GetNumericAttribute(UCC_AttributeSet::GetMaxHealthAttribute()));TestEqual(TEXT("Inn rest charges five copper"),P->CopperCoins,195);P->PurchaseInnRest();TestEqual(TEXT("Rest at full health costs nothing"),P->CopperCoins,195);
 auto* Books=World->SpawnActor<ACC_TownShop>();Books->ShopId=ECC_TownShopId::Books;Books->SetActorLocation({4000,0,0});
 auto* Scroll=NewObject<UCC_ConsumableDefinition>(PS);Scroll->ItemId=TEXT("TestTravelScroll");Scroll->Effect=ECC_ConsumableEffect::TravelScroll;Scroll->DestinationRegion=TEXT("Arena");Scroll->CopperPrice=20;P->ItemCatalog.Add(Scroll);
 Player->SetActorLocation({0,0,0});P->EnterTownShop(Inn);P->BuyItem(Scroll,1);TestEqual(TEXT("Inn cannot sell paper-shop scrolls"),P->GetItemCount(Scroll),0);
 Player->SetActorLocation({4000,0,0});P->EnterTownShop(Books);P->BuyItem(Scroll,2);TestEqual(TEXT("Paper shop sells scrolls"),P->GetItemCount(Scroll),2);
 TestEqual(TEXT("Destination resolves its own scroll"),P->FindTravelScroll(TEXT("Arena")),Scroll);TestNull(TEXT("Missing destination scroll cannot be substituted"),P->FindTravelScroll(TEXT("Heaven")));
 const auto* ScrollSave=P->CreateSnapshot();P->ConsumeTravelScroll(TEXT("Arena"));TestEqual(TEXT("Exactly one scroll is spent"),P->GetItemCount(Scroll),1);TestTrue(TEXT("Scroll stock restores from save"),P->RestoreSnapshot(ScrollSave));TestEqual(TEXT("Scroll inventory round trips"),P->GetItemCount(Scroll),2);
 World->DestroyWorld(false);GEngine->DestroyWorldContext(World);return true;
}
#endif
