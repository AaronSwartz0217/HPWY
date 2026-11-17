#include "Progression/CC_ConsumableDefinition.h"

bool UCC_ConsumableDefinition::IsValidItem() const
{
 if(ItemId.IsNone() || CopperPrice<1 || MaxStack<1 || MaxStack>999 || !FMath::IsFinite(Magnitude) || !FMath::IsFinite(Duration))return false;
 switch(Effect)
 {
 case ECC_ConsumableEffect::Experience:return Magnitude>=1 && Magnitude<=100000;
 case ECC_ConsumableEffect::CooldownHaste:return Magnitude>1 && Magnitude<=3 && Duration>0 && Duration<=60;
 case ECC_ConsumableEffect::ResetCooldowns:return true;
 case ECC_ConsumableEffect::TravelScroll:return DestinationRegion==TEXT("Arena")||DestinationRegion==TEXT("Heaven")||DestinationRegion==TEXT("Palace");
 default:return false;
 }
}
