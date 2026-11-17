#include "UI/CC_MuralMenuWidget.h"
#include "CC_MuralChoiceRow.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"

void UCC_MuralMenuWidget::BuildItemShop()
{
 TownShopChoice=AddChoice(PageBoxes[2]);TownShopChoice->OnSelectionChanged.RemoveDynamic(this,&ThisClass::ChoiceChanged);
 for(int32 Id=0;Id<=5;++Id)TownShopChoice->AddOption(Id==0?TEXT("全部商店（远程权限）"):CCTownShops::Name(static_cast<ECC_TownShopId>(Id)).ToString());
 TownShopChoice->SetSelectedIndex(0);TownShopChoice->OnSelectionChanged.AddDynamic(this,&ThisClass::TownShopChanged);
 TownShopDetails=AddText(PageBoxes[2],TEXT(""),18);
 RemoteShopPurchaseButton=AddAction(PageBoxes[2],TEXT("购买远程商店通行契"));RemoteShopPurchaseButton->OnClicked.AddDynamic(this,&ThisClass::BuyRemoteShop);
 InnRestButton=AddAction(PageBoxes[2],TEXT("客栈休整"));InnRestButton->OnClicked.AddDynamic(this,&ThisClass::RestAtInn);
 ShopGoodsHeading=AddText(PageBoxes[2],TEXT("随身道具 · 铜钱购买"),23);
 ItemCategoryChoice=AddChoice(PageBoxes[2]);ItemCategoryChoice->OnSelectionChanged.RemoveDynamic(this,&ThisClass::ChoiceChanged);
 ItemCategoryChoice->AddOption(TEXT("全部道具"));ItemCategoryChoice->AddOption(TEXT("经验药水"));ItemCategoryChoice->AddOption(TEXT("冷却药水"));ItemCategoryChoice->AddOption(TEXT("传送卷轴"));ItemCategoryChoice->SetSelectedIndex(0);
 ItemCategoryChoice->OnSelectionChanged.AddDynamic(this,&ThisClass::ItemCategoryChanged);
 auto Preview=[this](UVerticalBox* Parent,TObjectPtr<UImage>& Icon)
 {
  auto* Row=WidgetTree->ConstructWidget<UHorizontalBox>();Parent->AddChildToVerticalBox(Row)->SetPadding(FMargin(0,0,0,14));
  auto* Size=WidgetTree->ConstructWidget<USizeBox>();Size->SetWidthOverride(64);Size->SetHeightOverride(64);Row->AddChildToHorizontalBox(Size)->SetPadding(FMargin(0,0,16,0));
  Icon=WidgetTree->ConstructWidget<UImage>();Size->SetContent(Icon);
  auto* DetailsText=AddText(nullptr,TEXT(""),18);Row->AddChildToHorizontalBox(DetailsText)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));return DetailsText;
 };
 ShopItemChoice=AddChoice(PageBoxes[2]);ShopItemChoice->OnGenerateWidgetEvent.BindDynamic(this,&ThisClass::GenerateItemChoice);ShopItemDetails=Preview(PageBoxes[2],ShopItemIcon);
 BuyItemButton=AddAction(PageBoxes[2],TEXT("购买 1 份"));BuyItemButton->OnClicked.AddDynamic(this,&ThisClass::BuySelectedItem);
 EquipShopItemButton=AddAction(PageBoxes[2],TEXT("配置到道具栏 [5]"));EquipShopItemButton->OnClicked.AddDynamic(this,&ThisClass::EquipShopItem);
 AddText(PageBoxes[0],TEXT("随身道具 · 道具栏 [5]"),25);
 BagItemChoice=AddChoice(PageBoxes[0]);BagItemChoice->OnGenerateWidgetEvent.BindDynamic(this,&ThisClass::GenerateItemChoice);BagItemDetails=Preview(PageBoxes[0],BagItemIcon);
 EquipBagItemButton=AddAction(PageBoxes[0],TEXT("配置到道具栏 [5]"));EquipBagItemButton->OnClicked.AddDynamic(this,&ThisClass::EquipBagItem);
}
UWidget* UCC_MuralMenuWidget::GenerateItemChoice(FString Label)
{
    const UCC_ConsumableDefinition* Item = nullptr;
    if (Progression)
    {
        for (const auto& Entry : Progression->ItemCatalog)
        {
            if (Entry && Entry->DisplayName.ToString() == Label)
            {
                Item = Entry.Get();
                break;
            }
        }
    }

    auto* Material = Item ? Item->IconMaterial.LoadSynchronous() : nullptr;
    auto* Texture = Item && !Material ? Item->Icon.LoadSynchronous() : nullptr;
    auto* Row = CreateWidget<UCC_MuralChoiceRow>(this);
    if (Row)
        Row->SetContent(Label, true, Texture, Material);
    return Row;
}
UCC_ConsumableDefinition* UCC_MuralMenuWidget::SelectedItem(bool bShop) const
{
 const auto* Choice=bShop?ShopItemChoice.Get():BagItemChoice.Get();const auto& Options=bShop?ShopItemOptions:BagItemOptions;
 const int32 Index=Choice?Choice->GetSelectedIndex():INDEX_NONE;return Options.IsValidIndex(Index)?Options[Index].Get():nullptr;
}
void UCC_MuralMenuWidget::PopulateItems()
{
 if(!Progression||!ShopItemChoice)return;
 TGuardValue<bool> Updating(bUpdatingItems,true);
 auto* OldShop=SelectedItem(true);auto* OldBag=SelectedItem(false);
 ShopItemOptions.Empty();BagItemOptions.Empty();ShopItemChoice->ClearOptions();BagItemChoice->ClearOptions();
 const int32 Category=ItemCategoryChoice->GetSelectedIndex();
 for(auto& Entry:Progression->ItemCatalog)if(auto* Item=Entry.Get())
 {
  BagItemOptions.Add(Item);BagItemChoice->AddOption(Item->DisplayName.ToString());
  const bool Experience=Item->Effect==ECC_ConsumableEffect::Experience;
  const bool bIsScroll=Item->Effect==ECC_ConsumableEffect::TravelScroll;
  if((CCTownShops::ForItem(Item)==SelectedTownShop||(bRemoteBrowsing&&SelectedTownShop==ECC_TownShopId::None&&Progression->bRemoteShopUnlocked))&&(Category<=0||(Category==1&&Experience)||(Category==2&&!Experience&&!bIsScroll)||(Category==3&&bIsScroll))){ShopItemOptions.Add(Item);ShopItemChoice->AddOption(Item->DisplayName.ToString());}
 }
 ShopItemChoice->SetSelectedIndex(FMath::Max(0,ShopItemOptions.IndexOfByKey(OldShop)));
 if(!OldBag)OldBag=Progression->EquippedItem.Get();
 BagItemChoice->SetSelectedIndex(FMath::Max(0,BagItemOptions.IndexOfByKey(OldBag)));
 PopulateShopUpgrades();
}
void UCC_MuralMenuWidget::RefreshItems()
{
 if(!Progression||!ShopItemDetails)return;
 RefreshTownShopContext();
 auto* Shop=SelectedItem(true);auto* Bag=SelectedItem(false);auto* Equipped=Progression->EquippedItem.Get();
 auto ShowIcon=[](UImage* Image,const UCC_ConsumableDefinition* Item){if(!Image)return;auto* Texture=Item?Item->Icon.LoadSynchronous():nullptr;auto* Material=Item?Item->IconMaterial.LoadSynchronous():nullptr;UObject* Resource=Material?static_cast<UObject*>(Material):static_cast<UObject*>(Texture);if(Image->GetBrush().GetResourceObject()!=Resource){if(Material)Image->SetBrushFromMaterial(Material);else Image->SetBrushFromTexture(Texture);}Image->SetVisibility(Resource?ESlateVisibility::HitTestInvisible:ESlateVisibility::Hidden);};
 ShowIcon(ShopItemIcon,Shop);ShowIcon(BagItemIcon,Bag);
 const bool EquippedScroll=Equipped&&Equipped->Effect==ECC_ConsumableEffect::TravelScroll;
 const FString ItemSlotDescription=FString::Printf(TEXT("第 5 栏：%s · %s%s"),Equipped?*Equipped->DisplayName.ToString():TEXT("未配置"),EquippedScroll?TEXT("传送共用30秒冷却"):TEXT("药水共用15秒冷却"),Progression->GetEquippedItemCooldownRemaining()>0?*FString::Printf(TEXT("（剩余 %.1f 秒）"),Progression->GetEquippedItemCooldownRemaining()):TEXT(""));
 ShopItemDetails->SetText(FText::FromString(Shop?FString::Printf(TEXT("持有 %d 铜钱\n%s\n价格 %d 铜钱 / 份 · 已有 %d / %d\n%s"),Progression->CopperCoins,*Shop->Description.ToString(),Shop->CopperPrice,Progression->GetItemCount(Shop),Progression->GetItemStackLimit(Shop),*ItemSlotDescription):TEXT("暂无商品")));
 BagItemDetails->SetText(FText::FromString(Bag?FString::Printf(TEXT("%s\n库存 %d 份\n%s\n关闭菜单后按数字 5 使用；卷轴也可从 N 行旅选择。"),*Bag->Description.ToString(),Progression->GetItemCount(Bag),*ItemSlotDescription):TEXT("暂无道具")));
 BuyItemButton->SetIsEnabled(Shop&&Progression->CanAccessTownShop(CCTownShops::ForItem(Shop))&&Progression->CopperCoins>=Shop->CopperPrice&&Progression->GetItemCount(Shop)<Progression->GetItemStackLimit(Shop));
 if(auto* Label=Cast<UTextBlock>(BuyItemButton->GetContent()))Label->SetText(FText::FromString(Shop&&Progression->GetItemCount(Shop)>=Progression->GetItemStackLimit(Shop)?TEXT("库存已满 · 无需购买"):TEXT("购买 1 份")));
 EquipShopItemButton->SetIsEnabled(Shop&&Progression->GetItemCount(Shop)>0&&Equipped!=Shop);
 EquipBagItemButton->SetIsEnabled(Bag&&Progression->GetItemCount(Bag)>0&&Equipped!=Bag);
}
void UCC_MuralMenuWidget::ItemCategoryChanged(FString Item,ESelectInfo::Type Type){PopulateItems();RefreshData();}
void UCC_MuralMenuWidget::BuySelectedItem(){if(auto* Item=SelectedItem(true)){PendingItemAction=TEXT("购买成功");Progression->BuyItem(Item,1);}}
void UCC_MuralMenuWidget::EquipShopItem(){if(auto* Item=SelectedItem(true)){PendingItemAction=TEXT("已配置到第 5 栏，关闭菜单后按 5 使用");Progression->EquipItem(Item);}}
void UCC_MuralMenuWidget::EquipBagItem(){if(auto* Item=SelectedItem(false)){PendingItemAction=TEXT("已配置到第 5 栏，关闭菜单后按 5 使用");Progression->EquipItem(Item);}}
void UCC_MuralMenuWidget::ItemResult(ECC_ItemResult Result)
{
 const TCHAR* Messages[]={TEXT("道具操作完成"),TEXT("铜钱不足"),TEXT("该道具已用尽"),TEXT("道具或传送正在冷却"),TEXT("当前状态无法使用道具"),TEXT("道具未配置或无效"),TEXT("该道具已达到携带上限"),TEXT("当前无需使用，道具未消耗")};
 SetFeedback(Result==ECC_ItemResult::Success&&!PendingItemAction.IsEmpty()?PendingItemAction:Messages[FMath::Clamp(int32(Result),0,7)],Result!=ECC_ItemResult::Success);PendingItemAction.Empty();RefreshItems();
}
