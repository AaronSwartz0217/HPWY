#include "Progression/CC_SkillComponent.h"
#include "Progression/CC_ConfiguredAbility.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Characters/CC_BaseCharacter.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Player/CC_PlayerState.h"
#include "Player/CC_PlayerController.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"

bool UCC_SkillComponent::HasBlockingSkillAction() const
{
    const auto* C=Cast<ACC_BaseCharacter>(GetOwner());const auto* ASC=C?C->GetAbilitySystemComponent():nullptr;
    if(!ASC)return false;
    for(const auto& Spec:ASC->GetActivatableAbilities())if(Spec.IsActive())
        for(auto* Instance:Spec.GetAbilityInstances())if(const auto* A=Cast<UCC_ConfiguredAbility>(Instance);A&&A->BlocksNewSkillInput())return true;
    return false;
}
FGameplayAbilitySpecHandle UCC_SkillComponent::FindSkillHandle(int32 Slot) const
{
    if(Slot==4)return BasicAttackHandle;
    const auto* C=Cast<ACC_BaseCharacter>(GetOwner());const auto* ASC=C?C->GetAbilitySystemComponent():nullptr;
    if(ASC)for(const auto& Spec:ASC->GetActivatableAbilities())if(Spec.InputID==Slot&&Cast<UCC_SkillDefinition>(Spec.SourceObject.Get()))return Spec.Handle;
    return {};
}
bool UCC_SkillComponent::TrySkillInput(int32 Slot,AActor* Target,bool bAllowBuffer)
{
    auto* C=Cast<ACC_BaseCharacter>(GetOwner());auto* ASC=C?C->GetAbilitySystemComponent():nullptr;
    if(!C||!C->IsAlive()||!ASC)return false;
    const auto* PC=Cast<ACC_PlayerController>(C->GetController());if(PC&&PC->IsGameplayInputBlocked())return false;
    const auto Handle=FindSkillHandle(Slot);auto* Spec=ASC->FindAbilitySpecFromHandle(Handle);
    auto* Skill=Spec?Cast<UCC_SkillDefinition>(Spec->SourceObject.Get()):nullptr;
    if(!Skill)return false;
 if(IsStunned()&&Skill->DesignMechanic!=ECC_DesignMechanic::Invulnerable)return false;
 const bool Emergency=Skill->DesignMechanic==ECC_DesignMechanic::Invulnerable||Skill->DesignMechanic==ECC_DesignMechanic::Guard;
 if(Emergency&&!Spec->IsActive()&&!ASC->HasMatchingGameplayTag(UCC_ConfiguredAbility::GetSlotCooldownTag(Slot))&&ASC->GetNumericAttribute(UCC_AttributeSet::GetManaAttribute())>=Skill->ManaCost)
 {
  const auto* PS=C->GetPlayerState<ACC_PlayerState>();if(!PS||PS->Progression->IsSkillUnlocked(Skill))CancelPendingInput();
 }
    RequestedTarget=Target;
    const bool bActivated=ASC->TryActivateAbility(Handle);
    RequestedTarget.Reset();
    if(bActivated){ClearQueuedSkill();return true;}
    const float Window=FMath::Clamp(InputBufferSeconds,0.f,.35f),Cooldown=GetCooldownRemaining(Slot);
    if(!bAllowBuffer||Window<=0||!Cast<ACC_PlayerCharacter>(C)||Cooldown>Window||!FMath::IsFinite(Skill->ManaCost)||Skill->ManaCost<0||ASC->GetNumericAttribute(UCC_AttributeSet::GetManaAttribute())<Skill->ManaCost)return false;
    if(Spec->IsActive()&&Skill->DesignMechanic==ECC_DesignMechanic::Charge)return false;
    if(Skill->Behavior==ECC_SkillBehavior::Dash&&IsRooted())return false;
    const auto* PS=C->GetPlayerState<ACC_PlayerState>();if(PS&&!PS->Progression->IsSkillUnlocked(Skill))return false;
    if(Cooldown<=0&&!HasBlockingSkillAction()&&!Spec->IsActive())return false;
    QueuedSlot=Slot;QueuedTarget=Target;QueuedDefinition=Skill;QueuedUntil=GetWorld()->GetTimeSeconds()+Window;
    return true;
}
void UCC_SkillComponent::ClearQueuedSkill(){QueuedSlot=INDEX_NONE;QueuedUntil=0;QueuedTarget.Reset();QueuedDefinition.Reset();}
void UCC_SkillComponent::DrainQueuedSkill()
{
    if(QueuedSlot==INDEX_NONE)return;
    auto* C=Cast<ACC_BaseCharacter>(GetOwner());const auto* PC=C?Cast<ACC_PlayerController>(C->GetController()):nullptr;
    if(!C||!C->IsAlive()||IsStunned()||(PC&&PC->IsGameplayInputBlocked())||GetWorld()->GetTimeSeconds()>QueuedUntil){ClearQueuedSkill();return;}
    auto* ASC=C->GetAbilitySystemComponent();auto* Spec=ASC?ASC->FindAbilitySpecFromHandle(FindSkillHandle(QueuedSlot)):nullptr;
    if(!Spec||Spec->SourceObject.Get()!=QueuedDefinition.Get()||(QueuedDefinition->DesignMechanic==ECC_DesignMechanic::Charge&&!HeldSlots.Contains(QueuedSlot))){ClearQueuedSkill();return;}
    // Effect removal can lag the remaining-time calculation by one frame. Keep the input until its tag clears.
    if(HasBlockingSkillAction()||GetCooldownRemaining(QueuedSlot)>0||ASC->HasMatchingGameplayTag(UCC_ConfiguredAbility::GetSlotCooldownTag(QueuedSlot))||Spec->IsActive())return;
    const int32 Slot=QueuedSlot;auto Target=QueuedTarget;ClearQueuedSkill();TrySkillInput(Slot,Target.Get(),false);
}
void UCC_SkillComponent::CancelPendingInput()
{
    ClearQueuedSkill();HeldSlots.Empty();RequestedTarget.Reset();
    if(!GetOwner()->HasAuthority()){ServerCancelPendingInput();return;}
    auto* C=Cast<ACC_BaseCharacter>(GetOwner());auto* ASC=C?C->GetAbilitySystemComponent():nullptr;if(!ASC)return;
    TArray<FGameplayAbilitySpecHandle> Cancel;
    for(const auto& Spec:ASC->GetActivatableAbilities())if(Spec.IsActive())
        for(auto* Instance:Spec.GetAbilityInstances())if(const auto* A=Cast<UCC_ConfiguredAbility>(Instance);A&&A->BlocksNewSkillInput()){Cancel.AddUnique(Spec.Handle);break;}
    for(auto Handle:Cancel)ASC->CancelAbilityHandle(Handle);
}
void UCC_SkillComponent::ServerCancelPendingInput_Implementation(){CancelPendingInput();}
