#include "Characters/CC_LocomotionComponent.h"
#include "Characters/CC_BaseCharacter.h"
#include "Progression/CC_GameplayData.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "TimerManager.h"

namespace
{
void SpawnShieldCue(UWorld& World, UMaterialInterface* Material, FVector Location)
{
 auto* Sphere=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere"));
 if(!Sphere || !Material)return;

 auto* Cue=World.SpawnActor<AActor>(AActor::StaticClass(),Location,FRotator::ZeroRotator);
 if(!Cue)return;
 Cue->SetActorEnableCollision(false);
 Cue->SetLifeSpan(.55f);
 auto* Shell=NewObject<UStaticMeshComponent>(Cue);
 Cue->AddInstanceComponent(Shell);
 Cue->SetRootComponent(Shell);
 Shell->SetStaticMesh(Sphere);
 Shell->SetMaterial(0,Material);
 Shell->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Shell->SetCastShadow(false);
 Shell->SetCanEverAffectNavigation(false);
 Shell->RegisterComponent();
 Shell->SetWorldLocation(Location);
 Shell->SetWorldScale3D(FVector(1.32f,1.32f,1.9f));
 FTimerHandle Expand;
 World.GetTimerManager().SetTimer(Expand,FTimerDelegate::CreateWeakLambda(Shell,[Shell]()
 {
  Shell->SetWorldScale3D(FVector(1.6f,1.6f,2.1f));
 }),.20f,false);
}

void SpawnSkillFX(UWorld* World, const UCC_SkillDefinition* Skill, FVector Location, FRotator Rotation, bool bImpact)
{
	if (!World || !Skill || World->GetNetMode() == NM_DedicatedServer) return;
 // The player pulse uses a readable energy core and a short clean impact flash,
 // rather than inheriting the old projectile's camera-filling debris/sparks.
 if(Skill->ProjectileCoreMaterial)
 {
  if(!bImpact)return;
  auto* Pulse=World->SpawnActor<AActor>(AActor::StaticClass(),Location,Rotation);
  if(!Pulse)return;
  Pulse->Tags.Add(TEXT("CC.ParticleImpactFX"));Pulse->SetActorEnableCollision(false);Pulse->SetLifeSpan(.20f);
  auto* Mesh=NewObject<UStaticMeshComponent>(Pulse);Pulse->AddInstanceComponent(Mesh);Pulse->SetRootComponent(Mesh);
  Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));Mesh->SetMaterial(0,Skill->ProjectileCoreMaterial);Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);Mesh->SetCastShadow(false);Mesh->SetCanEverAffectNavigation(false);Mesh->RegisterComponent();Mesh->SetWorldLocation(Location);Mesh->SetWorldScale3D(FVector(.14f));
  FTimerHandle Expand,Shrink;
  World->GetTimerManager().SetTimer(Expand,FTimerDelegate::CreateWeakLambda(Pulse,[Mesh](){Mesh->SetWorldScale3D(FVector(.36f));}),.05f,false);
  World->GetTimerManager().SetTimer(Shrink,FTimerDelegate::CreateWeakLambda(Pulse,[Mesh](){Mesh->SetWorldScale3D(FVector(.06f));}),.13f,false);
  return;
 }
 if(bImpact && Skill->ShieldVisualMaterial)SpawnShieldCue(*World,Skill->ShieldVisualMaterial,Location);
	const FVector Scale(FMath::Max(0.01f, Skill->EffectScale));
	TArray<TWeakObjectPtr<UActorComponent>> Effects;
	if (UNiagaraSystem* System = bImpact ? Skill->ImpactNiagara.Get() : Skill->CastNiagara.Get())
		Effects.Add(UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, System, Location, Rotation, Scale));
	if (UParticleSystem* System = bImpact ? Skill->ImpactParticles.Get() : Skill->CastParticles.Get())
		Effects.Add(UGameplayStatics::SpawnEmitterAtLocation(World, System, Location, Rotation, Scale));
	if (Effects.IsEmpty()) return;
	FTimerHandle Timer;
	World->GetTimerManager().SetTimer(Timer, [Effects]()
	{
		for (const auto& Effect : Effects) if (Effect.IsValid()) Effect->DestroyComponent();
	}, FMath::Clamp(Skill->EffectLifetime, 0.1f, 10.f), false);
}
}

