#pragma once
#include "CoreMinimal.h"

/** Rasterize a mouse stroke as a bounded four-connected road, without holes. */
class SPIRITWFC_API FWFCPaintRaster
{
  public:
	static bool Stroke(FIntPoint A, FIntPoint B, int32 Width, int32 Depth, TArray<FIntPoint>& Cells);
};
