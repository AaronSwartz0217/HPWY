#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WFCSolver.h"
#include "WFCBlueprintLibrary.generated.h"

UCLASS()
class SPIRITWFC_API UWFCBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
  public:
	UFUNCTION(BlueprintCallable, Category = "WFC")
	static TArray<FIntPoint> RasterizeRoadStroke(FIntPoint Start, FIntPoint End, int32 Width, int32 Depth);
	/** Data-only zoning preview; an empty error indicates a valid plan (Free has no restrictions). */
	UFUNCTION(BlueprintCallable, Category = "WFC")
	static TArray<FWFCCellRule> PlanWFCLayout(const FWFCGenerationSettings& Settings, FString& Error);
	/** Computes data only; does not load meshes, spawn actors, or change a level. */
	UFUNCTION(BlueprintCallable, Category = "WFC")
	static FWFCGenerationResult SolveWFC(const UWFCTileSet* TileSet, const FWFCGenerationSettings& Settings);

	/** Re-checks a saved/edited solution instead of trusting the solver status. */
	UFUNCTION(BlueprintCallable, Category = "WFC")
	static bool ValidateWFC(const UWFCTileSet* TileSet, const FWFCGenerationSettings& Settings,
	                        const TArray<FWFCCellPlacement>& Cells, FString& Error, int32& WalkableComponents);
};
