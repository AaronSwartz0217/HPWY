#pragma once
#include "CoreMinimal.h"
#include "WFCTypes.generated.h"

class UStaticMesh;

UENUM(BlueprintType)
enum class EWFCMapMode : uint8
{
	Planar,
	Volume
};

UENUM(BlueprintType)
enum class EWFCLayoutPreset : uint8
{
	Free,
	Settlement,
	Painted,
	Maze
};

UENUM(BlueprintType)
enum class EWFCPaintKind : uint8
{
	Unspecified,
	Road,
	Plaza,
	Building,
	Ground
};

USTRUCT(BlueprintType)
struct SPIRITWFC_API FWFCPaintedCell
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FIntPoint Coordinate = FIntPoint::ZeroValue;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EWFCPaintKind Kind = EWFCPaintKind::Road;
	/** 0 chooses a seeded height; 1..3 fixes the building's story count. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "3")) int32 Stories = 0;
	/** Complete prefab replacement. Uses one logical story; layered kits use TileSet styles. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<UStaticMesh> BuildingMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FTransform BuildingTransform = FTransform::Identity;
};

USTRUCT(BlueprintType)
struct SPIRITWFC_API FWFCCellRule
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FIntVector Coordinate = FIntVector::ZeroValue;
	UPROPERTY(BlueprintReadOnly) FName RequiredTag;
	UPROPERTY(BlueprintReadOnly) int32 QuarterTurns = -1;
};

UENUM(BlueprintType)
enum class EWFCResultStatus : uint8
{
	Success,
	InvalidInput,
	Contradiction,
	BudgetExceeded,
	Disconnected
};

// Socket order is fixed: +X, -X, +Y, -Y, +Z, -Z. Empty names never match.
USTRUCT(BlueprintType)
struct SPIRITWFC_API FWFCSockets
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName PositiveX = TEXT("Closed");
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName NegativeX = TEXT("Closed");
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName PositiveY = TEXT("Closed");
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName NegativeY = TEXT("Closed");
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Up = TEXT("Sky");
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Down = TEXT("Ground");
	FName Get(int32 Direction) const;
	FWFCSockets Rotated(int32 QuarterTurns) const;
};

USTRUCT(BlueprintType)
struct SPIRITWFC_API FWFCPinnedCell
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FIntVector Coordinate = FIntVector::ZeroValue;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName TileId;
	// -1 permits every configured rotation of TileId.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "-1", ClampMax = "3")) int32 QuarterTurns = -1;
};

USTRUCT(BlueprintType)
struct SPIRITWFC_API FWFCGenerationSettings
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EWFCMapMode Mode = EWFCMapMode::Planar;
	/** Optional macro layout. Free preserves the original unconstrained solver. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EWFCLayoutPreset LayoutPreset = EWFCLayoutPreset::Free;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "1")) float BuildingDensity = 0.8f;
	/** Automatic buildings respect this cap; explicitly painted heights remain authoritative. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1",ClampMax="3")) int32 MaxBuildingStories = 3;
	/** Settlement-only adjoining main/wing pairs. Needs CompoundHouse/Base/Wing tile roles. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0",ClampMax="1")) float CompoundBuildingChance = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bFillUnpaintedBuildings = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FWFCPaintedCell> PaintedCells;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1", ClampMax = "64")) int32 Width = 12;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1", ClampMax = "64")) int32 Depth = 12;
	// Enforced in C++, not just editor metadata. Planar always uses one layer.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
	          meta = (ClampMin = "1", ClampMax = "3", EditCondition = "Mode == EWFCMapMode::Volume"))
	int32 Layers = 3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Seed = 1337;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "32")) int32 MaxRestarts = 8;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1", ClampMax = "20000000"))
	int32 MaxPropagationSteps = 2000000;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bConstrainHorizontalBoundary = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName HorizontalBoundarySocket = TEXT("Closed");
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bConstrainVerticalBoundary = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName BottomBoundarySocket = TEXT("Ground");
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName TopBoundarySocket = TEXT("Sky");
	// A graph-level postcondition, separate from local socket compatibility.
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bRequireConnectedWalkableCells = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FWFCPinnedCell> PinnedCells;
	FIntVector Dimensions() const
	{
		return FIntVector(Width, Depth, Mode == EWFCMapMode::Planar ? 1 : Layers);
	}
};

USTRUCT(BlueprintType)
struct SPIRITWFC_API FWFCCellPlacement
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FIntVector Coordinate = FIntVector::ZeroValue;
	UPROPERTY(BlueprintReadOnly) FName TileId;
	UPROPERTY(BlueprintReadOnly) int32 TileIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) int32 QuarterTurns = 0;
};

USTRUCT(BlueprintType)
struct SPIRITWFC_API FWFCGenerationResult
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) EWFCResultStatus Status = EWFCResultStatus::InvalidInput;
	UPROPERTY(BlueprintReadOnly) FString Message;
	UPROPERTY(BlueprintReadOnly) FIntVector Dimensions = FIntVector::ZeroValue;
	UPROPERTY(BlueprintReadOnly) FIntVector FailedCell = FIntVector(-1);
	UPROPERTY(BlueprintReadOnly) int32 RequestedSeed = 0;
	UPROPERTY(BlueprintReadOnly) int32 EffectiveSeed = 0;
	UPROPERTY(BlueprintReadOnly) int32 Attempts = 0;
	UPROPERTY(BlueprintReadOnly) int32 Decisions = 0;
	UPROPERTY(BlueprintReadOnly) int32 PropagationSteps = 0;
	UPROPERTY(BlueprintReadOnly) int32 WalkableComponents = 0;
	UPROPERTY(BlueprintReadOnly) double ElapsedMilliseconds = 0;
	// Only a fully validated solution is published. Failure never exposes a partial map.
	UPROPERTY(BlueprintReadOnly) TArray<FWFCCellPlacement> Cells;
};
