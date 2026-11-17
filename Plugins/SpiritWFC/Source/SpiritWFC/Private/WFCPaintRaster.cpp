#include "WFCPaintRaster.h"
bool FWFCPaintRaster::Stroke(FIntPoint A, FIntPoint B, int32 Width, int32 Depth, TArray<FIntPoint>& Cells)
{
	Cells.Reset();
	if (Width < 1 || Depth < 1 || Width > 64 || Depth > 64 || A.X < 0 || A.Y < 0 || B.X < 0 || B.Y < 0 ||
	    A.X >= Width || B.X >= Width || A.Y >= Depth || B.Y >= Depth)
		return false;
	int32 X = A.X, Y = A.Y;
	const int32 DX = FMath::Abs(B.X - X), DY = FMath::Abs(B.Y - Y);
	const int32 SX = X < B.X ? 1 : -1, SY = Y < B.Y ? 1 : -1;
	int32 Error = DX - DY;
	Cells.Add(A);
	while (X != B.X || Y != B.Y)
	{
		const int32 Twice = 2 * Error, OldX = X, OldY = Y;
		if (Twice > -DY)
		{
			Error -= DY;
			X += SX;
		}
		if (Twice < DX)
		{
			Error += DX;
			Y += SY;
		}
		if (X != OldX && Y != OldY)
			Cells.Add(FIntPoint(X, OldY));
		Cells.Add(FIntPoint(X, Y));
	}
	return true;
}
