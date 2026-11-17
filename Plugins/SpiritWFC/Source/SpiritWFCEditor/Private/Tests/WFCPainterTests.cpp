#include "SWFCLayoutPainter.h"
#include "Misc/AutomationTest.h"
#include "InputCoreTypes.h"
#include "Framework/Docking/TabManager.h"
#include "Engine/StaticMesh.h"
#include "Widgets/SWidget.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWFCPainterInputTest, "SpiritWFC.Editor.PainterMouseAndBrushes",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWFCPainterInputTest::RunTest(const FString&)
{
	for (float Scale : {1.f, 1.5f})
	{
		auto State = MakeShared<FWFCBrushState>();
		State->Width = 8;
		State->Depth = 6;
		auto Canvas = CreateWFCPaintCanvas(State);
		const auto G = FGeometry::MakeRoot(FVector2D(800, 600), FSlateLayoutTransform(Scale));
		TSet<FKey> Left = {EKeys::LeftMouseButton}, Right = {EKeys::RightMouseButton};
		auto Event = [&](FVector2D P, FKey Key, const TSet<FKey>& Buttons)
		{ return FPointerEvent(0, P * Scale, P * Scale, Buttons, Key, 0, FModifierKeysState()); };
		TestTrue(TEXT("Canvas handles paint input"),
		         Canvas->OnMouseButtonDown(G, Event({50, 550}, EKeys::LeftMouseButton, Left)).IsEventHandled());
		Canvas->OnMouseMove(G, Event({750, 50}, EKeys::LeftMouseButton, Left));
		Canvas->OnMouseButtonUp(G, Event({750, 50}, EKeys::LeftMouseButton, Left));
		TestTrue(TEXT("Drag reaches correct world grid endpoint at different DPI"), State->Cells.Contains(7 + 5 * 8));
		TestTrue(TEXT("Origin uses bottom-left +Y-up coordinates"), State->Cells.Contains(0));
		TestEqual(TEXT("One drag produces one undo snapshot"), State->Undo.Num(), 1);
		const int32 Before = State->Cells.Num();
		Canvas->OnMouseButtonDown(G, Event({50, 550}, EKeys::RightMouseButton, Right));
		Canvas->OnMouseButtonUp(G, Event({50, 550}, EKeys::RightMouseButton, Right));
		TestEqual(TEXT("Right mouse erases exactly one cell"), State->Cells.Num(), Before - 1);
		State->Cells.Reset();
		State->bRectangle = true;
		State->Brush = EWFCPaintKind::Ground;
		Canvas->OnMouseButtonDown(G, Event({150, 450}, EKeys::LeftMouseButton, Left));
		Canvas->OnMouseMove(G, Event({450, 250}, EKeys::LeftMouseButton, Left));
		Canvas->OnMouseButtonUp(G, Event({450, 250}, EKeys::LeftMouseButton, Left));
		TestEqual(TEXT("Rectangle fills exact inclusive bounds"), State->Cells.Num(), 12);
		State->bSelect = true;
		Canvas->OnMouseButtonDown(G, Event({150, 450}, EKeys::LeftMouseButton, Left));
		TestEqual(TEXT("Inspection does not paint over cells"), State->Cells.Num(), 12);
		TestTrue(TEXT("Inspection reads reserved-empty kind"), State->Brush == EWFCPaintKind::Ground);
		State->HouseMesh.Reset(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
		TestTrue(TEXT("Complete building source available"), State->HouseMesh.IsValid());
		State->Brush = EWFCPaintKind::Building;
		State->Stories = 3;
		State->HouseScale = 2;
		State->HouseYaw = 90;
		State->Paint(FIntPoint(5, 4), false);
		const auto& Cell = State->Cells[5 + 4 * 8];
		TestEqual(TEXT("Complete prefab uses one logical story"), Cell.Stories, 1);
		TestTrue(TEXT("Prefab mesh and transform captured in the plan"),
		         !Cell.BuildingMesh.IsNull() && Cell.BuildingTransform.GetScale3D().Equals(FVector(2)));
	}
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWFCPainterPanelTest,"SpiritWFC.Editor.PanelAndMenuRegistration",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWFCPainterPanelTest::RunTest(const FString&)
{
	TestTrue(TEXT("Editor tool tab registered"),FGlobalTabmanager::Get()->HasTabSpawner(TEXT("SpiritWFCLayoutPainter")));
	for(float Scale:{1.f,1.5f})
	{
		auto Panel=SNew(SWFCLayoutPainter);
		Panel->SlatePrepass(Scale);
		TestTrue(TEXT("Complete editor panel constructs and measures"),Panel->GetDesiredSize().X>0 && Panel->GetDesiredSize().Y>0);
	}
	return true;
}
#endif
