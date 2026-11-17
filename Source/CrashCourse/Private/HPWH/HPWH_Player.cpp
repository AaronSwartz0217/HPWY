#include "HPWH/HPWH_Player.h"

#include "HPWH/HPWH_GameMode.h"
#include "HPWH/HPWH_PlayerController.h"
#include "HPWH/HPWH_Enemies.h"
#include "HPWH/HPWH_SoulSpider.h"
#include "HPWH/HPWH_Crawler.h"
#include "HPWH/HPWH_Companion.h"
#include "HPWH/HPWH_World.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AHPWH_PlayerCharacter::AHPWH_PlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 600.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed = 480.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2800.f;
	GetCharacterMovement()->MaxAcceleration = 2400.f;

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
	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -96.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	TopDownBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("TopDownBoom"));
	TopDownBoom->SetupAttachment(GetRootComponent());
	TopDownBoom->TargetArmLength = TopDownArmLength;
	TopDownBoom->bUsePawnControlRotation = false;
	TopDownBoom->bInheritPitch = false;
	TopDownBoom->bInheritYaw = false;
	TopDownBoom->bInheritRoll = false;
	TopDownBoom->bDoCollisionTest = false;
	TopDownBoom->SetRelativeRotation(FRotator(TopDownPitch, 0.f, 0.f));

	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(TopDownBoom, USpringArmComponent::SocketName);
	TopDownCamera->bUsePawnControlRotation = false;
	TopDownCamera->SetAutoActivate(true);

	SoulLamp = CreateDefaultSubobject<UPointLightComponent>(TEXT("SoulLamp"));
	SoulLamp->SetupAttachment(GetRootComponent());
	SoulLamp->SetRelativeLocation(FVector(0.f, 0.f, 115.f));
	SoulLamp->SetIntensity(520.f);
	SoulLamp->SetAttenuationRadius(950.f);
	SoulLamp->SetLightColor(FLinearColor(1.f, 0.86f, 0.62f));

	SoulLampVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SoulLampVisual"));
	SoulLampVisual->SetupAttachment(GetRootComponent());
	SoulLampVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SoulLampVisual->SetRelativeLocation(FVector(0.f, 0.f, 112.f));
	SoulLampVisual->SetRelativeScale3D(FVector(0.16f));
	if (UStaticMesh* LampSphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")))
	{
		SoulLampVisual->SetStaticMesh(LampSphere);
	}

	Health = MaxHealth;
}

void AHPWH_PlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
	RefreshMoveSpeed();
	TopDownCamera->SetActive(true);

	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		if (UMaterialInstanceDynamic* BodyMaterial = UMaterialInstanceDynamic::Create(Base, this))
		{
			BodyMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.86f, 0.84f, 0.78f));
			for (int32 Index = 0; Index < GetMesh()->GetNumMaterials(); ++Index)
			{
				GetMesh()->SetMaterial(Index, BodyMaterial);
			}
		}
		if (UMaterialInstanceDynamic* LampMaterial = UMaterialInstanceDynamic::Create(Base, this))
		{
			LampMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, 0.9f, 0.62f));
			SoulLampVisual->SetMaterial(0, LampMaterial);
		}
	}
}

void AHPWH_PlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bFrightened && GetWorld()->GetTimeSeconds() >= FrightenedEnd)
	{
		ExitFrightened();
	}
	if (bRunLocked)
	{
		GetCharacterMovement()->StopMovementImmediately();
	}
	if (!bDead && !bRunLocked && !bSoulOut)
	{
		TryAttack();
	}
	AHPWH_GameMode* GameMode = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	if (!bDead && !bRunLocked && !bSoulOut && GetCharacterMovement()->IsMovingOnGround() &&
		GetVelocity().SizeSquared2D() > 2500.f)
	{
		FootstepTimer += DeltaSeconds;
		if (FootstepTimer >= 0.42f)
		{
			FootstepTimer = 0.f;
			if (GameMode)
			{
				GameMode->PlaySfxAt(EHPWH_Sfx::Footstep, GetActorLocation(), 0.35f);
			}
		}
	}
	else
	{
		FootstepTimer = 0.f;
	}

	if (AttackFlashEnd > 0.f && GetWorld()->GetTimeSeconds() >= AttackFlashEnd)
	{
		AttackFlashEnd = 0.f;
		SoulLamp->SetIntensity(520.f);
	}
}

