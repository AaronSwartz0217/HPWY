#include "HPWH/HPWH_Crawler.h"

#include "HPWH/HPWH_GameMode.h"
#include "HPWH/HPWH_Player.h"
#include "HPWH/HPWH_World.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AHPWH_Crawler::AHPWH_Crawler()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(170.f, 260.f);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 140.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed = 290.f;
	GetCharacterMovement()->GravityScale = 0.f;
	GetCharacterMovement()->SetMovementMode(MOVE_None);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannyMesh(
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (MannyMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(MannyMesh.Object);
	}
	static ConstructorHelpers::FClassFinder<UAnimInstance> MannyAnim(
		TEXT("/Game/Characters/Mannequins/Animations/ABP_Manny.ABP_Manny_C"));
	if (MannyAnim.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(MannyAnim.Class);
	}
	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -260.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	GetMesh()->SetRelativeScale3D(FVector(3.2f));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> CrawlerMesh(
		TEXT("/Game/zichan/model/徘徊者/SkeletalMeshes/徘徊者.徘徊者"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> CrawlerAnim(
		TEXT("/Game/HPWH/Locomotion/ZichanCrawler/ABP_ZichanCrawler"));

	if (CrawlerMesh.Succeeded() && CrawlerAnim.Succeeded())
	{
		bUsingImportedVisual = true;
		GetMesh()->SetSkeletalMesh(CrawlerMesh.Object);
		GetMesh()->SetAnimInstanceClass(CrawlerAnim.Class);
		const FBoxSphereBounds Bounds = CrawlerMesh.Object->GetBounds();
		const float Scale = 576.f / FMath::Max(1.f, 2.f * Bounds.BoxExtent.Z);
		GetMesh()->SetRelativeScale3D(FVector(Scale));
		GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -300.f - (Bounds.Origin.Z - Bounds.BoxExtent.Z) * Scale));

	}
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	EyeLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EyeLeft"));
	EyeLeft->SetupAttachment(GetRootComponent());
	EyeLeft->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EyeLeft->SetRelativeLocation(FVector(95.f, -42.f, 235.f));
	EyeLeft->SetRelativeScale3D(FVector(0.3f));
	if (SphereMesh.Succeeded())
	{
		EyeLeft->SetStaticMesh(SphereMesh.Object);
	}

	EyeRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EyeRight"));
	EyeRight->SetupAttachment(GetRootComponent());
	EyeRight->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EyeRight->SetRelativeLocation(FVector(95.f, 42.f, 235.f));
	EyeRight->SetRelativeScale3D(FVector(0.3f));
	if (SphereMesh.Succeeded())
	{
		EyeRight->SetStaticMesh(SphereMesh.Object);
	}

	EyeLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("EyeLight"));
	EyeLight->SetupAttachment(GetRootComponent());
	EyeLight->SetRelativeLocation(FVector(110.f, 0.f, 240.f));
	EyeLight->SetIntensity(1800.f);
	EyeLight->SetAttenuationRadius(1900.f);
	EyeLight->SetLightColor(FLinearColor(0.9f, 0.95f, 1.f));

	Tags.Add(TEXT("Crawler"));
}

void AHPWH_Crawler::BeginPlay()
{
	Super::BeginPlay();

	if (bUsingImportedVisual)
	{
		EyeLeft->SetVisibility(false);
		EyeRight->SetVisibility(false);
		return;
	}

	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!Base)
	{
		return;
	}
	if (UMaterialInstanceDynamic* Body = UMaterialInstanceDynamic::Create(Base, this))
	{
		Body->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.012f, 0.012f, 0.018f));
		GetMesh()->SetMaterial(0, Body);
	}
	if (UMaterialInstanceDynamic* Eye = UMaterialInstanceDynamic::Create(Base, this))
	{
		Eye->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, 1.f, 1.f));
		EyeLeft->SetMaterial(0, Eye);
		EyeRight->SetMaterial(0, Eye);
	}
}

void AHPWH_Crawler::Activate(bool bFromSoulMark)
{
	if (State != EHPWH_CrawlerState::Dormant)
	{
		return;
	}
	// 灵魂离体时普通怪锁定灵魂，匍匐者始终锁定肉身。
	State = EHPWH_CrawlerState::ChasingBody;
	LastRelocateTime = GetWorld()->GetTimeSeconds();
	LogicalLocation = HPWH_SnapDecimeter(GetActorLocation());
	LogicalLocation.Z = 300.f;

	if (AActor* Target = ResolveTarget())
	{
		if (FVector::Dist2D(LogicalLocation, Target->GetActorLocation()) > 4500.f)
		{
			RelocateNearPlayer();
		}
	}
	UpdatePresentation();
	if (auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->PlaySfx2D(EHPWH_Sfx::CrawlerAppear, 0.85f);
	}
}

