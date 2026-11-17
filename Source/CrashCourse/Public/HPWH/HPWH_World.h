#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HPWH/HPWH_Types.h"
#include "HPWH_World.generated.h"

class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class AHPWH_WorldDirector;
class AHPWH_POIBase;
class AHPWH_Crawler;
class AHPWH_Companion;
class AHPWH_EnemyBase;
class UHPWH_TerrainPrefabSet;

/** 单个格子：地面 + 实例化方块障碍。 */
UCLASS()
class CRASHCOURSE_API AHPWH_CellChunk : public AActor
{
	GENERATED_BODY()

public:
	AHPWH_CellChunk();

	void BuildCell(const FHPWH_CellRecord& Record, float CellSizeCm, UMaterialInterface* GroundMaterial,
		UMaterialInterface* ObstacleMaterial, bool bBuildProceduralObstacles = true,
		const TArray<UMaterialInterface*>* GroundTileMaterials = nullptr);

	/** 地面按格内细分采样，消除格子级色块；碰撞仍由整格地面承担。 */
	static constexpr int32 GroundTileCount = 3;

	UInstancedStaticMeshComponent* GetPropComponent(const TCHAR* MeshPath);

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> SceneRoot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Ground;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> Obstacles;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> GroundTiles;
	/** 导入资产的可视道具实例，按资源路径缓存。 */
	UPROPERTY() TMap<FString, TObjectPtr<UInstancedStaticMeshComponent>> PropComponents;

	UPROPERTY() FIntPoint Coordinate = FIntPoint::ZeroValue;
};

/** 圆形群系种子：位置使用格坐标，半径与边界噪声共同形成 SDF。 */
struct FHPWH_BiomeSeed
{
	FVector2D Position = FVector2D::ZeroVector;
	float Radius = 4.f;
	EHPWH_TerrainType Biome = EHPWH_TerrainType::Empty;
	FVector Params = FVector::ZeroVector;
};

/** 无限梦中案头：确定性 5×5 活动窗口、成片地貌、关键器物与怪物生成。 */
UCLASS()
class CRASHCOURSE_API AHPWH_WorldDirector : public AActor
{
	GENERATED_BODY()

public:
	AHPWH_WorldDirector();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void InitializeForRun(int32 InRunSeed, int32 InLayerIndex);
	void SetLayer(int32 InLayerIndex);
	void ResetForRespawn();

	FIntPoint WorldToCell(const FVector& WorldLocation) const;
	FVector CellCenter(const FIntPoint& Cell) const;
	bool IsCellVisible(const FIntPoint& Cell) const { return LoadedCells.Contains(Cell); }
	bool IsWorldPositionVisible(const FVector& WorldLocation) const
	{
		return IsCellVisible(HPWH_WorldToCell(WorldLocation, CellSizeCm));
	}
	float GetCellSize() const { return CellSizeCm; }
	int32 GetRunSeed() const { return RunSeed; }
	int32 GetLayerSeed() const { return LayerSeed; }
	int32 GetLayerIndex() const { return LayerIndex; }

	const TMap<FIntPoint, FHPWH_CellRecord>& GetRecords() const { return Records; }
	const TSet<FIntPoint>& GetPuzzleAnchors() const { return PuzzleAnchors; }
	const FHPWH_CellRecord* FindRecord(const FIntPoint& Cell) const;
	FIntPoint GetPOICell(int32 Index) const;
	FIntPoint GetExitCell() const { return POICells.Num() > 3 ? POICells[3] : FIntPoint(4, 0); }
	void RestoreProgress(const TMap<FIntPoint, FHPWH_CellRecord>& InRecords, const TArray<FIntPoint>& InAnchors);

	AHPWH_Crawler* GetCrawler() const { return Crawler; }
	AHPWH_Companion* GetCompanion() const { return Companion; }
	void NotifyEnemyKilled(const FIntPoint& Cell);
	void NotifyPuzzleSolved(const FIntPoint& Cell);
	void NotifyCoreCollected(const FIntPoint& Cell);

