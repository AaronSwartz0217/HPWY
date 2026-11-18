#include "HPWH/HPWH_POI.h"
#include "PhysicsEngine/BodySetup.h"

#include "HPWH/HPWH_GameMode.h"
#include "HPWH/HPWH_World.h"
#include "HPWH/HPWH_Player.h"
#include "HPWH/HPWH_Enemies.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	UStaticMesh* HPWHCubeMesh()
	{
		static TWeakObjectPtr<UStaticMesh> Cached;
		if (!Cached.IsValid())
		{
			Cached = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		}
		return Cached.Get();
	}

	UStaticMesh* HPWHCylinderMesh()
	{
		static TWeakObjectPtr<UStaticMesh> Cached;
		if (!Cached.IsValid())
		{
			Cached = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
		}
		return Cached.Get();
	}

	UMaterialInstanceDynamic* HPWHColoredMaterial(UObject* Outer, const FLinearColor& Color)
	{
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr,
			TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		if (!Base)
		{
			return nullptr;
		}
		UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Base, Outer);
		if (Mid)
		{
			Mid->SetVectorParameterValue(TEXT("Color"), Color);
		}
		return Mid;
	}

	UStaticMeshComponent* AddBox(AActor* Owner, USceneComponent* Parent, UStaticMesh* Mesh,
		const FVector& Location, const FVector& SizeCm, const FLinearColor& Color, bool bCollide,
		TArray<TObjectPtr<UStaticMeshComponent>>& OutMeshes)
	{
		UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(Owner);
		Component->SetupAttachment(Parent);
		Component->RegisterComponent();
		Component->SetStaticMesh(Mesh);
		Component->SetRelativeLocation(Location);
		Component->SetRelativeScale3D(SizeCm / 100.f);
		if (UMaterialInstanceDynamic* Material = HPWHColoredMaterial(Owner, Color))
		{
			Component->SetMaterial(0, Material);
		}
		Component->SetCollisionEnabled(bCollide ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		Component->SetCollisionProfileName(bCollide ? TEXT("BlockAll") : TEXT("NoCollision"));
		OutMeshes.Add(Component);
		return Component;
	}
}

AHPWH_POIBase::AHPWH_POIBase()
{
	PrimaryActorTick.bCanEverTick = true;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void AHPWH_POIBase::BeginPlay()
{
	Super::BeginPlay();
}

void AHPWH_POIBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 格子卸载时清理临时谜题物件，避免地砖残留和灯芯重复生成。
	for (AHPWH_PuzzleTile* Tile : Tiles)
	{
		if (IsValid(Tile))
		{
			Tile->Destroy();
		}
	}
	Tiles.Empty();
	TileWasInside.Empty();
	if (IsValid(Core))
	{
		Core->Destroy();
	}
	Core = nullptr;
	bPuzzleActive = false;
	Super::EndPlay(EndPlayReason);
}

bool AHPWH_POIBase::CanHPWHInteract(APawn* Interactor) const
{
	const auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	return !GM || GM->GetSoulState() == EHPWH_SoulState::Embodied;
}

void AHPWH_POIBase::InitializePOI(AHPWH_WorldDirector* InDirector, const FIntPoint& InCell, bool bAlreadySolved)
{
	Director = InDirector;
	Cell = InCell;
	BuildVisuals();

	if (bAlreadySolved)
	{
		bSolved = true;
		bPuzzleActive = false;
	}
}

void AHPWH_POIBase::BuildVisuals()
{
}

float AHPWH_POIBase::GetPuzzleTimeLimit() const
{
	const AHPWH_GameMode* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	return GetPuzzleBaseTime() + (GM ? GM->GetPuzzleTimeBonus() : 0.f);
}

float AHPWH_POIBase::GetPuzzleRemaining() const
{
	if (!IsPuzzleRunning() || !GetWorld())
	{
		return 0.f;
	}
	return FMath::Max(0.f, GetPuzzleTimeLimit() - (GetWorld()->GetTimeSeconds() - StartTime));
}

void AHPWH_POIBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bPuzzleActive || bSolved)
	{
		return;
	}
	auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	const AHPWH_PlayerCharacter* Body = GM ? GM->GetPlayerBody() : nullptr;
	if (!Body)
	{
		return;
	}

	// 解密时限由具体器物决定，可被“时限”强化延长。
	const float TimeLimit = GetPuzzleBaseTime() + (GM ? GM->GetPuzzleTimeBonus() : 0.f);
	if (GetWorld()->GetTimeSeconds() - StartTime > TimeLimit)
	{
		StartTime = GetWorld()->GetTimeSeconds();
		FailPuzzle();
		if (GM)
		{
			GM->PushMessage(GetTimeoutMessage(), 2.f);
		}
		return;
	}

	if (TileWasInside.Num() != Tiles.Num())
	{
		TileWasInside.Init(false, Tiles.Num());
	}

	const FVector PlayerLocation = Body->GetActorLocation();
	for (int32 Index = 0; Index < Tiles.Num(); ++Index)
	{
		AHPWH_PuzzleTile* Tile = Tiles[Index];
		if (!IsValid(Tile))
		{
			continue;
		}
		const bool bInside = Tile->IsPlayerInside(PlayerLocation);
		if (bInside && !TileWasInside[Index])
		{
			if (Tile->GetTileIndex() == StepIndex)
			{
				Tile->SetLit(true);
				if (GM)
				{
					GM->PlaySfxAt(GetTileSfx(), Tile->GetActorLocation(), 0.7f);
				}
				++StepIndex;
				if (StepIndex >= Tiles.Num())
				{
					SolvePuzzle();
					return;
				}
				GM->PushMessage(FString::Printf(TEXT("继续  下一块是 %d"), StepIndex + 1), 1.6f);
			}
			else
			{
				FailPuzzle();
			}
		}
		TileWasInside[Index] = bInside;
	}
}

