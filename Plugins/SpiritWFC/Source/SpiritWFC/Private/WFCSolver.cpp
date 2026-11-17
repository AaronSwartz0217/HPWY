#include "WFCSolver.h"
#include "HAL/PlatformTime.h"
#include "WFCLayoutPlanner.h"

namespace
{
const FIntVector Directions[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
const int32 Opposite[6] = {1, 0, 3, 2, 5, 4};
struct FMask
{
	uint64 Words[4] = {0, 0, 0, 0};
	void Add(int32 V)
	{
		Words[V / 64] |= uint64(1) << (V % 64);
	}
	bool Contains(int32 V) const
	{
		return (Words[V / 64] & (uint64(1) << (V % 64))) != 0;
	}
	bool Empty() const
	{
		return (Words[0] | Words[1] | Words[2] | Words[3]) == 0;
	}
	void Union(const FMask& Other)
	{
		for (int32 I = 0; I < 4; ++I)
			Words[I] |= Other.Words[I];
	}
	bool Intersect(const FMask& Other)
	{
		bool Changed = false;
		for (int32 I = 0; I < 4; ++I)
		{
			const uint64 Next = Words[I] & Other.Words[I];
			Changed |= Next != Words[I];
			Words[I] = Next;
		}
		return Changed;
	}
};
struct FVariant
{
	int32 TileIndex = 0, Turns = 0;
	double Weight = 1.;
	FWFCSockets Sockets;
};
struct FModel
{
	const UWFCTileSet* Set = nullptr;
	FIntVector Size;
	int32 CellCount = 0, DirectionCount = 4;
	TArray<FVariant> Variants;
	TArray<FMask> Compatible[6];
	TArray<FMask> PinnedDomains;
	int32 Index(FIntVector P) const
	{
		return P.X + Size.X * (P.Y + Size.Y * P.Z);
	}
	FIntVector Coordinate(int32 I) const
	{
		return FIntVector(I % Size.X, (I / Size.X) % Size.Y, I / (Size.X * Size.Y));
	}
	bool Inside(FIntVector P) const
	{
		return P.X >= 0 && P.Y >= 0 && P.Z >= 0 && P.X < Size.X && P.Y < Size.Y && P.Z < Size.Z;
	}
	bool Fits(int32 V, FIntVector P, const FWFCGenerationSettings& Settings) const
	{
		const auto& Variant = Variants[V];
		const auto& Tile = Set->Tiles[Variant.TileIndex];
		if (P.Z < Tile.MinLayer || P.Z > Tile.MaxLayer || (Tile.bBottomLayerOnly && P.Z != 0) ||
		    (Tile.bTopLayerOnly && P.Z != Size.Z - 1))
			return false;
		for (int32 D = 0; D < DirectionCount; ++D)
		{
			if (Inside(P + Directions[D]))
				continue;
			if (D < 4 && Settings.bConstrainHorizontalBoundary &&
			    Variant.Sockets.Get(D) != Settings.HorizontalBoundarySocket)
				return false;
			if (D == 4 && Settings.bConstrainVerticalBoundary && Variant.Sockets.Up != Settings.TopBoundarySocket)
				return false;
			if (D == 5 && Settings.bConstrainVerticalBoundary && Variant.Sockets.Down != Settings.BottomBoundarySocket)
				return false;
		}
		return PinnedDomains[Index(P)].Contains(V);
	}
};
bool Compile(const UWFCTileSet* Set, const FWFCGenerationSettings& Settings, FModel& Model, FString& Error)
{
	if (!Set)
	{
		Error = TEXT("TileSet is null.");
		return false;
	}
	if (!Set->Validate(Error))
		return false;
	if (Settings.Mode != EWFCMapMode::Planar && Settings.Mode != EWFCMapMode::Volume)
	{
		Error = TEXT("Unknown map mode.");
		return false;
	}
	const auto Size = Settings.Dimensions();
	const int64 Count = int64(Size.X) * Size.Y * Size.Z;
	if (Size.X < 1 || Size.X > 64 || Size.Y < 1 || Size.Y > 64 || Size.Z < 1 || Size.Z > 3 || Count > 4096)
	{
		Error = TEXT("Grid must be 1..64 by 1..64, at most 3 layers and 4096 total cells.");
		return false;
	}
	if (Settings.MaxRestarts < 0 || Settings.MaxRestarts > 32 || Settings.MaxPropagationSteps < 1 ||
	    Settings.MaxPropagationSteps > 20000000)
	{
		Error = TEXT("Invalid retry/work budget. Restarts 0..32, propagation steps 1..20000000.");
		return false;
	}
	Model.Set = Set;
	Model.Size = Size;
	Model.CellCount = int32(Count);
	Model.DirectionCount = Settings.Mode == EWFCMapMode::Volume ? 6 : 4;
	for (int32 I = 0; I < Set->Tiles.Num(); ++I)
	{
		const auto& Tile = Set->Tiles[I];
		if (!Tile.bEnabled)
			continue;
		for (int32 Turn : Tile.AllowedYawQuarterTurns)
		{
			FVariant V;
			V.TileIndex = I;
			V.Turns = Turn;
			V.Sockets = Tile.Sockets.Rotated(Turn);
			// Weight describes a prototype's total probability, independent of how many
			// rotated copies it has. Splitting prevents rotation-rich modules dominating.
			V.Weight = double(Tile.Weight) / Tile.AllowedYawQuarterTurns.Num();
			Model.Variants.Add(V);
		}
	}
	if (Settings.PinnedCells.Num() > Model.CellCount)
	{
		Error = TEXT("Pinned entry count cannot exceed the grid cell count.");
		return false;
	}
	FMask All;
	for (int32 V = 0; V < Model.Variants.Num(); ++V)
		All.Add(V);
	Model.PinnedDomains.Init(All, Model.CellCount);
	for (const auto& Pin : Settings.PinnedCells)
	{
		if (!Model.Inside(Pin.Coordinate) || Pin.QuarterTurns < -1 || Pin.QuarterTurns > 3)
		{
			Error = TEXT("A pinned cell is outside the grid or has an invalid rotation.");
			return false;
		}
		FMask Allowed;
		for (int32 V = 0; V < Model.Variants.Num(); ++V)
			if (Set->Tiles[Model.Variants[V].TileIndex].TileId == Pin.TileId &&
			    (Pin.QuarterTurns < 0 || Model.Variants[V].Turns == Pin.QuarterTurns))
				Allowed.Add(V);
		if (Allowed.Empty())
		{
			Error = TEXT("A pinned tile/rotation is missing or disabled.");
			return false;
		}
		Model.PinnedDomains[Model.Index(Pin.Coordinate)].Intersect(Allowed);
	}
	TArray<FWFCCellRule> Planned;
	if (!FWFCLayoutPlanner::Build(Settings, Planned, Error))
		return false;
	for (const auto& Rule : Planned)
	{
		FMask Allowed;
		for (int32 V = 0; V < Model.Variants.Num(); ++V)
			if (Set->Tiles[Model.Variants[V].TileIndex].Tags.Contains(Rule.RequiredTag) &&
			    (Rule.QuarterTurns < 0 || Rule.QuarterTurns == Model.Variants[V].Turns))
				Allowed.Add(V);
		if (Allowed.Empty())
		{
			Error = FString::Printf(TEXT("Layout requires tag %s with rotation %d."), *Rule.RequiredTag.ToString(),
			                        Rule.QuarterTurns);
			return false;
		}
		Model.PinnedDomains[Model.Index(Rule.Coordinate)].Intersect(Allowed);
	}
	for (int32 D = 0; D < Model.DirectionCount; ++D)
	{
		Model.Compatible[D].SetNum(Model.Variants.Num());
		for (int32 A = 0; A < Model.Variants.Num(); ++A)
			for (int32 B = 0; B < Model.Variants.Num(); ++B)
				if (Model.Variants[A].Sockets.Get(D) == Model.Variants[B].Sockets.Get(Opposite[D]))
					Model.Compatible[D][A].Add(B);
	}
	return true;
}
bool CheckSolution(const FModel& Model, const FWFCGenerationSettings& Settings, const TArray<FWFCCellPlacement>& Cells,
                   FString& Error, int32& Components)
{
	Components = 0;
	if (Cells.Num() != Model.CellCount)
	{
		Error = TEXT("The solution has an incorrect cell count.");
		return false;
	}
	TArray<int32> Chosen;
	Chosen.Init(INDEX_NONE, Model.CellCount);
	for (int32 I = 0; I < Cells.Num(); ++I)
	{
		const auto& Cell = Cells[I];
		if (Cell.Coordinate != Model.Coordinate(I))
		{
			Error = TEXT("Cells must use unique X-fastest row-major coordinates.");
			return false;
		}
		for (int32 V = 0; V < Model.Variants.Num(); ++V)
		{
			const auto& Variant = Model.Variants[V];
			if (Variant.TileIndex == Cell.TileIndex && Variant.Turns == Cell.QuarterTurns &&
			    Model.Set->Tiles[Variant.TileIndex].TileId == Cell.TileId)
			{
				Chosen[I] = V;
				break;
			}
		}
		if (Chosen[I] == INDEX_NONE || !Model.Fits(Chosen[I], Cell.Coordinate, Settings))
		{
			Error = TEXT("A cell violates its tile identity, layer, boundary or pinned constraint.");
			return false;
		}
	}
	for (int32 I = 0; I < Cells.Num(); ++I)
		for (int32 D = 0; D < Model.DirectionCount; ++D)
		{
			const auto P = Cells[I].Coordinate + Directions[D];
			if (Model.Inside(P) && !Model.Compatible[D][Chosen[I]].Contains(Chosen[Model.Index(P)]))
			{
				Error = TEXT("Neighbor sockets do not match.");
				return false;
			}
		}
	TArray<uint8> Seen;
	Seen.Init(0, Cells.Num());
	for (int32 Start = 0; Start < Cells.Num(); ++Start)
	{
		if (Seen[Start] || !Model.Set->Tiles[Cells[Start].TileIndex].bWalkable)
			continue;
		++Components;
		TArray<int32> Queue;
		Queue.Add(Start);
		Seen[Start] = 1;
		for (int32 Head = 0; Head < Queue.Num(); ++Head)
		{
			const int32 I = Queue[Head];
			for (int32 D = 0; D < Model.DirectionCount; ++D)
			{
				const auto P = Cells[I].Coordinate + Directions[D];
				if (!Model.Inside(P))
					continue;
				const int32 Next = Model.Index(P);
				if (!Seen[Next] && Model.Set->Tiles[Cells[Next].TileIndex].bWalkable &&
				    Model.Set->TraversableSockets.Contains(Model.Variants[Chosen[I]].Sockets.Get(D)))
				{
					Seen[Next] = 1;
					Queue.Add(Next);
				}
			}
		}
	}
	if (Settings.bRequireConnectedWalkableCells && Components != 1)
	{
		Error = FString::Printf(
		    TEXT("Expected one traversable component; found %d. Add doors/stair pins or adjust sockets."), Components);
		return false;
	}
	return true;
}
} // namespace

bool FWFCSolver::ValidateSolution(const UWFCTileSet* TileSet, const FWFCGenerationSettings& Settings,
                                  const TArray<FWFCCellPlacement>& Cells, FString& Error, int32& WalkableComponents)
{
	FModel Model;
	WalkableComponents = 0;
	Error.Reset();
	return Compile(TileSet, Settings, Model, Error) && CheckSolution(Model, Settings, Cells, Error, WalkableComponents);
}
FWFCGenerationResult FWFCSolver::Solve(const UWFCTileSet* TileSet, const FWFCGenerationSettings& Settings)
{
	const double Started = FPlatformTime::Seconds();
	FWFCGenerationResult Result;
	Result.RequestedSeed = Settings.Seed;
	FModel Model;
	auto Finish = [&]()
	{
		Result.ElapsedMilliseconds = (FPlatformTime::Seconds() - Started) * 1000.;
		return Result;
	};
	if (!Compile(TileSet, Settings, Model, Result.Message))
		return Finish();
	Result.Dimensions = Model.Size;
	for (int32 Attempt = 0; Attempt <= Settings.MaxRestarts; ++Attempt)
	{
		Result.Attempts = Attempt + 1;
		Result.Decisions = 0;
		Result.Cells.Reset();
		Result.FailedCell = FIntVector(-1);
		Result.WalkableComponents = 0;
		Result.EffectiveSeed = int32(uint32(Settings.Seed) + uint32(Attempt) * 0x9E3779B9u);
		FRandomStream Random(Result.EffectiveSeed);
		TArray<FMask> Domains;
		Domains.SetNum(Model.CellCount);
		TArray<int32> Counts;
		Counts.SetNumZeroed(Model.CellCount);
		TArray<double> Entropies;
		Entropies.SetNumZeroed(Model.CellCount);
		TArray<int32> Queue;
		int32 Head = 0;
		TArray<uint8> Queued;
		Queued.Init(0, Model.CellCount);
		bool Contradiction = false, BudgetExceeded = false;
		auto Enqueue = [&](int32 I)
		{
			if (!Queued[I])
			{
				Queue.Add(I);
				Queued[I] = 1;
			}
		};
		auto Refresh = [&](int32 I)
		{
			int32 Count = 0;
			double Sum = 0, WeightedLog = 0;
			for (int32 V = 0; V < Model.Variants.Num(); ++V)
				if (Domains[I].Contains(V))
				{
					++Count;
					const double W = Model.Variants[V].Weight;
					Sum += W;
					WeightedLog += W * FMath::Loge(W);
				}
			Counts[I] = Count;
			Entropies[I] = Count > 1 ? FMath::Loge(Sum) - WeightedLog / Sum : 0.;
			if (!Count)
			{
				Contradiction = true;
				Result.FailedCell = Model.Coordinate(I);
			}
		};
		for (int32 I = 0; I < Model.CellCount; ++I)
		{
			for (int32 V = 0; V < Model.Variants.Num(); ++V)
				if (Model.Fits(V, Model.Coordinate(I), Settings))
					Domains[I].Add(V);
			Refresh(I);
			Enqueue(I);
		}
		auto Propagate = [&]()
		{
			while (Head < Queue.Num() && !Contradiction && !BudgetExceeded)
			{
				const int32 I = Queue[Head++];
				Queued[I] = 0;
				for (int32 D = 0; D < Model.DirectionCount; ++D)
				{
					const auto P = Model.Coordinate(I) + Directions[D];
					if (!Model.Inside(P))
						continue;
					FMask Supported;
					for (int32 V = 0; V < Model.Variants.Num(); ++V)
						if (Domains[I].Contains(V))
						{
							if (++Result.PropagationSteps > Settings.MaxPropagationSteps)
							{
								BudgetExceeded = true;
								Result.FailedCell = P;
								return;
							}
							Supported.Union(Model.Compatible[D][V]);
						}
					const int32 Next = Model.Index(P);
					if (Domains[Next].Intersect(Supported))
					{
						Refresh(Next);
						Enqueue(Next);
						if (Contradiction)
							return;
					}
				}
			}
			Queue.Reset();
			Head = 0;
		};
		Propagate();
		while (!Contradiction && !BudgetExceeded)
		{
			int32 Selected = INDEX_NONE;
			double Lowest = TNumericLimits<double>::Max();
			// Greedy variable ordering: choose the currently most constrained cell.
			// Tiny seeded noise breaks entropy ties reproducibly; it is not a global score.
			for (int32 I = 0; I < Model.CellCount; ++I)
				if (Counts[I] > 1)
				{
					const double H = Entropies[I] + double(Random.FRand()) * 1.e-9;
					if (H < Lowest)
					{
						Lowest = H;
						Selected = I;
					}
				}
			if (Selected == INDEX_NONE)
				break;
			double Sum = 0;
			for (int32 V = 0; V < Model.Variants.Num(); ++V)
				if (Domains[Selected].Contains(V))
					Sum += Model.Variants[V].Weight;
			double Pick = double(Random.FRand()) * Sum;
			int32 Chosen = INDEX_NONE;
			for (int32 V = 0; V < Model.Variants.Num(); ++V)
				if (Domains[Selected].Contains(V))
				{
					Chosen = V;
					Pick -= Model.Variants[V].Weight;
					if (Pick <= 0)
						break;
				}
			Domains[Selected] = FMask();
			Domains[Selected].Add(Chosen);
			Refresh(Selected);
			Enqueue(Selected);
			++Result.Decisions;
			Propagate();
		}
		if (BudgetExceeded)
		{
			Result.Status = EWFCResultStatus::BudgetExceeded;
			Result.Message =
			    TEXT("Global propagation budget exceeded. Reduce grid/variants or raise the bounded budget.");
			return Finish();
		}
		if (Contradiction)
		{
			Result.Status = EWFCResultStatus::Contradiction;
			Result.Message = TEXT("No candidate remains. Check sockets, boundaries, floor restrictions and pins.");
			continue;
		}
		TArray<FWFCCellPlacement> Candidate;
		Candidate.Reserve(Model.CellCount);
		for (int32 I = 0; I < Model.CellCount; ++I)
			for (int32 V = 0; V < Model.Variants.Num(); ++V)
				if (Domains[I].Contains(V))
				{
					FWFCCellPlacement Cell;
					Cell.Coordinate = Model.Coordinate(I);
					Cell.TileIndex = Model.Variants[V].TileIndex;
					Cell.TileId = TileSet->Tiles[Cell.TileIndex].TileId;
					Cell.QuarterTurns = Model.Variants[V].Turns;
					Candidate.Add(Cell);
					break;
				}
		if (!CheckSolution(Model, Settings, Candidate, Result.Message, Result.WalkableComponents))
		{
			Result.Status = EWFCResultStatus::Disconnected;
			continue;
		}
		Result.Status = EWFCResultStatus::Success;
		Result.Message = TEXT("All local constraints and requested connectivity checks passed.");
		Result.Cells = MoveTemp(Candidate);
		return Finish();
	}
	return Finish();
}
