#include "WFCLayoutPlanner.h"

namespace
{
bool BuildMaze(const FWFCGenerationSettings& Settings, TArray<FWFCCellRule>& Rules, FString& Error)
{
    const FIntVector Size = Settings.Dimensions();
    if (Size.X < 9 || Size.Y < 9 || Size.X > 64 || Size.Y > 64 ||
        Size.Z < 1 || Size.Z > 3 || int64(Size.X) * Size.Y * Size.Z > 4096)
    {
        Error = TEXT("Maze needs a grid of at least 9x9 within the 4096-cell budget.");
        return false;
    }

    const FIntPoint Directions[] = {{1,0}, {-1,0}, {0,1}, {0,-1}};
    const FIntPoint Entrance(2,2);
    const FIntPoint FirstRoom(5,3);
    const auto Index = [Size](FIntPoint Point) { return Point.X + Size.X * Point.Y; };
    const auto InInterior = [Size](FIntPoint Point)
    {
        return Point.X > 0 && Point.Y > 0 && Point.X < Size.X - 1 && Point.Y < Size.Y - 1;
    };

    TBitArray<> Open(false, Size.X * Size.Y);
    for (int32 Y = 1; Y <= 3; ++Y)
        for (int32 X = 1; X <= 3; ++X)
            Open[Index({X,Y})] = true;
    Open[Index({4,3})] = true;
    Open[Index(FirstRoom)] = true;

    // Seeded depth-first carving visits each room and opens only one passage into it.
    FRandomStream Random(Settings.Seed);
    TArray<FIntPoint> Stack{FirstRoom};
    while (!Stack.IsEmpty())
    {
        const FIntPoint Current = Stack.Last();
        TArray<FIntPoint, TInlineAllocator<4>> Available;
        for (const FIntPoint Step : Directions)
        {
            const FIntPoint Next = Current + Step * 2;
            if (InInterior(Next) && !Open[Index(Next)])
                Available.Add(Step);
        }
        if (Available.IsEmpty())
        {
            Stack.Pop(EAllowShrinking::No);
            continue;
        }
        const FIntPoint Step = Available[Random.RandRange(0, Available.Num() - 1)];
        Open[Index(Current + Step)] = true;
        Open[Index(Current + Step * 2)] = true;
        Stack.Add(Current + Step * 2);
    }

    // A breadth-first search puts the goal at the farthest reachable dead end.
    TBitArray<> Seen(false, Open.Num());
    TArray<FIntPoint> Queue{Entrance};
    Seen[Index(Entrance)] = true;
    FIntPoint Exit = FirstRoom;
    for (int32 Head = 0; Head < Queue.Num(); ++Head)
    {
        const FIntPoint Current = Queue[Head];
        int32 Neighbors = 0;
        for (const FIntPoint Step : Directions)
        {
            const FIntPoint Next = Current + Step;
            if (!InInterior(Next) || !Open[Index(Next)])
                continue;
            ++Neighbors;
            if (!Seen[Index(Next)])
            {
                Seen[Index(Next)] = true;
                Queue.Add(Next);
            }
        }
        if (Neighbors == 1)
            Exit = Current;
    }

    Rules.Reserve(Size.X * Size.Y * Size.Z);
    for (int32 Z = 0; Z < Size.Z; ++Z)
        for (int32 Y = 0; Y < Size.Y; ++Y)
            for (int32 X = 0; X < Size.X; ++X)
            {
                const FIntPoint Point(X,Y);
                FWFCCellRule& Rule = Rules.AddDefaulted_GetRef();
                Rule.Coordinate = {X,Y,Z};
                Rule.QuarterTurns = 0;
                if (Z > 0) Rule.RequiredTag = TEXT("Air");
                else if (Point == Entrance) Rule.RequiredTag = TEXT("MazeEntry");
                else if (Point == Exit) Rule.RequiredTag = TEXT("MazeExit");
                else if (Open[Index(Point)]) Rule.RequiredTag = TEXT("MazePath");
                else Rule.RequiredTag = TEXT("MazeWall");
            }
    return true;
}
}