FString AHPWH_POIBase::GetHPWHInteractPrompt(APawn* Interactor) const
{
	if (bSolved)
	{
		return TEXT("器物已经安静下来");
	}
	return bPuzzleActive ? TEXT("谜题进行中") : TEXT("查看器物");
}

void AHPWH_POIBase::HPWHInteract(APawn* Interactor)
{
	auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	if (GM && GM->GetSoulState() != EHPWH_SoulState::Embodied)
	{
		return;
	}
	if (bSolved)
	{
		if (auto* MutableGM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
		{
			MutableGM->PushMessage(TEXT("这里已经没有灯芯了"), 2.f);
		}
		return;
	}
	if (bPuzzleActive)
	{
		return;
	}
	StartPuzzle();
}

void AHPWH_POIBase::StartPuzzle()
{
	if (bSolved || bPuzzleActive || !UsesTilePuzzle())
	{
		return;
	}
	if (auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->SetFloorState(EHPWH_FloorState::PuzzleActive);
		const float LimitSeconds = GetPuzzleBaseTime() + GM->GetPuzzleTimeBonus();
		GM->PushMessage(FString::Printf(TEXT("%s  %.0f 秒"), *GetStartHint(), LimitSeconds), 4.f);
	}

	ResetTiles();
	const FVector Base = GetActorLocation();
	const int32 TileCount = FMath::Max(1, GetPuzzleTileCount());
	const bool bReverse = IsReverseTileOrder();
	for (int32 Index = 0; Index < TileCount; ++Index)
	{
		const float Lateral = (Index - (TileCount - 1) * 0.5f) * 210.f;
		FVector Location = Base + FVector(430.f, Lateral, 0.f);
		if (ActorHasTag(TEXT("Courtyard")))
		{
			Location = Base + FVector(500.f, Lateral, 0.f);
		}
		AHPWH_PuzzleTile* Tile = GetWorld()->SpawnActor<AHPWH_PuzzleTile>(
			AHPWH_PuzzleTile::StaticClass(), Location, FRotator::ZeroRotator);
		if (Tile)
		{
			const int32 Order = bReverse ? (TileCount - 1 - Index) : Index;
			Tile->Configure(Order, this, Location);
			Tiles.Add(Tile);
		}
	}
	TileWasInside.Init(false, Tiles.Num());
	StepIndex = 0;
	bPuzzleActive = true;
	StartTime = GetWorld()->GetTimeSeconds();
}

FString AHPWH_POIBase::GetStartHint() const
{
	return TEXT("按 1 → 2 → 3 的顺序踩亮地砖");
}

FString AHPWH_POIBase::GetTimeoutMessage() const
{
	return TEXT("谜题超时  重新开始");
}

void AHPWH_POIBase::FailPuzzle()
{
	StepIndex = 0;
	for (AHPWH_PuzzleTile* Tile : Tiles)
	{
		if (IsValid(Tile))
		{
			Tile->SetLit(false);
		}
	}
	if (auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->PlaySfx2D(EHPWH_Sfx::SoulOut, 0.45f, 0.8f);
		GM->OnPuzzleFailed();
	}
}

void AHPWH_POIBase::SolvePuzzle()
{
	bPuzzleActive = false;
	bSolved = true;
	StepIndex = Tiles.Num();

	EHPWH_TerrainType Terrain = EHPWH_TerrainType::Empty;
	if (ActorHasTag(TEXT("Mirror")))
	{
		Terrain = EHPWH_TerrainType::Mirror;
	}
	else if (ActorHasTag(TEXT("Courtyard")))
	{
		Terrain = EHPWH_TerrainType::Courtyard;
	}
	else if (ActorHasTag(TEXT("Laptop")))
	{
		Terrain = EHPWH_TerrainType::Laptop;
	}

	if (auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->OnPuzzleSolved(Terrain);
	}
	if (Director)
	{
		Director->NotifyPuzzleSolved(Cell);
	}
}

void AHPWH_POIBase::SpawnCoreItem()
{
	if (IsValid(Core) || !GetWorld())
	{
		return;
	}
	Core = GetWorld()->SpawnActor<AHPWH_CoreItem>(
		AHPWH_CoreItem::StaticClass(), GetCoreSpawnLocation(), FRotator::ZeroRotator);
	if (Core)
	{
		Core->InitializeCore(Director, Cell);
	}
}

void AHPWH_POIBase::ResetPuzzle()
{
	for (AHPWH_PuzzleTile* Tile : Tiles)
	{
		if (IsValid(Tile))
		{
			Tile->Destroy();
		}
	}
	Tiles.Empty();
	TileWasInside.Empty();
	if (IsValid(Core))
	{
		Core->Destroy();
	}
	Core = nullptr;
	bPuzzleActive = false;
	bSolved = false;
	StepIndex = 0;
}

void AHPWH_POIBase::ResetTiles()
{
	for (AHPWH_PuzzleTile* Tile : Tiles)
	{
		if (IsValid(Tile))
		{
			Tile->Destroy();
		}
	}
	Tiles.Empty();
	TileWasInside.Empty();
}

FVector AHPWH_POIBase::GetCoreSpawnLocation() const
{
	return GetActorLocation() + FVector(300.f, 0.f, 90.f);
}

AHPWH_MirrorPOI::AHPWH_MirrorPOI()
{
	Tags.Add(TEXT("Mirror"));
}

void AHPWH_MirrorPOI::BuildVisuals()
{
	const FLinearColor Stone(0.34f, 0.33f, 0.30f);
	const FLinearColor Jade(0.42f, 0.62f, 0.60f);
	const FLinearColor Frame(0.20f, 0.24f, 0.22f);

	AddBox(this, SceneRoot, HPWHCylinderMesh(), FVector(0.f, 0.f, 22.f), FVector(340.f, 340.f, 44.f), Stone, true, BuiltMeshes);
	AddBox(this, SceneRoot, HPWHCylinderMesh(), FVector(0.f, 0.f, 210.f), FVector(260.f, 260.f, 18.f), Jade, true, BuiltMeshes)
		->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	AddBox(this, SceneRoot, HPWHCubeMesh(), FVector(0.f, 0.f, 220.f), FVector(30.f, 320.f, 30.f), Frame, true, BuiltMeshes);
	AddBox(this, SceneRoot, HPWHCubeMesh(), FVector(0.f, 0.f, 220.f), FVector(320.f, 30.f, 30.f), Frame, true, BuiltMeshes);

	if (UPointLightComponent* Light = NewObject<UPointLightComponent>(this))
	{
		Light->SetupAttachment(SceneRoot);
		Light->RegisterComponent();
		Light->SetRelativeLocation(FVector(0.f, 0.f, 240.f));
		Light->SetIntensity(450.f);
		Light->SetAttenuationRadius(1000.f);
		Light->SetLightColor(FLinearColor(0.55f, 0.82f, 0.82f));
	}

	// 用导入的八卦镜模型替换程序化外观，旧网格保留碰撞。
	for (UStaticMeshComponent* Mesh : BuiltMeshes)
	{
		if (Mesh)
		{
			Mesh->SetVisibility(false);
		}
	}
	AddImportedMesh(TEXT("/Game/HPWH/Assets/Models/八卦镜/StaticMeshes/bagua_mirror.bagua_mirror"),
		FVector(16.f), FVector(0.f, 0.f, 192.f));
}

UStaticMeshComponent* AHPWH_POIBase::AddImportedMesh(const TCHAR* MeshPath, const FVector& Scale,
	const FVector& Offset, const FRotator& Rotation)
{
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, MeshPath);
	if (!Mesh)
	{
		return nullptr;
	}
	UStaticMeshComponent* MeshComp = NewObject<UStaticMeshComponent>(this);
	MeshComp->SetupAttachment(SceneRoot);
	MeshComp->SetStaticMesh(Mesh);
	// 使用同目录下的双面材质，避免模型面片反向时整体发黑。
	const FString Path(MeshPath);
	const int32 MarkerIndex = Path.Find(TEXT("/StaticMeshes/"));
	if (MarkerIndex != INDEX_NONE)
	{
		const FString Folder = Path.Left(MarkerIndex);
		const FString Leaf = FPaths::GetCleanFilename(Folder);
		const FString MatPath = Folder + TEXT("/Materials/M_") + Leaf + TEXT("_TwoSided.M_") + Leaf + TEXT("_TwoSided");
		if (UMaterialInterface* TwoSided = LoadObject<UMaterialInterface>(nullptr, *MatPath))
		{
			MeshComp->SetMaterial(0, TwoSided);
		}
	}
	MeshComp->SetRelativeLocation(Offset);
	MeshComp->SetRelativeRotation(Rotation);
	MeshComp->SetRelativeScale3D(Scale);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AddInstanceComponent(MeshComp);
	MeshComp->RegisterComponent();
	return MeshComp;
}

