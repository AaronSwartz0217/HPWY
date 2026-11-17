#include "Progression/CC_ProgressionComponent.h"
#include "Progression/CC_SkillComponent.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Player/CC_PlayerController.h"
#include "World/CC_AdventureGameInstance.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "GameFramework/GameStateBase.h"
#include "Engine/World.h"

double UCC_ProgressionComponent::GetItemClock() const
{
 if(!GetWorld())return 0;
 if(const auto* State=GetWorld()->GetGameState())return State->GetServerWorldTimeSeconds();
 return GetWorld()->GetTimeSeconds();
}
float UCC_ProgressionComponent::GetItemCooldownRemaining() const
{
 return static_cast<float>(FMath::Max(0.,ItemCooldownEndsAt-GetItemClock()));
}
float UCC_ProgressionComponent::GetEquippedItemCooldownRemaining()const
{
 if(EquippedItem&&EquippedItem->Effect==ECC_ConsumableEffect::TravelScroll)
  if(const auto* GI=GetWorld()->GetGameInstance<UCC_AdventureGameInstance>())return GI->GetTravelCooldownRemaining();
 return GetItemCooldownRemaining();
}
UCC_ConsumableDefinition* UCC_ProgressionComponent::FindTravelScroll(FName Region)const
{
 for(UCC_ConsumableDefinition* Item:ItemCatalog)if(IsCatalogItem(Item)&&Item->Effect==ECC_ConsumableEffect::TravelScroll&&Item->DestinationRegion==Region)return Item;
 return nullptr;
}
bool UCC_ProgressionComponent::ConsumeTravelScroll(FName Region)
{
 if(!GetOwner()->HasAuthority())return false;
 auto* Item=FindTravelScroll(Region);auto* Stack=ItemStacks.FindByPredicate([Item](const FCC_ConsumableStack& S){return Item&&S.Item==Item&&S.Count>0;});
 if(!Stack)return false;
 --Stack->Count;Notify();return true;
}
int32 UCC_ProgressionComponent::GetItemCount(const UCC_ConsumableDefinition* Item) const
{
 const auto* Stack=ItemStacks.FindByPredicate([Item](const FCC_ConsumableStack& S){return Item && S.Item==Item;});
 return Stack?FMath::Max(0,Stack->Count):0;
}
int32 UCC_ProgressionComponent::GetItemStackLimit(const UCC_ConsumableDefinition* Item)const
{
 return Item?(bDeveloperInventory?999:Item->MaxStack):0;
}
bool UCC_ProgressionComponent::IsCatalogItem(const UCC_ConsumableDefinition* Item) const
{
 if(!IsValid(Item)||!Item->IsValidItem()||!ItemCatalog.Contains(Item))return false;
 int32 Matches=0;for(const auto& Entry:ItemCatalog)if(Entry&&Entry->ItemId==Item->ItemId)++Matches;
 return Matches==1;
}
void UCC_ProgressionComponent::ClientItemResult_Implementation(ECC_ItemResult Result){OnItemActionFinished.Broadcast(Result);}
void UCC_ProgressionComponent::BuyItem_Implementation(UCC_ConsumableDefinition* Item,int32 Quantity)
{
 const auto* Adventure=GetWorld()?GetWorld()->GetGameInstance<UCC_AdventureGameInstance>():nullptr;
 if(!IsReady() || (Adventure&&Adventure->bTravelling)){ClientItemResult(ECC_ItemResult::InvalidState);return;}
 if(!IsCatalogItem(Item)||Quantity<1||Quantity>GetItemStackLimit(Item)){ClientItemResult(ECC_ItemResult::InvalidItem);return;}
 if(!CanAccessTownShop(CCTownShops::ForItem(Item))){ClientItemResult(ECC_ItemResult::InvalidState);return;}
 const int32 Owned=GetItemCount(Item);
 if(Owned>GetItemStackLimit(Item)-Quantity){ClientItemResult(ECC_ItemResult::StackFull);return;}
 const int64 Cost=int64(Item->CopperPrice)*Quantity;
 if(Cost>CopperCoins){ClientItemResult(ECC_ItemResult::NotEnoughCoins);return;}
 auto* Stack=ItemStacks.FindByPredicate([Item](const FCC_ConsumableStack& S){return S.Item==Item;});
 if(!Stack){FCC_ConsumableStack New;New.Item=Item;ItemStacks.Add(New);Stack=&ItemStacks.Last();}
 CopperCoins-=int32(Cost);Stack->Count+=Quantity;
 if(!EquippedItem)EquippedItem=Item;
 Notify();ClientItemResult(ECC_ItemResult::Success);
 PersistTownPurchase();
}
void UCC_ProgressionComponent::EquipItem_Implementation(UCC_ConsumableDefinition* Item)
{
 if(!IsReady()){ClientItemResult(ECC_ItemResult::InvalidState);return;}
 if(Item && !IsCatalogItem(Item)){ClientItemResult(ECC_ItemResult::InvalidItem);return;}
 if(Item && GetItemCount(Item)<=0){ClientItemResult(ECC_ItemResult::OutOfStock);return;}
 EquippedItem=Item;Notify();ClientItemResult(ECC_ItemResult::Success);
}
void UCC_ProgressionComponent::UseEquippedItem_Implementation()
{
 ApplyConsumable(EquippedItem);
}

