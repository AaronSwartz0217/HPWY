#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "HPWH/HPWH_Types.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHPWHGridCoordinateTest,
	"CrashCourse.HPWH.GridCoordinates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHPWHGridCoordinateTest::RunTest(const FString& Parameters)
{
	const float CellSize = 800.f;

	TestEqual(TEXT("Cell 0 center is the geometric center"),
		HPWH_CellCenter(FIntPoint(0, 0), CellSize), FVector(400.f, 400.f, 0.f));
	TestEqual(TEXT("Negative cell center stays left/down"),
		HPWH_CellCenter(FIntPoint(-1, -1), CellSize), FVector(-400.f, -400.f, 0.f));

	bool bRoundTrip = true;
	for (int32 X = -12; X <= 12; ++X)
	{
		for (int32 Y = -12; Y <= 12; ++Y)
		{
			const FIntPoint Cell(X, Y);
			bRoundTrip &= HPWH_WorldToCell(HPWH_CellCenter(Cell, CellSize), CellSize) == Cell;
		}
	}
	TestTrue(TEXT("Every cell center maps back to its own cell"), bRoundTrip);

	TestEqual(TEXT("Cell min boundary belongs to the cell"),
		HPWH_WorldToCell(FVector(0.f, 0.f, 0.f), CellSize), FIntPoint(0, 0));
	TestEqual(TEXT("Cell max boundary belongs to the next cell"),
		HPWH_WorldToCell(FVector(800.f, 800.f, 0.f), CellSize), FIntPoint(1, 1));
	TestEqual(TEXT("Negative floor does not truncate toward zero"),
		HPWH_WorldToCell(FVector(-0.1f, -0.1f, 0.f), CellSize), FIntPoint(-1, -1));
	TestEqual(TEXT("Negative exact boundary stays in the lower cell"),
		HPWH_WorldToCell(FVector(-800.f, -800.f, 0.f), CellSize), FIntPoint(-1, -1));
	TestEqual(TEXT("Negative crossing uses floor"),
		HPWH_WorldToCell(FVector(-800.1f, -800.1f, 0.f), CellSize), FIntPoint(-2, -2));

	TestEqual(TEXT("Monster HP base is 10"), HPWH_MonsterMaxHealth(FIntPoint(0, 0)), 10.f);
	TestEqual(TEXT("Monster HP does not scale before ring 5"), HPWH_MonsterMaxHealth(FIntPoint(4, -4)), 10.f);
	TestEqual(TEXT("Monster HP adds 5 at ring 5"), HPWH_MonsterMaxHealth(FIntPoint(5, 0)), 15.f);
	TestEqual(TEXT("Monster HP uses the larger axis"), HPWH_MonsterMaxHealth(FIntPoint(-9, 3)), 15.f);
	TestEqual(TEXT("Monster HP adds 5 again at ring 10"), HPWH_MonsterMaxHealth(FIntPoint(10, 10)), 20.f);

	TestTrue(TEXT("Absolute x = 5 is a puzzle line"), HPWH_IsPuzzleLineCell(FIntPoint(5, 3)));
	TestTrue(TEXT("Absolute negative x = 15 is a puzzle line"), HPWH_IsPuzzleLineCell(FIntPoint(-15, 3)));
	TestTrue(TEXT("Absolute y = 15 is a puzzle line"), HPWH_IsPuzzleLineCell(FIntPoint(3, -15)));
	TestTrue(TEXT("Absolute x = 30 is a puzzle line"), HPWH_IsPuzzleLineCell(FIntPoint(30, -7)));
	TestTrue(TEXT("Later multiples of 30 stay puzzle lines"), HPWH_IsPuzzleLineCell(FIntPoint(-60, 3)));
	TestFalse(TEXT("25 is no longer a puzzle line"), HPWH_IsPuzzleLineCell(FIntPoint(25, 3)));
	TestFalse(TEXT("Non-multiple coordinates are not puzzle lines"), HPWH_IsPuzzleLineCell(FIntPoint(24, 3)));
	TestFalse(TEXT("Origin is never a puzzle line"), HPWH_IsPuzzleLineCell(FIntPoint(0, 0)));

	TSet<FIntPoint> Window;
	for (int32 DX = -2; DX <= 2; ++DX)
	{
		for (int32 DY = -2; DY <= 2; ++DY)
		{
			Window.Add(FIntPoint(DX, DY));
		}
	}
	TestEqual(TEXT("5x5 active window contains exactly 25 cells"), Window.Num(), 25);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHPWHTouchInputTest,
	"CrashCourse.HPWH.TouchInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHPWHTouchInputTest::RunTest(const FString& Parameters)
{
	const FVector2D Up = HPWH_TouchToMoveInput(FVector2D(0.f, -1.f));
	TestTrue(TEXT("上滑对应前进 +X"), FMath::IsNearlyEqual(Up.X, 0.f) && FMath::IsNearlyEqual(Up.Y, 1.f));

	const FVector2D Down = HPWH_TouchToMoveInput(FVector2D(0.f, 1.f));
	TestTrue(TEXT("下滑对应后退 -X"), FMath::IsNearlyEqual(Down.X, 0.f) && FMath::IsNearlyEqual(Down.Y, -1.f));

	const FVector2D Right = HPWH_TouchToMoveInput(FVector2D(1.f, 0.f));
	TestTrue(TEXT("右滑对应右移 +Y"), FMath::IsNearlyEqual(Right.X, 1.f) && FMath::IsNearlyEqual(Right.Y, 0.f));

	const FVector2D Left = HPWH_TouchToMoveInput(FVector2D(-1.f, 0.f));
	TestTrue(TEXT("左滑对应左移 -Y"), FMath::IsNearlyEqual(Left.X, -1.f) && FMath::IsNearlyEqual(Left.Y, 0.f));

	return true;
}

#endif
