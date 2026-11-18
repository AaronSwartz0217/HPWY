#include "HPWH/HPWH_SoulSpider.h"

#include "HPWH/HPWH_GameMode.h"
#include "HPWH/HPWH_Player.h"

#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AHPWH_SoulSpider::AHPWH_SoulSpider()
{
	PrimaryActorTick.bCanEverTick = true;
	// 摄魂蜘蛛属于 1.2 倍角色速度的快怪。
	SpeedMultiplier = 1.2f;
	AggroRadius = 2500.f;

	GetCapsuleComponent()->InitCapsuleSize(55.f, 50.f);
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
	GetMesh()->SetVisibility(false);

	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));

	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(GetRootComponent());
	VisualRoot->SetRelativeLocation(FVector(0.f, 0.f, -45.f));

	BodyBox = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpiderBody"));
	BodyBox->SetupAttachment(VisualRoot);
	BodyBox->SetStaticMesh(Cube);
	BodyBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyBox->SetRelativeLocation(FVector(0.f, 0.f, 24.f));
	BodyBox->SetRelativeScale3D(FVector(0.95f, 0.7f, 0.42f));

	for (int32 Index = 0; Index < 6; ++Index)
	{
		UStaticMeshComponent* Leg = CreateDefaultSubobject<UStaticMeshComponent>(
			*FString::Printf(TEXT("SpiderLeg%d"), Index));
		Leg->SetupAttachment(VisualRoot);
		Leg->SetStaticMesh(Cube);
		Leg->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		const float Angle = Index * 60.f;
		const FVector Direction(FMath::Cos(FMath::DegreesToRadians(Angle)),
			FMath::Sin(FMath::DegreesToRadians(Angle)), 0.f);
		Leg->SetRelativeLocation(Direction * 52.f + FVector(0.f, 0.f, 20.f));
		Leg->SetRelativeScale3D(FVector(0.30f, 0.08f, 0.08f));
		Legs.Add(Leg);
	}

	ImportedVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ImportedSpider"));
	ImportedVisual->SetupAttachment(VisualRoot);
	ImportedVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ImportedVisual->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,
		TEXT("/Game/HPWH/Assets/Models/摄魂蜘蛛/StaticMeshes/soul_spider.soul_spider")));
	if (UMaterialInterface* TwoSided = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/HPWH/Assets/Models/摄魂蜘蛛/Materials/M_摄魂蜘蛛_TwoSided.M_摄魂蜘蛛_TwoSided")))
	{
		ImportedVisual->SetMaterial(0, TwoSided);
	}
	ImportedVisual->SetRelativeLocation(FVector(0.f, 0.f, 35.f));
	ImportedVisual->SetRelativeScale3D(FVector(3.f));

	ApplyPose(true, 0.f);
}

void AHPWH_SoulSpider::BeginPlay()
{
	Super::BeginPlay();

	// 已接入导入模型，隐藏程序化方块身体与六条腿。
	const bool bHasImportedMesh = ImportedVisual && ImportedVisual->GetStaticMesh();
	if (BodyBox)
	{
		BodyBox->SetVisibility(!bHasImportedMesh);
	}
	for (UStaticMeshComponent* Leg : Legs)
	{
		if (IsValid(Leg))
		{
			Leg->SetVisibility(!bHasImportedMesh);
		}
	}

	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!Base)
	{
		return;
	}
	if (UMaterialInstanceDynamic* BodyMaterial = UMaterialInstanceDynamic::Create(Base, this))
	{
		BodyMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.14f, 0.13f, 0.15f));
		BodyBox->SetMaterial(0, BodyMaterial);
	}
	if (UMaterialInstanceDynamic* LegMaterial = UMaterialInstanceDynamic::Create(Base, this))
	{
		LegMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.24f, 0.21f, 0.20f));
		for (UStaticMeshComponent* Leg : Legs)
		{
			if (IsValid(Leg))
			{
				Leg->SetMaterial(0, LegMaterial);
			}
		}
	}
}

void AHPWH_SoulSpider::Tick(float DeltaSeconds)
{
	if (bDead)
	{
		return;
	}

	if (State == EHPWH_SpiderState::PlayingDead)
	{
		auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
		const AHPWH_PlayerCharacter* Body = GM ? GM->GetPlayerBody() : nullptr;
		if (Body && FVector::Dist2D(GetActorLocation(), Body->GetActorLocation()) <= 340.f)
		{
			ActivateSpider();
		}
		return;
	}

	if (State == EHPWH_SpiderState::FlipToUpright)
	{
		const float Now = GetWorld()->GetTimeSeconds();
		const float Alpha = FMath::Clamp(1.f - (FlipEnd - Now) / 0.9f, 0.f, 1.f);
		ApplyPose(false, Alpha);
		if (Alpha >= 1.f)
		{
			State = EHPWH_SpiderState::SlowChase;
		}
		return;
	}

	AHPWH_EnemyBase::Tick(DeltaSeconds);
}

void AHPWH_SoulSpider::ReceiveHPWHDamage(float Damage)
{
	if (State == EHPWH_SpiderState::PlayingDead)
	{
		ActivateSpider();
	}
	Super::ReceiveHPWHDamage(Damage);
}

void AHPWH_SoulSpider::ActivateSpider()
{
	if (State != EHPWH_SpiderState::PlayingDead)
	{
		return;
	}
	State = EHPWH_SpiderState::FlipToUpright;
	FlipEnd = GetWorld()->GetTimeSeconds() + 0.9f;
	ApplyPose(false, 0.f);

	if (auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->PushMessage(TEXT("沙沙——"), 1.5f);
	}
}

void AHPWH_SoulSpider::ApplyPose(bool bDormant, float FlipAlpha)
{
	if (!VisualRoot)
	{
		return;
	}
	const float Flip = bDormant ? 0.f : FlipAlpha;
	VisualRoot->SetRelativeRotation(FRotator(180.f * (1.f - Flip), 0.f, 0.f));

	for (int32 Index = 0; Index < Legs.Num(); ++Index)
	{
		if (!IsValid(Legs[Index]))
		{
			continue;
		}
		const float Pitch = bDormant ? -58.f : 16.f;
		Legs[Index]->SetRelativeRotation(FRotator(Pitch, Index * 60.f, 0.f));
	}
}

void AHPWH_SoulSpider::TickBehavior(float DeltaSeconds, AHPWH_PlayerCharacter* Target)
{
	FVector Direction = Target->GetActorLocation() - GetActorLocation();
	Direction.Z = 0.f;
	const float Distance = Direction.Size();
	if (Distance > 135.f)
	{
		AddMovementInput(Direction.GetSafeNormal(), 1.f);
		return;
	}
	if (GetWorld()->GetTimeSeconds() < NextAttackTime)
	{
		return;
	}
	NextAttackTime = GetWorld()->GetTimeSeconds() + 2.6f;

	if (auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->TriggerSoulOut();
	}
}