bool FWFCLayoutPlanner::Build(const FWFCGenerationSettings& Settings, TArray<FWFCCellRule>& Rules, FString& Error)
{
	Rules.Reset();
	Error.Reset();
	if (Settings.LayoutPreset == EWFCLayoutPreset::Maze)
		return BuildMaze(Settings, Rules, Error);
	if (Settings.LayoutPreset == EWFCLayoutPreset::Free)
		return true;
	const bool Painted = Settings.LayoutPreset == EWFCLayoutPreset::Painted;
	const auto Size = Settings.Dimensions();
	const int32 MinSize = Painted ? 3 : 9;
	if ((Settings.Mode != EWFCMapMode::Planar && Settings.Mode != EWFCMapMode::Volume) ||
	    (!Painted && Settings.LayoutPreset != EWFCLayoutPreset::Settlement) || Size.X < MinSize || Size.Y < MinSize ||
	    Size.X > 64 || Size.Y > 64 || Size.Z < 1 || Size.Z > 3 || int64(Size.X) * Size.Y * Size.Z > 4096 ||
	    !FMath::IsFinite(Settings.BuildingDensity) || Settings.BuildingDensity < 0 || Settings.BuildingDensity > 1 ||
        Settings.MaxBuildingStories<1 || Settings.MaxBuildingStories>3 || !FMath::IsFinite(Settings.CompoundBuildingChance) ||
        Settings.CompoundBuildingChance<0 || Settings.CompoundBuildingChance>1)
	{
		Error = TEXT("Invalid layout: Painted needs >=3x3, Settlement >=9x9, at most 64 per axis/3 layers/4096 cells; "
		             "density 0..1.");
		return false;
	}
	const FIntPoint Step[4] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
	const int32 W = Size.X, H = Size.Y;
	auto Index = [&](int32 X, int32 Y) { return X + W * Y; };
	auto Inside = [&](int32 X, int32 Y) { return X >= 0 && Y >= 0 && X < W && Y < H; };
	TArray<uint8> Street, Plaza;
	Street.Init(0, W * H);
	Plaza.Init(0, W * H);
	TArray<EWFCPaintKind> Fixed;
	Fixed.Init(EWFCPaintKind::Unspecified, W * H);
	TArray<int32> FixedStories;
	FixedStories.Init(0, W * H);
	FRandomStream Random(int32(uint32(Settings.Seed) ^ 0xA511E9B3u));
	if (Painted)
	{
		if (Settings.PaintedCells.Num() > W * H)
		{
			Error = TEXT("Too many painted cells.");
			return false;
		}
		TSet<int32> Seen;
		for (const auto& Cell : Settings.PaintedCells)
		{
			const int32 X = Cell.Coordinate.X, Y = Cell.Coordinate.Y;
			if (!Inside(X, Y) || Cell.Stories < 0 || Cell.Stories > 3 ||
			    uint8(Cell.Kind) > uint8(EWFCPaintKind::Ground))
			{
				Error = TEXT("Invalid painted cell coordinate, kind or story count.");
				return false;
			}
			const int32 I = Index(X, Y);
			if (Seen.Contains(I))
			{
				Error = TEXT("Duplicate painted coordinate.");
				return false;
			}
			Seen.Add(I);
			if (Cell.Kind == EWFCPaintKind::Building && Cell.Stories > Size.Z)
			{
				Error = TEXT("Painted building height exceeds the map layer count.");
				return false;
			}
			if (!Cell.BuildingTransform.IsValid() ||
			    (!Cell.BuildingMesh.IsNull() && (Cell.Kind != EWFCPaintKind::Building || Cell.Stories != 1)))
			{
				Error = TEXT("A complete building mesh requires a Building cell with exactly one logical story and a "
				             "valid transform.");
				return false;
			}
			Fixed[I] = Cell.Kind;
			FixedStories[I] = Cell.Stories;
			Street[I] = Cell.Kind == EWFCPaintKind::Road || Cell.Kind == EWFCPaintKind::Plaza;
			Plaza[I] = Cell.Kind == EWFCPaintKind::Plaza;
		}
	}
	else
	{
		const int32 CX = Random.RandRange(FMath::Max(3, W / 3), FMath::Min(W - 4, 2 * W / 3));
		const int32 CY = Random.RandRange(FMath::Max(3, H / 3), FMath::Min(H - 4, 2 * H / 3));
		for (int32 Y = 1; Y < H - 1; ++Y)
			for (int32 X = 1; X < W - 1; ++X)
			{
				const bool Square = FMath::Abs(X - CX) <= 1 && FMath::Abs(Y - CY) <= 1;
				Plaza[Index(X, Y)] = Square;
				Street[Index(X, Y)] = Square || X == 1 || X == W - 2 || Y == 1 || Y == H - 2 || X == CX || Y == CY;
			}
	}
	TArray<int32> Heights, Turns;
	Heights.Init(0, W * H);
	Turns.Init(0, W * H);
	for (int32 Y = Painted ? 0 : 2; Y < (Painted ? H : H - 2); ++Y)
		for (int32 X = Painted ? 0 : 2; X < (Painted ? W : W - 2); ++X)
		{
			const int32 I = Index(X, Y);
			if (Street[I] || Fixed[I] == EWFCPaintKind::Ground)
				continue;
			const bool Explicit = Fixed[I] == EWFCPaintKind::Building;
			if (Painted && !Explicit && !Settings.bFillUnpaintedBuildings)
				continue;
			TArray<int32> Fronts;
			for (int32 D = 0; D < 4; ++D)
				if (Inside(X + Step[D].X, Y + Step[D].Y) && Street[Index(X + Step[D].X, Y + Step[D].Y)])
					Fronts.Add(D);
			if (Fronts.IsEmpty())
			{
				if (Explicit)
				{
					Error = FString::Printf(TEXT("Painted building (%d,%d) must touch a road or plaza."), X, Y);
					return false;
				}
				continue;
			}
			if (!Explicit && Random.FRand() >= Settings.BuildingDensity)
				continue;
			const int32 FrontYaw[4] = {1, 3, 2, 0};
			Turns[I] = FrontYaw[Fronts[Random.RandRange(0, Fronts.Num() - 1)]];
			Heights[I] = Explicit && FixedStories[I] > 0 ? FixedStories[I] : Random.RandRange(1, FMath::Min(Size.Z,Settings.MaxBuildingStories));
		}
    // Reserve both cells before emitting rules, so pairs never consume roads or overlap.
    TArray<uint8> Compound;Compound.Init(0,W*H);
    if (!Painted && Settings.CompoundBuildingChance>0.f)
    {
        const FIntPoint Right[4]={{1,0},{0,1},{-1,0},{0,-1}};
        for (int32 Y=2;Y<H-2;++Y) for (int32 X=2;X<W-2;++X)
        {
            const int32 Main=Index(X,Y);if (Heights[Main]==0 || Compound[Main]) continue;
            const FIntPoint Direction=Right[Turns[Main]];
            const int32 WX=X+Direction.X,WY=Y+Direction.Y;
            if (WX<2 || WY<2 || WX>=W-2 || WY>=H-2) continue;
            const int32 Wing=Index(WX,WY);
            if (Heights[Wing]==0 || Compound[Wing] || Street[Wing] || Random.FRand()>=Settings.CompoundBuildingChance) continue;
            Compound[Main]=1;Compound[Wing]=2;
            Heights[Main]=FMath::Min(2,FMath::Min(Size.Z,Settings.MaxBuildingStories));Heights[Wing]=1;
            Turns[Wing]=Turns[Main];
        }
    }
	Rules.Reserve(W * H * Size.Z);
	for (int32 Z = 0; Z < Size.Z; ++Z)
		for (int32 Y = 0; Y < H; ++Y)
			for (int32 X = 0; X < W; ++X)
			{
				FWFCCellRule Rule;
				Rule.Coordinate = FIntVector(X, Y, Z);
				const int32 I = Index(X, Y), Height = Heights[I];
				if (Height > 0 && Z < Height)
				{
					Rule.RequiredTag = Height == 1
					                       ? TEXT("House")
					                       : (Z == 0 ? TEXT("Base") : (Z == Height - 1 ? TEXT("Top") : TEXT("Middle")));
                    if (Z==0 && Compound[I])
                        Rule.RequiredTag=Compound[I]==2 ? TEXT("CompoundWing") : Height==1 ? TEXT("CompoundHouse") : TEXT("CompoundBase");
					Rule.QuarterTurns = Turns[I];
				}
				else if (Z > 0)
					Rule.RequiredTag = TEXT("Air");
				else if (Street[I])
				{
					int32 Mask = 0;
					for (int32 D = 0; D < 4; ++D)
						if (Inside(X + Step[D].X, Y + Step[D].Y) && Street[Index(X + Step[D].X, Y + Step[D].Y)])
							Mask |= 1 << D;
					Rule.RequiredTag = FName(
					    *(FString(Plaza[I] ? TEXT("Plaza_") : TEXT("Road_")) + FString::Printf(TEXT("%02d"), Mask)));
					Rule.QuarterTurns = 0;
				}
				else
				{
					Rule.RequiredTag = TEXT("Ground");
					Rule.QuarterTurns = 0;
				}
				Rules.Add(Rule);
			}
	return true;
}
