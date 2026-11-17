#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "HPWH/HPWH_Types.h"
#include "HPWH_Prefabs.generated.h"

class AHPWH_WorldDirector;

/**
 * 分类区块预制体基类。
 * 约定：蓝图子类的 pivot 位于格子几何中心、地面 z=0；X/Y 为世界轴；内容应控制在 CellSize 内。
 * Mirror/Courtyard/出口格不会使用普通地形预制体，避免覆盖关键玩法。
 */
UCLASS(Blueprintable)
class CRASHCOURSE_API AHPWH_CellPrefabBase : public AActor
{
	GENERATED_BODY()

public:
	AHPWH_CellPrefabBase();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HPWH|Prefab")
	EHPWH_TerrainType TerrainType = EHPWH_TerrainType::Empty;

	UPROPERTY(BlueprintReadOnly, Category = "HPWH|Prefab")
	FIntPoint CellCoordinate = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Category = "HPWH|Prefab")
	float CellSizeCm = 800.f;

	/** 由无限地图在生成/重载格子时调用，蓝图可在这里做朝向、变体或随机装饰。 */
	UFUNCTION(BlueprintCallable, Category = "HPWH|Prefab")
	void PlaceInCell(const FIntPoint& InCell, float InCellSizeCm);

	UFUNCTION(BlueprintImplementableEvent, Category = "HPWH|Prefab")
	void OnPlacedInCell();
};

USTRUCT(BlueprintType)
struct FHPWH_TerrainPrefabEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HPWH|Prefab")
	EHPWH_TerrainType Terrain = EHPWH_TerrainType::Paper;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HPWH|Prefab")
	TSoftClassPtr<AHPWH_CellPrefabBase> Prefab;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HPWH|Prefab", meta = (ClampMin = "0.01"))
	float Weight = 1.f;
};

/**
 * 地形分类 → 预制体集合。
 * 同一分类可以配置多个变体，按格子种子确定性抽取，避免回头时变样。
 * 默认加载路径：/Game/HPWH/Prefabs/DA_HPWH_TerrainPrefabs
 */
UCLASS(BlueprintType)
class CRASHCOURSE_API UHPWH_TerrainPrefabSet : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HPWH|Prefab")
	TArray<FHPWH_TerrainPrefabEntry> Entries;

	/** 有可用预制体时，是否用它替换程序化生成的方块障碍。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HPWH|Prefab")
	bool bReplaceProceduralObstacles = true;

	UFUNCTION(BlueprintCallable, Category = "HPWH|Prefab")
	TSubclassOf<AHPWH_CellPrefabBase> PickPrefab(EHPWH_TerrainType Terrain, int32 VariantSeed) const;
};

/** 给蓝图/预制体使用的格子坐标工具。 */
UCLASS()
class CRASHCOURSE_API UHPWH_GridLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "HPWH|Grid", meta = (WorldContext = "WorldContextObject"))
	static float GetCellSize(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "HPWH|Grid", meta = (WorldContext = "WorldContextObject"))
	static FIntPoint WorldToCell(const UObject* WorldContextObject, FVector WorldLocation);

	UFUNCTION(BlueprintPure, Category = "HPWH|Grid", meta = (WorldContext = "WorldContextObject"))
	static FVector CellCenter(const UObject* WorldContextObject, const FIntPoint& Cell);
};
