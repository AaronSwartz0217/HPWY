#include "HPWH/HPWH_Enemies.h"

#include "HPWH/HPWH_GameMode.h"
#include "HPWH/HPWH_Player.h"
#include "HPWH/HPWH_World.h"
#include "HPWH/HPWH_ExperienceOrb.h"

#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AHPWH_EnemyBase::AHPWH_EnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(40.f, 90.f);
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	// 普通怪没有 AIController，允许无控制器时仍然应用移动输入。
	GetCharacterMovement()->bRunPhysicsWithNoController = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 480.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;

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
	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Health = MaxHealth;
}

void AHPWH_EnemyBase::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;

	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		if (UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Base, this))
		{
			Mid->SetVectorParameterValue(TEXT("Color"), GetTintColor());
			GetMesh()->SetMaterial(0, Mid);
		}
	}
}

void AHPWH_EnemyBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugEnemies")))
	{
		UE_LOG(LogTemp, Display, TEXT("HPWH: enemy %s endplay reason=%d deadd=%d"),
			*GetName(), static_cast<int32>(EndPlayReason), bDead ? 1 : 0);
	}
	Super::EndPlay(EndPlayReason);
}

void AHPWH_EnemyBase::InitEnemy(AHPWH_WorldDirector* InDirector, const FIntPoint& InCell, bool bInCounted)
{
	Director = InDirector;
	SourceCell = InCell;
	bCountedEncounter = bInCounted;
	MaxHealth = HPWH_MonsterMaxHealth(InCell);
	Health = MaxHealth;
	bDead = false;

	// 速度分类：以角色基础移速为基准，按种类倍率换算。
	MoveSpeed = HPWH_PlayerBaseSpeed * SpeedMultiplier;
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;

	SpawnLocation = GetActorLocation();
	PatrolTarget = SpawnLocation;
	NextPatrolTime = GetWorld() ? GetWorld()->GetTimeSeconds() + FMath::FRandRange(1.f, 4.f) : 0.f;
}

void AHPWH_EnemyBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bDead)
	{
		return;
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugEnemies")) && GetWorld())
	{
		const float Now = GetWorld()->GetTimeSeconds();
		if (Now >= NextDebugLogTime)
		{
			NextDebugLogTime = Now + 2.f;
			const AActor* DebugTarget = FindTarget();
			const float DebugDistance = DebugTarget
				? FVector::Dist2D(GetActorLocation(), DebugTarget->GetActorLocation()) : -1.f;
			const TCHAR* TargetName = Cast<AHPWH_SoulAvatar>(DebugTarget) ? TEXT("Soul")
				: (DebugTarget ? TEXT("Body") : TEXT("None"));
			UE_LOG(LogTemp, Display, TEXT("HPWH: enemy %s target=%s dist=%.1f pos=(%.1f,%.1f) speed=%.0f"),
				*GetName(), TargetName, DebugDistance / 100.f,
				GetActorLocation().X / 100.f, GetActorLocation().Y / 100.f,
				GetCharacterMovement()->MaxWalkSpeed);
		}
	}
	AActor* Target = FindTarget();
	if (!Target || FVector::Dist2D(GetActorLocation(), Target->GetActorLocation()) > AggroRadius)
	{
		TickPatrol(DeltaSeconds);
		return;
	}
	if (AHPWH_SoulAvatar* Soul = Cast<AHPWH_SoulAvatar>(Target))
	{
		TickChaseSoul(DeltaSeconds, Soul);
		return;
	}
	if (AHPWH_PlayerCharacter* Body = Cast<AHPWH_PlayerCharacter>(Target))
	{
		TickBehavior(DeltaSeconds, Body);
	}
}

void AHPWH_EnemyBase::TickChaseSoul(float DeltaSeconds, AHPWH_SoulAvatar* Soul)
{
	if (!Soul)
	{
		return;
	}
	FVector Direction = Soul->GetActorLocation() - GetActorLocation();
	Direction.Z = 0.f;
	const float Distance = Direction.Size();
	if (Distance > 110.f)
	{
		AddMovementInput(Direction.GetSafeNormal(), 1.f);
		return;
	}
	if (auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
	{
		// 灵魂碰到怪物即死亡。
		GM->HandleSoulKilled();
	}
}

void AHPWH_EnemyBase::TickPatrol(float DeltaSeconds)
{
	if (!GetWorld())
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	const FVector Location = GetActorLocation();
	if (Now >= NextPatrolTime || FVector::Dist2D(Location, PatrolTarget) < 70.f)
	{
		const float Angle = FMath::FRandRange(0.f, 2.f * PI);
		const float Radius = FMath::FRandRange(PatrolRadius * 0.25f, PatrolRadius);
		PatrolTarget = SpawnLocation + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);
		NextPatrolTime = Now + FMath::FRandRange(3.f, 6.f);
	}

	FVector Direction = PatrolTarget - Location;
	Direction.Z = 0.f;
	if (Direction.SizeSquared() > FMath::Square(30.f))
	{
		AddMovementInput(Direction.GetSafeNormal(), 0.45f);
	}
}