void AHPWH_Crawler::ResetDormant()
{
	State = EHPWH_CrawlerState::Dormant;
	DisruptEnd = 0.f;
	NextAttackTime = 0.f;
	CurrentSpeed = 0.f;
	LastLoggedSpeed = -1.f;
	LogicalLocation = HPWH_SnapDecimeter(Anchor);
	LogicalLocation.Z = 300.f;
	bPresentationVisible = false;
	SetActorHiddenInGame(true);
	SetActorLocation(LogicalLocation);
	GetCharacterMovement()->StopMovementImmediately();
}

void AHPWH_Crawler::Disrupt(float Duration)
{
	if (State == EHPWH_CrawlerState::Dormant)
	{
		return;
	}
	State = EHPWH_CrawlerState::Disrupted;
	DisruptEnd = FMath::Max(DisruptEnd, GetWorld()->GetTimeSeconds() + Duration);
}

AActor* AHPWH_Crawler::ResolveTarget() const
{
	auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM)
	{
		return nullptr;
	}
	return GM->GetPlayerBody();
}

float AHPWH_Crawler::ComputeCurrentSpeed(const APawn* PlayerBody) const
{
	const AHPWH_PlayerCharacter* Body = Cast<AHPWH_PlayerCharacter>(PlayerBody);
	const float PlayerSpeed = Body ? Body->GetCharacterMovement()->MaxWalkSpeed : HPWH_PlayerBaseSpeed;
	const bool bPlayerHurt = Body && Body->GetHitCount() > 0;
	const auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	const int32 Level = GM ? GM->GetCrawlerSpeedLevel() : 0;
	const float LevelMultiplier = 1.f + 0.1f * FMath::Max(0, Level);
	const float SlowMultiplier = GM ? 1.f - GM->GetRunStats().CrawlerSlow : 1.f;
	return PlayerSpeed * 0.2f * (bPlayerHurt ? 2.f : 1.f) * LevelMultiplier * SlowMultiplier;
}

void AHPWH_Crawler::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AnimationSpeed = 0.f;

	if (State == EHPWH_CrawlerState::Dormant)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (State == EHPWH_CrawlerState::Disrupted)
	{
		if (Now < DisruptEnd)
		{
			UpdatePresentation();
			return;
		}
		State = EHPWH_CrawlerState::ChasingBody;
	}

	AActor* Target = ResolveTarget();
	if (!Target)
	{
		return;
	}

	auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	const AHPWH_PlayerCharacter* Body = GM ? GM->GetPlayerBody() : nullptr;
	CurrentSpeed = ComputeCurrentSpeed(Body);
	if (FMath::Abs(CurrentSpeed - LastLoggedSpeed) > 0.5f)
	{
		LastLoggedSpeed = CurrentSpeed;
		UE_LOG(LogTemp, Display, TEXT("HPWH: crawler speed=%.1f cm/s (hurt=%d level=%d)"),
			CurrentSpeed, Body && Body->GetHitCount() > 0 ? 1 : 0, GM ? GM->GetCrawlerSpeedLevel() : 0);
	}

	FVector Direction = Target->GetActorLocation() - LogicalLocation;
	Direction.Z = 0.f;
	const float Distance = Direction.Size();
	if (Distance > 240.f && Distance > KINDA_SMALL_NUMBER)
	{
		// 连续推进，显示与查询时再吸附到 0.1 米，避免慢速时被反复舍入卡住。
		AnimationSpeed = FMath::Min(CurrentSpeed, (Distance - 240.f) / FMath::Max(DeltaSeconds, SMALL_NUMBER));
		LogicalLocation += Direction.GetSafeNormal() * AnimationSpeed * DeltaSeconds;
		LogicalLocation.Z = 300.f;
		SetActorRotation(FRotator(0.f, Direction.Rotation().Yaw, 0.f));
	}
	else if (Now >= NextAttackTime && bPresentationVisible)
	{
		AttackTarget(Target);
	}

	if (Distance > 4500.f && Now - LastRelocateTime > 8.f)
	{
		RelocateNearPlayer();
	}

	UpdatePresentation();

	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugCrawler")))
	{
		static float LastDebugLog = 0.f;
		if (Now - LastDebugLog > 2.f)
		{
			LastDebugLog = Now;
			const FVector ActorLocation = GetActorLocation();
			UE_LOG(LogTemp, Display,
				TEXT("HPWH: crawler debug logical=(%.1f,%.1f) actor=(%.1f,%.1f) hidden=%d visible=%d dist=%.1f"),
				LogicalLocation.X / 100.f, LogicalLocation.Y / 100.f,
				ActorLocation.X / 100.f, ActorLocation.Y / 100.f,
				IsHidden() ? 1 : 0, bPresentationVisible ? 1 : 0, Distance / 100.f);
		}
	}
}

