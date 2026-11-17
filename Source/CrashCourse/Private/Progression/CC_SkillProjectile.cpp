#include "Progression/CC_SkillProjectile.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Progression/CC_SkillProjectile.h"
#include "Progression/CC_ConfiguredAbility.h"
#include "Progression/CC_GameplayData.h"
#include "Characters/CC_BaseCharacter.h"
#include "Characters/CC_LocomotionComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "NiagaraFunctionLibrary.h"

ACC_SkillProjectile::ACC_SkillProjectile()
{
	bReplicates = true; SetReplicateMovement(true);
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision); Collision->InitSphereRadius(16.f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Collision->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->ProjectileGravityScale = 0.f; Movement->bRotationFollowsVelocity = true;
	// Initialize supplies world-space velocity, including the spread/yaw offset.
	Movement->bInitialVelocityInLocalSpace = false;
 VisibleCore=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisiblePulseCore"));VisibleCore->SetupAttachment(Collision);
 VisibleCore->SetCollisionEnabled(ECollisionEnabled::NoCollision);VisibleCore->SetGenerateOverlapEvents(false);VisibleCore->SetCastShadow(false);VisibleCore->SetVisibility(false);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> CoreMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));if(CoreMesh.Succeeded())VisibleCore->SetStaticMesh(CoreMesh.Object);
 SetLifeSpan(5.f);
}
void ACC_SkillProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACC_SkillProjectile, Definition);
}
void ACC_SkillProjectile::BeginPlay()
{
 Super::BeginPlay();
 // Construction/registration can reset movement ignores. Apply them again here.
 Collision->IgnoreActorWhenMoving(GetOwner(),true);Collision->IgnoreActorWhenMoving(GetInstigator(),true);
 if(AActor* Source=GetOwner()){TArray<AActor*> Attached;Source->GetAttachedActors(Attached,true,true);for(auto* Actor:Attached)Collision->IgnoreActorWhenMoving(Actor,true);}
	if (!HasAuthority())
	{
		Movement->SetComponentTickEnabled(false);
		Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	OnRep_Presentation();
}
void ACC_SkillProjectile::OnRep_Presentation()
{
 if(GetNetMode()==NM_DedicatedServer||!Definition)return;
 if(VisibleCore){VisibleCore->SetVisibility(Definition->ProjectileCoreMaterial!=nullptr);if(Definition->ProjectileCoreMaterial){VisibleCore->SetMaterial(0,Definition->ProjectileCoreMaterial);VisibleCore->SetRelativeScale3D(FVector(.48f,.22f,.22f)*FMath::Max(.25f,Definition->EffectScale));}}
 if(Trail||!Definition->ProjectileNiagara)return;
	Trail = UNiagaraFunctionLibrary::SpawnSystemAttached(Definition->ProjectileNiagara, Collision, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
	if (Trail) Trail->SetRelativeScale3D(FVector(FMath::Max(0.01f, Definition->EffectScale)));
}
void ACC_SkillProjectile::Initialize(UCC_SkillDefinition* Skill, float Damage, AActor* Target)
{
	Definition = Skill; Power = Damage;
	Collision->IgnoreActorWhenMoving(GetOwner(), true);
	Movement->InitialSpeed = Movement->MaxSpeed = FMath::Max(1.f, Skill->ProjectileSpeed);
	Movement->Velocity = GetActorForwardVector() * Movement->InitialSpeed;
	RemainingHits = Skill->Behavior == ECC_SkillBehavior::Piercing ? FMath::Clamp(Skill->Count, 1, 32) : 1;
	if (Skill->Behavior == ECC_SkillBehavior::Homing && IsValid(Target))
	{
		Movement->bIsHomingProjectile = true;
		Movement->HomingTargetComponent = Target->GetRootComponent();
		Movement->HomingAccelerationMagnitude = Movement->InitialSpeed * 4.f;
	}
	SetLifeSpan(FMath::Clamp(Skill->Range / Movement->InitialSpeed, 0.1f, 20.f));
}
void ACC_SkillProjectile::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	ACC_BaseCharacter* Target = Cast<ACC_BaseCharacter>(OtherActor);
	if (!HasAuthority() || !Definition || !Target || !Target->IsAlive() || !IsValid(GetOwner()) || UCC_ConfiguredAbility::AreAllies(GetOwner(), Target) || HitActors.Contains(Target)) return;
	HitActors.Add(Target);
	UCC_ConfiguredAbility::ApplyEffect(GetOwner(), Target, UCC_DamageEffect::StaticClass(), Power);
	Target->Locomotion->PresentImpact(Definition, Target->GetActorLocation() + FVector(0, 0, 25));
	if (--RemainingHits <= 0) Destroy();
}
void ACC_SkillProjectile::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);
 if(FParse::Param(FCommandLine::Get(),TEXT("ParticleDiagnostics")))UE_LOG(LogTemp,Display,TEXT("PLAYER_PARTICLE_HIT actor=%s component=%s owner=%s pos=%s"),*GetNameSafe(Other),*GetNameSafe(OtherComp),*GetNameSafe(GetOwner()),*HitLocation.ToString());
	if (HasAuthority())
	{
		if (ACC_BaseCharacter* Source = Cast<ACC_BaseCharacter>(GetOwner())) Source->Locomotion->PresentImpact(Definition, HitLocation);
		Destroy();
	}
}