void AHPWH_MirrorPOI::StartPuzzle()
{
	if (bSolved || bPuzzleActive || !GetWorld())
	{
		return;
	}
	if (auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->SetFloorState(EHPWH_FloorState::PuzzleActive);
		const float LimitSeconds = GetPuzzleBaseTime() + GM->GetPuzzleTimeBonus();
		GM->PushMessage(FString::Printf(TEXT("%s  %.0f 秒"), *GetStartHint(), LimitSeconds), 4.f);
	}

	ResetTiles();
	// 4 块地砖放在镜子前方弧线上，避免被放大的镜体压住。
	const FVector Base = GetActorLocation();
	const int32 Count = 4;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Lateral = (Index - (Count - 1) * 0.5f) * 200.f;
		const FVector Location = Base + FVector(560.f, Lateral, 6.f);
		AHPWH_PuzzleTile* Tile = GetWorld()->SpawnActor<AHPWH_PuzzleTile>(
			AHPWH_PuzzleTile::StaticClass(), Location, FRotator::ZeroRotator);
		if (Tile)
		{
			Tile->Configure(Count - 1 - Index, this, Location);
			Tiles.Add(Tile);
		}
	}
	TileWasInside.Init(false, Tiles.Num());
	StepIndex = 0;
	bPuzzleActive = true;
	StartTime = GetWorld()->GetTimeSeconds();
}

