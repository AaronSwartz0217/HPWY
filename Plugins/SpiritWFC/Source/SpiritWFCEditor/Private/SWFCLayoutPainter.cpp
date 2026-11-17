#include "SWFCLayoutPainter.h"
#include "WFCMapGenerator.h"
#include "WFCLayoutPlanner.h"
#include "WFCPaintRaster.h"
#include "Editor.h"
#include "Engine/Selection.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "AssetToolsModule.h"
#include "Factories/DataAssetFactory.h"
#include "FileHelpers.h"
#include "InputCoreTypes.h"
#include "PropertyCustomizationHelpers.h"
#include "Rendering/DrawElements.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Widgets/Colors/SColorPicker.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/Text/STextBlock.h"

void FWFCBrushState::Paint(FIntPoint P, bool Erase)
{
	if (P.X < 0 || P.Y < 0 || P.X >= Width || P.Y >= Depth)
		return;
	const int32 I = P.X + Width * P.Y;
	if (Erase || Brush == EWFCPaintKind::Unspecified)
		Cells.Remove(I);
	else
	{
		FWFCPaintedCell C;
		C.Coordinate = P;
		C.Kind = Brush;
		C.Stories = Brush == EWFCPaintKind::Building ? Stories : 0;
		if (Brush == EWFCPaintKind::Building && HouseMesh.IsValid())
		{
			C.BuildingMesh = HouseMesh.Get();
			C.Stories = 1;
			const auto Bounds = HouseMesh->GetBoundingBox();
			const auto Center = Bounds.GetCenter();
			const FQuat Rotation = FRotator(0, HouseYaw, 0).Quaternion();
			const FVector Offset = -Rotation.RotateVector(FVector(Center.X, Center.Y, 0) * HouseScale) +
			                       FVector(0, 0, -Bounds.Min.Z * HouseScale + HouseZ);
			C.BuildingTransform = FTransform(Rotation, Offset, FVector(HouseScale));
		}
		Cells.Add(I, C);
	}
	Errors.Reset();
	Message = TEXT("画布已修改。点击计算后保存关卡，或保存为规划资产。");
}
void FWFCBrushState::Stroke(FIntPoint A, FIntPoint B, bool Erase)
{
	TArray<FIntPoint> Points;
	if (FWFCPaintRaster::Stroke(A, B, Width, Depth, Points))
		for (const auto& P : Points)
			Paint(P, Erase);
}
void FWFCBrushState::FillRect(FIntPoint A, FIntPoint B, bool Erase)
{
	for (int32 Y = FMath::Max(0, FMath::Min(A.Y, B.Y)); Y <= FMath::Min(Depth - 1, FMath::Max(A.Y, B.Y)); ++Y)
		for (int32 X = FMath::Max(0, FMath::Min(A.X, B.X)); X <= FMath::Min(Width - 1, FMath::Max(A.X, B.X)); ++X)
			Paint(FIntPoint(X, Y), Erase);
}
void FWFCBrushState::Inspect(FIntPoint P)
{
	if (const auto* C = Cells.Find(P.X + Width * P.Y))
	{
		Brush = C->Kind;
		Stories = C->Stories;
		HouseMesh.Reset(C->BuildingMesh.LoadSynchronous());
		HouseScale = C->BuildingTransform.GetScale3D().X;
		HouseYaw = C->BuildingTransform.Rotator().Yaw;
		HouseZ = HouseMesh.IsValid()
		             ? C->BuildingTransform.GetTranslation().Z + HouseMesh->GetBoundingBox().Min.Z * HouseScale
		             : 0;
	}
	else
	{
		Brush = EWFCPaintKind::Unspecified;
		HouseMesh.Reset();
	}
	Message = FString::Printf(TEXT("已读取格子 (%d,%d) 的笔刷参数。退出选择模式后可重新填充这个格子。"), P.X, P.Y);
}
namespace
{
FLinearColor DefaultColor(EWFCPaintKind Kind)
{
	switch (Kind)
	{
	case EWFCPaintKind::Road:
		return FLinearColor(.85f, .85f, .80f);
	case EWFCPaintKind::Plaza:
		return FLinearColor(.80f, .62f, .12f);
	case EWFCPaintKind::Building:
		return FLinearColor(.18f, .48f, .25f);
	case EWFCPaintKind::Ground:
		return FLinearColor(.62f, .18f, .17f);
	default:
		return FLinearColor(.18f, .19f, .21f);
	}
}
EWFCPaintKind KindForTile(const FWFCTileDefinition& T)
{
	if (T.Tags.Contains(TEXT("House")) || T.Tags.Contains(TEXT("Base")))
		return EWFCPaintKind::Building;
	for (const auto& RoleTag : T.Tags)
	{
		if (RoleTag.ToString().StartsWith(TEXT("Road_")))
			return EWFCPaintKind::Road;
		if (RoleTag.ToString().StartsWith(TEXT("Plaza_")))
			return EWFCPaintKind::Plaza;
	}
	return EWFCPaintKind::Ground;
}
class SWFCPaintCanvas : public SLeafWidget
{
  public:
	SLATE_BEGIN_ARGS(SWFCPaintCanvas)
	{
	}
	SLATE_ARGUMENT(TSharedPtr<FWFCBrushState>, State) SLATE_END_ARGS() void Construct(const FArguments& Args)
	{
		State = Args._State;
		ForceVolatile(true);
	}
	virtual FVector2D ComputeDesiredSize(float) const override
	{
		return FVector2D(640, 640);
	}
	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out,
	                      int32 Layer, const FWidgetStyle&, bool) const override
	{
		const float Size = CellSize(G);
		const auto Offset = Origin(G, Size);
		for (int32 Y = 0; Y < State->Depth; ++Y)
			for (int32 X = 0; X < State->Width; ++X)
			{
				const int32 I = X + State->Width * Y;
				const auto* C = State->Cells.Find(I);
				const auto* Generated = State->Preview.Find(I);
				const auto Kind = C ? C->Kind : EWFCPaintKind::Unspecified;
				FLinearColor Color = State->Colors.Contains(Kind) ? State->Colors[Kind] : DefaultColor(Kind);
				if (!C && Generated && *Generated != EWFCPaintKind::Ground)
					Color = FMath::Lerp(
					    Color,
					    State->Colors.Contains(*Generated) ? State->Colors[*Generated] : DefaultColor(*Generated), .3f);
				// UE's Top viewport shows -Y to the right and +X down the screen.
				const FVector2D Pos = Offset + FVector2D(State->Depth - 1 - Y, X) * Size;
				const bool Conflict = State->Errors.Contains(I);
				if (Conflict)
					FSlateDrawElement::MakeBox(
					    Out, Layer, G.ToPaintGeometry(FVector2D(Size - 1), FSlateLayoutTransform(Pos)),
					    FAppStyle::GetBrush("WhiteBrush"), ESlateDrawEffect::None, FLinearColor::Red);
				const float Inset = Conflict ? 3 : 1;
				FSlateDrawElement::MakeBox(Out, Layer + 1,
				                           G.ToPaintGeometry(FVector2D(FMath::Max(1.f, Size - Inset * 2)),
				                                             FSlateLayoutTransform(Pos + FVector2D(Inset))),
				                           FAppStyle::GetBrush("WhiteBrush"), ESlateDrawEffect::None, Color);
				if (C && C->Kind == EWFCPaintKind::Building && Size >= 18)
					FSlateDrawElement::MakeText(
					    Out, Layer + 2,
					    G.ToPaintGeometry(FVector2D(Size), FSlateLayoutTransform(Pos + FVector2D(4, 1))),
					    FText::FromString(!C->BuildingMesh.IsNull()
					                          ? TEXT("M")
					                          : (C->Stories ? FString::FromInt(C->Stories) : TEXT("?"))),
					    FAppStyle::GetFontStyle("SmallFont"), ESlateDrawEffect::None, FLinearColor::White);
			}
		return Layer + 2;
	}
	virtual FReply OnMouseButtonDown(const FGeometry& G, const FPointerEvent& E) override
	{
		if (E.GetEffectingButton() != EKeys::LeftMouseButton && E.GetEffectingButton() != EKeys::RightMouseButton)
			return FReply::Unhandled();
		FIntPoint P;
		if (!Pick(G, E, P))
			return FReply::Unhandled();
		if (State->bSelect && E.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			State->Inspect(P);
			return FReply::Handled();
		}
		State->Remember();
		Painting = true;
		Erasing = E.GetEffectingButton() == EKeys::RightMouseButton;
		Last = Start = P;
		if (!State->bRectangle)
			State->Stroke(P, P, Erasing);
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}
	virtual FReply OnMouseMove(const FGeometry& G, const FPointerEvent& E) override
	{
		if (!Painting)
			return FReply::Unhandled();
		FIntPoint P;
		if (Pick(G, E, P))
		{
			if (!State->bRectangle)
				State->Stroke(Last, P, Erasing);
			Last = P;
		}
		return FReply::Handled();
	}
	virtual FReply OnMouseButtonUp(const FGeometry& G, const FPointerEvent& E) override
	{
		if (!Painting)
			return FReply::Unhandled();
		FIntPoint P;
		if (Pick(G, E, P))
			Last = P;
		if (State->bRectangle)
			State->FillRect(Start, Last, Erasing);
		Painting = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	virtual void OnMouseCaptureLost(const FCaptureLostEvent&) override
	{
		Painting = false;
	}

  private:
	TSharedPtr<FWFCBrushState> State;
	bool Painting = false, Erasing = false;
	FIntPoint Start, Last;
	float CellSize(const FGeometry& G) const
	{
		return FMath::Max(1.f, float(FMath::Min(G.GetLocalSize().X / State->Depth, G.GetLocalSize().Y / State->Width)));
	}
	FVector2D Origin(const FGeometry& G, float Size) const
	{
		return (G.GetLocalSize() - FVector2D(State->Depth, State->Width) * Size) * .5;
	}
	bool Pick(const FGeometry& G, const FPointerEvent& E, FIntPoint& P) const
	{
		const float Size = CellSize(G);
		const auto V = G.AbsoluteToLocal(E.GetScreenSpacePosition()) - Origin(G, Size);
		const int32 X = FMath::FloorToInt(V.Y / Size);
		const int32 Y = State->Depth - 1 - FMath::FloorToInt(V.X / Size);
		if (X < 0 || Y < 0 || X >= State->Width || Y >= State->Depth)
			return false;
		P = FIntPoint(X, Y);
		return true;
	}
};
} // namespace
TSharedRef<SWidget> CreateWFCPaintCanvas(TSharedPtr<FWFCBrushState> State)
{
	return SNew(SWFCPaintCanvas).State(State);
}