void AHPWH_PlayerCharacter::SetFrightened(float Duration)
{
	if (bSoulOut || bDead)
	{
		return;
	}
	// 只保留受击惊吓的屏幕反馈，不再切换第一人称视角。
	const float Now = GetWorld()->GetTimeSeconds();
	bFrightened = true;
	FrightenedEnd = Now + FMath::Max(Duration, FrightenedDuration);
}

void AHPWH_PlayerCharacter::ExitFrightened()
{
	if (!bFrightened)
	{
		return;
	}
	bFrightened = false;
}

float AHPWH_PlayerCharacter::GetBulletCooldownDuration() const
{
	const AHPWH_GameMode* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	const float Reduction = GM ? GM->GetRunStats().BulletCooldownReduction : 0.f;
	return FMath::Max(0.2f, BaseBulletCooldown - Reduction);
}

float AHPWH_PlayerCharacter::GetBulletCooldownRemaining() const
{
	const UWorld* World = GetWorld();
	return World ? FMath::Max(0.f, ShotCooldownEnd - World->GetTimeSeconds()) : 0.f;
}

bool AHPWH_PlayerCharacter::TryAttack()
{
	if (bDead || bRunLocked || bSoulOut || !GetWorld())
	{
		return false;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now < NextAttackTime)
	{
		return false;
	}

	auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	const FHPWH_RunStats& Stats = GM ? GM->GetRunStats() : FHPWH_RunStats();
	const float Damage = AttackDamage + Stats.AttackBonus;
	const float Range = AttackRangeCm * (1.f + Stats.AttackRangeBonus);
	const float Interval = FMath::Max(0.2f, BaseBulletCooldown - Stats.BulletCooldownReduction);

	const FVector Origin = GetActorLocation();
	TArray<AActor*> Targets;
	for (TActorIterator<AHPWH_EnemyBase> It(GetWorld()); It; ++It)
	{
		if (IsValid(*It) && !It->IsDead() &&
			FVector::Dist2D(Origin, It->GetActorLocation()) <= Range)
		{
			Targets.Add(*It);
		}
	}
	for (TActorIterator<AHPWH_Crawler> It(GetWorld()); It; ++It)
	{
		if (IsValid(*It) && FVector::Dist2D(Origin, It->GetActorLocation()) <= Range + 220.f)
		{
			Targets.Add(*It);
		}
	}
	// 叛变后的同伴按敌人处理，可以被普攻锁定。
	for (TActorIterator<AHPWH_Companion> It(GetWorld()); It; ++It)
	{
		if (IsValid(*It) && It->IsHostile() &&
			FVector::Dist2D(Origin, It->GetActorLocation()) <= Range)
		{
			Targets.Add(*It);
		}
	}
	// 只锁定真正看得到的目标，避免隔墙开火。
	Targets.RemoveAll([this, Origin](AActor* Target)
	{
		if (!IsValid(Target))
		{
			return true;
		}
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(HPWHShotLINE), false, this);
		Params.AddIgnoredActor(Target);
		const FVector Start = Origin + FVector(0.f, 0.f, 30.f);
		const FVector End = Target->GetActorLocation() + FVector(0.f, 0.f, 20.f);
		return GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
	});

	Targets.Sort([Origin](const AActor& A, const AActor& B)
	{
		return FVector::DistSquared2D(Origin, A.GetActorLocation()) <
			FVector::DistSquared2D(Origin, B.GetActorLocation());
	});

	// 只有目标进入射程才发射；没有目标时快速重试，不进入完整冷却。
	if (Targets.Num() == 0)
	{
		NextAttackTime = Now + 0.1f;
		return false;
	}

	const int32 BulletCount = FMath::Clamp(Stats.ProjectileCount, 1, 3);
	TArray<FVector> AimPoints;
	AimPoints.Reserve(BulletCount);
	for (int32 Index = 0; Index < BulletCount; ++Index)
	{
		const AActor* Target = Targets.IsValidIndex(Index) ? Targets[Index] : Targets[0];
		AimPoints.Add(IsValid(Target) ? Target->GetActorLocation() : Origin + GetActorForwardVector() * Range);
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 Index = 0; Index < BulletCount; ++Index)
	{
		FVector Aim = AimPoints[Index] - Origin;
		Aim.Z = 0.f;
		const FVector Direction = Aim.GetSafeNormal();
		const FVector Right = FVector::CrossProduct(FVector::UpVector, Direction);
		const float Lateral = (Index - (BulletCount - 1) * 0.5f) * 22.f;
		const FVector SpawnLocation = Origin + Direction * 45.f + Right * Lateral + FVector(0.f, 0.f, 30.f);

		AHPWH_Projectile* Bullet = GetWorld()->SpawnActor<AHPWH_Projectile>(
			AHPWH_Projectile::StaticClass(), SpawnLocation, Direction.Rotation(), Params);
		if (Bullet)
		{
			Bullet->InitProjectile(Direction, 2600.f, Damage, false, this);
		}
	}

	NextAttackTime = Now + Interval;
	ShotCooldownEnd = NextAttackTime;
	SoulLamp->SetIntensity(1800.f);
	AttackFlashEnd = Now + 0.12f;
	if (GM)
	{
		GM->PlaySfx2D(EHPWH_Sfx::Attack, 0.6f);
	}
	UE_LOG(LogTemp, Display, TEXT("HPWH: fire %d bullet(s) interval=%.2f targets=%d"),
		BulletCount, Interval, Targets.Num());
	return true;
}

