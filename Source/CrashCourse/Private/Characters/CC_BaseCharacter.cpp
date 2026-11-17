// Copyright Druid Mechanics


#include "CrashCourse/Public/Characters/CC_BaseCharacter.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "Progression/CC_SkillComponent.h"
#include "UI/CC_WidgetComponent.h"
#include "UI/CC_AttributeWidget.h"
#include "Characters/CC_LocomotionComponent.h"
#include "Characters/CC_EnemyCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PhysicsEngine/BodyInstance.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace CrashTags
{
	const FName Player = FName("Player");
}

ACC_BaseCharacter::ACC_BaseCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Locomotion = CreateDefaultSubobject<UCC_LocomotionComponent>(TEXT("Locomotion"));
	Skills = CreateDefaultSubobject<UCC_SkillComponent>(TEXT("Skills"));
	HealthDisplay = CreateDefaultSubobject<UCC_WidgetComponent>(TEXT("HealthDisplay"));
	HealthDisplay->SetupAttachment(GetRootComponent());
	HealthDisplay->SetRelativeLocation(FVector(0.f, 0.f, 125.f));
	HealthDisplay->SetWidgetSpace(EWidgetSpace::Screen);
	HealthDisplay->SetDrawSize(FVector2D(180.f, 32.f));
	HealthDisplay->SetWidgetClass(UCC_AttributeWidget::StaticClass());
	HealthDisplay->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Tick and refresh bone transforms whether rendered or not - for bone updates on a dedicated server
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	GetMesh()->bEnablePhysicsOnDedicatedServer = true;
}

void ACC_BaseCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, bAlive);
}

UAbilitySystemComponent* ACC_BaseCharacter::GetAbilitySystemComponent() const
{
	return nullptr;
}

void ACC_BaseCharacter::GiveStartupAbilities()
{
	if (!IsValid(GetAbilitySystemComponent())) return;
	
	for (const auto& Ability : StartupAbilities)
	{
		if (!Ability || GetAbilitySystemComponent()->FindAbilitySpecFromClass(Ability)) continue;
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(Ability);
		GetAbilitySystemComponent()->GiveAbility(AbilitySpec);
	}
}

void ACC_BaseCharacter::InitializeAttributes() const
{
	checkf(IsValid(InitializeAttributesEffect), TEXT("InitializeAttributesEffect not set."));

	FGameplayEffectContextHandle ContextHandle = GetAbilitySystemComponent()->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponent()->MakeOutgoingSpec(InitializeAttributesEffect, 1.f, ContextHandle);
	GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void ACC_BaseCharacter::OnHealthChanged(const FOnAttributeChangeData& AttributeChangeData)
{
	if (bAlive && AttributeChangeData.NewValue <= 0.f)
	{
		HandleDeath();
	}
}

void ACC_BaseCharacter::HandleDeath()
{
	if (!bAlive) return;
	bAlive = false;
	if (HasAuthority()) Skills->CancelActiveSkills();
	BeginDeathPresentation();
	if (HasAuthority()) ForceNetUpdate();
}

void ACC_BaseCharacter::HandleRespawn()
{
	bAlive = true;
	RestoreLivingPresentation();
	if (HasAuthority()) ForceNetUpdate();
}

void ACC_BaseCharacter::OnRep_Alive()
{
	if (bAlive) RestoreLivingPresentation();
	else BeginDeathPresentation();
}

void ACC_BaseCharacter::BeginDeathPresentation()
{
	if (bDeathPresentationActive) return;
	bDeathPresentationActive = true;
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	LivingMeshRelativeTransform = CharacterMesh->GetRelativeTransform();
	LivingMeshCollisionProfile = CharacterMesh->GetCollisionProfileName();
	LivingMeshCollision = CharacterMesh->GetCollisionEnabled();
	LivingMeshObjectType = CharacterMesh->GetCollisionObjectType();
	LivingMeshCollisionResponses = CharacterMesh->GetCollisionResponseToChannels();
	bLivingMeshPauseAnims = CharacterMesh->bPauseAnims;
	bLivingMeshGravity = CharacterMesh->IsGravityEnabled();
	bLivingMeshBlendPhysics = CharacterMesh->bBlendPhysics;
	LivingCapsuleCollision = GetCapsuleComponent()->GetCollisionEnabled();
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	LivingMovementMode = Movement->MovementMode;
	LivingCustomMovementMode = Movement->CustomMovementMode;
	DeathVelocity = GetVelocity().GetClampedToMaxSize(1000.f);
	Movement->StopMovementImmediately();
	Movement->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (HasAuthority() && IsA<ACC_EnemyCharacter>() && CorpseLifetime > 0.f)
	{
		GetWorldTimerManager().SetTimer(CorpseTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (!bAlive) Destroy();
		}), CorpseLifetime, false);
	}
	if (!bEnableDeathRagdoll || !GetWorld()) return;
	if (DeathToRagdollDelay <= 0.f) EnterRagdoll();
	else GetWorldTimerManager().SetTimer(RagdollTimer, this, &ThisClass::EnterRagdoll, DeathToRagdollDelay, false);
}