FString AHPWH_MirrorPOI::GetHPWHInteractPrompt(APawn* Interactor) const
{
	if (bSolved)
	{
		return TEXT("八卦镜已经安静");
	}
	return bPuzzleActive ? TEXT("八卦镜谜题进行中") : TEXT("查看八卦镜");
}

AHPWH_CourtyardPOI::AHPWH_CourtyardPOI()
{
	Tags.Add(TEXT("Courtyard"));
}

void AHPWH_CourtyardPOI::BuildVisuals()
{
	const FLinearColor Wall(0.30f, 0.27f, 0.23f);
	const FLinearColor Stone(0.24f, 0.23f, 0.21f);
	const FLinearColor Wood(0.20f, 0.14f, 0.10f);

	AddBox(this, SceneRoot, HPWHCubeMesh(), FVector(0.f, 0.f, 4.f), FVector(620.f, 620.f, 8.f), Stone, false, BuiltMeshes);
	AddBox(this, SceneRoot, HPWHCubeMesh(), FVector(0.f, 300.f, 130.f), FVector(640.f, 40.f, 260.f), Wall, true, BuiltMeshes);
	AddBox(this, SceneRoot, HPWHCubeMesh(), FVector(0.f, -300.f, 130.f), FVector(640.f, 40.f, 260.f), Wall, true, BuiltMeshes);
	AddBox(this, SceneRoot, HPWHCubeMesh(), FVector(-300.f, 0.f, 130.f), FVector(40.f, 560.f, 260.f), Wall, true, BuiltMeshes);
	AddBox(this, SceneRoot, HPWHCubeMesh(), FVector(300.f, 190.f, 130.f), FVector(40.f, 220.f, 260.f), Wall, true, BuiltMeshes);
	AddBox(this, SceneRoot, HPWHCubeMesh(), FVector(300.f, -190.f, 130.f), FVector(40.f, 220.f, 260.f), Wall, true, BuiltMeshes);

	AddBox(this, SceneRoot, HPWHCubeMesh(), FVector(300.f, 82.f, 170.f), FVector(46.f, 46.f, 340.f), Wood, true, BuiltMeshes);
	AddBox(this, SceneRoot, HPWHCubeMesh(), FVector(300.f, -82.f, 170.f), FVector(46.f, 46.f, 340.f), Wood, true, BuiltMeshes);
	AddBox(this, SceneRoot, HPWHCubeMesh(), FVector(300.f, 0.f, 345.f), FVector(46.f, 250.f, 46.f), Wood, true, BuiltMeshes);

	if (UPointLightComponent* Light = NewObject<UPointLightComponent>(this))
	{
		Light->SetupAttachment(SceneRoot);
		Light->RegisterComponent();
		Light->SetRelativeLocation(FVector(360.f, 0.f, 250.f));
		Light->SetIntensity(350.f);
		Light->SetAttenuationRadius(900.f);
		Light->SetLightColor(FLinearColor(0.95f, 0.72f, 0.45f));
	}

	// 四合院使用模型三角网格碰撞，保留旧墙作为资产不可用时的回退。
	if (UStaticMeshComponent* Courtyard = AddImportedMesh(
		TEXT("/Game/HPWH/Assets/Models/四合院模型/StaticMeshes/courtyard_model.courtyard_model"),
		FVector(0.21f), FVector(0.f, 0.f, 74.f)))
	{
		const UBodySetup* Body = Courtyard->GetBodySetup();
		const bool bUseMeshCollision = Body && Body->CollisionTraceFlag == CTF_UseComplexAsSimple;
		if (bUseMeshCollision)
		{
			Courtyard->SetCollisionProfileName(TEXT("BlockAll"));
			Courtyard->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
		for (UStaticMeshComponent* Mesh : BuiltMeshes)
		{
			if (!Mesh) continue;
			Mesh->SetVisibility(false);
			if (bUseMeshCollision) Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void AHPWH_CourtyardPOI::StartPuzzle()
{
	if (bSolved || bPuzzleActive)
	{
		return;
	}
	if (auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->SetFloorState(EHPWH_FloorState::PuzzleActive);
		const float LimitSeconds = GetPuzzleBaseTime() + GM->GetPuzzleTimeBonus();
		GM->PushMessage(FString::Printf(TEXT("%s  %.0f 秒"), *GetStartHint(), LimitSeconds), 4.f);
	}
	bPuzzleActive = true;
	StartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
}

void AHPWH_CourtyardPOI::FailPuzzle()
{
	bPuzzleActive = false;
	if (auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->PlaySfx2D(EHPWH_Sfx::SoulOut, 0.45f, 0.8f);
		GM->OnPuzzleFailed();
	}
}

void AHPWH_CourtyardPOI::Tick(float DeltaSeconds)
{
	if (!bPuzzleActive || bSolved || !GetWorld())
	{
		return;
	}
	auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM)
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	const float Limit = GetPuzzleBaseTime() + GM->GetPuzzleTimeBonus();
	if (Now - StartTime > Limit)
	{
		FailPuzzle();
		StartTime = Now;
		GM->PushMessage(GetTimeoutMessage(), 2.f);
		return;
	}

	// 任意普通怪物进入四合院范围即视为完成；徘徊者不是 EnemyBase，自然被排除。
	for (TActorIterator<AHPWH_EnemyBase> It(GetWorld()); It; ++It)
	{
		if (!IsValid(*It) || It->IsDead())
		{
			continue;
		}
		if (FVector::Dist2D(It->GetActorLocation(), GetActorLocation()) <= 320.f)
		{
			SolvePuzzle();
			return;
		}
	}
}

FString AHPWH_CourtyardPOI::GetHPWHInteractPrompt(APawn* Interactor) const
{
	if (bSolved)
	{
		return TEXT("四合院的门缝里已经没有光");
	}
	return bPuzzleActive ? TEXT("等待客人进入四合院") : TEXT("观察四合院院门");
}

FVector AHPWH_CourtyardPOI::GetCoreSpawnLocation() const
{
	return GetActorLocation() + FVector(440.f, 0.f, 90.f);
}

FString AHPWH_MirrorPOI::GetStartHint() const
{
	return TEXT("镜中数字是反的：按 1 → 2 → 3 → 4 踩亮");
}

FString AHPWH_CourtyardPOI::GetStartHint() const
{
	return TEXT("这个地方似乎需要一位客人");
}

AHPWH_LaptopPOI::AHPWH_LaptopPOI()
{
	Tags.Add(TEXT("Laptop"));
}

void AHPWH_LaptopPOI::BuildVisuals()
{
	const FLinearColor Shell(0.11f, 0.12f, 0.15f);
	const FLinearColor Screen(0.16f, 0.34f, 0.48f);
	const FLinearColor Keys(0.20f, 0.22f, 0.26f);

	AddBox(this, SceneRoot, HPWHCubeMesh(), FVector(0.f, 0.f, 16.f), FVector(280.f, 190.f, 26.f), Shell, true, BuiltMeshes);
	AddBox(this, SceneRoot, HPWHCubeMesh(), FVector(0.f, 0.f, 32.f), FVector(230.f, 140.f, 6.f), Keys, false, BuiltMeshes);
	UStaticMeshComponent* Lid = AddBox(this, SceneRoot, HPWHCubeMesh(),
		FVector(0.f, 92.f, 118.f), FVector(280.f, 14.f, 180.f), Shell, true, BuiltMeshes);
	if (Lid)
	{
		Lid->SetRelativeRotation(FRotator(-12.f, 0.f, 0.f));
	}
	UStaticMeshComponent* Display = AddBox(this, SceneRoot, HPWHCubeMesh(),
		FVector(0.f, 98.f, 118.f), FVector(240.f, 6.f, 140.f), Screen, false, BuiltMeshes);
	if (Display)
	{
		Display->SetRelativeRotation(FRotator(-12.f, 0.f, 0.f));
	}

	if (UPointLightComponent* Light = NewObject<UPointLightComponent>(this))
	{
		Light->SetupAttachment(SceneRoot);
		Light->RegisterComponent();
		Light->SetRelativeLocation(FVector(0.f, 120.f, 140.f));
		Light->SetIntensity(420.f);
		Light->SetAttenuationRadius(1000.f);
		Light->SetLightColor(FLinearColor(0.42f, 0.68f, 1.f));
	}

	// 笔记本模型约 38 cm，放大到 1.5 m 便于俯视识别。
	for (UStaticMeshComponent* Mesh : BuiltMeshes)
	{
		if (Mesh)
		{
			Mesh->SetVisibility(false);
		}
	}
	AddImportedMesh(TEXT("/Game/HPWH/Assets/Models/笔记本电脑/StaticMeshes/laptop.laptop"),
		FVector(24.f), FVector(0.f, 0.f, 216.f));
}

FString AHPWH_LaptopPOI::GetHPWHInteractPrompt(APawn* Interactor) const
{
	if (bSolved)
	{
		return TEXT("笔记本已经闭合");
	}
	if (bPuzzleActive)
	{
		return CollectedCats >= 3
			? TEXT("交付猫猫")
			: FString::Printf(TEXT("找回猫猫 %d/3"), CollectedCats);
	}
	return TEXT("打开笔记本电脑");
}

void AHPWH_LaptopPOI::HPWHInteract(APawn* Interactor)
{
	auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	if (GM && GM->GetSoulState() != EHPWH_SoulState::Embodied)
	{
		GM->PushMessage(TEXT("灵魂碰不到笔记本"), 2.f);
		return;
	}
	if (bSolved)
	{
		if (GM)
		{
			GM->PushMessage(TEXT("笔记本已经安静了"), 2.f);
		}
		return;
	}
	if (bPuzzleActive)
	{
		if (CollectedCats >= 3)
		{
			SolvePuzzle();
		}
		else if (GM)
		{
			GM->PushMessage(FString::Printf(TEXT("还差 %d 只猫猫"), 3 - CollectedCats), 2.f);
		}
		return;
	}
	StartPuzzle();
}

FString AHPWH_LaptopPOI::GetStartHint() const
{
	return TEXT("笔记本在找猫：附近两格内找回 3 只");
}

void AHPWH_LaptopPOI::StartPuzzle()
{
	if (bSolved || bPuzzleActive || !GetWorld())
	{
		return;
	}
	if (auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->SetFloorState(EHPWH_FloorState::PuzzleActive);
		const float LimitSeconds = GetPuzzleBaseTime() + GM->GetPuzzleTimeBonus();
		GM->PushMessage(FString::Printf(TEXT("%s  %.0f 秒"), *GetStartHint(), LimitSeconds), 4.f);
	}

	DestroyCats();
	CollectedCats = 0;
	bPuzzleActive = true;
	StartTime = GetWorld()->GetTimeSeconds();

	const FIntPoint Center = Cell;
	for (int32 Spawned = 0; Spawned < 3; ++Spawned)
	{
		FVector SpawnLocation = GetActorLocation() + FVector(400.f, 0.f, 60.f);
		for (int32 Attempt = 0; Attempt < 16; ++Attempt)
		{
			const int32 DX = FMath::RandRange(-2, 2);
			const int32 DY = FMath::RandRange(-2, 2);
			if (FMath::Max(FMath::Abs(DX), FMath::Abs(DY)) < 1)
			{
				continue;
			}
			const FVector CellCenter = Director
				? Director->CellCenter(Center + FIntPoint(DX, DY))
				: GetActorLocation();
			SpawnLocation = HPWH_SnapDecimeter(CellCenter + FVector(
				FMath::FRandRange(-260.f, 260.f), FMath::FRandRange(-260.f, 260.f), 60.f));
			break;
		}

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AHPWH_CatMeme* Cat = GetWorld()->SpawnActor<AHPWH_CatMeme>(
			AHPWH_CatMeme::StaticClass(), SpawnLocation, FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), Params))
		{
			Cat->InitializeCat(this, Spawned);
			CatMemes.Add(Cat);
			UE_LOG(LogTemp, Display, TEXT("HPWH: laptop cat %d at (%.1f,%.1f)m"), Spawned,
				SpawnLocation.X / 100.f, SpawnLocation.Y / 100.f);
		}
	}
	UE_LOG(LogTemp, Display, TEXT("HPWH: laptop puzzle started cell=(%d,%d) cats=%d"),
		Cell.X, Cell.Y, CatMemes.Num());
}

void AHPWH_LaptopPOI::FailPuzzle()
{
	DestroyCats();
	CollectedCats = 0;
	bPuzzleActive = false;
	if (auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->PlaySfx2D(EHPWH_Sfx::SoulOut, 0.45f, 0.8f);
		GM->OnPuzzleFailed();
	}
}

void AHPWH_LaptopPOI::ResetPuzzle()
{
	DestroyCats();
	CollectedCats = 0;
	Super::ResetPuzzle();
}

void AHPWH_LaptopPOI::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyCats();
	Super::EndPlay(EndPlayReason);
}