void AHPWH_PlayerCharacter::TakeHPWHDamage(float Damage, bool bTriggerFrightened)
{
	if (bDead || bRunLocked)
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now < NextDamageTime)
	{
		return;
	}
	const auto* StatsGM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	const float InvulnBonus = StatsGM ? StatsGM->GetRunStats().InvulnBonus : 0.f;
	NextDamageTime = Now + 0.5f + InvulnBonus;
	++HitCount;

	Health = FMath::Max(0.f, Health - Damage);
	if (auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
	{
		// 受击反馈：借用灵魂音效并压低音高，作为临时受击声。
		GM->PlaySfx2D(EHPWH_Sfx::SoulOut, 0.35f, 0.75f);
		GM->NotifyPlayerAttacked();
		GM->NotifyCompanionPlayerHurt();
	}
	if (bTriggerFrightened)
	{
		SetFrightened(FrightenedDuration);
	}
	if (Health <= 0.f)
	{
		bDead = true;
		if (auto* GameMode = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
		{
			GameMode->HandlePlayerDeath();
		}
	}
}

void AHPWH_PlayerCharacter::RestoreFullHealth()
{
	Health = MaxHealth;
	bDead = false;
}

void AHPWH_PlayerCharacter::AddMaxHealth(float Amount)
{
	MaxHealth += FMath::Max(0.f, Amount);
	Health = FMath::Min(MaxHealth, Health + FMath::Max(0.f, Amount));
}

void AHPWH_PlayerCharacter::HealHPWH(float Amount)
{
	if (bDead || Amount <= 0.f)
	{
		return;
	}
	Health = FMath::Min(MaxHealth, Health + Amount);
}

void AHPWH_PlayerCharacter::SetHealthForLoad(float NewHealth)
{
	Health = FMath::Clamp(NewHealth, 1.f, MaxHealth);
}

void AHPWH_PlayerCharacter::ResetMaxHealthForLoad()
{
	// 基础生命为 30，读档前先回到基础值，再叠加存档中的生命强化。
	MaxHealth = 30.f;
	Health = MaxHealth;
}

void AHPWH_PlayerCharacter::RefreshMoveSpeed()
{
	const auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	const float Bonus = GM ? GM->GetRunStats().MoveSpeedBonus : 0.f;
	GetCharacterMovement()->MaxWalkSpeed = HPWH_PlayerBaseSpeed * (1.f + Bonus);
}

void AHPWH_PlayerCharacter::EnterSoulOut()
{
	if (bSoulOut || bDead)
	{
		return;
	}
	auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM)
	{
		return;
	}

	bSoulOut = true;
	ExitFrightened();

	FVector SoulSpawn = GetActorLocation() + FVector(0.f, 0.f, 120.f);
	FIntPoint SoulCell = FIntPoint::ZeroValue;
	if (AHPWH_WorldDirector* Director = GM->GetDirector())
	{
		const FIntPoint BodyCell = Director->WorldToCell(GetActorLocation());
		for (int32 Attempt = 0; Attempt < 20; ++Attempt)
		{
			const int32 DX = FMath::RandRange(-3, 3);
			const int32 DY = FMath::RandRange(-3, 3);
			if (FMath::Max(FMath::Abs(DX), FMath::Abs(DY)) < 1)
			{
				continue;
			}
			SoulCell = BodyCell + FIntPoint(DX, DY);
			const FVector Center = Director->CellCenter(SoulCell);
			const FVector Candidate = Center + FVector(FMath::FRandRange(-220.f, 220.f),
				FMath::FRandRange(-220.f, 220.f), 120.f);
			SoulSpawn = HPWH_SnapDecimeter(Candidate);
			SoulSpawn.Z = 120.f;
			// 至少离开肉身 6 米，避免一离体就落在回体范围内。
			if (FVector::Dist2D(SoulSpawn, GetActorLocation()) < 600.f)
			{
				continue;
			}

			FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(HPWHSoulSpawn), false, this);
			const bool bBlocked = GetWorld()->OverlapBlockingTestByChannel(SoulSpawn, FQuat::Identity,
				ECC_Pawn, FCollisionShape::MakeCapsule(34.f, 62.f), QueryParams);
			if (bBlocked)
			{
				continue;
			}
			// 避免灵魂直接落在怪物身边，导致一离体就被秒杀。
			bool bEnemyNearby = false;
			for (TActorIterator<AHPWH_EnemyBase> EnemyIt(GetWorld()); EnemyIt; ++EnemyIt)
			{
				if (!EnemyIt->IsDead() &&
					FVector::Dist2D(EnemyIt->GetActorLocation(), SoulSpawn) < 1500.f)
				{
					bEnemyNearby = true;
					break;
				}
			}
			if (!bEnemyNearby)
			{
				break;
			}
		}
		UE_LOG(LogTemp, Display, TEXT("HPWH: soul knocked out to cell (%d,%d) world=(%.1f,%.1f)m"),
			SoulCell.X, SoulCell.Y, SoulSpawn.X / 100.f, SoulSpawn.Y / 100.f);
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AHPWH_SoulAvatar* Soul = GetWorld()->SpawnActor<AHPWH_SoulAvatar>(
		AHPWH_SoulAvatar::StaticClass(), SoulSpawn, FRotator::ZeroRotator, Params);
	if (!Soul)
	{
		bSoulOut = false;
		return;
	}
	GM->SetSoulAvatar(Soul);
	GM->PlaySfx2D(EHPWH_Sfx::SoulOut, 0.9f);
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->Possess(Soul);
	}
	GetCharacterMovement()->StopMovementImmediately();
}

