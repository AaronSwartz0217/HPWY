#include "HPWH/HPWH_Companion.h"

#include "HPWH/HPWH_Enemies.h"
#include "HPWH/HPWH_GameMode.h"
#include "HPWH/HPWH_Player.h"
#include "HPWH/HPWH_World.h"

#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FLinearColor HPWH_CompanionFriendlyColor(0.74f, 0.86f, 0.88f, 1.f);
	const FLinearColor HPWH_CompanionHostileColor(0.62f, 0.16f, 0.18f, 1.f);
}

AHPWH_Companion::AHPWH_Companion()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(35.f, 88.f);
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->bRunPhysicsWithNoController = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed = FollowSpeed;

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

	// 头顶微光用于在暗色地面上一眼认出同伴。
	CompanionLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("CompanionLight"));
	CompanionLight->SetupAttachment(GetMesh());
	CompanionLight->SetRelativeLocation(FVector(0.f, 0.f, 60.f));
	CompanionLight->SetIntensity(320.f);
	CompanionLight->SetAttenuationRadius(420.f);
	CompanionLight->SetLightColor(FLinearColor(0.62f, 0.86f, 0.94f));

	Health = MaxHealth;
}

void AHPWH_Companion::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
	UpdatePresentation();
}

void AHPWH_Companion::InitCompanion(AHPWH_WorldDirector* InDirector, const FIntPoint& InCell)
{
	Director = InDirector;
	SourceCell = InCell;
	SpawnLocation = GetActorLocation();
	State = EHPWH_CompanionState::Waiting;
	Health = MaxHealth;
	bGreeted = false;
	bDead = false;
	UpdatePresentation();
}

void AHPWH_Companion::ResetForRespawn()
{
	bDead = false;
	State = EHPWH_CompanionState::Waiting;
	Health = MaxHealth;
	bGreeted = false;
	NextBetrayalRollTime = 0.f;
	SetActorLocation(SpawnLocation);
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	UpdatePresentation();
}

void AHPWH_Companion::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bDead || !GetWorld())
	{
		return;
	}

	auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	AHPWH_PlayerCharacter* Player = GM ? GM->GetPlayerBody() : nullptr;
	if (!Player)
	{
		return;
	}

	const float Distance = FVector::Dist2D(GetActorLocation(), Player->GetActorLocation());
	const float Now = GetWorld()->GetTimeSeconds();

	switch (State)
	{
	case EHPWH_CompanionState::Waiting:
		// 玩家靠近时先打招呼，再显示“结伴同行”交互。
		if (!bGreeted && Distance <= 700.f && GM)
		{
			bGreeted = true;
			GM->PushMessage(TEXT("「你也是被拖进来的？一起走吧。」"), 3.4f);
			UE_LOG(LogTemp, Display, TEXT("HPWH: companion greeted player"));
		}
		break;
	case EHPWH_CompanionState::Following:
		TickFollow(DeltaSeconds, Player, Distance);
		break;
	case EHPWH_CompanionState::Hostile:
		TickHostile(DeltaSeconds, Player, Distance);
		break;
	default:
		break;
	}
}

void AHPWH_Companion::TickFollow(float DeltaSeconds, AHPWH_PlayerCharacter* Player, float Distance)
{
	if (Distance > 360.f)
	{
		FVector Direction = Player->GetActorLocation() - GetActorLocation();
		Direction.Z = 0.f;
		if (Direction.SizeSquared() > 1.f)
		{
			AddMovementInput(Direction.GetSafeNormal(), 1.f);
		}
	}

	if (!GetWorld())
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now < NextShotTime)
	{
		return;
	}
	if (AActor* Target = FindNearestEnemy())
	{
		if (FVector::Dist2D(GetActorLocation(), Target->GetActorLocation()) <= ShotRange)
		{
			TryFireAt(Target, false);
			NextShotTime = Now + ShotInterval;
		}
	}
}

void AHPWH_Companion::TickHostile(float DeltaSeconds, AHPWH_PlayerCharacter* Player, float Distance)
{
	if (!GetWorld())
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();

	if (Distance > 460.f)
	{
		FVector Direction = Player->GetActorLocation() - GetActorLocation();
		Direction.Z = 0.f;
		if (Direction.SizeSquared() > 1.f)
		{
			AddMovementInput(Direction.GetSafeNormal(), 1.f);
		}
	}

	if (Distance <= 170.f)
	{
		if (Now >= NextMeleeTime)
		{
			NextMeleeTime = Now + 1.3f;
			Player->TakeHPWHDamage(HPWH_MonsterAttackDamage, false);
			UE_LOG(LogTemp, Display, TEXT("HPWH: mimic attacked body"));
		}
		return;
	}

	if (Distance <= 1500.f && Now >= NextShotTime && HasLineOfSightTo(Player))
	{
		TryFireAt(Player, true);
		NextShotTime = Now + 1.6f;
	}
}