UCC_LocomotionComponent::UCC_LocomotionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.05f;
	SetIsReplicatedByDefault(true);
}
const FCC_MeshLocomotion* UCC_LocomotionComponent::GetCurrentEntry() const
{
	const ACC_BaseCharacter* Character = Cast<ACC_BaseCharacter>(GetOwner());
	if (!Library || !Character) return nullptr;
	const USkeletalMesh* Mesh = Character->GetMesh()->GetSkeletalMeshAsset();
	return Library->Entries.FindByPredicate([Mesh](const FCC_MeshLocomotion& E) { return E.Mesh == Mesh; });
}
void UCC_LocomotionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	ACC_BaseCharacter* Character = Cast<ACC_BaseCharacter>(GetOwner());
	if (!Character) return;
	// Reinitializing the animation instance while physics owns the pose would
	// reset the corpse, so leave the mesh untouched until HandleRespawn restores it.
	if (Character->IsRagdollActive()) return;
	USkeletalMeshComponent* Mesh = Character->GetMesh();
	const FCC_MeshLocomotion* Entry = GetCurrentEntry();
	if (!Entry) return;
    AlignMeshToGround();
	if (Entry->AnimClass)
	{
		if (Mesh->GetAnimationMode() != EAnimationMode::AnimationBlueprint || Mesh->GetAnimClass() != Entry->AnimClass)
			Mesh->SetAnimInstanceClass(Entry->AnimClass);
		return;
	}
	const float Speed = Character->IsAlive() ? Character->GetVelocity().Size2D() : 0.f;
	UAnimSequence* Clip = Speed > 5.f ? Entry->Run.Get() : Entry->Idle.Get();
	if (!Clip || Clip->GetSkeleton() != Mesh->GetSkeletalMeshAsset()->GetSkeleton()) return;
	UAnimSingleNodeInstance* Instance = Mesh->GetSingleNodeInstance();
	if (!Instance || Instance->GetCurrentAsset() != Clip) Mesh->PlayAnimation(Clip, true);
	if (UAnimSingleNodeInstance* Node = Mesh->GetSingleNodeInstance()) Node->SetPlayRate(Speed > 5.f ? FMath::Clamp(Speed / FMath::Max(1.f, Entry->ReferenceSpeed), 0.25f, 2.f) : 1.f);
}
void UCC_LocomotionComponent::PresentSkill(UCC_SkillDefinition* Skill, FVector TargetPoint, ECC_SkillPresentationPhase Phase)
{
	if (!GetOwner()->HasAuthority() || !Skill) return;
	ECC_AttackPose Pose = Skill->AttackPose;
	if (Skill->bAlternateAttackPose && Phase != ECC_SkillPresentationPhase::ChargeStart) { Pose = bNextCrossJab ? ECC_AttackPose::CrossJab : ECC_AttackPose::Jab; bNextCrossJab = !bNextCrossJab; }
	MulticastSkill(Skill, TargetPoint, Pose, Phase);
}
void UCC_LocomotionComponent::PresentImpact(UCC_SkillDefinition* Skill, FVector Location)
{
	if (GetOwner()->HasAuthority() && Skill) MulticastImpact(Skill, Location);
}
void UCC_LocomotionComponent::CancelPresentation()
{
	if (GetOwner()->HasAuthority()) MulticastCancel();
}
void UCC_LocomotionComponent::MulticastSkill_Implementation(UCC_SkillDefinition* Skill, FVector_NetQuantize TargetPoint, ECC_AttackPose Pose, ECC_SkillPresentationPhase Phase)
{
	ACC_BaseCharacter* Character = Cast<ACC_BaseCharacter>(GetOwner());
	if (!Character || !Character->IsAlive() || !Skill || GetWorld()->GetNetMode() == NM_DedicatedServer) return;
	const bool bPlayAttack = Pose != ECC_AttackPose::None && Phase != ECC_SkillPresentationPhase::ChargeStart;
	if (bPlayAttack) LastAttackPose = Pose;
	if (const FCC_MeshLocomotion* Entry = GetCurrentEntry(); Entry && bPlayAttack)
	{
		USkeletalMeshComponent* Mesh = Character->GetMesh();
		if (Entry->AnimClass && (Mesh->GetAnimationMode() != EAnimationMode::AnimationBlueprint || Mesh->GetAnimClass() != Entry->AnimClass)) Mesh->SetAnimInstanceClass(Entry->AnimClass);
		UAnimSequence* Clip = nullptr;
		switch(Pose)
		{
		case ECC_AttackPose::Uppercut: Clip=Entry->Uppercut; break;
		case ECC_AttackPose::CrossJab: Clip=Entry->CrossJab; break;
		case ECC_AttackPose::Cast: Clip=Entry->CastGesture ? Entry->CastGesture : Entry->CrossJab; break;
		case ECC_AttackPose::Guard: Clip=Entry->GuardStance; break;
		default: Clip=Entry->Jab; break;
		}
		if (Clip && Mesh->GetAnimInstance() && Clip->GetSkeleton() == Mesh->GetSkeletalMeshAsset()->GetSkeleton())
		{
			const float Rate=FMath::Max(.1f,Skill->AnimationPlayRate);
			const bool bFallbackCast = Pose == ECC_AttackPose::Cast && !Entry->CastGesture;
			if(bFallbackCast || Pose==ECC_AttackPose::Guard)
			{
				const float Portion=bFallbackCast?.42f:FMath::Min(1.f,.55f/FMath::Max(.01f,Clip->GetPlayLength()));
				if(auto* Montage=UAnimMontage::CreateSlotAnimationAsDynamicMontage_WithFractionalLoops(Clip,TEXT("DefaultSlot"),.09f,.13f,Portion))
					Mesh->GetAnimInstance()->Montage_Play(Montage,Rate);
			}
			else Mesh->GetAnimInstance()->PlaySlotAnimationAsDynamicMontage(Clip,TEXT("DefaultSlot"),.09f,.14f,Rate);
		}
	}
	if (Phase == ECC_SkillPresentationPhase::ChargeRelease || Phase == ECC_SkillPresentationPhase::AttackFollowThrough) return;
	const bool bGround = Skill->Behavior == ECC_SkillBehavior::GroundStrike || Skill->Behavior == ECC_SkillBehavior::DamageZone || Skill->DesignMechanic == ECC_DesignMechanic::RootArea;
	const FVector Location = bGround ? FVector(TargetPoint) : Character->GetActorLocation() + Character->GetActorForwardVector() * 55.f + FVector(0, 0, 25);
	SpawnSkillFX(GetWorld(), Skill, Location, Character->GetActorRotation(), false);
}
void UCC_LocomotionComponent::MulticastImpact_Implementation(UCC_SkillDefinition* Skill, FVector_NetQuantize Location)
{
	SpawnSkillFX(GetWorld(), Skill, Location, FRotator::ZeroRotator, true);
}
void UCC_LocomotionComponent::MulticastCancel_Implementation()
{
	if (const ACC_BaseCharacter* Character = Cast<ACC_BaseCharacter>(GetOwner()))
		if (UAnimInstance* Instance = Character->GetMesh()->GetAnimInstance()) Instance->Montage_Stop(0.08f);
}