void AHPWH_PlayerCharacter::ReturnToBody()
{
	if (!bSoulOut)
	{
		return;
	}
	auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	AHPWH_SoulAvatar* Soul = GM ? GM->GetSoulAvatar() : nullptr;
	APlayerController* PC = Soul ? Cast<APlayerController>(Soul->GetController()) : nullptr;
	if (!PC)
	{
		PC = Cast<APlayerController>(GetController());
	}
	if (PC)
	{
		PC->Possess(this);
	}
	if (Soul)
	{
		Soul->Destroy();
	}
	if (GM)
	{
		GM->SetSoulAvatar(nullptr);
		GM->SetSoulState(EHPWH_SoulState::Embodied);
		GM->SetMarkState(EHPWH_MarkState::Unmarked);
		GM->OnSoulReturnedToBody();
		GM->PlaySfx2D(EHPWH_Sfx::SoulOut, 0.55f, 1.25f);
	}
	bSoulOut = false;
}

void AHPWH_PlayerCharacter::ForceReturnToBody()
{
	if (bSoulOut)
	{
		ReturnToBody();
	}
	bSoulOut = false;
}

void AHPWH_PlayerCharacter::ForceReset()
{
	ForceReturnToBody();
	ExitFrightened();
	bDead = false;
	bRunLocked = false;
	NextAttackTime = 0.f;
	ShotCooldownEnd = 0.f;
	Health = MaxHealth;
	TeleportTo(FVector(0.f, 0.f, 120.f), FRotator::ZeroRotator);
	GetCharacterMovement()->StopMovementImmediately();
}

