#include "WFCTileSet.h"

FName FWFCSockets::Get(int32 Direction) const
{
	switch (Direction)
	{
	case 0:
		return PositiveX;
	case 1:
		return NegativeX;
	case 2:
		return PositiveY;
	case 3:
		return NegativeY;
	case 4:
		return Up;
	case 5:
		return Down;
	default:
		return NAME_None;
	}
}
FWFCSockets FWFCSockets::Rotated(int32 QuarterTurns) const
{
	FWFCSockets Result = *this;
	for (int32 Turn = 0; Turn < ((QuarterTurns % 4 + 4) % 4); ++Turn)
	{
		const auto Previous = Result;
		Result.PositiveX = Previous.NegativeY;
		Result.NegativeX = Previous.PositiveY;
		Result.PositiveY = Previous.PositiveX;
		Result.NegativeY = Previous.NegativeX;
	}
	return Result;
}
bool UWFCTileSet::Validate(FString& Error) const
{
	Error.Reset();
	if (CellSize.ContainsNaN() || CellSize.GetMin() <= 0 || CellSize.GetMax() > 1000000)
	{
		Error = TEXT("CellSize must have finite positive components <= 1000000 cm.");
		return false;
	}
	TSet<FName> Ids;
	int32 Variants = 0;
	for (const auto& Tile : Tiles)
	{
		if (Tile.TileId.IsNone() || Ids.Contains(Tile.TileId))
		{
			Error = TEXT("Every TileId must be nonempty and unique, including disabled entries.");
			return false;
		}
		Ids.Add(Tile.TileId);
		if (!Tile.bEnabled)
			continue;
		if (!FMath::IsFinite(Tile.Weight) || Tile.Weight <= 0 || Tile.AllowedYawQuarterTurns.IsEmpty())
		{
			Error = TEXT("Enabled tiles need a positive finite weight and at least one rotation.");
			return false;
		}
		TSet<int32> Turns;
		for (int32 Turn : Tile.AllowedYawQuarterTurns)
		{
			if (Turn < 0 || Turn > 3 || Turns.Contains(Turn))
			{
				Error = TEXT("Rotations must be unique integers 0..3.");
				return false;
			}
			Turns.Add(Turn);
		}
		if (Tile.MinLayer < 0 || Tile.MaxLayer > 2 || Tile.MinLayer > Tile.MaxLayer || !Tile.MeshTransform.IsValid())
		{
			Error = TEXT("Invalid layer range (0..2) or nonfinite mesh transform.");
			return false;
		}
		for (int32 Direction = 0; Direction < 6; ++Direction)
			if (Tile.Sockets.Get(Direction).IsNone())
			{
				Error = TEXT("All six socket names must be explicit. Use Closed/Ground/Sky rather than None.");
				return false;
			}
		Variants += Turns.Num();
	}
	if (Variants < 1 || Variants > 256)
	{
		Error = TEXT("The enabled set must compile to 1..256 rotated variants.");
		return false;
	}
	return true;
}
