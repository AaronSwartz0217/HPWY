#pragma once
#include "CoreMinimal.h"
#include "WFCTileSet.h"

// Stateless pure algorithm entry point: no world, actors, asset loading or GAS.
// Adjacency/socket rotations are compiled once per solve. Max 256 variants,
// 4096 cells and three layers are enforced to bound synchronous work/memory.
class SPIRITWFC_API FWFCSolver
{
  public:
	static FWFCGenerationResult Solve(const UWFCTileSet* TileSet, const FWFCGenerationSettings& Settings);
	static bool ValidateSolution(const UWFCTileSet* TileSet, const FWFCGenerationSettings& Settings,
	                             const TArray<FWFCCellPlacement>& Cells, FString& Error, int32& WalkableComponents);
};