void AHPWH_PlayerCharacter::SetRunLocked(bool bLocked)
{
	bRunLocked = bLocked;
	if (bRunLocked)
	{
		GetCharacterMovement()->StopMovementImmediately();
	}
}

AHPWH_SoulAvatar::AHPWH_SoulAvatar()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(34.f, 62.f);
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 600.f, 0.f);
	// 灵魂速度 = 角色基础移速 × 1.2。
	GetCharacterMovement()->MaxWalkSpeed = HPWH_PlayerBaseSpeed * HPWH_SoulSpeedMultiplier;

	TopDownBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("SoulBoom"));
	TopDownBoom->SetupAttachment(GetRootComponent());
	TopDownBoom->TargetArmLength = 1750.f;
	TopDownBoom->bUsePawnControlRotation = false;
	TopDownBoom->bInheritPitch = false;
	TopDownBoom->bInheritYaw = false;
	TopDownBoom->bInheritRoll = false;
	TopDownBoom->bDoCollisionTest = false;
	TopDownBoom->SetRelativeRotation(FRotator(-72.f, 0.f, 0.f));

	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("SoulCamera"));
	TopDownCamera->SetupAttachment(TopDownBoom, USpringArmComponent::SocketName);
	TopDownCamera->bUsePawnControlRotation = false;

	GhostBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GhostBody"));
	GhostBody->SetupAttachment(GetRootComponent());
	GhostBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GhostBody->SetRelativeScale3D(FVector(0.55f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		GhostBody->SetStaticMesh(SphereMesh.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BasicMaterial.Succeeded())
	{
		GhostBody->SetMaterial(0, BasicMaterial.Object);
	}

	SoulLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("SoulLight"));
	SoulLight->SetupAttachment(GetRootComponent());
	SoulLight->SetIntensity(700.f);
	SoulLight->SetAttenuationRadius(1200.f);
	SoulLight->SetLightColor(FLinearColor(0.72f, 0.9f, 1.f));

	GetMesh()->SetVisibility(false);
}

void AHPWH_SoulAvatar::BeginPlay()
{
	Super::BeginPlay();
	RefreshMoveSpeed();

	// 灵魂使用半透明无光材质：可见但明显区别于肉身。
	UMaterialInterface* GhostMaterial = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Engine/EngineDebugMaterials/M_SimpleUnlitTranslucent.M_SimpleUnlitTranslucent"));
	if (!GhostMaterial)
	{
		GhostMaterial = GhostBody->GetMaterial(0);
	}
	if (GhostMaterial)
	{
		if (UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(GhostMaterial, this))
		{
			const FLinearColor GhostColor(0.62f, 0.86f, 1.f, 0.42f);
			Mid->SetVectorParameterValue(TEXT("Color"), GhostColor);
			Mid->SetVectorParameterValue(TEXT("BaseColor"), GhostColor);
			GhostBody->SetMaterial(0, Mid);
		}
	}
}

void AHPWH_SoulAvatar::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bContactDeathTriggered || !GetWorld())
	{
		return;
	}
	auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM || GM->GetSoulState() != EHPWH_SoulState::SoulOut)
	{
		return;
	}

	const FVector Location = GetActorLocation();
	for (TActorIterator<AHPWH_EnemyBase> It(GetWorld()); It; ++It)
	{
		if (!IsValid(*It) || It->IsDead())
		{
			continue;
		}
		if (FVector::Dist2D(Location, It->GetActorLocation()) <= 120.f)
		{
			bContactDeathTriggered = true;
			GM->HandleSoulKilled();
			return;
		}
	}
	for (TActorIterator<AHPWH_Crawler> It(GetWorld()); It; ++It)
	{
		if (!IsValid(*It) || It->IsHidden())
		{
			continue;
		}
		if (FVector::Dist2D(Location, It->GetActorLocation()) <= 260.f)
		{
			bContactDeathTriggered = true;
			GM->HandleSoulKilled();
			return;
		}
	}
}

