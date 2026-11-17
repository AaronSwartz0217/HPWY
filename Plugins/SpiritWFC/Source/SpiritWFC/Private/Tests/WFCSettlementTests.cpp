#include "WFCLayoutPlanner.h"
#include "WFCPaintRaster.h"
#include "WFCSolver.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
FWFCTileDefinition RoleTile(FName Tag, FName Horizontal, FName Down, FName Up)
{
	FWFCTileDefinition T;
	T.TileId = Tag;
	T.Tags.Add(Tag);
	T.Sockets.PositiveX = T.Sockets.NegativeX = T.Sockets.PositiveY = T.Sockets.NegativeY = Horizontal;
	T.Sockets.Down = Down;
	T.Sockets.Up = Up;
	return T;
}
void Populate(UWFCTileSet* Set)
{
	Set->TraversableSockets = {TEXT("Ground"), TEXT("Road"), TEXT("Stair")};
	Set->Tiles.Add(RoleTile(TEXT("Ground"), TEXT("Ground"), TEXT("Ground"), TEXT("Sky")));
	auto Air = RoleTile(TEXT("Air"), TEXT("Closed"), TEXT("Sky"), TEXT("Sky"));
	Air.bWalkable = false;
	Air.bRenderMesh = false;
	Set->Tiles.Add(Air);
	for (int32 Mask = 0; Mask < 16; ++Mask)
		for (int32 Kind = 0; Kind < 2; ++Kind)
		{
			const FName Tag(*(FString(Kind ? TEXT("Plaza_") : TEXT("Road_")) + FString::Printf(TEXT("%02d"), Mask)));
			auto T = RoleTile(Tag, TEXT("Ground"), TEXT("Ground"), TEXT("Sky"));
			T.Sockets.PositiveX = Mask & 1 ? TEXT("Road") : TEXT("Ground");
			T.Sockets.NegativeX = Mask & 2 ? TEXT("Road") : TEXT("Ground");
			T.Sockets.PositiveY = Mask & 4 ? TEXT("Road") : TEXT("Ground");
			T.Sockets.NegativeY = Mask & 8 ? TEXT("Road") : TEXT("Ground");
			Set->Tiles.Add(T);
		}
	for (const auto Tag : {FName(TEXT("House")), FName(TEXT("Base")), FName(TEXT("Middle")), FName(TEXT("Top"))})
	{
		const bool Upper = Tag == TEXT("Middle") || Tag == TEXT("Top");
		auto T = RoleTile(Tag, Upper ? FName(TEXT("Closed")) : FName(TEXT("Ground")),
		                  Upper ? FName(TEXT("Stair")) : FName(TEXT("Ground")),
		                  Tag == TEXT("House") || Tag == TEXT("Top") ? FName(TEXT("Sky")) : FName(TEXT("Stair")));
		T.AllowedYawQuarterTurns = {0, 1, 2, 3};
		T.MinLayer = Upper ? 1 : 0;
		T.MaxLayer = Upper ? 2 : 0;
		Set->Tiles.Add(T);
	}
}
bool Street(FName Tag)
{
	const auto S = Tag.ToString();
	return S.StartsWith(TEXT("Road_")) || S.StartsWith(TEXT("Plaza_"));
}
} // namespace
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWFCSettlementTest, "SpiritWFC.Settlement.StructureAndReproducibility",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWFCSettlementTest::RunTest(const FString&)
{
	TStrongObjectPtr<UWFCTileSet> Set(NewObject<UWFCTileSet>());
	Populate(Set.Get());
	FWFCGenerationSettings S;
	S.Width = 14;
	S.Depth = 12;
	S.Mode = EWFCMapMode::Volume;
	S.Layers = 3;
	S.LayoutPreset = EWFCLayoutPreset::Settlement;
	S.BuildingDensity = 1;
	S.bConstrainHorizontalBoundary = false;
	const FIntVector Faces[4] = {{0, -1, 0}, {1, 0, 0}, {0, 1, 0}, {-1, 0, 0}};
	FString PreviousLayout;
	bool SeedChangesPlan = false;
	for (int32 Seed = 0; Seed < 24; ++Seed)
	{
		S.Seed = Seed;
		TArray<FWFCCellRule> Rules, Again;
		FString Error;
		TestTrue(TEXT("Valid plan"), FWFCLayoutPlanner::Build(S, Rules, Error));
		FWFCLayoutPlanner::Build(S, Again, Error);
		TestEqual(TEXT("Full zoning grid"), Rules.Num(), 14 * 12 * 3);
		FString Signature;
		int32 PlazaCount = 0, Buildings = 0, AirCount = 0, StreetCount = 0;
		for (int32 I = 0; I < Rules.Num(); ++I)
		{
			const auto& R = Rules[I];
			Signature += R.RequiredTag.ToString() + FString::FromInt(R.QuarterTurns);
			TestTrue(TEXT("Planner seed reproducible"),
			         R.RequiredTag == Again[I].RequiredTag && R.QuarterTurns == Again[I].QuarterTurns);
			PlazaCount += R.RequiredTag.ToString().StartsWith(TEXT("Plaza_"));
			AirCount += R.RequiredTag == TEXT("Air");
			StreetCount += R.Coordinate.Z == 0 && Street(R.RequiredTag);
			if (R.RequiredTag == TEXT("House") || R.RequiredTag == TEXT("Base"))
			{
				++Buildings;
				const auto Front = R.Coordinate + Faces[R.QuarterTurns];
				TestTrue(TEXT("Every ground-floor entrance faces a street"),
				         Street(Rules[Front.X + S.Width * Front.Y].RequiredTag));
			}
			if (R.RequiredTag == TEXT("Middle") || R.RequiredTag == TEXT("Top"))
			{
				const auto& Below = Rules[I - S.Width * S.Depth];
				TestTrue(TEXT("No unsupported floating floor"),
				         Below.RequiredTag == TEXT("Base") || Below.RequiredTag == TEXT("Middle"));
				TestEqual(TEXT("Stair orientation persists across floors"), R.QuarterTurns, Below.QuarterTurns);
			}
		}
		if (Seed > 0)
			SeedChangesPlan |= Signature != PreviousLayout;
		PreviousLayout = Signature;
		TestEqual(TEXT("Nine-cell plaza preserved"), PlazaCount, 9);
		TestTrue(TEXT("Buildings and upper open air both exist"), Buildings > 0 && AirCount > 0);
		TSet<int32> Seen;
		TArray<int32> Queue;
		for (int32 I = 0; I < S.Width * S.Depth; ++I)
			if (Street(Rules[I].RequiredTag))
			{
				Queue.Add(I);
				Seen.Add(I);
				break;
			}
		for (int32 Head = 0; Head < Queue.Num(); ++Head)
		{
			const int32 I = Queue[Head], X = I % S.Width, Y = I / S.Width;
			for (const auto& D : Faces)
			{
				const int32 NX = X + D.X, NY = Y + D.Y, Next = NX + NY * S.Width;
				if (NX >= 0 && NY >= 0 && NX < S.Width && NY < S.Depth && !Seen.Contains(Next) &&
				    Street(Rules[Next].RequiredTag))
				{
					Seen.Add(Next);
					Queue.Add(Next);
				}
			}
		}
		TestEqual(TEXT("All roads and plaza belong to one network"), Seen.Num(), StreetCount);
		const auto Result = FWFCSolver::Solve(Set.Get(), S);
		TestTrue(TEXT("WFC satisfies zoning, ports and whole walkable graph"),
		         Result.Status == EWFCResultStatus::Success);
		TestEqual(TEXT("Exactly one connected walkable component"), Result.WalkableComponents, 1);
	}
	TestTrue(TEXT("Different seeds change macro layout"), SeedChangesPlan);
	S.Width = 8;
	TArray<FWFCCellRule> Rules;
	FString Error;
	TestFalse(TEXT("Reject too-small settlement"), FWFCLayoutPlanner::Build(S, Rules, Error));
	S.Width = 14;
	S.BuildingDensity = 1.1;
	TestTrue(TEXT("Reject invalid density at runtime"),
	         FWFCSolver::Solve(Set.Get(), S).Status == EWFCResultStatus::InvalidInput);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWFCPaintedTest, "SpiritWFC.Painted.PreserveSketchAndFill",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWFCPaintedTest::RunTest(const FString&)
{
	TStrongObjectPtr<UWFCTileSet> Set(NewObject<UWFCTileSet>());
	Populate(Set.Get());
	FWFCGenerationSettings S;
	S.Mode = EWFCMapMode::Volume;
	S.Layers = 3;
	S.Width = S.Depth = 10;
	S.LayoutPreset = EWFCLayoutPreset::Painted;
	S.bConstrainHorizontalBoundary = false;
	S.bFillUnpaintedBuildings = false;
	S.BuildingDensity = 0;
	auto Add = [&](FIntPoint P, EWFCPaintKind Kind, int32 Stories = 0)
	{
		FWFCPaintedCell C;
		C.Coordinate = P;
		C.Kind = Kind;
		C.Stories = Stories;
		S.PaintedCells.Add(C);
	};
	for (int32 X = 1; X <= 7; ++X)
		Add(FIntPoint(X, 3), EWFCPaintKind::Road);
	for (int32 Y = 4; Y <= 7; ++Y)
		Add(FIntPoint(7, Y), EWFCPaintKind::Road);
	Add(FIntPoint(3, 4), EWFCPaintKind::Plaza);
	Add(FIntPoint(4, 4), EWFCPaintKind::Plaza);
	Add(FIntPoint(3, 2), EWFCPaintKind::Building, 2);
	Add(FIntPoint(8, 5), EWFCPaintKind::Building, 3);
	Add(FIntPoint(4, 2), EWFCPaintKind::Ground);
	for (int32 Seed = 0; Seed < 20; ++Seed)
	{
		S.Seed = Seed;
		TArray<FWFCCellRule> Rules;
		FString Error;
		TestTrue(TEXT("Manual plan is valid"), FWFCLayoutPlanner::Build(S, Rules, Error));
		if (Rules.Num() != 300)
		{
			AddError(Error);
			return false;
		}
		int32 StreetCount = 0, BaseCount = 0;
		for (const auto& R : Rules)
		{
			StreetCount += Street(R.RequiredTag);
			BaseCount += R.RequiredTag == TEXT("Base");
		}
		TestEqual(TEXT("Do not invent extra road cells"), StreetCount, 13);
		TestEqual(TEXT("Only explicitly painted buildings in strict mode"), BaseCount, 2);
		TestEqual(TEXT("Reserved empty remains ground"), Rules[4 + 2 * 10].RequiredTag, FName(TEXT("Ground")));
		TestEqual(TEXT("Two-story roof is placed at layer one"), Rules[3 + 2 * 10 + 100].RequiredTag,
		          FName(TEXT("Top")));
		TestEqual(TEXT("Space above two-story house is air"), Rules[3 + 2 * 10 + 200].RequiredTag, FName(TEXT("Air")));
		TestEqual(TEXT("Three-story middle is structural"), Rules[8 + 5 * 10 + 100].RequiredTag, FName(TEXT("Middle")));
		TestTrue(TEXT("Painted scene solves with connected floors"),
		         FWFCSolver::Solve(Set.Get(), S).Status == EWFCResultStatus::Success);
	}
	S.bFillUnpaintedBuildings = true;
	S.BuildingDensity = 1;
	auto Filled = FWFCSolver::Solve(Set.Get(), S);
	TestTrue(TEXT("Optional automatic fill succeeds"), Filled.Status == EWFCResultStatus::Success);
	if (Filled.Cells.Num() == 300)
		TestEqual(TEXT("Fill respects reserved ground"), Filled.Cells[24].TileId, FName(TEXT("Ground")));
	Add(FIntPoint(0, 0), EWFCPaintKind::Building, 1);
	TestTrue(TEXT("Report a building with no access"),
	         FWFCSolver::Solve(Set.Get(), S).Status == EWFCResultStatus::InvalidInput);
	S.PaintedCells.Pop();
	const auto Duplicate=S.PaintedCells[0];
	S.PaintedCells.Add(Duplicate);
	TestTrue(TEXT("Reject ambiguous duplicate painted cells"),
	         FWFCSolver::Solve(Set.Get(), S).Status == EWFCResultStatus::InvalidInput);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWFCRasterTest, "SpiritWFC.Painted.ContinuousPixelStrokes",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWFCRasterTest::RunTest(const FString&)
{
	for (const auto B : TArray<FIntPoint>{{13, 7}, {1, 13}, {13, 13}, {0, 0}})
	{
		TArray<FIntPoint> Cells;
		TestTrue(TEXT("Rasterize bounded stroke"), FWFCPaintRaster::Stroke(FIntPoint::ZeroValue, B, 14, 14, Cells));
		TestEqual(TEXT("Exact endpoint"), Cells.Last(), B);
		for (int32 I = 1; I < Cells.Num(); ++I)
			TestEqual(TEXT("Fast/diagonal strokes remain four-connected"),
			          FMath::Abs(Cells[I].X - Cells[I - 1].X) + FMath::Abs(Cells[I].Y - Cells[I - 1].Y), 1);
	}
	TArray<FIntPoint> Cells;
	TestFalse(TEXT("Reject enormous out-of-bounds endpoints before looping"),
	          FWFCPaintRaster::Stroke(FIntPoint(-100000000, 0), FIntPoint(1, 1), 14, 14, Cells));
	return true;
}
#endif
