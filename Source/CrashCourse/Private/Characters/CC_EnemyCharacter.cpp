// Copyright Druid Mechanics


#include "Characters/CC_EnemyCharacter.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AIController.h"
#include "AbilitySystem/CC_AbilitySystemComponent.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "GameplayTags/CCTags.h"
#include "Net/UnrealNetwork.h"
#include "Progression/CC_SkillComponent.h"
#include "Progression/CC_GameplayData.h"
#include "Components/SkeletalMeshComponent.h"
#include "UI/CC_WidgetComponent.h"


ACC_EnemyCharacter::ACC_EnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	HealthDisplay->SetVisibility(false);

	AbilitySystemComponent = CreateDefaultSubobject<UCC_AbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UCC_AttributeSet>("AttributeSet");
}

void ACC_EnemyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, bIsBeingLaunched);
	DOREPLIFETIME(ThisClass, ProfileMeshScale);
}

UAbilitySystemComponent* ACC_EnemyCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UAttributeSet* ACC_EnemyCharacter::GetAttributeSet() const
{
	return AttributeSet;
}

void ACC_EnemyCharacter::StopMovementUntilLanded()
{
	bIsBeingLaunched = true;
	AAIController* AIController = GetController<AAIController>();
	if (!IsValid(AIController)) return;
	AIController->StopMovement();
	if (!LandedDelegate.IsAlreadyBound(this, &ThisClass::EnableMovementOnLanded))
	{
		LandedDelegate.AddDynamic(this, &ThisClass::EnableMovementOnLanded);
	}
}

void ACC_EnemyCharacter::EnableMovementOnLanded(const FHitResult& Hit)
{
	bIsBeingLaunched = false;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, CCTags::Events::Enemy::EndAttack, FGameplayEventData());
	LandedDelegate.RemoveAll(this);
}

void ACC_EnemyCharacter::BeginPlay()
{
	// Older enemy blueprints can have a visible component template.
	HealthDisplay->SetVisibility(false);
	Super::BeginPlay();

	if (!IsValid(GetAbilitySystemComponent())) return;

	GetAbilitySystemComponent()->InitAbilityActorInfo(this, this);
	OnASCInitialized.Broadcast(GetAbilitySystemComponent(), GetAttributeSet());

	if (!HasAuthority()) return;

	GiveStartupAbilities();
	InitializeAttributes();
	ApplyCombatProfile();
	Skills->Initialize();

	UCC_AttributeSet* CC_AttributeSet = Cast<UCC_AttributeSet>(GetAttributeSet());
	if (!IsValid(CC_AttributeSet)) return;
	
	GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(CC_AttributeSet->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
}

void ACC_EnemyCharacter::HandleDeath()
{
	Super::HandleDeath();

	AAIController* AIController = GetController<AAIController>();
	if (!IsValid(AIController)) return;
	AIController->StopMovement();
}


bool ACC_EnemyCharacter::ApplyCombatProfile()
{
 if (!HasAuthority() || !CombatBalance || !GetAbilitySystemComponent()) return false;
 float Health = 100.f, Attack = 15.f;
 const USkeletalMesh* ProfileMesh = GetMesh()->GetSkeletalMeshAsset();
 if (!CombatBalance->Resolve(StatProfileId, Health, Attack, ProfileMesh)) return false;
 GetAbilitySystemComponent()->SetNumericAttributeBase(UCC_AttributeSet::GetMaxHealthAttribute(), Health);
 GetAbilitySystemComponent()->SetNumericAttributeBase(UCC_AttributeSet::GetHealthAttribute(), Health);
 GetAbilitySystemComponent()->SetNumericAttributeBase(UCC_AttributeSet::GetAttackAttribute(), Attack);
 const FCC_CombatProfile* Profile = CombatBalance->FindProfile(StatProfileId, ProfileMesh);
 ExperienceReward = CombatBalance->ResolveExperienceReward(StatProfileId, ProfileMesh);
 CopperReward = CombatBalance->ResolveCopperReward(StatProfileId, ProfileMesh);
 if (Profile && Profile->Loadout) Skills->DefaultLoadout = Profile->Loadout;
	if (OriginalProfileMeshScale.IsNearlyZero()) OriginalProfileMeshScale = GetMesh()->GetRelativeScale3D();
	ProfileMeshScale = OriginalProfileMeshScale * (Profile && Profile->bElite ? Profile->EliteScale : 1.f);
	OnRep_ProfileScale();
 return true;
}

void ACC_EnemyCharacter::OnRep_ProfileScale() { if (!ProfileMeshScale.IsNearlyZero()) GetMesh()->SetRelativeScale3D(ProfileMeshScale); }
