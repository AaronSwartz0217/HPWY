#include "Progression/CC_SkillComponent.h"
#include "Progression/CC_ConfiguredAbility.h"
#include "Characters/CC_BaseCharacter.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Engine/World.h"

namespace
{
FGameplayEffectQuery SkillCooldownQuery()
{
 FGameplayTagContainer Tags;
 for(int32 Slot=0;Slot<5;++Slot)Tags.AddTag(UCC_ConfiguredAbility::GetSlotCooldownTag(Slot));
 return FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(Tags);
}
}
double UCC_SkillComponent::CooldownClock() const
{
 if(const auto* State=GetWorld()->GetGameState())return State->GetServerWorldTimeSeconds();
 return GetWorld()->GetTimeSeconds();
}
float UCC_SkillComponent::GetCooldownHasteRemaining() const {return float(FMath::Max(0.,CooldownHasteEndsAt-CooldownClock()));}
void UCC_SkillComponent::ActivateCooldownHaste(float Duration,float Multiplier)
{
 if(!GetOwner()->HasAuthority())return;
 CooldownHasteStartsAt=CooldownClock();CooldownHasteEndsAt=CooldownHasteStartsAt+FMath::Clamp(Duration,.1f,60.f);CooldownHasteMultiplier=FMath::Clamp(Multiplier,1.f,3.f);
 GetOwner()->ForceNetUpdate();
}
void UCC_SkillComponent::ClearCooldownHaste(){CooldownHasteEndsAt=CooldownHasteStartsAt=0;CooldownHasteMultiplier=1.f;}
void UCC_SkillComponent::RefreshSkillCooldowns()
{
 if(!GetOwner()->HasAuthority())return;
 if(auto* Character=Cast<ACC_BaseCharacter>(GetOwner()))if(auto* ASC=Character->GetAbilitySystemComponent())ASC->RemoveActiveEffects(SkillCooldownQuery());
}
void UCC_SkillComponent::TickCooldownHaste(float Delta)
{
 if(CooldownHasteMultiplier<=1)return;
 const double Now=CooldownClock();
 const float Extra=float(FMath::Clamp(FMath::Min(Now,CooldownHasteEndsAt)-FMath::Max(Now-Delta,CooldownHasteStartsAt),0.,double(Delta)))*(CooldownHasteMultiplier-1.f);
 if(Extra>0)if(auto* Character=Cast<ACC_BaseCharacter>(GetOwner()))if(auto* ASC=Character->GetAbilitySystemComponent())
 {
  for(auto Handle:ASC->GetActiveEffects(SkillCooldownQuery()))if(const auto* Effect=ASC->GetActiveGameplayEffect(Handle))
  {
   const float Remaining=Effect->GetTimeRemaining(GetWorld()->GetTimeSeconds());
   if(Remaining<=Extra)ASC->RemoveActiveGameplayEffect(Handle);
   else ASC->ModifyActiveEffectStartTime(Handle,-Extra);
  }
 }
 if(Now>=CooldownHasteEndsAt)ClearCooldownHaste();
}
