#include "UI/CC_MuralMenuWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Player/CC_PlayerController.h"
#include "Characters/CC_BaseCharacter.h"
#include "AbilitySystem/CC_AttributeSet.h"

void UCC_MuralMenuWidget::OpenTownShop(ECC_TownShopId Shop)
{
 bRemoteBrowsing=false;SelectedTownShop=Shop;
 {TGuardValue<bool> Guard(bUpdatingItems,true);TownShopChoice->SetSelectedIndex(int32(Shop));}
 PopulateItems();ShowPage(ECC_MuralMenuPage::Trade);
}
void UCC_MuralMenuWidget::OpenRemoteShop()
{
 if(!Progression||!Progression->bRemoteShopUnlocked){SetFeedback(TEXT("请先到小镇归灯客栈购买远程商店通行契。"),true);return;}
 bRemoteBrowsing=true;SelectedTownShop=ECC_TownShopId::None;
 {TGuardValue<bool> Guard(bUpdatingItems,true);ItemCategoryChoice->SetSelectedIndex(0);}
 {TGuardValue<bool> Guard(bUpdatingItems,true);TownShopChoice->SetSelectedIndex(int32(SelectedTownShop));}
 PopulateItems();ShowPage(ECC_MuralMenuPage::Trade);
}
void UCC_MuralMenuWidget::TownShopChanged(FString Item,ESelectInfo::Type Type)
{
 if(bUpdatingItems||!Progression)return;
 const auto New=static_cast<ECC_TownShopId>(TownShopChoice->GetSelectedIndex());
 if(!Progression->bRemoteShopUnlocked&&New!=Progression->GetLocalTownShop())return;
 SelectedTownShop=New;bRemoteBrowsing=Progression->GetLocalTownShop()!=New||New==ECC_TownShopId::None;
 {TGuardValue<bool> Guard(bUpdatingItems,true);ItemCategoryChoice->SetSelectedIndex(0);}
 PopulateItems();RefreshData();
}
void UCC_MuralMenuWidget::PopulateShopUpgrades()
{
 if(!UpgradeChoice||!Progression)return;
 auto* Previous=GetSelectedUpgrade();UpgradeChoice->ClearOptions();ShopUpgradeOptions.Empty();
 for(const auto& Upgrade:Progression->UpgradeCatalog)if(Upgrade&&Upgrade->Type!=ECC_UpgradeType::MaxMana&&(CCTownShops::ForUpgrade(Upgrade)==SelectedTownShop||(bRemoteBrowsing&&SelectedTownShop==ECC_TownShopId::None&&Progression->bRemoteShopUnlocked)))
 {
  const TCHAR* Names[]={TEXT("生命上限"),TEXT("法力上限"),TEXT("攻击力"),TEXT("防御力"),TEXT("移动速度"),TEXT("技能等级")};
  ShopUpgradeOptions.Add(Upgrade);UpgradeChoice->AddOption(Names[FMath::Clamp(int32(Upgrade->Type),0,5)]);
 }
 UpgradeChoice->SetSelectedIndex(FMath::Max(0,ShopUpgradeOptions.IndexOfByKey(Previous)));
}
UCC_UpgradeDefinition* UCC_MuralMenuWidget::GetSelectedUpgrade() const
{
 const int32 Index=UpgradeChoice?UpgradeChoice->GetSelectedIndex():INDEX_NONE;
 return ShopUpgradeOptions.IsValidIndex(Index)?ShopUpgradeOptions[Index].Get():nullptr;
}
void UCC_MuralMenuWidget::RefreshTownShopContext()
{
 const auto Local=Progression->GetLocalTownShop();
 if(SelectedTownShop==ECC_TownShopId::None&&!bRemoteBrowsing&&(Local!=ECC_TownShopId::None||Progression->bRemoteShopUnlocked))
 {
  SelectedTownShop=Local!=ECC_TownShopId::None?Local:ECC_TownShopId::Inn;bRemoteBrowsing=Local==ECC_TownShopId::None;
  {TGuardValue<bool> Guard(bUpdatingItems,true);TownShopChoice->SetSelectedIndex(int32(SelectedTownShop));}PopulateItems();
 }
 RemoteShopShortcut->SetIsEnabled(Progression->bRemoteShopUnlocked);
 RemoteShopHint->SetText(FText::FromString(Progression->bRemoteShopUnlocked?TEXT("通行契已解锁 · 随时购买各家店铺商品，价格与门店一致。"):FString::Printf(TEXT("尚未解锁 · 到小镇归灯客栈购买通行契（%d 铜币，一次购买永久使用）。"),Progression->RemoteShopPrice)));
 TownShopChoice->SetIsEnabled(Progression->bRemoteShopUnlocked);
 const bool AllShops=bRemoteBrowsing&&SelectedTownShop==ECC_TownShopId::None&&Progression->bRemoteShopUnlocked;
 const bool Access=AllShops||Progression->CanAccessTownShop(SelectedTownShop);
 FString Intro=(AllShops?FString(TEXT("全部商店 · 药水、传送卷轴及属性锻炼。可在上方切换单家店铺。")):CCTownShops::Description(SelectedTownShop).ToString())+FString::Printf(TEXT("\n持有 %d 铜币 · %d 经验"),Progression->CopperCoins,Progression->Experience);
 if(!Access&&CCTownShops::IsShop(SelectedTownShop))Intro+=TEXT("\n已离开门店；购买通行契后可远程交易。");
 TownShopDetails->SetText(FText::FromString(Intro));
 if(Page==ECC_MuralMenuPage::Trade){PageTitle->SetText(AllShops?FText::FromString(TEXT("远程商店 · 全部商品")):CCTownShops::Name(SelectedTownShop));PageSubtitle->SetText(FText::FromString(bRemoteBrowsing?TEXT("远程商店 · 各家店铺同价购入"):TEXT("小镇门店 · 铜钱买药，经验锻炼")));}
 const bool PhysicalInn=SelectedTownShop==ECC_TownShopId::Inn&&Local==ECC_TownShopId::Inn&&!bRemoteBrowsing;
 RemoteShopPurchaseButton->SetVisibility(PhysicalInn?ESlateVisibility::Visible:ESlateVisibility::Collapsed);RemoteShopPurchaseButton->SetIsEnabled(PhysicalInn&&!Progression->bRemoteShopUnlocked&&Progression->CopperCoins>=Progression->RemoteShopPrice);
 if(auto* Text=Cast<UTextBlock>(RemoteShopPurchaseButton->GetContent()))Text->SetText(FText::FromString(Progression->bRemoteShopUnlocked?TEXT("远程商店已解锁"):FString::Printf(TEXT("购买远程商店通行契 · %d 铜币"),Progression->RemoteShopPrice)));
 const auto* Character=Cast<ACC_BaseCharacter>(GetOwningPlayerPawn());const auto* Attributes=Character?Cast<UCC_AttributeSet>(Character->GetAttributeSet()):nullptr;const bool Wounded=Attributes&&Attributes->GetHealth()<Attributes->GetMaxHealth();
 InnRestButton->SetVisibility(PhysicalInn?ESlateVisibility::Visible:ESlateVisibility::Collapsed);InnRestButton->SetIsEnabled(PhysicalInn&&Wounded&&Progression->CopperCoins>=Progression->InnRestPrice);
 if(auto* Text=Cast<UTextBlock>(InnRestButton->GetContent()))Text->SetText(FText::FromString(Wounded?FString::Printf(TEXT("客栈休整 · 回满生命 · %d 铜币"),Progression->InnRestPrice):TEXT("生命已满 · 无需休整")));
 const bool HasGoods=AllShops||SelectedTownShop==ECC_TownShopId::Inn||SelectedTownShop==ECC_TownShopId::Ceramic||SelectedTownShop==ECC_TownShopId::Bamboo||SelectedTownShop==ECC_TownShopId::Books;
 ShopGoodsHeading->SetText(FText::FromString(AllShops?TEXT("药水与传送卷轴 · 铜钱购买"):SelectedTownShop==ECC_TownShopId::Books?TEXT("传送卷轴 · 铜钱购买"):TEXT("随身药水 · 铜钱购买")));
 for(UWidget* Widget:{static_cast<UWidget*>(ShopGoodsHeading.Get()),static_cast<UWidget*>(ItemCategoryChoice.Get()),static_cast<UWidget*>(ShopItemChoice.Get()),static_cast<UWidget*>(BuyItemButton.Get()),static_cast<UWidget*>(EquipShopItemButton.Get())})if(Widget)Widget->SetVisibility(HasGoods?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
 if(ShopItemIcon&&ShopItemIcon->GetParent()&&ShopItemIcon->GetParent()->GetParent())ShopItemIcon->GetParent()->GetParent()->SetVisibility(HasGoods?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
 const bool HasUpgrades=!ShopUpgradeOptions.IsEmpty();
 for(UWidget* Widget:{static_cast<UWidget*>(ShopUpgradeHeading.Get()),static_cast<UWidget*>(UpgradeChoice.Get()),static_cast<UWidget*>(PurchaseButton.Get())})if(Widget)Widget->SetVisibility(HasUpgrades?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
 if(GrowthShopSection)GrowthShopSection->SetVisibility((AllShops||SelectedTownShop==ECC_TownShopId::Books)?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
}
void UCC_MuralMenuWidget::BuyRemoteShop(){if(Progression){PendingItemAction=TEXT("远程商店已解锁，可在背包切换各家店铺");Progression->PurchaseRemoteShop();}}
void UCC_MuralMenuWidget::RestAtInn(){if(Progression){PendingItemAction=TEXT("休整完毕，生命已恢复");Progression->PurchaseInnRest();}}