void UCC_ProgressionComponent::UseItem_Implementation(UCC_ConsumableDefinition* Item)
{
 ApplyConsumable(Item);
}

void UCC_ProgressionComponent::ApplyConsumable(UCC_ConsumableDefinition* Item)
{
 auto* Character=GetCharacter();
 const auto* PC=Character?Cast<ACC_PlayerController>(Character->GetController()):nullptr;
 if(!IsReady() || (PC&&PC->IsGameplayInputBlocked()) || (Character&&Character->Skills->IsStunned())){ClientItemResult(ECC_ItemResult::InvalidState);return;}
 if(!IsCatalogItem(Item)){ClientItemResult(ECC_ItemResult::InvalidItem);return;}
 auto* Stack=ItemStacks.FindByPredicate([Item](const FCC_ConsumableStack& S){return S.Item==Item;});
 if(!Stack||Stack->Count<=0){ClientItemResult(ECC_ItemResult::OutOfStock);return;}
 if(Item->Effect==ECC_ConsumableEffect::TravelScroll)
 {
  auto* GI=GetWorld()->GetGameInstance<UCC_AdventureGameInstance>();
  if(GI&&GI->GetTravelCooldownRemaining()>0){ClientItemResult(ECC_ItemResult::OnCooldown);return;}
  const bool Started=GI&&GI->RequestFastTravel(Item->DestinationRegion);
  ClientItemResult(Started?ECC_ItemResult::Success:ECC_ItemResult::InvalidState);return;
 }
 if(GetItemCooldownRemaining()>0){ClientItemResult(ECC_ItemResult::OnCooldown);return;}
 if(Item->Effect==ECC_ConsumableEffect::Experience && Experience>MAX_int32-FMath::RoundToInt(Item->Magnitude)){ClientItemResult(ECC_ItemResult::AlreadyFull);return;}
 if(Item->Effect==ECC_ConsumableEffect::ResetCooldowns)
 {
  bool Any=false;for(int32 Slot=0;Slot<5;++Slot)Any|=Character->Skills->GetCooldownRemaining(Slot)>0;
  if(!Any){ClientItemResult(ECC_ItemResult::AlreadyFull);return;}
 }
 --Stack->Count;ItemCooldownEndsAt=GetItemClock()+FMath::Max(15.f,ItemCooldownSeconds);
 switch(Item->Effect)
 {
 case ECC_ConsumableEffect::Experience:AddExperience(FMath::RoundToInt(Item->Magnitude));break;
 case ECC_ConsumableEffect::CooldownHaste:Character->Skills->ActivateCooldownHaste(Item->Duration,Item->Magnitude);break;
 case ECC_ConsumableEffect::ResetCooldowns:Character->Skills->RefreshSkillCooldowns();break;
 default:break;
 }
 Notify();ClientItemResult(ECC_ItemResult::Success);
}
void UCC_ProgressionComponent::ClearConsumableEffects()
{
 if(auto* Character=GetCharacter())if(Character->Skills)Character->Skills->ClearCooldownHaste();
}
