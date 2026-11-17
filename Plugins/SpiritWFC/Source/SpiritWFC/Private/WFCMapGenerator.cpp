#include "WFCMapGenerator.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "Materials/MaterialInterface.h"
#include "WFCSolver.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"

AWFCMapGenerator::AWFCMapGenerator()
{
 bReplicates=true;bAlwaysRelevant=true;
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("WFCRoot")));
	GetRootComponent()->SetMobility(EComponentMobility::Static);
}
void AWFCMapGenerator::BeginPlay()
{
	Super::BeginPlay();
	if (bGenerateOnBeginPlay)
		GenerateMap();
}
bool AWFCMapGenerator::GenerateMap()
{
	LastResult = FWFCSolver::Solve(TileSet, Settings);
	if (LastResult.Status != EWFCResultStatus::Success)
	{
		OnGenerationFinished.Broadcast(LastResult);
		return false;
	}
	TMap<FString, UHierarchicalInstancedStaticMeshComponent*> Groups;
	TMap<int32, const FWFCPaintedCell*> Replacements;
	if (Settings.LayoutPreset == EWFCLayoutPreset::Painted)
		for (const auto& Painted : Settings.PaintedCells)
			if (!Painted.BuildingMesh.IsNull())
				Replacements.Add(Painted.Coordinate.X + Settings.Width * Painted.Coordinate.Y, &Painted);
	auto Replacement = [&](const FWFCCellPlacement& Cell) -> const FWFCPaintedCell*
	{
		return Cell.Coordinate.Z == 0 ? Replacements.FindRef(Cell.Coordinate.X + Settings.Width * Cell.Coordinate.Y)
		                              : nullptr;
	};
	auto GroupKey = [&](const FWFCCellPlacement& Cell)
	{
		const auto* Custom = Replacement(Cell);
		return Custom ? TEXT("Override:") + Custom->BuildingMesh.ToSoftObjectPath().ToString()
		              : FString::Printf(TEXT("Tile:%d"), Cell.TileIndex);
	};
	TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> Staged;
	auto Fail = [&](const FString& Message)
	{
		for (auto Component : Staged)
			if (IsValid(Component))
			{
				RemoveInstanceComponent(Component);
				Component->DestroyComponent();
			}
		LastResult.Status = EWFCResultStatus::InvalidInput;
		LastResult.Message = Message;
		LastResult.Cells.Reset();
		OnGenerationFinished.Broadcast(LastResult);
		return false;
	};
	// Preload each used prototype exactly once, outside the pure solver. Until all
	// resources/instances are ready, old visible components remain intact.
	for (const auto& Cell : LastResult.Cells)
	{
		if (!TileSet->Tiles[Cell.TileIndex].bRenderMesh)
			continue;
		const FString Key = GroupKey(Cell);
		if (Groups.Contains(Key))
			continue;
		const auto& Tile = TileSet->Tiles[Cell.TileIndex];
		const auto* Custom = Replacement(Cell);
		UStaticMesh* Mesh = Custom ? Custom->BuildingMesh.LoadSynchronous() : Tile.Mesh.LoadSynchronous();
		if (!Mesh)
			return Fail(FString::Printf(TEXT("Tile %s has no loadable static mesh."), *Tile.TileId.ToString()));
		TArray<UMaterialInterface*> Materials;
		if (!Custom)
			for (const auto& Reference : Tile.Materials)
			{
				auto* Material = Reference.LoadSynchronous();
				if (!Material)
					return Fail(
					    FString::Printf(TEXT("Tile %s has a missing material override."), *Tile.TileId.ToString()));
				Materials.Add(Material);
			}
		auto* Component = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, NAME_None, RF_Transactional);
		AddInstanceComponent(Component);
		Staged.Add(Component);
		Component->SetupAttachment(GetRootComponent());
		Component->SetMobility(EComponentMobility::Static);
		Component->SetStaticMesh(Mesh);
        // ISM component bodies must inherit the mesh-authored stair slope explicitly.
        if (const auto* Body=Mesh->GetBodySetup()) Component->SetWalkableSlopeOverride(Body->WalkableSlopeOverride);
        Component->CanCharacterStepUpOn=ECB_Yes;
		Component->SetCollisionEnabled(bEnableCollision ? ECollisionEnabled::QueryAndPhysics
		                                                : ECollisionEnabled::NoCollision);
		Component->SetCollisionProfileName(bEnableCollision ? TEXT("BlockAll") : TEXT("NoCollision"));
		Component->SetCanEverAffectNavigation(bAffectNavigation && bEnableCollision);
		for (int32 I = 0; I < Materials.Num(); ++I)
			Component->SetMaterial(I, Materials[I]);
		Component->bAutoRebuildTreeOnInstanceChanges = false;
		Component->SetVisibility(false);
		Component->RegisterComponent();
		Groups.Add(Key, Component);
	}
	for (const auto& Cell : LastResult.Cells)
	{
		if (!TileSet->Tiles[Cell.TileIndex].bRenderMesh)
			continue;
		const FTransform Placement(FRotator(0, 90.f * Cell.QuarterTurns, 0),
		                           FVector(Cell.Coordinate) * TileSet->CellSize);
		const auto* Custom = Replacement(Cell);
		const FTransform& Local = Custom ? Custom->BuildingTransform : TileSet->Tiles[Cell.TileIndex].MeshTransform;
		if (Groups[GroupKey(Cell)]->AddInstance(Local * Placement) == INDEX_NONE)
			return Fail(TEXT("Unable to allocate a map instance."));
	}
	// Finish all runtime instance clusters before swapping visibility.
	// Per-instance async rebuilds can leave the new scene only partly rendered.
	for (auto Component : Staged)
	{
		Component->BuildTreeIfOutdated(false, true);
		Component->bAutoRebuildTreeOnInstanceChanges = true;
	}
	ClearGeneratedMap();
	GeneratedComponents = MoveTemp(Staged);
	for (auto Component : GeneratedComponents)
		Component->SetVisibility(true);
	DisplayedResult = LastResult;
	DisplayedTileSet = TileSet;
	DisplayedSettings = Settings;
	OnGenerationFinished.Broadcast(LastResult);
	return true;
}
void AWFCMapGenerator::ClearGeneratedMap()
{
#if WITH_EDITOR
	Modify();
#endif
	for (auto Component : GeneratedComponents)
		if (IsValid(Component))
		{
			RemoveInstanceComponent(Component);
			Component->DestroyComponent();
		}
	GeneratedComponents.Reset();
	DisplayedResult = FWFCGenerationResult();
	DisplayedTileSet = nullptr;
	DisplayedSettings = FWFCGenerationSettings();
}
int32 AWFCMapGenerator::GetGeneratedInstanceCount() const
{
	int32 Count = 0;
	for (auto Component : GeneratedComponents)
		if (IsValid(Component))
			Count += Component->GetInstanceCount();
	return Count;
}

