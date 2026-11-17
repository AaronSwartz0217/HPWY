#pragma once
#include "WFCTileSet.h"
#include "WFCLayoutAsset.generated.h"

/** Reusable editable plan, separate from the generated level geometry. */
UCLASS(BlueprintType)
class SPIRITWFC_API UWFCLayoutAsset : public UDataAsset
{
	GENERATED_BODY()
  public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC") FWFCGenerationSettings Settings;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC") TObjectPtr<UWFCTileSet> TileSet;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC") TMap<EWFCPaintKind, FLinearColor> Palette;
};
