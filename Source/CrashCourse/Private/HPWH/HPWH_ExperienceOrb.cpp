#include "HPWH/HPWH_ExperienceOrb.h"

#include "HPWH/HPWH_GameMode.h"
#include "HPWH/HPWH_Player.h"

#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

AHPWH_ExperienceOrb::AHPWH_ExperienceOrb()
{
	PrimaryActorTick.bCanEverTick = true;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(12.f);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Collision->SetGenerateOverlapEvents(true);
	Collision->SetLinearDamping(0.55f);

	Particles = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Particles"));
	Particles->SetupAttachment(Collision);
	Particles->SetAutoActivate(true);

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> OrbSystem(
		TEXT("/Game/HPWH/Effects/NS_ExperienceOrb.NS_ExperienceOrb"));
	if (OrbSystem.Succeeded())
	{
		Particles->SetAsset(OrbSystem.Object);
	}

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Collision);
	Glow->SetIntensity(220.f);
	Glow->SetAttenuationRadius(320.f);
	Glow->SetLightColor(FLinearColor(0.45f, 0.82f, 1.f));

	Tags.Add(TEXT("ExperienceOrb"));
}

void AHPWH_ExperienceOrb::BeginPlay()
{
	Super::BeginPlay();

	// 沿用 GAS 经验球的物理散射阶段。
	Collision->SetSimulatePhysics(true);
	Collision->SetEnableGravity(true);
	Collision->SetPhysicsLinearVelocity(LaunchVelocity);
}

void AHPWH_ExperienceOrb::BeginSeeking()
{
	if (bSeeking)
	{
		return;
	}
	bSeeking = true;
	Age = 0.f;
	// 追踪阶段关闭物理，只保留 Pawn 重叠，避免被地面弹开。
	Collision->SetSimulatePhysics(false);
	Collision->SetEnableGravity(false);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AHPWH_ExperienceOrb::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Age += DeltaSeconds;
	if (Age >= MaximumLifetime)
	{
		Destroy();
		return;
	}
	if (!bSeeking)
	{
		if (Age >= ScatterDuration)
		{
			BeginSeeking();
		}
		return;
	}

	auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	AHPWH_PlayerCharacter* Body = GM ? GM->GetPlayerBody() : nullptr;
	// 灵魂离体时先停在原地，回体后再继续追踪。
	if (!Body || (GM && GM->GetSoulState() != EHPWH_SoulState::Embodied))
	{
		return;
	}
	if (TryCollect(Body))
	{
		return;
	}

	const FVector Target = Body->GetActorLocation() + FVector(0.f, 0.f, 55.f);
	const FVector Delta = Target - GetActorLocation();
	const float Distance = Delta.Size();
	SeekSpeed = FMath::Min(MaximumSeekSpeed, SeekSpeed + SeekAcceleration * DeltaSeconds);
	const FVector Next = GetActorLocation() + Delta.GetClampedToMaxSize(SeekSpeed * DeltaSeconds);
	SetActorLocation(Next, true);

	TryCollect(Body);
}

bool AHPWH_ExperienceOrb::TryCollect(AHPWH_PlayerCharacter* Body)
{
	if (!bSeeking || !IsValid(Body) || Body->IsRunLocked())
	{
		return false;
	}
	if (FVector::Dist2D(GetActorLocation(), Body->GetActorLocation()) > CollectRadius)
	{
		return false;
	}

	auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM)
	{
		return false;
	}
	GM->AddExperience(ExperienceValue);
	Particles->DeactivateImmediate();
	Destroy();
	return true;
}

void AHPWH_ExperienceOrb::SpawnOrb(UObject* WorldContextObject, const FVector& Origin, int32 ExperienceAmount)
{
	if (ExperienceAmount <= 0)
	{
		return;
	}
	UWorld* World = GEngine
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull)
		: nullptr;
	if (!World)
	{
		return;
	}

	const float Angle = FMath::FRandRange(0.f, 2.f * PI);
	const FVector Radial(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
	FTransform Transform(FRotator::ZeroRotator,
		Origin + Radial * FMath::FRandRange(10.f, 30.f) + FVector(0.f, 0.f, FMath::FRandRange(5.f, 25.f)));
	AHPWH_ExperienceOrb* Orb = World->SpawnActorDeferred<AHPWH_ExperienceOrb>(
		AHPWH_ExperienceOrb::StaticClass(), Transform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Orb)
	{
		return;
	}
	Orb->ExperienceValue = ExperienceAmount;
	Orb->LaunchVelocity = Radial * FMath::FRandRange(140.f, 260.f) + FVector(0.f, 0.f, FMath::FRandRange(220.f, 360.f));
	Orb->FinishSpawning(Transform);
}
