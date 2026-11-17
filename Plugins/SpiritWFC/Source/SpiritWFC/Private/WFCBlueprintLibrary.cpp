#include "WFCBlueprintLibrary.h"
#include "WFCLayoutPlanner.h"
#include "WFCPaintRaster.h"
TArray<FIntPoint> UWFCBlueprintLibrary::RasterizeRoadStroke(FIntPoint Start, FIntPoint End, int32 Width, int32 Depth)
{
	TArray<FIntPoint> Cells;
	FWFCPaintRaster::Stroke(Start, End, Width, Depth, Cells);
	return Cells;
}
TArray<FWFCCellRule> UWFCBlueprintLibrary::PlanWFCLayout(const FWFCGenerationSettings& Settings, FString& Error)
{
	TArray<FWFCCellRule> Rules;
	FWFCLayoutPlanner::Build(Settings, Rules, Error);
	return Rules;
}
FWFCGenerationResult UWFCBlueprintLibrary::SolveWFC(const UWFCTileSet* TileSet, const FWFCGenerationSettings& Settings)
{
	return FWFCSolver::Solve(TileSet, Settings);
}
bool UWFCBlueprintLibrary::ValidateWFC(const UWFCTileSet* TileSet, const FWFCGenerationSettings& Settings,
                                       const TArray<FWFCCellPlacement>& Cells, FString& Error,
                                       int32& WalkableComponents)
{
	return FWFCSolver::ValidateSolution(TileSet, Settings, Cells, Error, WalkableComponents);
}
