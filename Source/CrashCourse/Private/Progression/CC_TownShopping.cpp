#include "Progression/CC_ProgressionComponent.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Player/CC_PlayerController.h"
#include "Player/CC_PlayerState.h"
#include "World/CC_AdventureGameInstance.h"
#include "UI/CC_MuralMenuWidget.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

ECC_TownShopId UCC_ProgressionComponent::GetLocalTownShop() const
{
 const auto* Adventure=GetWorld()?GetWorld()->GetGameInstance<UCC_AdventureGameInstance>():nullptr;
 if(Adventure&&Adventure->RegionForMap(UGameplayStatics::GetCurrentLevelName(this,true))!=TEXT("Town"))return ECC_TownShopId::None;
 return IsValid(ActiveTownShop)&&ActiveTownShop->CanInteract(GetCharacter())?ActiveTownShop->ShopId:ECC_TownShopId::None;
}
bool UCC_ProgressionComponent::CanAccessTownShop(ECC_TownShopId Shop) const
{
 return CCTownShops::IsShop(Shop)&&(bRemoteShopUnlocked||GetLocalTownShop()==Shop);
}
void UCC_ProgressionComponent::EnterTownShop_Implementation(ACC_TownShop* Shop)
{
 // The demo ships without the town shop module; entering a shop is a no-op.
}
void UCC_ProgressionComponent::ClientOpenTownShop_Implementation(ECC_TownShopId Shop)
{
 // The demo ships without the town shop module; never open shop UI.
}
void UCC_ProgressionComponent::PurchaseRemoteShop_Implementation()
{
 if(!IsReady()||GetLocalTownShop()!=ECC_TownShopId::Inn){ClientItemResult(ECC_ItemResult::InvalidState);return;}
 if(bRemoteShopUnlocked){ClientItemResult(ECC_ItemResult::AlreadyFull);return;}
 const int32 Price=FMath::Max(1,RemoteShopPrice);if(CopperCoins<Price){ClientItemResult(ECC_ItemResult::NotEnoughCoins);return;}
 CopperCoins-=Price;bRemoteShopUnlocked=true;Notify();PersistTownPurchase();ClientItemResult(ECC_ItemResult::Success);
}
void UCC_ProgressionComponent::PurchaseInnRest_Implementation()
{
 if(!IsReady()||GetLocalTownShop()!=ECC_TownShopId::Inn){ClientItemResult(ECC_ItemResult::InvalidState);return;}
 auto* ASC=GetCharacter()->GetAbilitySystemComponent();
 const float MaxHealth=ASC->GetNumericAttribute(UCC_AttributeSet::GetMaxHealthAttribute());
 if(ASC->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute())>=MaxHealth){ClientItemResult(ECC_ItemResult::AlreadyFull);return;}
 const int32 Price=FMath::Max(1,InnRestPrice);if(CopperCoins<Price){ClientItemResult(ECC_ItemResult::NotEnoughCoins);return;}
 CopperCoins-=Price;ASC->SetNumericAttributeBase(UCC_AttributeSet::GetHealthAttribute(),MaxHealth);Notify();PersistTownPurchase();ClientItemResult(ECC_ItemResult::Success);
}
void UCC_ProgressionComponent::PersistTownPurchase()
{
 if(GetWorld()->GetNetMode()==NM_Standalone)if(auto* Adventure=GetWorld()->GetGameInstance<UCC_AdventureGameInstance>())Adventure->SaveAfterShopPurchase();
}