void UCC_LocomotionComponent::AlignMeshToGround()
{
    auto* Character=Cast<ACC_BaseCharacter>(GetOwner());
    if (!bAlignMeshToCapsule || !Character || Character->IsRagdollActive()) return;
    USkeletalMeshComponent* Mesh=Character->GetMesh();
    const USkeletalMesh* Asset=Mesh->GetSkeletalMeshAsset();
    if (!Asset || !GetCurrentEntry()) return;
    const auto* Movement=Character->GetCharacterMovement();
    if (Movement->IsMovingOnGround() && Movement->CurrentFloor.IsWalkableFloor())
        LastGroundClearance=FMath::Max(0.f,Movement->CurrentFloor.GetDistanceToFloor())/
            FMath::Max(.001f,FMath::Abs(Character->GetActorScale3D().Z));
    // The reference bounds are stable through punches/jumps; animated foot motion stays intact.
    const FTransform RelativeGeometry(Mesh->GetRelativeRotation(),FVector::ZeroVector,Mesh->GetRelativeScale3D());
    const FBox ReferenceBox=Asset->GetBounds().GetBox().TransformBy(RelativeGeometry);
    const float TargetZ=-Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()-ReferenceBox.Min.Z-LastGroundClearance+VisualGroundOffset;
    FVector Location=Mesh->GetRelativeLocation();
    if (!FMath::IsNearlyEqual(Location.Z,TargetZ,.05f))
    {
        Location.Z=TargetZ;
        Mesh->SetRelativeLocation(Location);
        FVector Base=Character->GetBaseTranslationOffset();Base.Z=TargetZ;
        Character->CacheInitialMeshOffset(Base,Character->GetBaseRotationOffset().Rotator());
    }
}