bool AHPWH_SoulAvatar::CanReturnToBody() const
{
	auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	const AHPWH_PlayerCharacter* Body = GM ? GM->GetPlayerBody() : nullptr;
	const float Bonus = GM ? GM->GetRunStats().ReturnRangeBonus : 0.f;
	return Body && FVector::Dist2D(GetActorLocation(), Body->GetActorLocation()) <= 280.f * (1.f + Bonus);
}

void AHPWH_SoulAvatar::RefreshMoveSpeed()
{
	const auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	const float Bonus = GM ? GM->GetRunStats().SoulSpeedBonus : 0.f;
	GetCharacterMovement()->MaxWalkSpeed = HPWH_PlayerBaseSpeed * HPWH_SoulSpeedMultiplier * (1.f + Bonus);
}

void AHPWH_SoulAvatar::TryReturnToBody()
{
	auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	AHPWH_PlayerCharacter* Body = GM ? GM->GetPlayerBody() : nullptr;
	if (!Body)
	{
		return;
	}
	const float Bonus = GM ? GM->GetRunStats().ReturnRangeBonus : 0.f;
	if (FVector::Dist2D(GetActorLocation(), Body->GetActorLocation()) > 280.f * (1.f + Bonus))
	{
		GM->PushMessage(TEXT("靠近肉身才能回体"), 2.f);
		return;
	}
	Body->ReturnToBody();
}

AHPWH_Projectile::AHPWH_Projectile()
{
	PrimaryActorTick.bCanEverTick = false;
	InitialLifeSpan = 2.2f;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(18.f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Collision->SetGenerateOverlapEvents(true);
	Collision->OnComponentBeginOverlap.AddDynamic(this, &AHPWH_Projectile::OnOverlap);

	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	Visual->SetupAttachment(Collision);
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Visual->SetRelativeScale3D(FVector(0.32f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		Visual->SetStaticMesh(SphereMesh.Object);
	}

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Collision);
	Glow->SetIntensity(500.f);
	Glow->SetAttenuationRadius(320.f);

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->InitialSpeed = 2600.f;
	Movement->MaxSpeed = 3000.f;
	Movement->ProjectileGravityScale = 0.f;
	Movement->bRotationFollowsVelocity = true;
}

void AHPWH_Projectile::InitProjectile(const FVector& Direction, float Speed, float InDamage, bool bInHostile, AActor* InInstigator)
{
	Damage = InDamage;
	bHostile = bInHostile;
	SetOwner(InInstigator);
	SetInstigator(Cast<APawn>(InInstigator));
	if (Movement)
	{
		Movement->InitialSpeed = Speed;
		Movement->MaxSpeed = Speed;
		Movement->Velocity = Direction.GetSafeNormal() * Speed;
	}

	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (Base)
	{
		UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Base, this);
		if (Mid)
		{
			const FLinearColor Color = bHostile ? FLinearColor(0.45f, 0.6f, 1.f) : FLinearColor(1.f, 0.82f, 0.42f);
			Mid->SetVectorParameterValue(TEXT("Color"), Color);
			Visual->SetMaterial(0, Mid);
			Glow->SetLightColor(Color);
		}
	}
}

void AHPWH_Projectile::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!IsValid(OtherActor) || OtherActor == GetOwner())
	{
		return;
	}

	if (bHostile)
	{
		if (AHPWH_PlayerCharacter* Player = Cast<AHPWH_PlayerCharacter>(OtherActor))
		{
			Player->TakeHPWHDamage(Damage, false);
			Destroy();
		}
		return;
	}

	if (AHPWH_Companion* Companion = Cast<AHPWH_Companion>(OtherActor))
	{
		if (Companion->IsHostile())
		{
			Companion->ReceiveHPWHDamage(Damage);
			Destroy();
		}
		return;
	}
	if (AHPWH_EnemyBase* Enemy = Cast<AHPWH_EnemyBase>(OtherActor))
	{
		Enemy->ReceiveHPWHDamage(Damage);
		Destroy();
		return;
	}
	if (AHPWH_Crawler* Crawler = Cast<AHPWH_Crawler>(OtherActor))
	{
		Crawler->Disrupt(1.6f);
		Destroy();
	}
}