void AHPWH_LaptopPOI::CollectCat(AHPWH_CatMeme* Cat)
{
	if (bSolved)
	{
		return;
	}
	++CollectedCats;
	if (auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (IsValid(Cat))
		{
			GM->PlaySfxAt(EHPWH_Sfx::TileLaptop, Cat->GetActorLocation(), 0.7f);
		}
		if (CollectedCats >= 3)
		{
			GM->PushMessage(TEXT("三只猫猫都找到了  回笔记本交付"), 3.f);
		}
		else
		{
			GM->PushMessage(FString::Printf(TEXT("猫猫 %d/3"), CollectedCats), 2.f);
		}
	}
	UE_LOG(LogTemp, Display, TEXT("HPWH: laptop cat collected %d/3"), CollectedCats);
}

void AHPWH_LaptopPOI::DestroyCats()
{
	for (AHPWH_CatMeme* Cat : CatMemes)
	{
		if (IsValid(Cat))
		{
			Cat->Destroy();
		}
	}
	CatMemes.Empty();
}

AHPWH_CatMeme::AHPWH_CatMeme()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("CatRoot"));
	SetRootComponent(Root);

	CatBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CatBody"));
	CatBody->SetupAttachment(Root);
	CatBody->SetStaticMesh(HPWHCubeMesh());
	CatBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CatBody->SetRelativeLocation(FVector(0.f, 0.f, 26.f));
	CatBody->SetRelativeScale3D(FVector(0.46f, 0.30f, 0.34f));

	EarLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EarLeft"));
	EarLeft->SetupAttachment(Root);
	EarLeft->SetStaticMesh(HPWHCubeMesh());
	EarLeft->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EarLeft->SetRelativeLocation(FVector(-10.f, 18.f, 54.f));
	EarLeft->SetRelativeScale3D(FVector(0.10f, 0.06f, 0.16f));

	EarRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EarRight"));
	EarRight->SetupAttachment(Root);
	EarRight->SetStaticMesh(HPWHCubeMesh());
	EarRight->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EarRight->SetRelativeLocation(FVector(-10.f, -18.f, 54.f));
	EarRight->SetRelativeScale3D(FVector(0.10f, 0.06f, 0.16f));

	CatGlow = CreateDefaultSubobject<UPointLightComponent>(TEXT("CatGlow"));
	CatGlow->SetupAttachment(Root);
	CatGlow->SetRelativeLocation(FVector(0.f, 0.f, 70.f));
	CatGlow->SetIntensity(150.f);
	CatGlow->SetAttenuationRadius(260.f);
	CatGlow->SetLightColor(FLinearColor(1.f, 0.72f, 0.35f));

	Tags.Add(TEXT("CatMeme"));
}

