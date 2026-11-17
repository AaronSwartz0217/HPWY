#include "World/CC_TownShop.h"
#include "UI/CC_GameFont.h"
#include "Progression/CC_ConsumableDefinition.h"
#include "Progression/CC_GameplayData.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

bool CCTownShops::IsShop(ECC_TownShopId Id){return Id>=ECC_TownShopId::Inn&&Id<=ECC_TownShopId::Books;}
FText CCTownShops::Name(ECC_TownShopId Id)
{
 const TCHAR* Names[]={TEXT("选择店铺"),TEXT("归灯客栈"),TEXT("问斤铜铺"),TEXT("留火陶坊"),TEXT("听雨竹行"),TEXT("旧纸摊")};
 return FText::FromString(Names[FMath::Clamp(int32(Id),0,5)]);
}
FText CCTownShops::Description(ECC_TownShopId Id)
{
 const TCHAR* Texts[]={TEXT("进入小镇店铺后按交互键交易；归灯客栈出售远程商店权限。"),TEXT("阿照 · 小、中、大经验药水；门店休整；远程商店通行契。"),TEXT("严师傅 · 攻击与防御锻炼，跨形态保留。"),TEXT("陶婆 · 生命调养；一键刷新CD药水。"),TEXT("竹匠小满 · 机动锻炼；CD加速药水。"),TEXT("识字的钱掌柜 · 山河社稷、万象天宫传送卷轴；技艺进修与力量传习。")};
 return FText::FromString(Texts[FMath::Clamp(int32(Id),0,5)]);
}
ECC_TownShopId CCTownShops::ForItem(const UCC_ConsumableDefinition* Item)
{
 if(!Item)return ECC_TownShopId::None;
 switch(Item->Effect){case ECC_ConsumableEffect::Experience:return ECC_TownShopId::Inn;case ECC_ConsumableEffect::CooldownHaste:return ECC_TownShopId::Bamboo;case ECC_ConsumableEffect::ResetCooldowns:return ECC_TownShopId::Ceramic;case ECC_ConsumableEffect::TravelScroll:return ECC_TownShopId::Books;default:return ECC_TownShopId::None;}
}
ECC_TownShopId CCTownShops::ForUpgrade(const UCC_UpgradeDefinition* Upgrade)
{
 if(!Upgrade)return ECC_TownShopId::None;
 switch(Upgrade->Type){case ECC_UpgradeType::Attack:case ECC_UpgradeType::Defense:return ECC_TownShopId::Bronze;case ECC_UpgradeType::MaxHealth:return ECC_TownShopId::Ceramic;case ECC_UpgradeType::MoveSpeed:return ECC_TownShopId::Bamboo;case ECC_UpgradeType::SkillLevel:case ECC_UpgradeType::MaxMana:return ECC_TownShopId::Books;default:return ECC_TownShopId::None;}
}
TSharedRef<SWidget> UCC_TownShopSign::RebuildWidget()
{
 if(!WidgetTree->RootWidget)
 {
  auto* Frame=WidgetTree->ConstructWidget<UBorder>();WidgetTree->RootWidget=Frame;Frame->SetPadding(FMargin(12,6));Frame->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0,0,0,.8f),3.f,FLinearColor(.65f,.48f,.24f,.6f),1.f));
  Label=WidgetTree->ConstructWidget<UTextBlock>();auto Font= CCGameFont::Kai(Label->GetFont());Font.Size=17;Label->SetFont(Font);Label->SetColorAndOpacity(FLinearColor(.9f,.8f,.57f));Frame->SetContent(Label);Label->SetJustification(ETextJustify::Center);
 }
 if(auto* Shop=BoundShop.Get())SetShop(Shop);
 return Super::RebuildWidget();
}
void UCC_TownShopSign::SetShop(ACC_TownShop* Shop){BoundShop=Shop;if(Label&&Shop)Label->SetText(Shop->GetShopName());}
ACC_TownShop::ACC_TownShop()
{
 bReplicates=true;bAlwaysRelevant=true;PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickInterval=.2f;
 InteractionZone=CreateDefaultSubobject<UBoxComponent>(TEXT("Doorway"));SetRootComponent(InteractionZone);InteractionZone->InitBoxExtent({220,220,180});InteractionZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);InteractionZone->SetCollisionObjectType(ECC_WorldDynamic);InteractionZone->SetCollisionResponseToAllChannels(ECR_Ignore);InteractionZone->SetCollisionResponseToChannel(ECC_Pawn,ECR_Overlap);InteractionZone->SetCanEverAffectNavigation(false);
 Sign=CreateDefaultSubobject<UWidgetComponent>(TEXT("ShopSign"));Sign->SetupAttachment(RootComponent);Sign->SetRelativeLocation({0,0,175});Sign->SetWidgetSpace(EWidgetSpace::Screen);Sign->SetDrawAtDesiredSize(true);Sign->SetWidgetClass(UCC_TownShopSign::StaticClass());Sign->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
void ACC_TownShop::BeginPlay(){Super::BeginPlay();Sign->SetVisibility(false);}
void ACC_TownShop::Tick(float Delta)
{
 Super::Tick(Delta);Sign->SetVisibility(false);
}
bool ACC_TownShop::CanInteract(const ACC_PlayerCharacter* Player) const
{
 // The demo ships without the town shop module; shop actors stay inert.
 return false;
}