void ACC_BaseCharacter::EnterRagdoll()
{
	if (bAlive || !bDeathPresentationActive || bRagdollActive) return;
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	UPhysicsAsset* Physics = CharacterMesh->GetPhysicsAsset();
	// Some imported meshes have an empty PhysicsAsset, or bodies for a different
	// skeleton. Keep their Death animation instead of attempting an invalid simulation.
	const bool bHasUsableBody = Physics && Physics->SkeletalBodySetups.ContainsByPredicate([CharacterMesh](const USkeletalBodySetup* Body)
	{
		return IsValid(Body) && Body->AggGeom.GetElementCount() > 0 && CharacterMesh->GetBoneIndex(Body->BoneName) != INDEX_NONE;
	});
	if (!bHasUsableBody)
	{
		UE_LOG(LogTemp, Warning, TEXT("Death ragdoll skipped for %s: no physics body matches its mesh bones."), *GetName());
		return;
	}
	CharacterMesh->SetCollisionProfileName(TEXT("Ragdoll"));
	CharacterMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CharacterMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	CharacterMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	const bool bHasCreatedBody = CharacterMesh->Bodies.ContainsByPredicate([](const FBodyInstance* Body)
	{
		return Body && Body->IsValidBodyInstance();
	});
	if (!bHasCreatedBody)
	{
		CharacterMesh->SetCollisionProfileName(LivingMeshCollisionProfile);
		CharacterMesh->SetCollisionObjectType(LivingMeshObjectType);
		CharacterMesh->SetCollisionResponseToChannels(LivingMeshCollisionResponses);
		CharacterMesh->SetCollisionEnabled(LivingMeshCollision);
		UE_LOG(LogTemp, Warning, TEXT("Death ragdoll skipped for %s: physics bodies could not be created."), *GetName());
		return;
	}
	// Freeze the final animated pose, then let the constrained bodies continue it.
	CharacterMesh->RefreshBoneTransforms();
	CharacterMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CharacterMesh->bPauseAnims = true;
	CharacterMesh->SetEnableGravity(true);
	CharacterMesh->SetAllBodiesSimulatePhysics(true);
	CharacterMesh->SetSimulatePhysics(true);
	CharacterMesh->SetAllBodiesPhysicsBlendWeight(1.f);
	CharacterMesh->SetAllPhysicsLinearVelocity(DeathVelocity);
	CharacterMesh->WakeAllRigidBodies();
	bRagdollActive = true;
}

void ACC_BaseCharacter::RestoreLivingPresentation()
{
	GetWorldTimerManager().ClearTimer(RagdollTimer);
	GetWorldTimerManager().ClearTimer(CorpseTimer);
	if (!bDeathPresentationActive) return;
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (bRagdollActive)
	{
		CharacterMesh->SetAllPhysicsLinearVelocity(FVector::ZeroVector);
		CharacterMesh->SetAllPhysicsAngularVelocityInRadians(FVector::ZeroVector);
		CharacterMesh->SetAllBodiesSimulatePhysics(false);
		CharacterMesh->SetSimulatePhysics(false);
		CharacterMesh->SetAllBodiesPhysicsBlendWeight(0.f);
		CharacterMesh->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		CharacterMesh->SetRelativeTransform(LivingMeshRelativeTransform, false, nullptr, ETeleportType::TeleportPhysics);
	}
	CharacterMesh->bPauseAnims = bLivingMeshPauseAnims;
	CharacterMesh->bBlendPhysics = bLivingMeshBlendPhysics;
	CharacterMesh->SetEnableGravity(bLivingMeshGravity);
	CharacterMesh->SetCollisionProfileName(LivingMeshCollisionProfile);
	CharacterMesh->SetCollisionObjectType(LivingMeshObjectType);
	CharacterMesh->SetCollisionResponseToChannels(LivingMeshCollisionResponses);
	CharacterMesh->SetCollisionEnabled(LivingMeshCollision);
	GetCapsuleComponent()->SetCollisionEnabled(LivingCapsuleCollision);
	CharacterMesh->InitAnim(true);
	CharacterMesh->ResetAnimInstanceDynamics();
	GetCharacterMovement()->StopMovementImmediately();
	// A proxy may receive the server's dead MOVE_None before its Alive RepNotify.
	// Do not preserve that replicated death mode as the client's living mode.
	const EMovementMode RestoredMode = !HasAuthority() && LivingMovementMode == MOVE_None
		? MOVE_Walking : static_cast<EMovementMode>(LivingMovementMode);
	GetCharacterMovement()->SetMovementMode(RestoredMode, LivingCustomMovementMode);
	bRagdollActive = false;
	bDeathPresentationActive = false;
}

void ACC_BaseCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(RagdollTimer);
	GetWorldTimerManager().ClearTimer(CorpseTimer);
	Super::EndPlay(EndPlayReason);
}

void ACC_BaseCharacter::ResetAttributes()
{
	checkf(IsValid(ResetAttributesEffect), TEXT("ResetAttributesEffect not set."));
	if (!IsValid(GetAbilitySystemComponent())) return;

	FGameplayEffectContextHandle ContextHandle = GetAbilitySystemComponent()->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponent()->MakeOutgoingSpec(ResetAttributesEffect, 1.f, ContextHandle);
	GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}