	void SpawnRemainingEnemies(const FIntPoint& Cell);

private:
	void RefreshAroundPlayer();
	void SpawnCell(const FIntPoint& Cell);
	void DespawnCell(const FIntPoint& Cell);
	void BuildPOIPlacement();
	void BuildBiomePlacement();
	FHPWH_BiomeSeed MakeLatticeSeed(int32 LX, int32 LY) const;
	FVector2D WarpPoint(const FVector2D& RawPoint) const;
	FVector2D WarpedCellPoint(const FIntPoint& Cell) const;
	FVector BiomeParams(EHPWH_TerrainType Biome) const;
	EHPWH_TerrainType BiomeFromParams(const FVector& Params) const;
	void EvaluateBiome(const FIntPoint& Cell, EHPWH_TerrainType& OutBiome,
		float& OutDensityScale, int32& OutZone, FLinearColor& OutTint) const;
	void EvaluateBiomeAt(const FVector2D& RawPoint, const FIntPoint& Cell, EHPWH_TerrainType& OutBiome,
		float& OutDensityScale, int32& OutZone, FLinearColor& OutTint) const;
	FHPWH_CellRecord GenerateRecord(const FIntPoint& Cell) const;
	FIntPoint PickPOICell(int32 Index, int32 MinRing, int32 MaxRing) const;
	bool IsNearMirrorClearing(const FIntPoint& Cell) const;
	bool IsReservedPOICell(const FIntPoint& Cell) const;
	bool IsPuzzleAnchorCell(const FIntPoint& Cell) const { return PuzzleAnchors.Contains(Cell); }
	bool CanPlacePuzzleAnchor(const FIntPoint& Cell, const TSet<FIntPoint>& Desired) const;
	void EnsurePuzzleAnchor(const TSet<FIntPoint>& Desired);
	EHPWH_TerrainType PuzzleTerrainForCell(const FIntPoint& Cell) const;
	void SpawnCompanion();

	FLinearColor TerrainColor(EHPWH_TerrainType Terrain) const;
	UMaterialInterface* GetTerrainMaterial(EHPWH_TerrainType Terrain);
	UMaterialInterface* GetBlendedTerrainMaterial(const FLinearColor& Tint);
	UMaterialInterface* GetObstacleMaterial(EHPWH_TerrainType Terrain);

	void SpawnEncounterMonsters(const FHPWH_CellRecord& Record);

	UPROPERTY(EditAnywhere, Category = "HPWH|Grid") float CellSizeCm = 800.f;
	UPROPERTY(EditAnywhere, Category = "HPWH|Grid") int32 ActiveRadius = 2;
	UPROPERTY(EditAnywhere, Category = "HPWH|Grid") float OriginSafeRadiusCm = 2500.f;
	UPROPERTY(EditAnywhere, Category = "HPWH|Grid") int32 SpecialPuzzleLineInterval = 30;
	UPROPERTY(EditAnywhere, Category = "HPWH|Spawn") float MinMonsterSpawnDistanceCm = 900.f;
	UPROPERTY(EditAnywhere, Category = "HPWH|Crawler") FVector CrawlerAnchor = FVector(5000.f, 5000.f, 300.f);

	/** 地形分类预制体集合；为空时自动尝试默认路径。 */
	UPROPERTY(EditAnywhere, Category = "HPWH|Prefab", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHPWH_TerrainPrefabSet> TerrainPrefabs;

	UPROPERTY() int32 RunSeed = 20261117;
	UPROPERTY() int32 LayerSeed = 20261117;
	UPROPERTY() int32 LayerIndex = 1;

	UPROPERTY() TMap<FIntPoint, FHPWH_CellRecord> Records;
	UPROPERTY() TMap<FIntPoint, TObjectPtr<AHPWH_CellChunk>> LoadedCells;
	UPROPERTY() TMap<FIntPoint, TObjectPtr<AActor>> POIActors;
	TMap<FIntPoint, TArray<TWeakObjectPtr<AActor>>> CellActors;
	UPROPERTY() TArray<FIntPoint> POICells;
	TSet<FIntPoint> PuzzleAnchors;
	TArray<FHPWH_BiomeSeed> ForcedBiomeSeeds;
	static constexpr float BiomeSeedSpacing = 6.f;
	UPROPERTY() TObjectPtr<AHPWH_Crawler> Crawler;
	UPROPERTY() TObjectPtr<AHPWH_Companion> Companion;

	UPROPERTY() TObjectPtr<UMaterialInterface> BaseMaterial;
	UPROPERTY() TObjectPtr<UMaterialInterface> FloorMaterial;
	UPROPERTY() TObjectPtr<UStaticMesh> CubeMesh;
	UPROPERTY() TMap<uint8, TObjectPtr<UMaterialInstanceDynamic>> TerrainMaterials;
	UPROPERTY() TMap<uint32, TObjectPtr<UMaterialInstanceDynamic>> BlendedTerrainMaterials;
	UPROPERTY() TMap<uint8, TObjectPtr<UMaterialInstanceDynamic>> ObstacleMaterials;

	FIntPoint CenterCell = FIntPoint(999999, 999999);
	float UpdateAccumulator = 0.f;
	float RuntimeElapsed = 0.f;
	bool bInitialized = false;
	bool bLoggedRuntime = false;
};