void AHPWH_CatMeme::InitializeCat(AHPWH_LaptopPOI* InOwner, int32 InIndex)
{
	OwnerPOI = InOwner;
	const FLinearColor Fur = InIndex % 2 == 0
		? FLinearColor(0.94f, 0.55f, 0.22f)
		: FLinearColor(0.92f, 0.88f, 0.78f);
	if (UMaterialInstanceDynamic* BodyMaterial = HPWHColoredMaterial(this, Fur))
	{
		CatBody->SetMaterial(0, BodyMaterial);
	}
	if (UMaterialInstanceDynamic* EarMaterial = HPWHColoredMaterial(this, FLinearColor(0.98f, 0.74f, 0.52f)))
	{
		EarLeft->SetMaterial(0, EarMaterial);
		EarRight->SetMaterial(0, EarMaterial);
	}
}

void AHPWH_CatMeme::HPWHInteract(APawn* Interactor)
{
	if (bCollected)
	{
		return;
	}
	auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	if (GM && GM->GetSoulState() != EHPWH_SoulState::Embodied)
	{
		return;
	}
	bCollected = true;
	if (OwnerPOI)
	{
		OwnerPOI->CollectCat(this);
	}
	Destroy();
}

bool AHPWH_CatMeme::CanHPWHInteract(APawn* Interactor) const
{
	if (bCollected)
	{
		return false;
	}
	const auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	return !GM || GM->GetSoulState() == EHPWH_SoulState::Embodied;
}