void AHPWH_Companion::TryFireAt(AActor* Target, bool bHostileShot)
{
	if (!Target || !GetWorld())
	{
		return;
	}
	FVector Aim = Target->GetActorLocation() + FVector(0.f, 0.f, 40.f) - GetActorLocation();
	Aim.Z = 0.f;
	if (Aim.IsNearlyZero())
	{
		return;
	}
	const FVector Direction = Aim.GetSafeNormal();
	const FVector Start = GetActorLocation() + Direction * 55.f + FVector(0.f, 0.f, 45.f);

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AHPWH_Projectile* Projectile = GetWorld()->SpawnActor<AHPWH_Projectile>(
		AHPWH_Projectile::StaticClass(), Start, Direction.Rotation(), Params))
	{
		const float Damage = bHostileShot ? HPWH_MonsterAttackDamage : FriendlyShotDamage;
		Projectile->InitProjectile(Direction, 2400.f, Damage, bHostileShot, this);
		if (auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
		{
			GM->PlaySfxAt(EHPWH_Sfx::Attack, Start, bHostileShot ? 0.3f : 0.24f, bHostileShot ? 0.7f : 1.1f);
		}
		if (FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugCompanion")))
		{
			UE_LOG(LogTemp, Display, TEXT("HPWH: companion fired hostile=%d"), bHostileShot ? 1 : 0);
		}
	}
}

AActor* AHPWH_Companion::FindNearestEnemy() const
{
	if (!GetWorld())
	{
		return nullptr;
	}
	AActor* Best = nullptr;
	float BestDistance = TNumericLimits<float>::Max();
	for (TActorIterator<AHPWH_EnemyBase> It(GetWorld()); It; ++It)
	{
		if (!IsValid(*It) || It->IsDead())
		{
			continue;
		}
		const float Distance = FVector::Dist2D(GetActorLocation(), It->GetActorLocation());
		if (Distance < BestDistance && HasLineOfSightTo(*It))
		{
			BestDistance = Distance;
			Best = *It;
		}
	}
	return Best;
}

bool AHPWH_Companion::HasLineOfSightTo(const AActor* Target) const
{
	if (!Target || !GetWorld())
	{
		return false;
	}
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HPWHCompanionShot), false, this);
	Params.AddIgnoredActor(Target);
	const FVector Start = GetActorLocation() + FVector(0.f, 0.f, 40.f);
	const FVector End = Target->GetActorLocation() + FVector(0.f, 0.f, 30.f);
	return !GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
}

void AHPWH_Companion::NotifyPlayerHurt()
{
	if (bDead || State != EHPWH_CompanionState::Following || !GetWorld())
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	// 受伤很频繁，加一次判定冷却，避免一两秒内必定叛变。
	if (Now < NextBetrayalRollTime)
	{
		return;
	}
	NextBetrayalRollTime = Now + 5.f;

	const float Roll = FMath::FRand();
	UE_LOG(LogTemp, Display, TEXT("HPWH: companion betrayal roll=%.2f"), Roll);
	if (Roll < 0.5f)
	{
		Betray();
	}
}

void AHPWH_Companion::ForceBetray()
{
	if (State == EHPWH_CompanionState::Hostile || bDead)
	{
		return;
	}
	Betray();
}

void AHPWH_Companion::JoinParty()
{
	if (bDead || State != EHPWH_CompanionState::Waiting)
	{
		return;
	}
	State = EHPWH_CompanionState::Following;
	GetCharacterMovement()->MaxWalkSpeed = FollowSpeed;
	UpdatePresentation();
	if (auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->PushMessage(TEXT("同伴加入了队伍 · 他会跟着你"), 2.8f);
		GM->PlaySfx2D(EHPWH_Sfx::CardAppear, 0.5f);
	}
	UE_LOG(LogTemp, Display, TEXT("HPWH: companion joined party"));
}

void AHPWH_Companion::Betray()
{
	if (bDead)
	{
		return;
	}
	State = EHPWH_CompanionState::Hostile;
	GetCharacterMovement()->MaxWalkSpeed = HPWH_PlayerBaseSpeed;
	NextShotTime = GetWorld() ? GetWorld()->GetTimeSeconds() + 0.8f : 0.f;
	NextMeleeTime = 0.f;
	UpdatePresentation();
	if (auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
	{
		// 叛变必须给出明确提示，避免玩家把红名当成普通怪物。
		GM->PushMessage(TEXT("同伴停住了……「模拟人」露出了真面目"), 4.5f, true);
		GM->PlaySfx2D(EHPWH_Sfx::CrawlerAppear, 0.8f);
	}
	UE_LOG(LogTemp, Display, TEXT("HPWH: companion betrayed, mimic awakened"));
}

void AHPWH_Companion::ReceiveHPWHDamage(float Damage)
{
	if (bDead || State != EHPWH_CompanionState::Hostile)
	{
		return;
	}
	Health -= Damage;
	if (Health <= 0.f)
	{
		Die();
	}
}

void AHPWH_Companion::Die()
{
	if (bDead)
	{
		return;
	}
	bDead = true;
	State = EHPWH_CompanionState::Down;
	GetCharacterMovement()->StopMovementImmediately();
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
	if (auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->PushMessage(TEXT("模拟人倒下了"), 3.f);
	}
	UE_LOG(LogTemp, Display, TEXT("HPWH: mimic killed"));
	Destroy();
}

void AHPWH_Companion::UpdatePresentation()
{
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!Base)
	{
		return;
	}
	const FLinearColor Color = State == EHPWH_CompanionState::Hostile
		? HPWH_CompanionHostileColor : HPWH_CompanionFriendlyColor;
	if (UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Base, this))
	{
		Mid->SetVectorParameterValue(TEXT("Color"), Color);
		GetMesh()->SetMaterial(0, Mid);
	}
	if (CompanionLight)
	{
		CompanionLight->SetLightColor(State == EHPWH_CompanionState::Hostile
			? FLinearColor(0.9f, 0.25f, 0.22f) : FLinearColor(0.62f, 0.86f, 0.94f));
	}
}

FString AHPWH_Companion::GetHPWHInteractPrompt(APawn* Interactor) const
{
	return State == EHPWH_CompanionState::Waiting ? TEXT("结伴同行") : FString();
}

bool AHPWH_Companion::CanHPWHInteract(APawn* Interactor) const
{
	return !bDead && State == EHPWH_CompanionState::Waiting;
}

void AHPWH_Companion::HPWHInteract(APawn* Interactor)
{
	JoinParty();
}
