#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"
#include "WFCSolver.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
FWFCTileDefinition Tile(FName Id, FName Horizontal = TEXT("Door"))
{
	FWFCTileDefinition T;
	T.TileId = Id;
	T.Sockets.PositiveX = T.Sockets.NegativeX = T.Sockets.PositiveY = T.Sockets.NegativeY = Horizontal;
	return T;
}
FWFCGenerationSettings OpenGrid(int32 Width = 8, int32 Depth = 8)
{
	FWFCGenerationSettings S;
	S.Width = Width;
	S.Depth = Depth;
	S.bConstrainHorizontalBoundary = false;
	return S;
}
} // namespace
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWFCPlanarTest, "SpiritWFC.Solver.PlanarDeterminismAndValidation",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWFCPlanarTest::RunTest(const FString&)
{
	TStrongObjectPtr<UWFCTileSet> Set(NewObject<UWFCTileSet>());
	auto A = Tile(TEXT("A"));
	A.AllowedYawQuarterTurns = {0, 1, 2, 3};
	auto B = Tile(TEXT("B"));
	B.Weight = 2;
	Set->Tiles = {A, B};
	auto Settings = OpenGrid();
	Settings.Layers = 99; // Ignored in planar mode, not an accidental 99-floor grid.
	for (int32 Seed = 0; Seed < 32; ++Seed)
	{
		Settings.Seed = Seed;
		const auto First = FWFCSolver::Solve(Set.Get(), Settings), Second = FWFCSolver::Solve(Set.Get(), Settings);
		TestTrue(TEXT("A planar solution exists"), First.Status == EWFCResultStatus::Success);
		TestEqual(TEXT("Exactly one floor"), First.Dimensions.Z, 1);
		TestEqual(TEXT("Complete cell count"), First.Cells.Num(), 64);
		TestEqual(TEXT("Reproducible attempt seed"), First.EffectiveSeed, Second.EffectiveSeed);
		if (First.Cells.Num() == Second.Cells.Num())
			for (int32 I = 0; I < First.Cells.Num(); ++I)
				TestTrue(TEXT("Same seed reproduces tile and orientation"),
				         First.Cells[I].TileId == Second.Cells[I].TileId &&
				             First.Cells[I].QuarterTurns == Second.Cells[I].QuarterTurns);
		FString Error;
		int32 Components = 0;
		TestTrue(TEXT("Independent result validation"),
		         FWFCSolver::ValidateSolution(Set.Get(), Settings, First.Cells, Error, Components));
	}
	auto Bad = FWFCSolver::Solve(Set.Get(), Settings).Cells;
	Bad[0].Coordinate = FIntVector(1, 1, 0);
	FString Error;
	int32 Components;
	TestFalse(TEXT("Reject duplicate/out of order saved cells"),
	          FWFCSolver::ValidateSolution(Set.Get(), Settings, Bad, Error, Components));
	Settings.Width = 65;
	TestTrue(TEXT("Runtime size limit"),
	         FWFCSolver::Solve(Set.Get(), Settings).Status == EWFCResultStatus::InvalidInput);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWFCVolumeTest, "SpiritWFC.Solver.ThreeLayersAndStairConnectivity",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWFCVolumeTest::RunTest(const FString&)
{
	TStrongObjectPtr<UWFCTileSet> Set(NewObject<UWFCTileSet>());
	auto Bottom = Tile(TEXT("Bottom"));
	Bottom.bBottomLayerOnly = true;
	Bottom.Sockets.Up = TEXT("Floor");
	auto Middle = Tile(TEXT("Middle"));
	Middle.MinLayer = Middle.MaxLayer = 1;
	Middle.Sockets.Down = Middle.Sockets.Up = TEXT("Floor");
	auto Top = Tile(TEXT("Top"));
	Top.bTopLayerOnly = true;
	Top.Sockets.Down = TEXT("Floor");
	auto StairBottom = Bottom;
	StairBottom.TileId = TEXT("StairBottom");
	StairBottom.Sockets.Up = TEXT("Stair");
	auto StairMiddle = Middle;
	StairMiddle.TileId = TEXT("StairMiddle");
	StairMiddle.Sockets.Down = StairMiddle.Sockets.Up = TEXT("Stair");
	auto StairTop = Top;
	StairTop.TileId = TEXT("StairTop");
	StairTop.Sockets.Down = TEXT("Stair");
	Set->Tiles = {Bottom, Middle, Top, StairBottom, StairMiddle, StairTop};
	auto S = OpenGrid(6, 6);
	S.Mode = EWFCMapMode::Volume;
	S.Layers = 3;
	FWFCPinnedCell Pin;
	Pin.Coordinate = FIntVector(2, 2, 0);
	Pin.TileId = TEXT("StairBottom");
	S.PinnedCells.Add(Pin);
	for (int32 Seed = 0; Seed < 24; ++Seed)
	{
		S.Seed = Seed;
		const auto Result = FWFCSolver::Solve(Set.Get(), S);
		TestTrue(TEXT("Connected three-floor solution"), Result.Status == EWFCResultStatus::Success);
		TestEqual(TEXT("108 cells"), Result.Cells.Num(), 108);
		TestEqual(TEXT("Exactly one traversable component"), Result.WalkableComponents, 1);
		if (Result.Cells.Num() == 108)
			TestEqual(TEXT("Stair alignment propagates to roof"), Result.Cells[2 + 6 * (2 + 6 * 2)].TileId,
			          FName(TEXT("StairTop")));
	}
	S.Layers = 4;
	TestTrue(TEXT("C++ refuses a fourth layer"),
	         FWFCSolver::Solve(Set.Get(), S).Status == EWFCResultStatus::InvalidInput);
	S.Layers = 3;
	S.PinnedCells.Reset();
	Set->Tiles = {Bottom, Middle, Top};
	S.MaxRestarts = 1;
	const auto Disconnected = FWFCSolver::Solve(Set.Get(), S);
	TestTrue(TEXT("Matching structural floors do not imply stairs"),
	         Disconnected.Status == EWFCResultStatus::Disconnected);
	TestEqual(TEXT("Three isolated floors"), Disconnected.WalkableComponents, 3);
	TestTrue(TEXT("Failure does not expose a partial map"), Disconnected.Cells.IsEmpty());
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWFCConstraintsTest, "SpiritWFC.Solver.RotationContradictionAndBudgets",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWFCConstraintsTest::RunTest(const FString&)
{
	FWFCSockets Ports;
	Ports.PositiveX = TEXT("East");
	Ports.NegativeX = TEXT("West");
	Ports.PositiveY = TEXT("North");
	Ports.NegativeY = TEXT("South");
	TestEqual(TEXT("Positive UE yaw moves east to north"), Ports.Rotated(1).PositiveY, FName(TEXT("East")));
	TestEqual(TEXT("Positive X receives old negative Y"), Ports.Rotated(1).PositiveX, FName(TEXT("South")));
	TestEqual(TEXT("Four rotations restore sockets"), Ports.Rotated(4).PositiveX, Ports.PositiveX);
	TStrongObjectPtr<UWFCTileSet> Set(NewObject<UWFCTileSet>());
	auto A = Tile(TEXT("A"));
	Set->Tiles = {A};
	auto S = OpenGrid();
	S.MaxPropagationSteps = 1;
	auto R = FWFCSolver::Solve(Set.Get(), S);
	TestTrue(TEXT("Bound propagation work across retries"), R.Status == EWFCResultStatus::BudgetExceeded);
	TestTrue(TEXT("No partial result on budget exhaustion"), R.Cells.IsEmpty());
	S = OpenGrid(2, 1);
	S.MaxRestarts = 2;
	A.Sockets.PositiveX = TEXT("Unmatchable");
	Set->Tiles = {A};
	R = FWFCSolver::Solve(Set.Get(), S);
	TestTrue(TEXT("Impossible neighboring sockets fail"), R.Status == EWFCResultStatus::Contradiction);
	TestEqual(TEXT("Exact bounded attempts"), R.Attempts, 3);
	A = Tile(TEXT("A"));
	Set->Tiles = {A};
	S = OpenGrid();
	FWFCPinnedCell Pin;
	Pin.TileId = TEXT("Missing");
	S.PinnedCells = {Pin};
	TestTrue(TEXT("Missing pinned prototype is invalid input"),
	         FWFCSolver::Solve(Set.Get(), S).Status == EWFCResultStatus::InvalidInput);
	S.PinnedCells.Reset();
	S.bConstrainHorizontalBoundary = true;
	TestTrue(TEXT("Exterior boundary participates in solving"),
	         FWFCSolver::Solve(Set.Get(), S).Status == EWFCResultStatus::Contradiction);
	S = OpenGrid();
	Set->Tiles[0].Weight = 0;
	TestTrue(TEXT("Zero weight rejected"), FWFCSolver::Solve(Set.Get(), S).Status == EWFCResultStatus::InvalidInput);
	Set->Tiles = {A, A};
	TestTrue(TEXT("Duplicate IDs rejected"), FWFCSolver::Solve(Set.Get(), S).Status == EWFCResultStatus::InvalidInput);
	Set->Tiles = {A};
	S.Mode = EWFCMapMode::Volume;
	S.Width = S.Depth = 64;
	S.Layers = 3;
	TestTrue(TEXT("Bound total cell allocation"),
	         FWFCSolver::Solve(Set.Get(), S).Status == EWFCResultStatus::InvalidInput);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWFCMaskCapacityTest, "SpiritWFC.Solver.FullCandidateMaskCapacity",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWFCMaskCapacityTest::RunTest(const FString&)
{
	TStrongObjectPtr<UWFCTileSet> Set(NewObject<UWFCTileSet>());
	for (int32 I = 0; I < 256; ++I)
		Set->Tiles.Add(Tile(FName(*FString::Printf(TEXT("Variant%d"), I))));
	auto S = OpenGrid(2, 1);
	FWFCPinnedCell Pin;
	Pin.Coordinate = FIntVector(1, 0, 0);
	Pin.TileId = TEXT("Variant255");
	S.PinnedCells.Add(Pin);
	const auto Result = FWFCSolver::Solve(Set.Get(), S);
	TestTrue(TEXT("Full four-word candidate mask is supported"), Result.Status == EWFCResultStatus::Success);
	if (Result.Cells.Num() == 2)
		TestEqual(TEXT("Pinned variant in the highest mask word survives propagation"), Result.Cells[1].TileId,
		          Pin.TileId);
	Set->Tiles.Add(Tile(TEXT("Overflow")));
	TestTrue(TEXT("A 257th variant is rejected before allocation"),
	         FWFCSolver::Solve(Set.Get(), S).Status == EWFCResultStatus::InvalidInput);
	return true;
}
#endif