AHPWH_NextFloorPortal::AHPWH_NextFloorPortal()
{
	PrimaryActorTick.bCanEverTick = true;

	PortalRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PortalRoot"));
	SetRootComponent(PortalRoot);

	PortalDisc = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalDisc"));
	PortalDisc->SetupAttachment(PortalRoot);
	PortalDisc->SetStaticMesh(HPWHCylinderMesh());
	PortalDisc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PortalDisc->SetRelativeLocation(FVector(0.f, 0.f, 12.f));
	PortalDisc->SetRelativeScale3D(FVector(2.6f, 2.6f, 0.06f));

	PortalRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalRing"));
	PortalRing->SetupAttachment(PortalRoot);
	PortalRing->SetStaticMesh(HPWHCylinderMesh());
	PortalRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PortalRing->SetRelativeLocation(FVector(0.f, 0.f, 130.f));
	PortalRing->SetRelativeScale3D(FVector(2.8f, 2.8f, 0.10f));
	PortalRing->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));

	PortalLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PortalLight"));
	PortalLight->SetupAttachment(PortalRoot);
	PortalLight->SetRelativeLocation(FVector(0.f, 0.f, 140.f));
	PortalLight->SetIntensity(1200.f);
	PortalLight->SetAttenuationRadius(1100.f);
	PortalLight->SetLightColor(FLinearColor(0.95f, 0.16f, 0.16f));

	Tags.Add(TEXT("Portal"));
}

void AHPWH_NextFloorPortal::BeginPlay()
{
	Super::BeginPlay();
	if (UMaterialInstanceDynamic* DiscMaterial = HPWHColoredMaterial(this, FLinearColor(0.55f, 0.04f, 0.05f)))
	{
		PortalDisc->SetMaterial(0, DiscMaterial);
	}
	if (UMaterialInstanceDynamic* RingMaterial = HPWHColoredMaterial(this, FLinearColor(0.95f, 0.16f, 0.16f)))
	{
		PortalRing->SetMaterial(0, RingMaterial);
	}
}

void AHPWH_NextFloorPortal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (PortalRing)
	{
		PortalRing->AddLocalRotation(FRotator(0.f, 55.f * DeltaSeconds, 0.f));
	}
	if (PortalLight && GetWorld())
	{
		PortalLight->SetIntensity(1000.f + FMath::Sin(GetWorld()->GetTimeSeconds() * 3.2f) * 320.f);
	}
}

void AHPWH_NextFloorPortal::InitializePortal(AHPWH_WorldDirector* InDirector)
{
	Director = InDirector;
}

bool AHPWH_NextFloorPortal::CanHPWHInteract(APawn* Interactor) const
{
	const auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	return !GM || (GM->GetSoulState() == EHPWH_SoulState::Embodied && !GM->IsPortalChoiceActive());
}

void AHPWH_NextFloorPortal::HPWHInteract(APawn* Interactor)
{
	if (auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->OpenFloorPortalChoice();
	}
}

AHPWH_PuzzleTile::AHPWH_PuzzleTile()
{
	PrimaryActorTick.bCanEverTick = false;

	Plate = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Plate"));
	SetRootComponent(Plate);
	Plate->SetStaticMesh(HPWHCubeMesh());
	Plate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Plate->SetRelativeScale3D(FVector(1.6f, 1.6f, 0.1f));

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Plate);
	Label->SetRelativeLocation(FVector(0.f, 0.f, 38.f));
	Label->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetVerticalAlignment(EVRTA_TextCenter);
	Label->SetWorldSize(70.f);
	Label->SetTextRenderColor(FColor(230, 225, 200));
}

void AHPWH_PuzzleTile::Configure(int32 InIndex, AHPWH_POIBase* InOwner, const FVector& WorldLocation)
{
	TileIndex = InIndex;
	OwnerPOI = InOwner;
	SetActorLocation(WorldLocation);
	Label->SetText(FText::FromString(FString::FromInt(InIndex + 1)));
	PlateMaterial = HPWHColoredMaterial(this, FLinearColor(0.18f, 0.17f, 0.15f));
	if (PlateMaterial)
	{
		Plate->SetMaterial(0, PlateMaterial);
	}
}

