#include "Progression/CC_ExperienceOrb.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Player/CC_PlayerState.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

ACC_ExperienceOrb::ACC_ExperienceOrb()
{
	bReplicates = true;
	SetReplicateMovement(true);
	SetNetUpdateFrequency(30.f);
	PrimaryActorTick.bCanEverTick = true;
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(7.f);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Collision->SetGenerateOverlapEvents(true);
	Collision->SetLinearDamping(0.3f);
	Particles = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Particles"));
	Particles->SetupAttachment(Collision);
	Particles->SetAutoActivate(false);
}
void ACC_ExperienceOrb::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACC_ExperienceOrb, Recipient);
	DOREPLIFETIME(ACC_ExperienceOrb, ExperienceValue);
	DOREPLIFETIME(ACC_ExperienceOrb, Phase);
	DOREPLIFETIME(ACC_ExperienceOrb, ParticleSystem);
}
void ACC_ExperienceOrb::BeginPlay()
{
	Super::BeginPlay();
	OnRep_ParticleSystem();
	if (HasAuthority())
	{
		SeekSpeed = InitialSeekSpeed;
		Collision->SetSimulatePhysics(true);
		Collision->SetEnableGravity(true);
		Collision->SetPhysicsLinearVelocity(LaunchVelocity);
	}
	else
	{
		// AActor replicates the server's physics state during the scatter phase.
		// Keep a valid physics collision mode for that simulated proxy; only the
		// authority performs homing and overlap-based rewards.
		Collision->SetGenerateOverlapEvents(false);
		SetActorTickEnabled(false);
	}
}
void ACC_ExperienceOrb::OnRep_ParticleSystem()
{
	if (GetNetMode() == NM_DedicatedServer || !ParticleSystem) return;
	if (Particles->GetAsset() != ParticleSystem) Particles->SetAsset(ParticleSystem);
	if (!Particles->IsActive()) Particles->Activate(true);
}
void ACC_ExperienceOrb::BeginSeeking()
{
	if (!HasAuthority() || Phase != ECC_ExperienceOrbPhase::Scattering) return;
	Phase = ECC_ExperienceOrbPhase::Seeking;
	Collision->SetSimulatePhysics(false);
	Collision->SetEnableGravity(false);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ForceNetUpdate();
}
void ACC_ExperienceOrb::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority() || Phase == ECC_ExperienceOrbPhase::Collected) return;
	Age += DeltaSeconds;
	if (!IsValid(Recipient) || Age >= MaximumLifetime) { Destroy(); return; }
	if (Phase == ECC_ExperienceOrbPhase::Scattering && Age >= ScatterDuration) BeginSeeking();
	if (Phase != ECC_ExperienceOrbPhase::Seeking) return;
	ACC_PlayerCharacter* Character = Cast<ACC_PlayerCharacter>(Recipient->GetPawn());
	if (!Character || !Character->IsAlive()) return;
	if (TryCollect(Character)) return;
	const FVector Target = Character->GetActorLocation() + FVector(0.f, 0.f, 25.f);
	SeekSpeed = FMath::Min(MaximumSeekSpeed, SeekSpeed + SeekAcceleration * DeltaSeconds);
	const FVector Delta = Target - GetActorLocation();
	const FVector Next = GetActorLocation() + Delta.GetClampedToMaxSize(SeekSpeed * DeltaSeconds);
	SetActorLocation(Next, true);
	if (!IsActorBeingDestroyed()) TryCollect(Character);
}
bool ACC_ExperienceOrb::TryCollect(ACC_PlayerCharacter* Character)
{
	if (!HasAuthority() || Phase != ECC_ExperienceOrbPhase::Seeking || !IsValid(Recipient) || !IsValid(Character) || !Character->IsAlive() || Character->GetPlayerState<ACC_PlayerState>() != Recipient || !Recipient->Progression) return false;
	// Test contact with the capsule volume, rather than accepting a remote request.
	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	const FVector ToOrb = GetActorLocation() - Capsule->GetComponentLocation();
	const float Radius = Capsule->GetScaledCapsuleRadius();
	const float HalfLine = FMath::Max(0.f, Capsule->GetScaledCapsuleHalfHeight() - Radius);
	const FVector Axis = Capsule->GetUpVector();
	const FVector Closest = Capsule->GetComponentLocation() + Axis * FMath::Clamp(FVector::DotProduct(ToOrb, Axis), -HalfLine, HalfLine);
	if (FVector::DistSquared(GetActorLocation(), Closest) > FMath::Square(Radius + Collision->GetScaledSphereRadius() + 2.f)) return false;
	Phase = ECC_ExperienceOrbPhase::Collected;
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (!GrantReward())
	{
		Phase = ECC_ExperienceOrbPhase::Seeking;
		Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		return false;
	}
	Particles->DeactivateImmediate();
	Destroy();
	return true;
}
bool ACC_ExperienceOrb::GrantReward()
{
	Recipient->Progression->AddExperience(ExperienceValue);
	return true;
}
void ACC_ExperienceOrb::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	TryCollect(Cast<ACC_PlayerCharacter>(OtherActor));
}
int32 ACC_ExperienceOrb::SpawnBurst(UObject* WorldContextObject, ACC_PlayerState* InRecipient, FVector Origin, int32 ExperienceAmount, UNiagaraSystem* System)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World || !IsValid(InRecipient) || !InRecipient->HasAuthority() || World->GetNetMode() == NM_Client || ExperienceAmount <= 0) return 0;
	if (!System)
		System = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/HPWH/Effects/NS_ExperienceOrb.NS_ExperienceOrb"));
	const int32 Count = FMath::Min(ExperienceAmount, 128);
	int32 Spawned = 0;
	for (int32 I = 0; I < Count; ++I)
	{
		const float Angle = (I + FMath::FRand()) * UE_TWO_PI / Count;
		const FVector Radial(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
		const FTransform Transform(FRotator::ZeroRotator, Origin + Radial * FMath::FRandRange(10.f, 25.f) + FVector(0.f, 0.f, FMath::FRandRange(5.f, 25.f)));
		ACC_ExperienceOrb* Orb = World->SpawnActorDeferred<ACC_ExperienceOrb>(ACC_ExperienceOrb::StaticClass(), Transform, InRecipient, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Orb) continue;
		Orb->Recipient = InRecipient;
		Orb->ExperienceValue = ExperienceAmount / Count + (I < ExperienceAmount % Count ? 1 : 0);
		Orb->ParticleSystem = System;
		Orb->LaunchVelocity = Radial * FMath::FRandRange(180.f, 320.f) + FVector(0.f, 0.f, FMath::FRandRange(260.f, 430.f));
		Orb->FinishSpawning(Transform);
		++Spawned;
	}
	return Spawned;
}
