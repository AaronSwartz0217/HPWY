#pragma once
#include "WFCTypes.h"

/** Deterministic macro zoning; WFC still selects/propagates matching variants. */
class SPIRITWFC_API FWFCLayoutPlanner
{
  public:
	static bool Build(const FWFCGenerationSettings& Settings, TArray<FWFCCellRule>& Rules, FString& Error);
};