void AHPWH_PuzzleTile::SetLit(bool bInLit)
{
	bLit = bInLit;
	if (PlateMaterial)
	{
		PlateMaterial->SetVectorParameterValue(TEXT("Color"), bLit
			? FLinearColor(0.35f, 0.85f, 0.38f)
			: FLinearColor(0.18f, 0.17f, 0.15f));
	}
	Label->SetTextRenderColor(bLit ? FColor(220, 255, 210) : FColor(150, 145, 125));
}

bool AHPWH_PuzzleTile::IsPlayerInside(const FVector& PlayerLocation) const
{
	return FVector::Dist2D(PlayerLocation, GetActorLocation()) <= 140.f;
}

AHPWH_CoreItem::AHPWH_CoreItem()
{
	PrimaryActorTick.bCanEverTick = true;

	CoreMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoreMesh"));
	SetRootComponent(CoreMesh);
	CoreMesh->SetStaticMesh(HPWHCylinderMesh());
	CoreMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CoreMesh->SetRelativeScale3D(FVector(0.42f, 0.42f, 1.15f));

	CoreLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("CoreLight"));
	CoreLight->SetupAttachment(CoreMesh);
	CoreLight->SetIntensity(450.f);
	CoreLight->SetAttenuationRadius(900.f);
	CoreLight->SetLightColor(FLinearColor(1.f, 0.9f, 0.62f));
}

void AHPWH_CoreItem::InitializeCore(AHPWH_WorldDirector* InDirector, const FIntPoint& InCell)
{
	Director = InDirector;
	Cell = InCell;
}

void AHPWH_CoreItem::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AddActorLocalRotation(FRotator(0.f, 90.f * DeltaSeconds, 0.f));
	CoreMesh->SetRelativeLocation(FVector(0.f, 0.f, FMath::Sin(GetWorld()->GetTimeSeconds() * 2.2f) * 14.f));
}

void AHPWH_CoreItem::HPWHInteract(APawn* Interactor)
{
	auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM)
	{
		return;
	}
	if (GM->GetSoulState() != EHPWH_SoulState::Embodied)
	{
		GM->PushMessage(TEXT("灵魂无法触碰灯芯"), 2.f);
		return;
	}
	GM->CollectCore();
	GM->PlaySfx2D(EHPWH_Sfx::PuzzleSolve, 0.55f, 1.1f);
	if (Director)
	{
		Director->NotifyCoreCollected(Cell);
	}
	Destroy();
}

bool AHPWH_CoreItem::CanHPWHInteract(APawn* Interactor) const
{
	const auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	return !GM || GM->GetSoulState() == EHPWH_SoulState::Embodied;
}

AHPWH_ExitGate::AHPWH_ExitGate()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	DoorGlow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorGlow"));
	DoorGlow->SetupAttachment(SceneRoot);
	DoorGlow->SetStaticMesh(HPWHCubeMesh());
	DoorGlow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DoorGlow->SetRelativeLocation(FVector(0.f, 0.f, 170.f));
	DoorGlow->SetRelativeScale3D(FVector(0.12f, 2.2f, 3.2f));

	DoorLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("DoorLight"));
	DoorLight->SetupAttachment(SceneRoot);
	DoorLight->SetRelativeLocation(FVector(0.f, 0.f, 180.f));
	DoorLight->SetIntensity(800.f);
	DoorLight->SetAttenuationRadius(1000.f);
	DoorLight->SetLightColor(FLinearColor(0.55f, 0.95f, 0.7f));
}

void AHPWH_ExitGate::InitializePOI(AHPWH_WorldDirector* InDirector, const FIntPoint& InCell)
{
	Director = InDirector;
	const FLinearColor Frame(0.16f, 0.15f, 0.13f);
	AddBox(this, SceneRoot, HPWHCubeMesh(), FVector(0.f, 130.f, 180.f), FVector(50.f, 50.f, 360.f), Frame, true, BuiltMeshes);
	AddBox(this, SceneRoot, HPWHCubeMesh(), FVector(0.f, -130.f, 180.f), FVector(50.f, 50.f, 360.f), Frame, true, BuiltMeshes);
	AddBox(this, SceneRoot, HPWHCubeMesh(), FVector(0.f, 0.f, 370.f), FVector(50.f, 300.f, 50.f), Frame, true, BuiltMeshes);

	if (UMaterialInstanceDynamic* Material = HPWHColoredMaterial(this, FLinearColor(0.35f, 0.95f, 0.65f)))
	{
		DoorGlow->SetMaterial(0, Material);
	}
}

FString AHPWH_ExitGate::GetHPWHInteractPrompt(APawn* Interactor) const
{
	auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	return GM && GM->IsCoreCollected() ? TEXT("离开梦境") : TEXT("出口封印中");
}

bool AHPWH_ExitGate::CanHPWHInteract(APawn* Interactor) const
{
	const auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	return !GM || GM->GetSoulState() == EHPWH_SoulState::Embodied;
}

void AHPWH_ExitGate::HPWHInteract(APawn* Interactor)
{
	if (auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->TryExit(Interactor);
	}
}
