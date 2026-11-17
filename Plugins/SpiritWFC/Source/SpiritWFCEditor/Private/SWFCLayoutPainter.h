#pragma once
#include "Widgets/SCompoundWidget.h"
#include "WFCTypes.h"
#include "WFCTileSet.h"
#include "WFCLayoutAsset.h"
#include "UObject/StrongObjectPtr.h"

class AWFCMapGenerator;
class UWFCTileSet;
struct FWFCBrushState
{
	TWeakObjectPtr<AWFCMapGenerator> Target;
	TStrongObjectPtr<UWFCTileSet> TileSet;
	TStrongObjectPtr<UWFCLayoutAsset> Plan;
	TStrongObjectPtr<UStaticMesh> HouseMesh;
	FWFCGenerationSettings Config;
	float HouseScale = 1, HouseYaw = 0, HouseZ = 0;
	bool bRectangle = false, bSelect = false;
	int32 Width = 14, Depth = 14, Stories = 1;
	float Density = .8f;
	bool bFill = true;
	EWFCPaintKind Brush = EWFCPaintKind::Road;
	TMap<int32, FWFCPaintedCell> Cells;
	TArray<TMap<int32, FWFCPaintedCell>> Undo;
	TArray<TMap<int32, FWFCPaintedCell>> Redo;
	TMap<EWFCPaintKind, FLinearColor> Colors;
	TMap<int32, EWFCPaintKind> Preview;
	TSet<int32> Errors;
	FString Message = TEXT("选择关卡中的 WFC 生成器，然后点击读取。左键画，右键擦除；应用后保存关卡。");
	void Remember()
	{
		if (Undo.Num() >= 32)
			Undo.RemoveAt(0);
		Undo.Add(Cells);
		Redo.Reset();
		Errors.Reset();
	}
	void Paint(FIntPoint Coordinate, bool Erase);
	void Stroke(FIntPoint A, FIntPoint B, bool Erase);
	void FillRect(FIntPoint A, FIntPoint B, bool Erase);
	void Inspect(FIntPoint Coordinate);
};

TSharedRef<SWidget> CreateWFCPaintCanvas(TSharedPtr<FWFCBrushState> State);

class SWFCLayoutPainter : public SCompoundWidget
{
  public:
	SLATE_BEGIN_ARGS(SWFCLayoutPainter)
	{
	}
	SLATE_END_ARGS()
	void Construct(const FArguments& Args);

  private:
	TSharedPtr<FWFCBrushState> State;
	FReply LoadSelection();
	FReply LoadGenerated();
	FReply Apply();
	FReply GenerateAutomatic();
	FReply SavePlan();
	FReply LoadPlan();
	FWFCGenerationSettings GatherSettings() const;
	void UpdatePreview();
	void FitHouse();
	TSharedRef<SWidget> BrushButton(const FString& Label, EWFCPaintKind Kind);
};