TSharedRef<SWidget> SWFCLayoutPainter::BrushButton(const FString& Label, EWFCPaintKind Kind)
{
	return SNew(SBorder).Padding(0).OnMouseButtonDown_Lambda(
	    [this, Kind](const FGeometry&, const FPointerEvent& E)
	    {
		    if (E.GetEffectingButton() != EKeys::RightMouseButton)
			    return FReply::Unhandled();
		    FColorPickerArgs Picker;
		    Picker.InitialColor = State->Colors[Kind];
		    Picker.bUseAlpha = false;
		    Picker.OnColorCommitted = FOnLinearColorValueChanged::CreateLambda(
		        [Shared = State, Kind](FLinearColor Color) { Shared->Colors.Add(Kind, Color); });
		    OpenColorPicker(Picker);
		    return FReply::Handled();
	    })[SNew(SButton)
	           .Text(FText::FromString(Label))
	           .ToolTipText(FText::FromString(TEXT("左键选笔刷，右键修改显示颜色")))
	           .ButtonColorAndOpacity_Lambda(
	               [this, Kind]()
	               { return FSlateColor(State->Brush == Kind ? State->Colors[Kind] : FLinearColor(.18, .18, .18)); })
	           .ForegroundColor_Lambda([this,Kind](){return FSlateColor(State->Brush==Kind && State->Colors[Kind].GetLuminance()>.45f ? FLinearColor::Black : FLinearColor::White);})
	           .OnClicked_Lambda(
	               [this, Kind]()
	               {
		               State->Brush = Kind;
		               State->bSelect = false;
		               return FReply::Handled();
	               })];
}
void SWFCLayoutPainter::Construct(const FArguments&)
{
	State = MakeShared<FWFCBrushState>();
	for (int32 I = 0; I <= int32(EWFCPaintKind::Ground); ++I)
		State->Colors.Add(EWFCPaintKind(I), DefaultColor(EWFCPaintKind(I)));
	auto Toolbar = SNew(SWrapBox).UseAllottedSize(true);
	auto Button = [&](const TCHAR* Label, FOnClicked Action)
	{ Toolbar->AddSlot().Padding(2)[SNew(SButton).Text(FText::FromString(Label)).OnClicked(Action)]; };
	Button(TEXT("读取选中生成器"), FOnClicked::CreateSP(this, &SWFCLayoutPainter::LoadSelection));
	Button(TEXT("导入当前布局"), FOnClicked::CreateSP(this, &SWFCLayoutPainter::LoadGenerated));
	Button(TEXT("撤销"), FOnClicked::CreateLambda(
	                         [this]()
	                         {
		                         if (!State->Undo.IsEmpty())
		                         {
			                         State->Redo.Add(State->Cells);
			                         State->Cells = State->Undo.Pop();
			                         State->Errors.Reset();
		                         }
		                         return FReply::Handled();
	                         }));
	Button(TEXT("重做"), FOnClicked::CreateLambda(
	                         [this]()
	                         {
		                         if (!State->Redo.IsEmpty())
		                         {
			                         State->Undo.Add(State->Cells);
			                         State->Cells = State->Redo.Pop();
			                         State->Errors.Reset();
		                         }
		                         return FReply::Handled();
	                         }));
	Button(TEXT("清空"), FOnClicked::CreateLambda(
	                         [this]()
	                         {
		                         State->Remember();
		                         State->Cells.Reset();
		                         return FReply::Handled();
	                         }));
	Button(TEXT("保存规划资产"), FOnClicked::CreateSP(this, &SWFCLayoutPainter::SavePlan));
	auto Brushes = SNew(SWrapBox).UseAllottedSize(true);
	for (const auto& Pair : TArray<TPair<FString, EWFCPaintKind>>{{TEXT("白：道路"), EWFCPaintKind::Road},
	                                                              {TEXT("绿：建筑"), EWFCPaintKind::Building},
	                                                              {TEXT("黄：广场"), EWFCPaintKind::Plaza},
	                                                              {TEXT("红：留空"), EWFCPaintKind::Ground},
	                                                              {TEXT("灰：自动/橡皮"), EWFCPaintKind::Unspecified}})
		Brushes->AddSlot().Padding(2)[BrushButton(Pair.Key, Pair.Value)];
	auto Side = SNew(SVerticalBox);
	auto Add = [&](TSharedRef<SWidget> W) { Side->AddSlot().AutoHeight().Padding(0, 4)[W]; };
	auto Label = [&](const TCHAR* Text) { Add(SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(Text))); };
	Label(TEXT("左键填格，右键擦除。向右 +X，向上 +Y。\n浅色表示计算结果，实色表示手工约束。"));
	Add(SNew(SCheckBox)
	        .IsChecked_Lambda([this]()
	                          { return State->bRectangle ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
	        .OnCheckStateChanged_Lambda(
	            [this](ECheckBoxState V)
	            {
		            State->bRectangle = V == ECheckBoxState::Checked;
	            })[SNew(STextBlock).Text(FText::FromString(TEXT("矩形填充")))]);
	Add(SNew(SCheckBox)
	        .IsChecked_Lambda([this]() { return State->bSelect ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
	        .OnCheckStateChanged_Lambda(
	            [this](ECheckBoxState V)
	            {
		            State->bSelect = V == ECheckBoxState::Checked;
	            })[SNew(STextBlock).Text(FText::FromString(TEXT("选择/读取格子参数")))]);
	Label(TEXT("建筑层数 0=随机；整栋替换固定为逻辑1层"));
	Add(SNew(SSpinBox<int32>)
	        .MinValue(0)
	        .MaxValue(3)
	        .Value_Lambda([this]() { return State->Stories; })
	        .OnValueChanged_Lambda([this](int32 V) { State->Stories = V; }));
	Label(TEXT("完整房屋模型（空=用样式库）"));
	Add(SNew(SObjectPropertyEntryBox)
	        .AllowedClass(UStaticMesh::StaticClass())
	        .DisplayUseSelected(true)
	        .DisplayBrowse(true)
	        .ObjectPath_Lambda([this]()
	                           { return State->HouseMesh.IsValid() ? State->HouseMesh->GetPathName() : FString(); })
	        .OnObjectChanged_Lambda(
	            [this](const FAssetData& A)
	            {
		            State->HouseMesh.Reset(Cast<UStaticMesh>(A.GetAsset()));
		            FitHouse();
	            }));
	Label(TEXT("替换房屋缩放 / 额外朝向 / 高度偏移(cm)"));
	Add(SNew(SSpinBox<float>)
	        .MinValue(.001)
	        .MaxValue(100)
	        .Value_Lambda([this]() { return State->HouseScale; })
	        .OnValueChanged_Lambda([this](float V) { State->HouseScale = V; }));
	Add(SNew(SSpinBox<float>)
	        .MinValue(-360)
	        .MaxValue(360)
	        .Value_Lambda([this]() { return State->HouseYaw; })
	        .OnValueChanged_Lambda([this](float V) { State->HouseYaw = V; }));
	Add(SNew(SSpinBox<float>)
	        .MinValue(-10000)
	        .MaxValue(10000)
	        .Value_Lambda([this]() { return State->HouseZ; })
	        .OnValueChanged_Lambda([this](float V) { State->HouseZ = V; }));
	Add(SNew(SButton)
	        .Text(FText::FromString(TEXT("按地块宽度自动适配模型")))
	        .OnClicked_Lambda(
	            [this]()
	            {
		            FitHouse();
		            return FReply::Handled();
	            }));
	Add(SNew(SCheckBox)
	        .IsChecked_Lambda(
	            [this]()
	            {
		            return State->Config.Mode == EWFCMapMode::Volume ? ECheckBoxState::Checked
		                                                             : ECheckBoxState::Unchecked;
	            })
	        .OnCheckStateChanged_Lambda(
	            [this](ECheckBoxState V)
	            {
		            State->Config.Mode = V == ECheckBoxState::Checked ? EWFCMapMode::Volume : EWFCMapMode::Planar;
	            })[SNew(STextBlock).Text(FText::FromString(TEXT("立体建筑模式")))]);
	Label(TEXT("可用楼层数 / 随机种子"));
	Add(SNew(SSpinBox<int32>)
	        .MinValue(1)
	        .MaxValue(3)
	        .Value_Lambda([this]() { return State->Config.Layers; })
	        .OnValueChanged_Lambda([this](int32 V) { State->Config.Layers = V; }));
	Add(SNew(SSpinBox<int32>)
	        .Value_Lambda([this]() { return State->Config.Seed; })
	        .OnValueChanged_Lambda([this](int32 V) { State->Config.Seed = V; }));
	Label(TEXT("画布宽 / 高（改变尺寸后越界笔画会被剔除）"));
	Add(SNew(SSpinBox<int32>)
	        .MinValue(3)
	        .MaxValue(64)
	        .Value_Lambda([this]() { return State->Width; })
	        .OnValueChanged_Lambda(
	            [this](int32 V)
	            {
		            State->Remember();
		            TMap<int32, FWFCPaintedCell> Next;
		            for (const auto& P : State->Cells)
			            if (P.Value.Coordinate.X < V)
				            Next.Add(P.Value.Coordinate.X + V * P.Value.Coordinate.Y, P.Value);
		            State->Width = V;
		            State->Cells = MoveTemp(Next);
		            State->Preview.Reset();
		            State->Undo.Reset();
		            State->Redo.Reset();
	            }));
	Add(SNew(SSpinBox<int32>)
	        .MinValue(3)
	        .MaxValue(64)
	        .Value_Lambda([this]() { return State->Depth; })
	        .OnValueChanged_Lambda(
	            [this](int32 V)
	            {
		            State->Depth = V;
		            for (auto It = State->Cells.CreateIterator(); It; ++It)
			            if (It.Value().Coordinate.Y >= V)
				            It.RemoveCurrent();
		            State->Preview.Reset();
		            State->Undo.Reset();
		            State->Redo.Reset();
	            }));
	Label(TEXT("整套建筑样式 / 规则资产"));
	Add(SNew(SObjectPropertyEntryBox)
	        .AllowedClass(UWFCTileSet::StaticClass())
	        .DisplayUseSelected(true)
	        .DisplayBrowse(true)
	        .ObjectPath_Lambda([this]()
	                           { return State->TileSet.IsValid() ? State->TileSet->GetPathName() : FString(); })
	        .OnObjectChanged_Lambda([this](const FAssetData& A)
	                                { State->TileSet.Reset(Cast<UWFCTileSet>(A.GetAsset())); }));
	Add(SNew(SCheckBox)
	        .IsChecked_Lambda([this]() { return State->bFill ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
	        .OnCheckStateChanged_Lambda(
	            [this](ECheckBoxState V)
	            {
		            State->bFill = V == ECheckBoxState::Checked;
	            })[SNew(STextBlock).Text(FText::FromString(TEXT("补齐灰格：自动选择临街建筑")))]);
	Add(SNew(SSpinBox<float>)
	        .MinValue(0)
	        .MaxValue(1)
	        .Delta(.05)
	        .Value_Lambda([this]() { return State->Density; })
	        .OnValueChanged_Lambda([this](float V) { State->Density = V; }));
	Add(SNew(SButton).Text(FText::FromString(TEXT("计算：保留笔画并生成"))).OnClicked(this, &SWFCLayoutPainter::Apply));
	Add(SNew(SButton)
	        .Text(FText::FromString(TEXT("完全自动生成（保留本窗口笔画）")))
	        .OnClicked(this, &SWFCLayoutPainter::GenerateAutomatic));
	Label(TEXT("可复用规划资产"));
	Add(SNew(SObjectPropertyEntryBox)
	        .AllowedClass(UWFCLayoutAsset::StaticClass())
	        .DisplayBrowse(true)
	        .ObjectPath_Lambda([this]() { return State->Plan.IsValid() ? State->Plan->GetPathName() : FString(); })
	        .OnObjectChanged_Lambda([this](const FAssetData& A)
	                                { State->Plan.Reset(Cast<UWFCLayoutAsset>(A.GetAsset())); }));
	Add(SNew(SButton)
	        .Text(FText::FromString(TEXT("载入规划（替换当前画布）")))
	        .OnClicked(this, &SWFCLayoutPainter::LoadPlan));
	Label(TEXT("关掉自动补齐即严格按图放建筑。道路保持画出的形状，曲线栅格化为拐角。\n模型替换后请检查尺寸、门朝向与碰"
	           "撞。未应用/未保存的画布关闭后会丢失。"));
	ChildSlot
	    [SNew(SVerticalBox) + SVerticalBox::Slot().AutoHeight().Padding(6)[Toolbar] +
	     SVerticalBox::Slot().AutoHeight().Padding(6)[Brushes] +
	     SVerticalBox::Slot().FillHeight(1).Padding(
	         6)[SNew(SHorizontalBox) +
	            SHorizontalBox::Slot().FillWidth(1)[SNew(SBorder).Padding(8)[CreateWFCPaintCanvas(State)]] +
	            SHorizontalBox::Slot().AutoWidth().Padding(
	                8, 0)[SNew(SBox).WidthOverride(300)[SNew(SScrollBox) + SScrollBox::Slot()[Side]]]] +
	     SVerticalBox::Slot().AutoHeight().Padding(8)
	         [SNew(STextBlock).AutoWrapText(true).Text_Lambda([this]() { return FText::FromString(State->Message); })]];
	LoadSelection();
}
void SWFCLayoutPainter::FitHouse()
{
	if (!State->HouseMesh.IsValid())
		return;
	const auto B = State->HouseMesh->GetBoundingBox();
	const float Span = FMath::Max(B.GetSize().X, B.GetSize().Y);
	const float Width =
	    State->TileSet.IsValid() ? FMath::Min(State->TileSet->CellSize.X, State->TileSet->CellSize.Y) : 600;
	State->HouseScale = Span > .001f ? Width * .85f / Span : 1;
	State->HouseYaw = State->HouseZ = 0;
	State->Stories = 1;
}
FWFCGenerationSettings SWFCLayoutPainter::GatherSettings() const
{
	auto S = State->Config;
	S.Width = State->Width;
	S.Depth = State->Depth;
	S.LayoutPreset = EWFCLayoutPreset::Painted;
	S.bConstrainHorizontalBoundary = false;
	S.bFillUnpaintedBuildings = State->bFill;
	S.BuildingDensity = State->Density;
	S.PaintedCells.Reset();
	for (int32 Y = 0; Y < State->Depth; ++Y)
		for (int32 X = 0; X < State->Width; ++X)
			if (const auto* C = State->Cells.Find(X + Y * State->Width))
				S.PaintedCells.Add(*C);
	return S;
}
FReply SWFCLayoutPainter::LoadSelection()
{
	if (!GEditor || GEditor->PlayWorld)
	{
		State->Message = TEXT("请停止运行后编辑规划。");
		return FReply::Handled();
	}
	auto* A = GEditor->GetSelectedActors()->GetTop<AWFCMapGenerator>();
	if (!A || A->Settings.Width < 3 || A->Settings.Depth < 3 || A->Settings.Width > 64 || A->Settings.Depth > 64)
	{
		State->Message = TEXT("请在世界大纲选中 WFC 生成器，或先把 BP_WFC_Settlement 放入关卡。");
		return FReply::Handled();
	}
	State->Target = A;
	State->TileSet.Reset(A->TileSet);
	State->Config = A->Settings;
	State->Width = A->Settings.Width;
	State->Depth = A->Settings.Depth;
	State->Density = A->Settings.BuildingDensity;
	State->bFill = A->Settings.bFillUnpaintedBuildings;
	State->Cells.Reset();
	State->Undo.Reset();
	State->Redo.Reset();
	State->Errors.Reset();
	for (const auto& C : A->Settings.PaintedCells)
		if (C.Coordinate.X >= 0 && C.Coordinate.Y >= 0 && C.Coordinate.X < State->Width &&
		    C.Coordinate.Y < State->Depth)
			State->Cells.Add(C.Coordinate.X + State->Width * C.Coordinate.Y, C);
	UpdatePreview();
	State->Message = TEXT("已读取选中生成器。可填格、载入规划，或导入当前布局后修改。");
	return FReply::Handled();
}
void SWFCLayoutPainter::UpdatePreview()
{
	State->Preview.Reset();
	const auto* A = State->Target.Get();
	if (!A)
		return;
	const auto* Set = A->DisplayedTileSet ? A->DisplayedTileSet.Get() : A->TileSet.Get();
	if (!Set)
		return;
	if (A->DisplayedResult.Dimensions.X != State->Width || A->DisplayedResult.Dimensions.Y != State->Depth)
		return;
	for (const auto& C : A->DisplayedResult.Cells)
		if (C.Coordinate.Z == 0 && Set->Tiles.IsValidIndex(C.TileIndex))
			State->Preview.Add(C.Coordinate.X + State->Width * C.Coordinate.Y, KindForTile(Set->Tiles[C.TileIndex]));
}
FReply SWFCLayoutPainter::LoadGenerated()
{
	const auto* A = State->Target.Get();
	if (!A)
	{
		State->Message = TEXT("请先读取生成器。");
		return FReply::Handled();
	}
	const auto* Set = A->DisplayedTileSet ? A->DisplayedTileSet.Get() : A->TileSet.Get();
	if (!Set || A->DisplayedResult.Dimensions.X != State->Width || A->DisplayedResult.Dimensions.Y != State->Depth)
	{
		State->Message = TEXT("当前结果和画布尺寸不一致，请重新读取或计算。");
		return FReply::Handled();
	}
	State->Remember();
	State->Cells.Reset();
	const auto& Snapshot = A->DisplayedTileSet ? A->DisplayedSettings : A->Settings;
	for (const auto& C : A->DisplayedResult.Cells)
	{
		if (C.Coordinate.Z != 0 || !Set->Tiles.IsValidIndex(C.TileIndex))
			continue;
		FWFCPaintedCell P;
		P.Coordinate = FIntPoint(C.Coordinate.X, C.Coordinate.Y);
		P.Kind = KindForTile(Set->Tiles[C.TileIndex]);
		if (P.Kind == EWFCPaintKind::Building)
		{
			P.Stories = 1;
			for (const auto& U : A->DisplayedResult.Cells)
				if (U.Coordinate.X == C.Coordinate.X && U.Coordinate.Y == C.Coordinate.Y && U.Coordinate.Z > 0 &&
				    Set->Tiles.IsValidIndex(U.TileIndex) && Set->Tiles[U.TileIndex].bWalkable)
					++P.Stories;
			for (const auto& Old : Snapshot.PaintedCells)
				if (Old.Coordinate == P.Coordinate && !Old.BuildingMesh.IsNull())
					P = Old;
		}
		State->Cells.Add(P.Coordinate.X + State->Width * P.Coordinate.Y, P);
	}
	State->Message = TEXT("已导入当前布局，所有导入格子现在都是手工约束。");
	return FReply::Handled();
}
FReply SWFCLayoutPainter::Apply()
{
	auto* A = State->Target.Get();
	if (!A || !State->TileSet.IsValid() || !GEditor || GEditor->PlayWorld)
	{
		State->Message = TEXT("请先读取生成器并指定样式规则；运行时不能编辑。");
		return FReply::Handled();
	}
	const auto S = GatherSettings();
	State->Errors.Reset();
	for (const auto& P : S.PaintedCells)
		if (P.Kind == EWFCPaintKind::Building)
		{
			bool Front = false;
			for (const auto& D : TArray<FIntPoint>{{1, 0}, {-1, 0}, {0, 1}, {0, -1}})
				if (const auto* N = State->Cells.Find(P.Coordinate.X + D.X + (P.Coordinate.Y + D.Y) * State->Width))
					Front |= N->Coordinate == P.Coordinate + D &&
					         (N->Kind == EWFCPaintKind::Road || N->Kind == EWFCPaintKind::Plaza);
			if (!Front)
				State->Errors.Add(P.Coordinate.X + P.Coordinate.Y * State->Width);
		}
	TArray<FWFCCellRule> Rules;
	FString Error;
	if (!FWFCLayoutPlanner::Build(S, Rules, Error))
	{
		State->Message = TEXT("规划无效：") + Error;
		return FReply::Handled();
	}
	const FScopedTransaction Transaction(FText::FromString(TEXT("Apply WFC layout")));
	A->Modify();
	A->Settings = S;
	A->TileSet = State->TileSet.Get();
	const bool OK = A->GenerateMap();
	if (!OK && A->LastResult.FailedCell.X >= 0)
		State->Errors.Add(A->LastResult.FailedCell.X + A->LastResult.FailedCell.Y * State->Width);
	UpdatePreview();
	State->Message = OK ? FString::Printf(TEXT("生成成功：%d 个可见实例。请 Ctrl+S 保存关卡，或保存规划资产。"),
	                                      A->GetGeneratedInstanceCount())
	                    : TEXT("计算失败，保留原预览：") + A->LastResult.Message;
	return FReply::Handled();
}
FReply SWFCLayoutPainter::GenerateAutomatic()
{
	auto* A = State->Target.Get();
	if (!A || !State->TileSet.IsValid() || !GEditor || GEditor->PlayWorld)
		return FReply::Handled();
	auto S = GatherSettings();
	S.LayoutPreset = EWFCLayoutPreset::Settlement;
	TArray<FWFCCellRule> Rules;
	FString Error;
	if (!FWFCLayoutPlanner::Build(S, Rules, Error))
	{
		State->Message = Error;
		return FReply::Handled();
	}
	const FScopedTransaction Transaction(FText::FromString(TEXT("Generate automatic WFC layout")));
	A->Modify();
	A->Settings = S;
	A->TileSet = State->TileSet.Get();
	const bool OK = A->GenerateMap();
	UpdatePreview();
	State->Message =
	    OK ? TEXT("已完全自动生成；本窗口的笔画未清除。保存关卡或再点击按图计算。") : A->LastResult.Message;
	return FReply::Handled();
}
FReply SWFCLayoutPainter::SavePlan()
{
	auto& Tools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	auto* Factory = NewObject<UDataAssetFactory>();
	Factory->DataAssetClass = UWFCLayoutAsset::StaticClass();
	auto* Plan = Cast<UWFCLayoutAsset>(Tools.CreateAssetWithDialog(
	    TEXT("DA_WFC_Plan"), TEXT("/Game/ProceduralGeneration/WFC/Layouts"), UWFCLayoutAsset::StaticClass(), Factory));
	if (!Plan)
		return FReply::Handled();
	Plan->Settings = GatherSettings();
	Plan->TileSet = State->TileSet.Get();
	Plan->Palette = State->Colors;
	Plan->MarkPackageDirty();
	State->Plan.Reset(Plan);
	State->Message = UEditorLoadingAndSavingUtils::SavePackages({Plan->GetOutermost()}, true)
	                     ? TEXT("规划资产已保存，可在其他生成器上载入。")
	                     : TEXT("规划已创建，尚未保存到磁盘。");
	return FReply::Handled();
}
FReply SWFCLayoutPainter::LoadPlan()
{
	const auto* Plan = State->Plan.Get();
	if (!Plan)
		return FReply::Handled();
	const auto& S = Plan->Settings;
	if (S.Width < 3 || S.Width > 64 || S.Depth < 3 || S.Depth > 64)
	{
		State->Message = TEXT("规划尺寸无效。");
		return FReply::Handled();
	}
	State->Config = S;
	State->Width = S.Width;
	State->Depth = S.Depth;
	State->Density = S.BuildingDensity;
	State->bFill = S.bFillUnpaintedBuildings;
	State->TileSet.Reset(Plan->TileSet);
	State->Cells.Reset();
	State->Undo.Reset();
	State->Redo.Reset();
	State->Errors.Reset();
	State->Preview.Reset();
	for (const auto& C : S.PaintedCells)
		if (C.Coordinate.X >= 0 && C.Coordinate.Y >= 0 && C.Coordinate.X < S.Width && C.Coordinate.Y < S.Depth)
			State->Cells.Add(C.Coordinate.X + S.Width * C.Coordinate.Y, C);
	for (const auto& C : Plan->Palette)
		State->Colors.Add(C.Key, C.Value);
	State->Message = TEXT("已载入规划，点击计算应用到选中的生成器。");
	return FReply::Handled();
}