void AHPWH_Crawler::UpdatePresentation()
{
	if (!Director)
	{
		return;
	}
	const FVector PresentationLocation = HPWH_SnapDecimeter(LogicalLocation);
	const bool bVisible = Director->IsWorldPositionVisible(PresentationLocation);
	if (bVisible != bPresentationVisible)
	{
		bPresentationVisible = bVisible;
		SetActorHiddenInGame(!bVisible);
		const FIntPoint Cell = Director->WorldToCell(PresentationLocation);
		UE_LOG(LogTemp, Display, TEXT("HPWH: crawler %s logical=(%.1f,%.1f)m cell=(%d,%d) speed=%.1f"),
			bVisible ? TEXT("visible") : TEXT("hidden"),
			PresentationLocation.X / 100.f, PresentationLocation.Y / 100.f, Cell.X, Cell.Y, CurrentSpeed);
	}
	if (bVisible)
	{
		SetActorLocation(PresentationLocation, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void AHPWH_Crawler::DebugPlaceNearPlayer(float DistanceCm)
{
	auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	const AHPWH_PlayerCharacter* Body = GM ? GM->GetPlayerBody() : nullptr;
	if (!Body)
	{
		return;
	}
	FVector Back = -Body->GetActorForwardVector();
	Back.Z = 0.f;
	if (Back.IsNearlyZero())
	{
		Back = FVector(-1.f, 0.f, 0.f);
	}
	LogicalLocation = HPWH_SnapDecimeter(Body->GetActorLocation() + Back.GetSafeNormal() * DistanceCm);
	LogicalLocation.Z = 300.f;
	UpdatePresentation();
}

void AHPWH_Crawler::AttackTarget(AActor* Target)
{
	NextAttackTime = GetWorld()->GetTimeSeconds() + 2.5f;
	auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM)
	{
		return;
	}
	if (GM->GetSoulState() == EHPWH_SoulState::SoulOut)
	{
		// 灵魂离体期间碰到肉身：不扣血，直接扣通关率。
		GM->HandleCrawlerCaughtBody();
		return;
	}
	if (AHPWH_PlayerCharacter* Body = Cast<AHPWH_PlayerCharacter>(Target))
	{
		Body->TakeHPWHDamage(HPWH_MonsterAttackDamage, true);
	}
}

void AHPWH_Crawler::RelocateNearPlayer()
{
	auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	const AHPWH_PlayerCharacter* Body = GM ? GM->GetPlayerBody() : nullptr;
	if (!Body)
	{
		return;
	}
	const FVector PlayerLocation = Body->GetActorLocation();
	FVector ViewDirection = FVector::ForwardVector;
	if (const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		ViewDirection = PC->GetControlRotation().Vector();
	}

	const float BaseAngle = FMath::FRandRange(0.f, 2.f * PI);
	const float CellSize = Director ? Director->GetCellSize() : 800.f;
	const int32 ExtraCells = GM ? GM->GetRunStats().CrawlerDistanceCells : 0;
	const float RelocateDistance = 3400.f + ExtraCells * CellSize;
	for (int32 Attempt = 0; Attempt < 10; ++Attempt)
	{
		const float Angle = BaseAngle + Attempt * (2.f * PI / 10.f);
		FVector Direction(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
		if (FVector::DotProduct(Direction, ViewDirection) > 0.15f)
		{
			continue;
		}
		LogicalLocation = HPWH_SnapDecimeter(PlayerLocation + Direction * RelocateDistance);
		LogicalLocation.Z = 300.f;
		LastRelocateTime = GetWorld()->GetTimeSeconds();
		UpdatePresentation();
		return;
	}
}