namespace {
uint32 SceneLayoutHash(const FWFCGenerationResult& Result){uint32 Hash=0;for(const auto& C:Result.Cells){Hash=HashCombineFast(Hash,GetTypeHash(C.TileId));Hash=HashCombineFast(Hash,GetTypeHash(C.Coordinate));Hash=HashCombineFast(Hash,GetTypeHash(C.QuarterTurns));}return Hash;}
}
void AWFCMapGenerator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AWFCMapGenerator,RuntimeSeed);DOREPLIFETIME(AWFCMapGenerator,RuntimeLayout);DOREPLIFETIME(AWFCMapGenerator,RuntimeRevision);}
bool AWFCMapGenerator::CanRegenerateRandomScene()const{return TileSet&&GetWorld()&&GetWorld()->GetTimeSeconds()-LastRuntimeGeneration>=.5f;}
bool AWFCMapGenerator::RegenerateRandomScene(){return RegenerateScenePreset(EWFCLayoutPreset::Settlement);}
bool AWFCMapGenerator::RegenerateScenePreset(EWFCLayoutPreset Preset){
 if(!HasAuthority()||!CanRegenerateRandomScene()||(Preset!=EWFCLayoutPreset::Settlement&&Preset!=EWFCLayoutPreset::Maze))return false;
 const auto Previous=Settings;const uint32 Before=SceneLayoutHash(DisplayedResult);
 for(int32 Attempt=0;Attempt<8;++Attempt){
  Settings=Previous;Settings.LayoutPreset=Preset;Settings.PaintedCells.Reset();
  Settings.Seed=FMath::RandRange(1,MAX_int32-1);if(Settings.Seed==Previous.Seed)++Settings.Seed;
  if(GenerateMap()&&SceneLayoutHash(DisplayedResult)!=Before){RuntimeSeed=Settings.Seed;RuntimeLayout=Preset;RuntimeRevision=RuntimeRevision==MAX_int32?1:RuntimeRevision+1;LastRuntimeGeneration=GetWorld()->GetTimeSeconds();ForceNetUpdate();return true;}
 }
 Settings=Previous;return false;
}
void AWFCMapGenerator::OnRep_RuntimeGeneration(){
 if(RuntimeRevision<=0)return;Settings.LayoutPreset=RuntimeLayout;Settings.PaintedCells.Reset();Settings.Seed=RuntimeSeed;GenerateMap();LastRuntimeGeneration=GetWorld()->GetTimeSeconds();
}