void AHPWH_EnemyBase::TickBehavior(float DeltaSeconds, AHPWH_PlayerCharacter* Target)
{
	FVector Direction = Target->GetActorLocation() - GetActorLocation();
	Direction.Z = 0.f;
	const float Distance = Direction.Size();
	if (Distance > 140.f)
	{
		AddMovementInput(Direction.GetSafeNormal(), 1.f);
	}
	else if (GetWorld()->GetTimeSeconds() >= NextAttackTime)
	{
		NextAttackTime = GetWorld()->GetTimeSeconds() + 1.4f;
		Target->TakeHPWHDamage(HPWH_MonsterAttackDamage, false);
		if (auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
		{
			GM->PlaySfxAt(EHPWH_Sfx::Attack, GetActorLocation(), 0.26f, 0.7f);
		}
		if (FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugEnemies")))
		{
			UE_LOG(LogTemp, Display, TEXT("HPWH: enemy %s attacked body"), *GetName());
		}
	}
}

void AHPWH_EnemyBase::ReceiveHPWHDamage(float Damage)
{
	if (bDead)
	{
		return;
	}
	Health -= Damage;
	if (Health <= 0.f)
	{
		Die();
	}
}

void AHPWH_EnemyBase::Die()
{
	if (bDead)
	{
		return;
	}
	bDead = true;
	if (bCountedEncounter && Director)
	{
		Director->NotifyEnemyKilled(SourceCell);
	}
	if (auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->OnEnemyKilled();
	}
	// 每只怪物掉落 5 点经验，采用 GAS 经验球的散射与追踪表现。
	AHPWH_ExperienceOrb::SpawnOrb(this, GetActorLocation() + FVector(0.f, 0.f, 70.f), 5);
	if (auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
	{
		// 死亡反馈：暂用怪物出场音效降调，等专用死亡音效到位后替换。
		GM->PlaySfxAt(EHPWH_Sfx::EnemySpawn, GetActorLocation(), 0.32f, 0.75f);
	}
	Destroy();
}

AActor* AHPWH_EnemyBase::FindTarget() const
{
	auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM)
	{
		return nullptr;
	}
	if (GM->GetSoulState() == EHPWH_SoulState::SoulOut)
	{
		if (AHPWH_SoulAvatar* Soul = GM->GetSoulAvatar())
		{
			return Soul;
		}
	}
	return GM->GetPlayerBody();
}

AHPWH_MeleeMonster::AHPWH_MeleeMonster()
{
	// 近战怪：基础移速的 120%，靠速度压迫走位。
	SpeedMultiplier = 1.2f;
}

void AHPWH_MeleeMonster::TickBehavior(float DeltaSeconds, AHPWH_PlayerCharacter* Target)
{
	FVector Direction = Target->GetActorLocation() - GetActorLocation();
	Direction.Z = 0.f;
	const float Distance = Direction.Size();
	if (Distance > 165.f)
	{
		AddMovementInput(Direction.GetSafeNormal(), 1.f);
		return;
	}
	if (GetWorld()->GetTimeSeconds() >= NextAttackTime)
	{
		NextAttackTime = GetWorld()->GetTimeSeconds() + 1.35f;
		Target->TakeHPWHDamage(HPWH_MonsterAttackDamage, false);
		if (auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
		{
			GM->PlaySfxAt(EHPWH_Sfx::Attack, GetActorLocation(), 0.28f, 0.75f);
		}
		if (FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugEnemies")))
		{
			UE_LOG(LogTemp, Display, TEXT("HPWH: enemy %s attacked body"), *GetName());
		}
	}
}

AHPWH_RangedMonster::AHPWH_RangedMonster()
{
	// 远程怪：基础移速的 80%，拉开距离射击。
	SpeedMultiplier = 0.8f;
	AggroRadius = 1600.f;
}

void AHPWH_RangedMonster::TickBehavior(float DeltaSeconds, AHPWH_PlayerCharacter* Target)
{
	FVector Direction = Target->GetActorLocation() - GetActorLocation();
	Direction.Z = 0.f;
	const float Distance = Direction.Size();
	const FVector Flat = Direction.GetSafeNormal();

	if (Distance > 950.f)
	{
		AddMovementInput(Flat, 1.f);
	}
	else if (Distance < 520.f && Distance > 1.f)
	{
		AddMovementInput(-Flat, 1.f);
	}

	if (Distance <= 1500.f && GetWorld()->GetTimeSeconds() >= NextAttackTime)
	{
		NextAttackTime = GetWorld()->GetTimeSeconds() + 2.2f;
		const FVector Start = GetActorLocation() + Flat * 70.f + FVector(0.f, 0.f, 50.f);
		FVector Aim = Target->GetActorLocation() + FVector(0.f, 0.f, 40.f) - Start;
		Aim.Z = 0.f;
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.Instigator = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AHPWH_Projectile* Projectile = GetWorld()->SpawnActor<AHPWH_Projectile>(
			AHPWH_Projectile::StaticClass(), Start, Aim.Rotation(), Params))
		{
			Projectile->InitProjectile(Aim.GetSafeNormal(), 900.f, HPWH_MonsterAttackDamage, true, this);
		}
		if (auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
		{
			GM->PlaySfxAt(EHPWH_Sfx::Attack, Start, 0.3f, 0.95f);
		}
	}
}

AHPWH_CoinMonster::AHPWH_CoinMonster()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(24.f, 42.f);
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->bRunPhysicsWithNoController = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 480.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed = 100.f;

	CoinMesh = GetMesh();
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> CoinAsset(
		TEXT("/Game/HPWH/Coins/Art/SK_WuzhuCoin.SK_WuzhuCoin"));
	if (CoinAsset.Succeeded())
	{
		CoinMesh->SetSkeletalMesh(CoinAsset.Object);
	}
	// 沿用 GasCrash 工程里的原动画蓝图状态机（待机 / 移动混合）。
	static ConstructorHelpers::FClassFinder<UAnimInstance> CoinAnim(
		TEXT("/Game/HPWH/Coins/Art/ABP_WuzhuCoin.ABP_WuzhuCoin_C"));
	if (CoinAnim.Succeeded())
	{
		CoinMesh->SetAnimInstanceClass(CoinAnim.Class);
	}
	CoinMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Keep the native coin rig upright and align the bottom of the mesh with the capsule.
	constexpr float CoinScale = 0.30257f;
	CoinMesh->SetRelativeRotation(FRotator::ZeroRotator);
	CoinMesh->SetRelativeScale3D(FVector(CoinScale));
	const float Bottom = CoinAsset.Succeeded() ? CoinAsset.Object->GetBounds().GetBox().Min.Z : 0.f;
	CoinMesh->SetRelativeLocation(FVector(0.f, 0.f, -42.f - CoinScale * Bottom));

	CoinLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("CoinLight"));
	CoinLight->SetupAttachment(CoinMesh);
	CoinLight->SetRelativeLocation(FVector(0.f, 0.f, 26.f));
	CoinLight->SetIntensity(140.f);
	CoinLight->SetAttenuationRadius(240.f);
	CoinLight->SetLightColor(FLinearColor(0.95f, 0.78f, 0.32f));
}

void AHPWH_CoinMonster::BeginPlay()
{
	Super::BeginPlay();
	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugCoin")))
	{
		const FBox LocalBox = CoinMesh && CoinMesh->GetSkeletalMeshAsset()
			? CoinMesh->GetSkeletalMeshAsset()->GetBounds().GetBox() : FBox(ForceInit);
		const FBox WorldBox = CoinMesh ? CoinMesh->Bounds.GetBox() : FBox(ForceInit);
		UE_LOG(LogTemp, Display, TEXT("HPWH: coin asset=%s mat=%s local=(%.1f,%.1f,%.1f) world=(%.1f,%.1f,%.1f) loc=(%.1f,%.1f,%.1f)"),
			CoinMesh && CoinMesh->GetSkeletalMeshAsset() ? *CoinMesh->GetSkeletalMeshAsset()->GetName() : TEXT("none"),
			CoinMesh && CoinMesh->GetMaterial(0) ? *CoinMesh->GetMaterial(0)->GetName() : TEXT("none"),
			LocalBox.GetSize().X, LocalBox.GetSize().Y, LocalBox.GetSize().Z,
			WorldBox.GetSize().X, WorldBox.GetSize().Y, WorldBox.GetSize().Z,
			GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z);
	}
	SpawnLocation = GetActorLocation();
	PatrolTarget = SpawnLocation;
	NextPatrolTime = 0.f;
}

void AHPWH_CoinMonster::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!GetWorld())
	{
		return;
	}

	// 小范围随机巡逻，动画蓝图根据实际速度在待机与移动间混合。
	const float Now = GetWorld()->GetTimeSeconds();
	FVector Flat = PatrolTarget - GetActorLocation();
	Flat.Z = 0.f;
	if (Now >= NextPatrolTime || Flat.SizeSquared() < FMath::Square(40.f))
	{
		const float Angle = FMath::FRandRange(0.f, 2.f * PI);
		const float Radius = FMath::FRandRange(150.f, 600.f);
		PatrolTarget = SpawnLocation +
			FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);
		NextPatrolTime = Now + FMath::FRandRange(5.f, 9.f);
	}
	if (Flat.SizeSquared() > FMath::Square(30.f))
	{
		AddMovementInput(Flat.GetSafeNormal(), 0.5f);
	}

	auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	const AHPWH_PlayerCharacter* Body = GM ? GM->GetPlayerBody() : nullptr;
	if (!Body)
	{
		return;
	}
	if (FVector::Dist2D(GetActorLocation(), Body->GetActorLocation()) <= 150.f)
	{
		if (!bExperienceGranted)
		{
			bExperienceGranted = true;
			AHPWH_ExperienceOrb::SpawnOrb(this, GetActorLocation() + FVector(0.f, 0.f, 50.f), 5);
		}
		if (Now >= NextVoiceTime)
		{
			NextVoiceTime = Now + 3.f;
			if (GM)
			{
				GM->PushMessage(TEXT("叮——"), 1.6f);
			}
		}
	}
}
