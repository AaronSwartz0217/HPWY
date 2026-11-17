#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WFCTypes.h"
#include "WFCTileSet.generated.h"

class UStaticMesh;
class UMaterialInterface;

USTRUCT(BlueprintType)
struct SPIRITWFC_API FWFCTileDefinition
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName TileId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bEnabled = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.001")) float Weight = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FWFCSockets Sockets;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<int32> AllowedYawQuarterTurns = { 0 };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "2")) int32 MinLayer = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "2")) int32 MaxLayer = 2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bBottomLayerOnly = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bTopLayerOnly = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bWalkable = true;
	/** Semantic roles used by optional layout presets, e.g. Ground / Road_05 / Base / Air. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSet<FName> Tags;
	/** Air/void participates in solving without allocating a mesh component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bRenderMesh = true;
	// Optional visuals. Meshes do not participate in solving, allowing headless tests.
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<UStaticMesh> Mesh;
	// Cell-local mesh transform, then yaw rotation, then generator actor transform.
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FTransform MeshTransform = FTransform::Identity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<TSoftObjectPtr<UMaterialInterface>> Materials;
};

UCLASS(BlueprintType)
class SPIRITWFC_API UWFCTileSet : public UDataAsset
{
	GENERATED_BODY()
  public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector CellSize = FVector(400, 400, 300);
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FWFCTileDefinition> Tiles;
	// Only these socket names permit graph traversal. "Floor" can support a room
	// above it without claiming a staircase exists between the two floors.
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TSet<FName> TraversableSockets = { TEXT("Door"), TEXT("Stair") };
	UFUNCTION(BlueprintPure, Category = "WFC") bool Validate(FString& Error) const;
};
