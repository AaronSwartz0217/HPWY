#include "HPWH/HPWH_World.h"

#include "HPWH/HPWH_GameMode.h"
#include "HPWH/HPWH_Player.h"
#include "HPWH/HPWH_Enemies.h"
#include "HPWH/HPWH_SoulSpider.h"
#include "HPWH/HPWH_Crawler.h"
#include "HPWH/HPWH_POI.h"
#include "HPWH/HPWH_Companion.h"
#include "HPWH/HPWH_Prefabs.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "UObject/StrongObjectPtr.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AHPWH_CellChunk::AHPWH_CellChunk()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	Ground = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ground"));
	Ground->SetupAttachment(SceneRoot);
	Ground->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Ground->SetCollisionProfileName(TEXT("BlockAll"));
	Ground->SetCanEverAffectNavigation(false);

	Obstacles = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Obstacles"));
	Obstacles->SetupAttachment(SceneRoot);
	Obstacles->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Obstacles->SetCollisionProfileName(TEXT("BlockAll"));
	Obstacles->SetCastShadow(true);
	Obstacles->SetCanEverAffectNavigation(false);
}

namespace
{
	/** 导入模型旁通常有一份双面材质，用于修正反向面片导致的发黑。 */
	UMaterialInterface* LoadTwoSidedModelMaterial(const TCHAR* MeshPath)
	{
		const FString Path(MeshPath);
		const FString Marker(TEXT("/StaticMeshes/"));
		const int32 MarkerIndex = Path.Find(Marker);
		if (MarkerIndex == INDEX_NONE)
		{
			return nullptr;
		}
		const FString Folder = Path.Left(MarkerIndex);
		const FString Leaf = FPaths::GetCleanFilename(Folder);
		const FString MatPath = Folder + TEXT("/Materials/M_") + Leaf + TEXT("_TwoSided.M_") + Leaf + TEXT("_TwoSided");
		return LoadObject<UMaterialInterface>(nullptr, *MatPath);
	}

	/** 运行时生成旧报纸页面纹理；Variant 0/1/2 对应三种版式。 */
	UTexture2D* HPWHNewsprintTexture(int32 Variant)
	{
		Variant = FMath::Clamp(Variant, 0, 2);
		static TStrongObjectPtr<UTexture2D> Cached[3];
		if (Cached[Variant].IsValid())
		{
			return Cached[Variant].Get();
		}

		constexpr int32 Size = 512;
		UTexture2D* Texture = UTexture2D::CreateTransient(Size, Size, PF_B8G8R8A8);
		if (!Texture)
		{
			return nullptr;
		}
		Texture->SRGB = true;
		Texture->Filter = TF_Bilinear;
		Texture->AddressX = TA_Clamp;
		Texture->AddressY = TA_Clamp;

		FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
		uint8* Pixels = static_cast<uint8*>(Mip.BulkData.Lock(LOCK_READ_WRITE));
		FRandomStream Stream(0x4E5753 + Variant * 7919);

		auto Put = [Pixels, Size](int32 X, int32 Y, const FColor& Color)
		{
			if (X < 0 || Y < 0 || X >= Size || Y >= Size)
			{
				return;
			}
			uint8* Pixel = Pixels + (Y * Size + X) * 4;
			Pixel[0] = Color.B;
			Pixel[1] = Color.G;
			Pixel[2] = Color.R;
			Pixel[3] = 255;
		};

		// 纸张底色：三种版式用不同的泛黄程度与噪声。
		const int32 BaseR = Variant == 1 ? 218 : (Variant == 2 ? 199 : 214);
		const int32 BaseG = Variant == 1 ? 207 : (Variant == 2 ? 190 : 207);
		const int32 BaseB = Variant == 1 ? 191 : (Variant == 2 ? 171 : 188);
		const int32 NoiseRange = Variant == 2 ? 10 : 7;
		for (int32 Y = 0; Y < Size; ++Y)
		{
			for (int32 X = 0; X < Size; ++X)
			{
				const int32 Noise = Stream.RandRange(-NoiseRange, NoiseRange);
				Put(X, Y, FColor(
					static_cast<uint8>(FMath::Clamp(BaseR + Noise, 0, 255)),
					static_cast<uint8>(FMath::Clamp(BaseG + Noise, 0, 255)),
					static_cast<uint8>(FMath::Clamp(BaseB + Noise, 0, 255))));
			}
		}

		auto FillRect = [&Put](int32 X0, int32 Y0, int32 X1, int32 Y1, const FColor& Color)
		{
			for (int32 Y = Y0; Y < Y1; ++Y)
			{
				for (int32 X = X0; X < X1; ++X)
				{
					Put(X, Y, Color);
				}
			}
		};
		auto HLinePx = [&FillRect](int32 X0, int32 Y, int32 X1, int32 Thickness, const FColor& Color)
		{
			FillRect(X0, Y, X1, Y + Thickness, Color);
		};
		auto VLinePx = [&FillRect](int32 X, int32 Y0, int32 Y1, int32 Thickness, const FColor& Color)
		{
			FillRect(X, Y0, X + Thickness, Y1, Color);
		};

		if (Variant == 0)
		{

		// 报头、栏线、正文行与一张小图，拼出报纸版式。
		for (int32 Y = 26; Y < 84; ++Y)
		{
			for (int32 X = 34; X < 478; ++X)
			{
				Put(X, Y, FColor(44, 40, 36));
			}
		}
		for (int32 Y = 92; Y < 95; ++Y)
		{
			for (int32 X = 34; X < 478; ++X)
			{
				Put(X, Y, FColor(70, 64, 56));
			}
		}
		for (int32 Y = 104; Y < 106; ++Y)
		{
			for (int32 X = 34; X < 478; ++X)
			{
				Put(X, Y, FColor(96, 90, 80));
			}
		}

		const int32 ColumnX[3] = { 36, 188, 340 };
		const int32 ColumnWidth = 136;
		for (int32 Column = 0; Column < 3; ++Column)
		{
			int32 Y = 116;
			while (Y < 486)
			{
				if (Column == 1 && Y > 146 && Y < 262)
				{
					Y = 262;
					continue;
				}
				const int32 LineWidth = FMath::RoundToInt(ColumnWidth * Stream.FRandRange(0.62f, 1.f));
				const int32 LineHeight = Stream.RandRange(4, 7);
				const uint8 Tone = static_cast<uint8>(Stream.RandRange(58, 96));
				for (int32 DY = 0; DY < LineHeight; ++DY)
				{
					for (int32 DX = 0; DX < LineWidth; ++DX)
					{
						Put(ColumnX[Column] + DX, Y + DY, FColor(Tone, Tone - 4, Tone - 10));
					}
				}
				Y += LineHeight + Stream.RandRange(5, 8);
				if (Stream.FRand() < 0.12f)
				{
					Y += 8;
				}
			}
		}

		// 小图：灰度块 + 图注。
		for (int32 Y = 148; Y < 244; ++Y)
		{
			for (int32 X = 190; X < 326; ++X)
			{
				const int32 Shade = 96 + ((X - 190) * 46) / 136 + Stream.RandRange(-10, 10);
				const uint8 Tone = static_cast<uint8>(FMath::Clamp(Shade, 0, 255));
				Put(X, Y, FColor(Tone, Tone, Tone));
			}
		}
		for (int32 Line = 0; Line < 3; ++Line)
		{
			for (int32 Y = 250 + Line * 10; Y < 254 + Line * 10; ++Y)
			{
				for (int32 X = 190; X < 190 + 118 - Line * 18; ++X)
				{
					Put(X, Y, FColor(72, 68, 60));
				}
			}
		}

		// 中缝折痕与页脚。
		for (int32 Y = 0; Y < Size; ++Y)
		{
			for (int32 X = 254; X < 258; ++X)
			{
				Put(X, Y, FColor(198, 191, 173));
			}
		}
		for (int32 Y = 492; Y < 496; ++Y)
		{
			for (int32 X = 210; X < 300; ++X)
			{
				Put(X, Y, FColor(120, 114, 102));
			}
		}
		}
		else if (Variant == 1)
		{
			// 号外版：黑底报头、红色号外块、大标题与大图。
			FillRect(28, 22, 484, 88, FColor(38, 34, 30));
			FillRect(402, 30, 472, 80, FColor(122, 32, 30));
			FillRect(408, 38, 466, 44, FColor(216, 206, 190));
			FillRect(408, 50, 450, 54, FColor(216, 206, 190));
			FillRect(408, 60, 458, 64, FColor(216, 206, 190));
			HLinePx(28, 94, 484, 3, FColor(52, 48, 42));
			HLinePx(28, 101, 484, 1, FColor(90, 84, 74));

			HLinePx(32, 114, 330, 16, FColor(40, 36, 32));
			HLinePx(32, 138, 300, 12, FColor(44, 40, 36));
			HLinePx(32, 158, 250, 8, FColor(52, 48, 42));

			// 大图：灰度渐变 + 扫描线。
			for (int32 Y = 182; Y < 352; ++Y)
			{
				for (int32 X = 32; X < 250; ++X)
				{
					const int32 Shade = 76 + ((X - 32) * 72) / 218 + Stream.RandRange(-9, 9);
					const int32 Scan = (Y % 6 == 0) ? -18 : 0;
					const uint8 Tone = static_cast<uint8>(FMath::Clamp(Shade + Scan, 0, 255));
					Put(X, Y, FColor(Tone, Tone, Tone));
				}
			}
			HLinePx(32, 360, 222, 3, FColor(74, 70, 62));
			HLinePx(32, 368, 184, 2, FColor(86, 82, 72));

			// 右栏正文。
			int32 RightY = 114;
			while (RightY < 478)
			{
				const int32 LineWidth = Stream.RandRange(150, 202);
				const int32 LineHeight = Stream.RandRange(4, 6);
				const uint8 Tone = static_cast<uint8>(Stream.RandRange(52, 92));
				FillRect(270, RightY, 270 + LineWidth, RightY + LineHeight, FColor(Tone, Tone - 4, Tone - 10));
				RightY += LineHeight + Stream.RandRange(5, 8);
				if (Stream.FRand() < 0.10f)
				{
					RightY += 10;
				}
			}

			// 大图下方的双栏正文。
			const int32 LowerX[2] = { 32, 146 };
			for (int32 Column = 0; Column < 2; ++Column)
			{
				int32 LY = 386;
				while (LY < 478)
				{
					const int32 LineWidth = Stream.RandRange(58, 100);
					const int32 LineHeight = Stream.RandRange(4, 6);
					const uint8 Tone = static_cast<uint8>(Stream.RandRange(56, 96));
					FillRect(LowerX[Column], LY, LowerX[Column] + LineWidth, LY + LineHeight, FColor(Tone, Tone - 4, Tone - 10));
					LY += LineHeight + Stream.RandRange(5, 8);
				}
			}

			VLinePx(254, 100, 490, 4, FColor(198, 190, 174));
			HLinePx(210, 494, 300, 4, FColor(120, 114, 102));

			// 红色圆章。
			for (int32 Y = 392; Y < 452; ++Y)
			{
				for (int32 X = 400; X < 460; ++X)
				{
					const float DX = static_cast<float>(X - 430);
					const float DY = static_cast<float>(Y - 422);
					const float Dist = FMath::Sqrt(DX * DX + DY * DY);
					if (Dist > 23.f && Dist < 27.f)
					{
						Put(X, Y, FColor(128, 34, 32));
					}
					else if (Dist < 18.f && (Y % 6) < 2)
					{
						Put(X, Y, FColor(140, 42, 38));
					}
				}
			}
		}
		else
		{
			// 分类广告版：四栏小框、咖啡渍与蓝色印章。
			FillRect(32, 26, 480, 52, FColor(46, 42, 36));
			HLinePx(32, 58, 480, 2, FColor(72, 66, 58));
			HLinePx(32, 64, 480, 1, FColor(96, 88, 76));

			const int32 ColumnX[4] = { 32, 146, 260, 374 };
			constexpr int32 ColumnW = 100;
			for (int32 Column = 0; Column < 4; ++Column)
			{
				int32 Y = 78;
				while (Y < 472)
				{
					const int32 BoxH = Stream.RandRange(42, 72);
					HLinePx(ColumnX[Column], Y, ColumnX[Column] + ColumnW, 1, FColor(78, 72, 62));
					HLinePx(ColumnX[Column], Y + BoxH, ColumnX[Column] + ColumnW, 1, FColor(78, 72, 62));
					VLinePx(ColumnX[Column], Y, Y + BoxH, 1, FColor(78, 72, 62));
					VLinePx(ColumnX[Column] + ColumnW - 1, Y, Y + BoxH, 1, FColor(78, 72, 62));

					int32 LY = Y + 6;
					while (LY < Y + BoxH - 6)
					{
						const int32 LineWidth = Stream.RandRange(40, ColumnW - 10);
						const uint8 Tone = static_cast<uint8>(Stream.RandRange(62, 102));
						HLinePx(ColumnX[Column] + 5, LY, ColumnX[Column] + 5 + LineWidth, 2, FColor(Tone, Tone - 4, Tone - 10));
						LY += Stream.RandRange(6, 9);
					}
					Y += BoxH + Stream.RandRange(8, 14);
				}
			}
			VLinePx(140, 70, 484, 1, FColor(150, 142, 126));
			VLinePx(254, 70, 484, 1, FColor(150, 142, 126));
			VLinePx(368, 70, 484, 1, FColor(150, 142, 126));

			// 咖啡渍：外圈较深，内圈有随机斑点。
			for (int32 Y = 330; Y < 450; ++Y)
			{
				for (int32 X = 350; X < 470; ++X)
				{
					const float DX = static_cast<float>(X - 410);
					const float DY = static_cast<float>(Y - 390);
					const float Dist = FMath::Sqrt(DX * DX + DY * DY);
					if (Dist > 48.f && Dist < 55.f)
					{
						Put(X, Y, FColor(164, 146, 116));
					}
					else if (Dist < 44.f && Stream.FRand() < 0.06f)
					{
						Put(X, Y, FColor(178, 160, 130));
					}
				}
			}

			// 蓝色圆章与页脚。
			for (int32 Y = 392; Y < 440; ++Y)
			{
				for (int32 X = 58; X < 106; ++X)
				{
					const float DX = static_cast<float>(X - 82);
					const float DY = static_cast<float>(Y - 416);
					const float Dist = FMath::Sqrt(DX * DX + DY * DY);
					if (Dist > 18.f && Dist < 21.f)
					{
						Put(X, Y, FColor(44, 62, 86));
					}
				}
			}
			HLinePx(210, 494, 300, 3, FColor(118, 112, 100));
		}

		Mip.BulkData.Unlock();
		Texture->UpdateResource();
		Cached[Variant].Reset(Texture);
		return Texture;
	}
}

UInstancedStaticMeshComponent* AHPWH_CellChunk::GetPropComponent(const TCHAR* MeshPath)
{
	const FString Key(MeshPath);
	if (TObjectPtr<UInstancedStaticMeshComponent>* Found = PropComponents.Find(Key))
	{
		return Found->Get();
	}
	UInstancedStaticMeshComponent* Comp = NewObject<UInstancedStaticMeshComponent>(this);
	Comp->SetupAttachment(SceneRoot);
	Comp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Comp->SetCollisionProfileName(TEXT("BlockAll"));
	Comp->SetCanEverAffectNavigation(false);
	Comp->SetCastShadow(true);
	AddInstanceComponent(Comp);
	Comp->RegisterComponent();
	PropComponents.Add(Key, Comp);
	return Comp;
}

void AHPWH_CellChunk::BuildCell(const FHPWH_CellRecord& Record, float CellSizeCm,
	UMaterialInterface* GroundMaterial, UMaterialInterface* ObstacleMaterial, bool bBuildProceduralObstacles,
	const TArray<UMaterialInterface*>* GroundTileMaterials)
{
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	Coordinate = Record.Coordinate;

	if (Cube)
	{
		Ground->SetStaticMesh(Cube);
		Obstacles->SetStaticMesh(Cube);
	}
	Ground->SetMaterial(0, GroundMaterial);
	Obstacles->SetMaterial(0, ObstacleMaterial);
	Ground->SetRelativeLocation(FVector(0.f, 0.f, -5.f));
	Ground->SetRelativeScale3D(FVector(CellSizeCm / 100.f, CellSizeCm / 100.f, 0.1f));

	// 格内细分地面：每个小格按自身坐标采样群系色，边界带随 SDF 连续过渡。
	const int32 TileTotal = GroundTileCount * GroundTileCount;
	const bool bUseTiles = GroundTileMaterials && GroundTileMaterials->Num() == TileTotal;
	Ground->SetVisibility(!bUseTiles);
	if (GroundTiles.Num() != TileTotal)
	{
		for (const TObjectPtr<UStaticMeshComponent>& Old : GroundTiles)
		{
			if (Old)
			{
				Old->DestroyComponent();
			}
		}
		GroundTiles.Reset();
		for (int32 Index = 0; Index < TileTotal; ++Index)
		{
			UStaticMeshComponent* Tile = NewObject<UStaticMeshComponent>(this);
			Tile->SetupAttachment(SceneRoot);
			Tile->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Tile->SetCanEverAffectNavigation(false);
			Tile->SetCastShadow(false);
			AddInstanceComponent(Tile);
			Tile->RegisterComponent();
			GroundTiles.Add(Tile);
		}
	}
	if (bUseTiles)
	{
		const float TileSize = CellSizeCm / static_cast<float>(GroundTileCount);
		for (int32 Y = 0; Y < GroundTileCount; ++Y)
		{
			for (int32 X = 0; X < GroundTileCount; ++X)
			{
				UStaticMeshComponent* Tile = GroundTiles[Y * GroundTileCount + X];
				if (!Tile)
				{
					continue;
				}
				Tile->SetStaticMesh(Cube);
				Tile->SetMaterial(0, (*GroundTileMaterials)[Y * GroundTileCount + X]);
				Tile->SetRelativeLocation(FVector(
					-CellSizeCm * 0.5f + (X + 0.5f) * TileSize,
					-CellSizeCm * 0.5f + (Y + 0.5f) * TileSize,
					-5.f));
				Tile->SetRelativeScale3D(FVector(TileSize / 100.f, TileSize / 100.f, 0.1f));
				Tile->SetVisibility(true);
			}
		}
	}
	else
	{
		for (const TObjectPtr<UStaticMeshComponent>& Tile : GroundTiles)
		{
			if (Tile)
			{
				Tile->SetVisibility(false);
			}
		}
	}
	Obstacles->ClearInstances();
	// 每次重建先清空可视道具实例。
	for (const TPair<FString, TObjectPtr<UInstancedStaticMeshComponent>>& Pair : PropComponents)
	{
		if (Pair.Value)
		{
			Pair.Value->ClearInstances();
			Pair.Value->SetVisibility(false);
		}
	}
	// 使用导入资产的群系仍可保留程序化方块作为可见障碍。
	const bool bImportedTerrainProps =
		Record.Terrain == EHPWH_TerrainType::Books ||
		Record.Terrain == EHPWH_TerrainType::Paper ||
		Record.Terrain == EHPWH_TerrainType::Blocks ||
		Record.Terrain == EHPWH_TerrainType::Desk;
	// Paper / Blocks / Books 保留程序化方块作为可见障碍；Desk 仍由导入台灯承担。
	const bool bPreserveProceduralObstacles =
		Record.Terrain == EHPWH_TerrainType::Paper ||
		Record.Terrain == EHPWH_TerrainType::Blocks ||
		Record.Terrain == EHPWH_TerrainType::Books;
	Obstacles->SetVisibility(!bImportedTerrainProps || bPreserveProceduralObstacles);
	if (!bBuildProceduralObstacles)
	{
		return;
	}

	FRandomStream Stream(static_cast<int32>(HPWHHash4(
		static_cast<uint32>(Record.LayerSeed),
		static_cast<uint32>(Record.Coordinate.X),
		static_cast<uint32>(Record.Coordinate.Y),
		0x0B5u)));

	const float Half = CellSizeCm * 0.5f - 70.f;

	// 生成期占位：除报纸外，道具与程序化方块尽量不互相重叠。
	struct FFootprint
	{
		FVector2D Center;
		float Radius;
	};
	TArray<FFootprint> OccupiedCircles;
	TArray<FBox2D> OccupiedBoxes;
	auto IsFree = [&OccupiedCircles, &OccupiedBoxes](const FVector2D& P, float Radius)
	{
		for (const FFootprint& F : OccupiedCircles)
		{
			const float MinDist = F.Radius + Radius;
			if (FVector2D::DistSquared(P, F.Center) < MinDist * MinDist)
			{
				return false;
			}
		}
		for (const FBox2D& B : OccupiedBoxes)
		{
			const FVector2D Closest(FMath::Clamp(P.X, B.Min.X, B.Max.X),
				FMath::Clamp(P.Y, B.Min.Y, B.Max.Y));
			if (FVector2D::DistSquared(P, Closest) < Radius * Radius)
			{
				return false;
			}
		}
		return true;
	};
	auto ReserveCircle = [&OccupiedCircles](const FVector2D& P, float Radius)
	{
		FFootprint F;
		F.Center = P;
		F.Radius = Radius;
		OccupiedCircles.Add(F);
	};
	auto ReserveBox = [&OccupiedBoxes](float X, float Y, float SX, float SY, float Yaw)
	{
		const float Rad = FMath::DegreesToRadians(Yaw);
		const float C = FMath::Abs(FMath::Cos(Rad));
		const float S = FMath::Abs(FMath::Sin(Rad));
		const float EX = (SX * C + SY * S) * 0.5f;
		const float EY = (SX * S + SY * C) * 0.5f;
		OccupiedBoxes.Add(FBox2D(FVector2D(X - EX, Y - EY), FVector2D(X + EX, Y + EY)));
	};

	auto AddBox = [this, &Stream, Half](float X, float Y, float SX, float SY, float SZ, float Yaw)
	{
		X = FMath::Clamp(X, -Half, Half);
		Y = FMath::Clamp(Y, -Half, Half);
		const FTransform Transform(FRotator(0.f, Yaw, 0.f),
			FVector(X, Y, SZ * 0.5f),
			FVector(SX / 100.f, SY / 100.f, SZ / 100.f));
		Obstacles->AddInstance(Transform);
	};

	auto RandomPoint = [&Stream, Half](float Margin)
	{
		return FVector2D(Stream.FRandRange(-Half + Margin, Half - Margin),
			Stream.FRandRange(-Half + Margin, Half - Margin));
	};

	auto PlaceBox = [&AddBox, &IsFree, &ReserveCircle](float X, float Y, float SX, float SY, float SZ, float Yaw)
	{
		const float Radius = 0.5f * FMath::Sqrt(SX * SX + SY * SY);
		const FVector2D P(X, Y);
		if (!IsFree(P, Radius))
		{
			return false;
		}
		ReserveCircle(P, Radius);
		AddBox(X, Y, SX, SY, SZ, Yaw);
		return true;
	};

	auto PlaceRandomBox = [&AddBox, &RandomPoint, &IsFree, &ReserveCircle](float Margin, float SX, float SY, float SZ, float Yaw)
	{
		const float Radius = 0.5f * FMath::Sqrt(SX * SX + SY * SY);
		for (int32 Attempt = 0; Attempt < 24; ++Attempt)
		{
			const FVector2D P = RandomPoint(Margin);
			if (!IsFree(P, Radius))
			{
				continue;
			}
			ReserveCircle(P, Radius);
			AddBox(P.X, P.Y, SX, SY, SZ, Yaw);
			return true;
		}
		return false;
	};

	// 导入资产实例：按模型原始尺寸换算，底部贴地；Layers 控制向上叠放层数。
	auto AddProp = [this, &Stream, Half, &IsFree, &ReserveCircle](const TCHAR* MeshPath, float Scale, int32 Count, float Range, int32 StackLayers = 1, bool bVaryBlockSize = false)
	{
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, MeshPath);
		if (!Mesh)
		{
			return;
		}
		UInstancedStaticMeshComponent* Comp = GetPropComponent(MeshPath);
		if (!Comp)
		{
			return;
		}
		Comp->SetStaticMesh(Mesh);
		// 显式同步模型材质；优先使用双面版本，避免反向面片整体发黑。
		if (UMaterialInterface* TwoSided = LoadTwoSidedModelMaterial(MeshPath))
		{
			Comp->SetMaterial(0, TwoSided);
		}
		else
		{
			for (int32 MaterialIndex = 0; MaterialIndex < Mesh->GetStaticMaterials().Num(); ++MaterialIndex)
			{
				Comp->SetMaterial(MaterialIndex, Mesh->GetMaterial(MaterialIndex));
			}
		}
		Comp->SetVisibility(true);
		// Use a separate stream so changing block scale does not move other props.
		FRandomStream SizeStream(Stream.GetCurrentSeed());
		const FBoxSphereBounds Bounds = Mesh->GetBounds();
		const float FootprintRadius = FMath::Sqrt(
			Bounds.BoxExtent.X * Bounds.BoxExtent.X +
			Bounds.BoxExtent.Y * Bounds.BoxExtent.Y) * Scale;
		for (int32 I = 0; I < Count; ++I)
		{
			FVector2D P(0.f, 0.f);
			bool bPlaced = false;
			for (int32 Attempt = 0; Attempt < 24; ++Attempt)
			{
				P = FVector2D(
					FMath::Clamp(Stream.FRandRange(-Range, Range), -Half, Half),
					FMath::Clamp(Stream.FRandRange(-Range, Range), -Half, Half));
				if (IsFree(P, FootprintRadius))
				{
					bPlaced = true;
					break;
				}
			}
			if (!bPlaced)
			{
				continue;
			}
			ReserveCircle(P, FootprintRadius);
			const float Yaw = Stream.FRandRange(0.f, 360.f);
			float StackBase = 0.f;
			for (int32 Layer = 0; Layer < StackLayers; ++Layer)
			{
				const float InstanceScale = Scale * (bVaryBlockSize ? SizeStream.FRandRange(0.7f, 1.f) : 1.f);
				const float Z = StackBase + (Bounds.BoxExtent.Z - Bounds.Origin.Z) * InstanceScale;
				Comp->AddInstance(FTransform(FRotator(0.f, Yaw + Layer * 17.f, 0.f),
					FVector(P.X, P.Y, Z), FVector(InstanceScale)));
				StackBase += 2.f * Bounds.BoxExtent.Z * InstanceScale;
			}
		}
	};

	// 报纸纸片：比书页更大的薄片，按群系铺在地面；尺寸再整体放大一倍。
	auto AddNewsprintSheets = [this, &Stream](int32 Count, float Range)
	{
		UTexture2D* Newsprint[3] = {
			HPWHNewsprintTexture(0),
			HPWHNewsprintTexture(1),
			HPWHNewsprintTexture(2)
		};
		UMaterialInterface* SheetBase = LoadObject<UMaterialInterface>(nullptr,
			TEXT("/Game/HPWH/Assets/Materials/M_HPWH_Newsprint.M_HPWH_Newsprint"));
		UStaticMesh* SheetMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		UInstancedStaticMeshComponent* Comps[3] = {
			GetPropComponent(TEXT("HPWH_Newsprint")),
			GetPropComponent(TEXT("HPWH_Newsprint_B")),
			GetPropComponent(TEXT("HPWH_Newsprint_C"))
		};
		if (!SheetBase || !SheetMesh)
		{
			return;
		}

		UMaterialInstanceDynamic* SheetMaterials[3] = { nullptr, nullptr, nullptr };
		for (int32 Variant = 0; Variant < 3; ++Variant)
		{
			if (!Newsprint[Variant] || !Comps[Variant])
			{
				continue;
			}
			Comps[Variant]->SetStaticMesh(SheetMesh);
			Comps[Variant]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Comps[Variant]->SetCollisionProfileName(TEXT("NoCollision"));
			Comps[Variant]->SetCastShadow(false);
			SheetMaterials[Variant] = UMaterialInstanceDynamic::Create(SheetBase, this);
			if (!SheetMaterials[Variant])
			{
				continue;
			}
			SheetMaterials[Variant]->SetTextureParameterValue(TEXT("Newsprint"), Newsprint[Variant]);
			Comps[Variant]->SetMaterial(0, SheetMaterials[Variant]);
			Comps[Variant]->SetVisibility(true);
		}

		for (int32 I = 0; I < Count; ++I)
		{
			const int32 Variant = Stream.RandRange(0, 2);
			if (!Comps[Variant] || !SheetMaterials[Variant])
			{
				continue;
			}
			const float SizeScale = Stream.FRandRange(3.0f, 4.0f);
			const float X = Stream.FRandRange(-Range, Range);
			const float Y = Stream.FRandRange(-Range, Range);
			const float Width = Stream.FRandRange(110.f, 165.f) * SizeScale;
			const float Depth = Stream.FRandRange(80.f, 125.f) * SizeScale;
			const FRotator Tilt(Stream.FRandRange(-4.f, 4.f), Stream.FRandRange(0.f, 360.f),
				Stream.FRandRange(-4.f, 4.f));
			Comps[Variant]->AddInstance(FTransform(Tilt, FVector(X, Y, 3.f + I * 0.25f),
				FVector(Width / 100.f, Depth / 100.f, 0.02f)));
		}
	};

	switch (Record.Terrain)
	{
	case EHPWH_TerrainType::Paper:
	{
		const int32 Count = 8 + FMath::RoundToInt(Record.Density * 10.f);
		for (int32 I = 0; I < Count; ++I)
		{
			const float Size = Stream.FRandRange(24.f, 76.f);
			const float Height = Stream.FRandRange(18.f, 56.f);
			PlaceRandomBox(60.f, Size, Size, Height, Stream.FRandRange(0.f, 180.f));
		}
		for (int32 I = 0; I < 5; ++I)
		{
			PlaceRandomBox(50.f, Stream.FRandRange(80.f, 150.f), Stream.FRandRange(70.f, 130.f), 5.f,
				Stream.FRandRange(0.f, 180.f));
		}
		break;
	}
	case EHPWH_TerrainType::Blocks:
	{
		const int32 Count = 7 + FMath::RoundToInt(Record.Density * 7.f);
		for (int32 I = 0; I < Count; ++I)
		{
			const int32 GX = Stream.RandRange(-1, 1);
			const int32 GY = Stream.RandRange(-1, 1);
			const float X = GX * CellSizeCm * 0.26f + Stream.FRandRange(-40.f, 40.f);
			const float Y = GY * CellSizeCm * 0.26f + Stream.FRandRange(-40.f, 40.f);
			const float SX = Stream.FRandRange(80.f, 190.f);
			const float SY = Stream.FRandRange(80.f, 190.f);
			const float SZ = Stream.FRandRange(70.f, 240.f);
			PlaceBox(X, Y, SX, SY, SZ, Stream.RandRange(0, 3) * 90.f);
		}
		break;
	}
	case EHPWH_TerrainType::Obstacle:
	{
		const bool bHorizontal = Stream.RandRange(0, 1) == 0;
		const float WallLength = CellSizeCm * Stream.FRandRange(0.58f, 0.76f);
		const float Gap = Stream.FRandRange(120.f, 220.f);
		for (int32 Row = -1; Row <= 1; ++Row)
		{
			const float Offset = Row * CellSizeCm * 0.24f;
			const float Shift = Stream.FRandRange(-CellSizeCm * 0.10f, CellSizeCm * 0.10f);
			const float Segment = (WallLength - Gap) * 0.5f;
			const float Height = Stream.FRandRange(180.f, 320.f);
			const float A = Shift - Gap * 0.5f - Segment * 0.5f;
			const float B = Shift + Gap * 0.5f + Segment * 0.5f;
			if (bHorizontal)
			{
				AddBox(A, Offset, Segment, 46.f, Height, 0.f);
				ReserveBox(A, Offset, Segment, 46.f, 0.f);
				AddBox(B, Offset, Segment, 46.f, Height, 0.f);
				ReserveBox(B, Offset, Segment, 46.f, 0.f);
			}
			else
			{
				AddBox(Offset, A, 46.f, Segment, Height, 0.f);
				ReserveBox(Offset, A, 46.f, Segment, 0.f);
				AddBox(Offset, B, 46.f, Segment, Height, 0.f);
				ReserveBox(Offset, B, 46.f, Segment, 0.f);
			}
		}
		for (int32 I = 0; I < 4; ++I)
		{
			PlaceRandomBox(90.f, Stream.FRandRange(50.f, 100.f), Stream.FRandRange(50.f, 100.f), 120.f, 0.f);
		}
		break;
	}
	case EHPWH_TerrainType::Desk:
	{
		AddBox(-CellSizeCm * 0.24f, -CellSizeCm * 0.18f, 46.f, 46.f, 300.f, 0.f);
		AddBox(-CellSizeCm * 0.24f, -CellSizeCm * 0.18f, 170.f, 150.f, 40.f, 20.f);
		ReserveBox(-CellSizeCm * 0.24f, -CellSizeCm * 0.18f, 170.f, 150.f, 20.f);
		for (int32 I = 0; I < 6; ++I)
		{
			PlaceRandomBox(90.f, Stream.FRandRange(100.f, 150.f), Stream.FRandRange(120.f, 170.f), 22.f,
				Stream.FRandRange(-25.f, 25.f));
		}
		break;
	}
	case EHPWH_TerrainType::Books:
	{
		const int32 Count = 6 + FMath::RoundToInt(Record.Density * 5.f);
		for (int32 I = 0; I < Count; ++I)
		{
			PlaceRandomBox(80.f, Stream.FRandRange(110.f, 170.f), Stream.FRandRange(140.f, 210.f), 22.f,
				Stream.FRandRange(0.f, 180.f));
		}
		break;
	}
	case EHPWH_TerrainType::Empty:
	{
		const int32 Count = Stream.RandRange(0, 3);
		for (int32 I = 0; I < Count; ++I)
		{
			const float Size = Stream.FRandRange(12.f, 30.f);
			PlaceRandomBox(80.f, Size, Size, Size, Stream.FRandRange(0.f, 180.f));
		}
		break;
	}
	default:
		break;
	}

	// 导入网格自带碰撞；只清理不保留程序化方块的群系。
	if (bImportedTerrainProps && !bPreserveProceduralObstacles)
	{
		Obstacles->ClearInstances();
	}

	// 用导入模型替换方块道具；碰撞由导入网格承担。
	switch (Record.Terrain)
	{
	case EHPWH_TerrainType::Books:
		AddProp(TEXT("/Game/HPWH/Assets/Models/书堆/StaticMeshes/book_pile.book_pile"), 16.f, 1, 300.f, 2);
		AddProp(TEXT("/Game/HPWH/Assets/Models/三本书/StaticMeshes/three_books.three_books"), 16.f, 2, 360.f, 2);
		AddProp(TEXT("/Game/HPWH/Assets/Models/一本书/StaticMeshes/one_book.one_book"), 20.f, 3, 380.f, 2);
		AddProp(TEXT("/Game/HPWH/Assets/Models/积木/StaticMeshes/block.block"), 18.f, 2, 340.f, 2, true);
		break;
	case EHPWH_TerrainType::Paper:
		// 先放体积大的积木，再让纸团填充剩余空间，避免大件被挤掉。
		AddProp(TEXT("/Game/HPWH/Assets/Models/积木/StaticMeshes/block.block"), 18.f, 3, 360.f, 2, true);
		AddProp(TEXT("/Game/HPWH/Assets/Models/纸团/StaticMeshes/paper_ball.paper_ball"), 16.f, 16, 310.f);
		break;
	case EHPWH_TerrainType::Blocks:
		AddProp(TEXT("/Game/HPWH/Assets/Models/积木/StaticMeshes/block.block"), 20.f, 8, 420.f, 2, true);
		break;
	case EHPWH_TerrainType::Desk:
		// 台灯只在一部分案头格生成，降低密度。
		if (Stream.FRand() < 0.5f)
		{
			AddProp(TEXT("/Game/HPWH/Assets/Models/台灯/StaticMeshes/desk_lamp.desk_lamp"), 10.f, 1, 300.f);
		}
		break;
	default:
		break;
	}

	// 报纸页按群系铺地：纸张与书本地带最多，案头与积木地带少量点缀。
	switch (Record.Terrain)
	{
	case EHPWH_TerrainType::Paper:
		AddNewsprintSheets(6, 300.f);
		break;
	case EHPWH_TerrainType::Books:
		AddNewsprintSheets(4, 300.f);
		break;
	case EHPWH_TerrainType::Desk:
		AddNewsprintSheets(3, 260.f);
		break;
	case EHPWH_TerrainType::Blocks:
		AddNewsprintSheets(3, 280.f);
		break;
	case EHPWH_TerrainType::Empty:
		if (Stream.FRand() < 0.30f)
		{
			AddNewsprintSheets(1, 260.f);
		}
		break;
	default:
		break;
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugProps")))
	{
		UE_LOG(LogTemp, Display, TEXT("HPWH: props terrain=%d boxes=%d obstacleVisible=%d"),
			static_cast<int32>(Record.Terrain), Obstacles->GetInstanceCount(),
			Obstacles->IsVisible() ? 1 : 0);
		for (const TPair<FString, TObjectPtr<UInstancedStaticMeshComponent>>& Pair : PropComponents)
		{
			if (Pair.Value && Pair.Value->GetInstanceCount() > 0)
			{
				UE_LOG(LogTemp, Display, TEXT("HPWH: prop mesh=%s count=%d visible=%d material=%d"),
					*Pair.Key, Pair.Value->GetInstanceCount(), Pair.Value->IsVisible() ? 1 : 0,
					Pair.Value->GetMaterial(0) ? 1 : 0);
			}
		}
	}
}

AHPWH_WorldDirector::AHPWH_WorldDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.f;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterialFinder(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BasicMaterialFinder.Succeeded())
	{
		BaseMaterial = BasicMaterialFinder.Object;
	}
	CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	FloorMaterial = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/HPWH/Assets/Materials/M_HPWH_Floor.M_HPWH_Floor"));
}

void AHPWH_WorldDirector::BeginPlay()
{
	Super::BeginPlay();

	if (!TerrainPrefabs)
	{
		TerrainPrefabs = LoadObject<UHPWH_TerrainPrefabSet>(nullptr,
			TEXT("/Game/HPWH/Prefabs/DA_HPWH_TerrainPrefabs.DA_HPWH_TerrainPrefabs"));
	}

	if (auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (!bInitialized)
		{
			InitializeForRun(GM->GetRunSeed(), GM->GetLayerIndex());
		}
	}
}

void AHPWH_WorldDirector::InitializeForRun(int32 InRunSeed, int32 InLayerIndex)
{
	RunSeed = InRunSeed;
	LayerIndex = FMath::Max(1, InLayerIndex);
	LayerSeed = static_cast<int32>(HPWHHash4(static_cast<uint32>(RunSeed), static_cast<uint32>(LayerIndex), 0x4C59u, 0x5345u));
	bInitialized = true;

	BuildPOIPlacement();
	BuildBiomePlacement();

	if (!Crawler)
	{
		Crawler = GetWorld()->SpawnActor<AHPWH_Crawler>(AHPWH_Crawler::StaticClass(), CrawlerAnchor, FRotator::ZeroRotator);
	}
	if (Crawler)
	{
		Crawler->SetDirector(this);
		Crawler->ResetDormant();
	}

	RefreshAroundPlayer();
	SpawnCompanion();
	UE_LOG(LogTemp, Display, TEXT("HPWH: director init layer=%d seed=%d mirror=(%d,%d) courtyard=(%d,%d) exit=(%d,%d) cells=%d"),
		LayerIndex, LayerSeed,
		GetPOICell(0).X, GetPOICell(0).Y,
		GetPOICell(1).X, GetPOICell(1).Y,
		GetExitCell().X, GetExitCell().Y, LoadedCells.Num());
}

void AHPWH_WorldDirector::SetLayer(int32 InLayerIndex)
{
	if (IsValid(Companion))
	{
		Companion->Destroy();
		Companion = nullptr;
	}

	for (auto It = LoadedCells.CreateIterator(); It; ++It)
	{
		if (IsValid(It.Value()))
		{
			It.Value()->Destroy();
		}
	}
	LoadedCells.Empty();

	for (auto It = CellActors.CreateIterator(); It; ++It)
	{
		for (const TWeakObjectPtr<AActor>& Actor : It.Value())
		{
			if (Actor.IsValid())
			{
				Actor->Destroy();
			}
		}
	}
	CellActors.Empty();

	for (auto It = POIActors.CreateIterator(); It; ++It)
	{
		if (IsValid(It.Value()))
		{
			It.Value()->Destroy();
		}
	}
	POIActors.Empty();

	for (TActorIterator<AHPWH_CoreItem> CoreIt(GetWorld()); CoreIt; ++CoreIt)
	{
		CoreIt->Destroy();
	}

	Records.Empty();
	POICells.Empty();
	PuzzleAnchors.Empty();
	CenterCell = FIntPoint(999999, 999999);
	InitializeForRun(RunSeed, InLayerIndex);
}

void AHPWH_WorldDirector::ResetForRespawn()
{
	for (auto It = CellActors.CreateIterator(); It; ++It)
	{
		for (const TWeakObjectPtr<AActor>& Actor : It.Value())
		{
			if (Actor.IsValid())
			{
				Actor->Destroy();
			}
		}
	}
	CellActors.Empty();

	for (auto& Pair : Records)
	{
		Pair.Value.KilledCount = 0;
		Pair.Value.bPuzzleSolved = false;
		Pair.Value.bCoreCollected = false;
	}

	for (auto It = POIActors.CreateIterator(); It; ++It)
	{
		if (IsValid(It.Value()))
		{
			if (AHPWH_POIBase* POI = Cast<AHPWH_POIBase>(It.Value()))
			{
				POI->ResetPuzzle();
			}
			It.Value()->Destroy();
		}
	}
	POIActors.Empty();

	for (TActorIterator<AHPWH_CoreItem> CoreIt(GetWorld()); CoreIt; ++CoreIt)
	{
		CoreIt->Destroy();
	}

	for (auto It = LoadedCells.CreateIterator(); It; ++It)
	{
		if (IsValid(It.Value()))
		{
			It.Value()->Destroy();
		}
	}
	LoadedCells.Empty();
	CenterCell = FIntPoint(999999, 999999);

	if (Crawler)
	{
		Crawler->ResetDormant();
	}
	if (IsValid(Companion))
	{
		Companion->ResetForRespawn();
	}
	else
	{
		SpawnCompanion();
	}
	RefreshAroundPlayer();
}

void AHPWH_WorldDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	RuntimeElapsed += DeltaSeconds;
	UpdateAccumulator += DeltaSeconds;
	if (UpdateAccumulator < 0.2f)
	{
		return;
	}
	UpdateAccumulator = 0.f;

	auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	AHPWH_PlayerCharacter* Body = GM ? GM->GetPlayerBody() : nullptr;
	APawn* Pawn = Body;
	if (GM && GM->GetSoulAvatar())
	{
		Pawn = GM->GetSoulAvatar();
	}
	if (!Pawn)
	{
		const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
		Pawn = PC ? PC->GetPawn() : nullptr;
	}
	if (!Pawn)
	{
		return;
	}

	const FIntPoint Cell = WorldToCell(Pawn->GetActorLocation());
	if (Cell != CenterCell)
	{
		CenterCell = Cell;
		RefreshAroundPlayer();
	}

	if (!bLoggedRuntime && RuntimeElapsed > 3.f)
	{
		bLoggedRuntime = true;
		const FVector World = Pawn->GetActorLocation();
		const FVector Center = CellCenter(Cell);
		const FVector MinWorld = HPWH_CellCenter(Cell - FIntPoint(ActiveRadius, ActiveRadius), CellSizeCm);
		const FVector MaxWorld = HPWH_CellCenter(Cell + FIntPoint(ActiveRadius, ActiveRadius), CellSizeCm);
		UE_LOG(LogTemp, Display,
			TEXT("HPWH: runtime alive pawn=%s world=(%.0f,%.0f) cell=(%d,%d) center=(%.0f,%.0f) window=[%.0f..%.0f] loaded=%d crawler=%s"),
			*Pawn->GetName(), World.X, World.Y, Cell.X, Cell.Y, Center.X, Center.Y,
			MinWorld.X, MaxWorld.X, LoadedCells.Num(),
			Crawler ? *UEnum::GetValueAsString(Crawler->GetCrawlerState()) : TEXT("none"));
	}
}

FIntPoint AHPWH_WorldDirector::WorldToCell(const FVector& WorldLocation) const
{
	return HPWH_WorldToCell(WorldLocation, CellSizeCm);
}

FVector AHPWH_WorldDirector::CellCenter(const FIntPoint& Cell) const
{
	return HPWH_CellCenter(Cell, CellSizeCm);
}

const FHPWH_CellRecord* AHPWH_WorldDirector::FindRecord(const FIntPoint& Cell) const
{
	return Records.Find(Cell);
}

FIntPoint AHPWH_WorldDirector::GetPOICell(int32 Index) const
{
	return POICells.IsValidIndex(Index) ? POICells[Index] : FIntPoint(Index + 3, 0);
}

void AHPWH_WorldDirector::BuildPOIPlacement()
{
	POICells.Empty();
	POICells.Add(PickPOICell(0, 3, 5));
	POICells.Add(PickPOICell(1, 3, 5));
	POICells.Add(PickPOICell(2, 3, 5));
	POICells.Add(PickPOICell(3, 4, 6));

	for (int32 I = 1; I < POICells.Num(); ++I)
	{
		for (int32 J = 0; J < I; ++J)
		{
			// 同一 5×5 窗口内最多只能出现一个解密器物，固定器物之间保持至少 5 格。
			int32 Guard = 0;
			while ((POICells[I] == POICells[J] ||
				FMath::Max(FMath::Abs(POICells[I].X - POICells[J].X),
					FMath::Abs(POICells[I].Y - POICells[J].Y)) < 5) && Guard < 8)
			{
				POICells[I] += FIntPoint(1 + I, -1 - I);
				++Guard;
			}
		}
	}
}

FIntPoint AHPWH_WorldDirector::PickPOICell(int32 Index, int32 MinRing, int32 MaxRing) const
{
	FRandomStream Stream(static_cast<int32>(HPWHHash4(
		static_cast<uint32>(LayerSeed), static_cast<uint32>(Index), 0x504F49u, 0x5354u)));

	for (int32 Attempt = 0; Attempt < 32; ++Attempt)
	{
		const int32 X = Stream.RandRange(-MaxRing, MaxRing);
		const int32 Y = Stream.RandRange(-MaxRing, MaxRing);
		const int32 Ring = FMath::Max(FMath::Abs(X), FMath::Abs(Y));
		if (Ring < MinRing)
		{
			continue;
		}
		const FIntPoint Candidate(X, Y);
		bool bConflicts = false;
		for (const FIntPoint& Existing : POICells)
		{
			if (Candidate == Existing)
			{
				bConflicts = true;
				break;
			}
		}
		if (!bConflicts)
		{
			return Candidate;
		}
	}

	const FIntPoint Fallbacks[4] = {
		FIntPoint(MinRing, 0), FIntPoint(0, MinRing), FIntPoint(-MinRing, MinRing), FIntPoint(MinRing, -MinRing)
	};
	return Fallbacks[FMath::Clamp(Index, 0, 3)];
}

bool AHPWH_WorldDirector::CanPlacePuzzleAnchor(const FIntPoint& Cell, const TSet<FIntPoint>& Desired) const
{
	if (!Desired.Contains(Cell) || Cell == GetExitCell())
	{
		return false;
	}
	if (CellCenter(Cell).Size2D() < OriginSafeRadiusCm)
	{
		return false;
	}
	// 与固定器物、已有解密锚点保持至少 5 格，保证任何 5×5 窗口最多一个。
	for (const FIntPoint& Fixed : POICells)
	{
		if (FMath::Max(FMath::Abs(Cell.X - Fixed.X), FMath::Abs(Cell.Y - Fixed.Y)) < 5)
		{
			return false;
		}
	}
	for (const FIntPoint& Anchor : PuzzleAnchors)
	{
		if (FMath::Max(FMath::Abs(Cell.X - Anchor.X), FMath::Abs(Cell.Y - Anchor.Y)) < 5)
		{
			return false;
		}
	}
	// 兼容旧存档：记录中已有的解密器物同样参与间距判定。
	for (const auto& Pair : Records)
	{
		if (Pair.Value.bHasPOI &&
			FMath::Max(FMath::Abs(Cell.X - Pair.Key.X), FMath::Abs(Cell.Y - Pair.Key.Y)) < 5)
		{
			return false;
		}
	}
	return true;
}

void AHPWH_WorldDirector::EnsurePuzzleAnchor(const TSet<FIntPoint>& Desired)
{
	// 当前窗口已有解密器物时不再新增。
	for (const FIntPoint& Cell : Desired)
	{
		if (POICells.Contains(Cell) || PuzzleAnchors.Contains(Cell))
		{
			return;
		}
		if (const FHPWH_CellRecord* Record = Records.Find(Cell))
		{
			if (Record->bHasPOI)
			{
				return;
			}
		}
	}

	// 只有窗口触及解密线（|x| 或 |y| = 5 / 15 / 30…）时才允许生成。
	bool bOnPuzzleLine = false;
	for (const FIntPoint& Cell : Desired)
	{
		if (HPWH_IsPuzzleLineCell(Cell, SpecialPuzzleLineInterval))
		{
			bOnPuzzleLine = true;
			break;
		}
	}
	if (!bOnPuzzleLine)
	{
		return;
	}

	FRandomStream Stream(static_cast<int32>(HPWHHash4(
		static_cast<uint32>(LayerSeed), static_cast<uint32>(CenterCell.X),
		static_cast<uint32>(CenterCell.Y), 0x505A41u)));

	FIntPoint Anchor = FIntPoint(999999, 999999);
	bool bFound = false;
	const bool bNearPlayer = FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugAnchorNear")) ||
		Stream.FRand() < 0.30f;

	if (bNearPlayer)
	{
		TArray<FIntPoint> Candidates;
		for (int32 DX = -2; DX <= 2; ++DX)
		{
			for (int32 DY = -2; DY <= 2; ++DY)
			{
				if (DX == 0 && DY == 0)
				{
					continue;
				}
				const FIntPoint Candidate = CenterCell + FIntPoint(DX, DY);
				if (CanPlacePuzzleAnchor(Candidate, Desired))
				{
					Candidates.Add(Candidate);
				}
			}
		}
		if (Candidates.Num() > 0)
		{
			Anchor = Candidates[Stream.RandRange(0, Candidates.Num() - 1)];
			bFound = true;
		}
	}

	if (!bFound)
	{
		// 退回到窗口内离玩家最近的解密线格。
		int32 BestDistance = TNumericLimits<int32>::Max();
		for (const FIntPoint& Cell : Desired)
		{
			if (!HPWH_IsPuzzleLineCell(Cell, SpecialPuzzleLineInterval) ||
				!CanPlacePuzzleAnchor(Cell, Desired))
			{
				continue;
			}
			const int32 Distance = FMath::Max(FMath::Abs(Cell.X - CenterCell.X), FMath::Abs(Cell.Y - CenterCell.Y));
			if (Distance < BestDistance)
			{
				BestDistance = Distance;
				Anchor = Cell;
				bFound = true;
			}
		}
	}

	if (!bFound)
	{
		return;
	}

	PuzzleAnchors.Add(Anchor);
	if (FHPWH_CellRecord* Existing = Records.Find(Anchor))
	{
		Existing->Terrain = PuzzleTerrainForCell(Anchor);
		Existing->Density = 0.f;
		Existing->bHasPOI = true;
	}
	// 锚点可能已在窗口内加载过，重建该格让解密器物立即出现。
	if (LoadedCells.Contains(Anchor))
	{
		DespawnCell(Anchor);
	}
	UE_LOG(LogTemp, Display, TEXT("HPWH: puzzle anchor (%d,%d) near=%d window=(%d,%d)"),
		Anchor.X, Anchor.Y, bNearPlayer ? 1 : 0, CenterCell.X, CenterCell.Y);
}

bool AHPWH_WorldDirector::IsNearMirrorClearing(const FIntPoint& Cell) const
{
	if (POICells.Num() < 1)
	{
		return false;
	}
	const FIntPoint MirrorCell = POICells[0];
	const int32 D = FMath::Max(FMath::Abs(Cell.X - MirrorCell.X), FMath::Abs(Cell.Y - MirrorCell.Y));
	return D <= 1 && D > 0;
}

bool AHPWH_WorldDirector::IsReservedPOICell(const FIntPoint& Cell) const
{
	if (POICells.Contains(Cell) || Cell == GetExitCell())
	{
		return true;
	}
	return HPWH_IsPuzzleLineCell(Cell, SpecialPuzzleLineInterval);
}

EHPWH_TerrainType AHPWH_WorldDirector::PuzzleTerrainForCell(const FIntPoint& Cell) const
{
	const uint32 Hash = HPWHHash4(static_cast<uint32>(LayerSeed), static_cast<uint32>(Cell.X),
		static_cast<uint32>(Cell.Y), 0x50555Au);
	const float Roll = HPWHHashUnit(Hash);
	if (Roll < 0.34f)
	{
		return EHPWH_TerrainType::Mirror;
	}
	if (Roll < 0.67f)
	{
		return EHPWH_TerrainType::Courtyard;
	}
	return EHPWH_TerrainType::Laptop;
}

FHPWH_BiomeSeed AHPWH_WorldDirector::MakeLatticeSeed(int32 LX, int32 LY) const
{
	FHPWH_BiomeSeed Seed;
	const uint32 Hash = HPWHHash4(static_cast<uint32>(LayerSeed),
		static_cast<uint32>(LX), static_cast<uint32>(LY), 0x53454544u);
	const float JitterX = HPWHHashUnit(HPWHHash4(Hash, 1, 0, 0)) * 4.f - 2.f;
	const float JitterY = HPWHHashUnit(HPWHHash4(Hash, 2, 0, 0)) * 4.f - 2.f;
	Seed.Position = FVector2D((LX + 0.5f) * BiomeSeedSpacing + JitterX,
		(LY + 0.5f) * BiomeSeedSpacing + JitterY);
	Seed.Radius = 3.8f + HPWHHashUnit(HPWHHash4(Hash, 3, 0, 0)) * 1.6f;

	const float Roll = HPWHHashUnit(HPWHHash4(Hash, 4, 0, 0));
	if (Roll < 0.30f)
	{
		Seed.Biome = EHPWH_TerrainType::Books;
	}
	else if (Roll < 0.55f)
	{
		Seed.Biome = EHPWH_TerrainType::Paper;
	}
	else if (Roll < 0.72f)
	{
		Seed.Biome = EHPWH_TerrainType::Blocks;
	}
	else if (Roll < 0.87f)
	{
		Seed.Biome = EHPWH_TerrainType::Desk;
	}
	else
	{
		Seed.Biome = EHPWH_TerrainType::Obstacle;
	}
	Seed.Params = BiomeParams(Seed.Biome);
	return Seed;
}

FVector2D AHPWH_WorldDirector::WarpPoint(const FVector2D& RawPoint) const
{
	// 域扭曲在 L1 之前完成，SDF 与边界带共享同一套查询坐标。
	const float WarpX = (HPWHValueNoise(RawPoint.X * 0.06f + 11.3f, RawPoint.Y * 0.06f - 4.7f,
		static_cast<uint32>(LayerSeed) ^ 0x575241u) * 2.f - 1.f) * 1.6f;
	const float WarpY = (HPWHValueNoise(RawPoint.X * 0.06f - 8.9f, RawPoint.Y * 0.06f + 17.1f,
		static_cast<uint32>(LayerSeed) ^ 0x575242u) * 2.f - 1.f) * 1.6f;
	return FVector2D(RawPoint.X + WarpX, RawPoint.Y + WarpY);
}

FVector2D AHPWH_WorldDirector::WarpedCellPoint(const FIntPoint& Cell) const
{
	return WarpPoint(FVector2D(Cell.X + 0.5f, Cell.Y + 0.5f));
}

FVector AHPWH_WorldDirector::BiomeParams(EHPWH_TerrainType Biome) const
{
	// 连续参数：柔软度、结构感、堆叠密度；只混合参数，不插值离散 ID。
	switch (Biome)
	{
	case EHPWH_TerrainType::Books:    return FVector(0.78f, 0.34f, 0.86f);
	case EHPWH_TerrainType::Paper:    return FVector(0.92f, 0.12f, 0.56f);
	case EHPWH_TerrainType::Blocks:   return FVector(0.22f, 0.90f, 0.62f);
	case EHPWH_TerrainType::Desk:     return FVector(0.45f, 0.70f, 0.72f);
	case EHPWH_TerrainType::Obstacle: return FVector(0.10f, 0.86f, 1.00f);
	default:                          return FVector(0.50f, 0.50f, 0.50f);
	}
}

EHPWH_TerrainType AHPWH_WorldDirector::BiomeFromParams(const FVector& Params) const
{
	const EHPWH_TerrainType Types[] = {
		EHPWH_TerrainType::Books, EHPWH_TerrainType::Paper, EHPWH_TerrainType::Blocks,
		EHPWH_TerrainType::Desk, EHPWH_TerrainType::Obstacle
	};
	EHPWH_TerrainType Best = Types[0];
	float BestDistance = TNumericLimits<float>::Max();
	for (const EHPWH_TerrainType Type : Types)
	{
		const float Distance = FVector::DistSquared(Params, BiomeParams(Type));
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			Best = Type;
		}
	}
	return Best;
}

void AHPWH_WorldDirector::BuildBiomePlacement()
{
	ForcedBiomeSeeds.Reset();
	// 固定器物处放置高权重书堆种子，让解密目标始终被书堆包围。
	for (int32 Index = 0; Index < 3 && POICells.IsValidIndex(Index); ++Index)
	{
		FHPWH_BiomeSeed Seed;
		Seed.Position = FVector2D(POICells[Index].X + 0.5f, POICells[Index].Y + 0.5f);
		Seed.Radius = 4.5f;
		Seed.Biome = EHPWH_TerrainType::Books;
		Seed.Params = BiomeParams(Seed.Biome);
		ForcedBiomeSeeds.Add(Seed);
	}

	// 出生点外侧再保证一片书堆，方向由层种子决定。
	FRandomStream Stream(static_cast<int32>(HPWHHash4(
		static_cast<uint32>(LayerSeed), 0x424F4F4Bu, 0, 0)));
	static const FIntPoint Directions[] = {
		FIntPoint(1, 0), FIntPoint(0, 1), FIntPoint(-1, 0), FIntPoint(0, -1)
	};
	const FIntPoint Direction = Directions[Stream.RandRange(0, 3)];
	FHPWH_BiomeSeed SpawnSeed;
	SpawnSeed.Position = FVector2D(Direction.X * 7.f + 0.5f, Direction.Y * 7.f + 0.5f);
	SpawnSeed.Radius = 4.5f;
	SpawnSeed.Biome = EHPWH_TerrainType::Books;
	SpawnSeed.Params = BiomeParams(SpawnSeed.Biome);
	ForcedBiomeSeeds.Add(SpawnSeed);

	FString SeedText;
	for (const FHPWH_BiomeSeed& Seed : ForcedBiomeSeeds)
	{
		SeedText += FString::Printf(TEXT("(%.1f,%.1f r=%.1f %d) "),
			Seed.Position.X, Seed.Position.Y, Seed.Radius, static_cast<int32>(Seed.Biome));
	}
	UE_LOG(LogTemp, Display, TEXT("HPWH: biome seeds=%d %s"),
		ForcedBiomeSeeds.Num(), *SeedText);
}

void AHPWH_WorldDirector::SpawnCompanion()
{
	if (IsValid(Companion) || !GetWorld())
	{
		return;
	}

	// 只在已加载的环带 2 格内选点，保证脚下一定有地面。
	TArray<FIntPoint> Candidates;
	for (int32 X = -2; X <= 2; ++X)
	{
		for (int32 Y = -2; Y <= 2; ++Y)
		{
			if (FMath::Max(FMath::Abs(X), FMath::Abs(Y)) != 2)
			{
				continue;
			}
			const FIntPoint Cell(X, Y);
			if (IsReservedPOICell(Cell) || IsPuzzleAnchorCell(Cell))
			{
				continue;
			}
			if (const FHPWH_CellRecord* Record = Records.Find(Cell))
			{
				if (Record->bHasPOI)
				{
					continue;
				}
			}
			Candidates.Add(Cell);
		}
	}
	if (Candidates.Num() == 0)
	{
		return;
	}

	const uint32 Hash = HPWHHash4(static_cast<uint32>(LayerSeed), 0x434F4D50u, 0x4E5043u, 5u);
	const FIntPoint Cell = Candidates[Hash % static_cast<uint32>(Candidates.Num())];
	const FVector Offset(
		(HPWHHashUnit(HPWHHash4(Hash, 1, 0, 0)) * 2.f - 1.f) * 180.f,
		(HPWHHashUnit(HPWHHash4(Hash, 2, 0, 0)) * 2.f - 1.f) * 180.f,
		110.f);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Companion = GetWorld()->SpawnActor<AHPWH_Companion>(AHPWH_Companion::StaticClass(),
		CellCenter(Cell) + Offset, FRotator::ZeroRotator, Params);
	if (Companion)
	{
		Companion->InitCompanion(this, Cell);
		UE_LOG(LogTemp, Display, TEXT("HPWH: companion spawned at cell (%d,%d)"), Cell.X, Cell.Y);
	}
}

void AHPWH_WorldDirector::EvaluateBiome(const FIntPoint& Cell, EHPWH_TerrainType& OutBiome,
	float& OutDensityScale, int32& OutZone, FLinearColor& OutTint) const
{
	EvaluateBiomeAt(FVector2D(Cell.X + 0.5f, Cell.Y + 0.5f), Cell, OutBiome, OutDensityScale, OutZone, OutTint);
}

void AHPWH_WorldDirector::EvaluateBiomeAt(const FVector2D& RawPoint, const FIntPoint& Cell,
	EHPWH_TerrainType& OutBiome, float& OutDensityScale, int32& OutZone, FLinearColor& OutTint) const
{
	OutBiome = EHPWH_TerrainType::Empty;
	OutDensityScale = 1.f;
	OutZone = 0;
	OutTint = HPWH_TerrainColor(EHPWH_TerrainType::Empty);

	struct FCandidate
	{
		float Sdf = 0.f;
		FHPWH_BiomeSeed Seed;
	};

	const FVector2D Point = WarpPoint(RawPoint);
	TArray<FCandidate> Candidates;
	Candidates.Reserve(32);

	auto AddSeed = [&](const FHPWH_BiomeSeed& Seed)
	{
		const float Distance = FVector2D::Distance(Point, Seed.Position);
		const float EdgeNoise = (HPWHValueNoise(Point.X * 0.13f + Seed.Position.X,
			Point.Y * 0.13f + Seed.Position.Y, static_cast<uint32>(LayerSeed) ^ 0x454447u) * 2.f - 1.f) * 0.55f;
		FCandidate Candidate;
		Candidate.Sdf = Distance - Seed.Radius + EdgeNoise;
		Candidate.Seed = Seed;
		Candidates.Add(Candidate);
	};

	const int32 BaseLX = FMath::FloorToInt(static_cast<float>(Point.X) / BiomeSeedSpacing);
	const int32 BaseLY = FMath::FloorToInt(static_cast<float>(Point.Y) / BiomeSeedSpacing);
	for (int32 OffsetX = -2; OffsetX <= 2; ++OffsetX)
	{
		for (int32 OffsetY = -2; OffsetY <= 2; ++OffsetY)
		{
			AddSeed(MakeLatticeSeed(BaseLX + OffsetX, BaseLY + OffsetY));
		}
	}
	for (const FHPWH_BiomeSeed& Seed : ForcedBiomeSeeds)
	{
		AddSeed(Seed);
	}

	Candidates.Sort([](const FCandidate& A, const FCandidate& B) { return A.Sdf < B.Sdf; });
	if (Candidates.Num() == 0)
	{
		OutBiome = EHPWH_TerrainType::Books;
		OutTint = HPWH_TerrainColor(OutBiome);
		OutZone = 3;
		return;
	}

	const FCandidate& First = Candidates[0];
	OutBiome = First.Seed.Biome;
	OutTint = HPWH_TerrainColor(First.Seed.Biome);
	if (Candidates.Num() == 1)
	{
		return;
	}
	const FCandidate& Second = Candidates[1];
	const float Gap = Second.Sdf - First.Sdf;

	// 空白覆盖：最近圆也没覆盖时，用前三个候选的距离反比加权参数与颜色。
	if (First.Sdf > 0.f)
	{
		FVector Mixed = FVector::ZeroVector;
		FLinearColor MixedColor(0.f, 0.f, 0.f, 1.f);
		float TotalWeight = 0.f;
		for (int32 Index = 0; Index < FMath::Min(3, Candidates.Num()); ++Index)
		{
			const float Weight = 1.f / (FMath::Max(Candidates[Index].Sdf, 0.f) + 0.25f);
			Mixed += Candidates[Index].Seed.Params * Weight;
			const FLinearColor Color = HPWH_TerrainColor(Candidates[Index].Seed.Biome);
			MixedColor += Color * Weight;
			TotalWeight += Weight;
		}
		Mixed /= FMath::Max(TotalWeight, KINDA_SMALL_NUMBER);
		MixedColor /= FMath::Max(TotalWeight, KINDA_SMALL_NUMBER);
		OutBiome = BiomeFromParams(Mixed);
		OutTint = MixedColor;
		OutDensityScale = 0.78f;
		OutZone = 2;
		return;
	}

	// 边界带：gap < W 时地面颜色做连续混合，物件按权重做抖动混布。
	// W 保持 2.5～3 格宽，让颜色在多个格子内渐变，而不是一格内硬切。
	const float BoundaryWidth = FMath::Max(2.5f, FMath::Min(First.Seed.Radius, Second.Seed.Radius) * 0.55f);
	if (Gap < BoundaryWidth)
	{
		float WeightSecond = 0.5f * (1.f - Gap / BoundaryWidth);
		const float Noise = (HPWHValueNoise(Point.X * 0.19f, Point.Y * 0.19f,
			static_cast<uint32>(LayerSeed) ^ 0x424E44u) * 2.f - 1.f) * 0.12f;
		WeightSecond = FMath::Clamp(WeightSecond + Noise, 0.f, 1.f);

		const FLinearColor ColorFirst = HPWH_TerrainColor(First.Seed.Biome);
		const FLinearColor ColorSecond = HPWH_TerrainColor(Second.Seed.Biome);
		OutTint = ColorFirst + (ColorSecond - ColorFirst) * WeightSecond;

		const float Dither = HPWHHashUnit(HPWHHash4(static_cast<uint32>(LayerSeed),
			static_cast<uint32>(Cell.X), static_cast<uint32>(Cell.Y), 0x444954u));
		OutBiome = Dither < WeightSecond ? Second.Seed.Biome : First.Seed.Biome;
		OutDensityScale = 0.72f;
		OutZone = 1;

		// 较窄的中心缝使用更柔和的过渡地貌，形成可读的特殊边界带。
		if (Gap < BoundaryWidth * 0.45f)
		{
			const EHPWH_TerrainType A = First.Seed.Biome;
			const EHPWH_TerrainType B = Second.Seed.Biome;
			if (A == EHPWH_TerrainType::Obstacle || B == EHPWH_TerrainType::Obstacle)
			{
				OutBiome = A == EHPWH_TerrainType::Obstacle ? B : A;
				OutDensityScale = 0.42f;
			}
			else if ((A == EHPWH_TerrainType::Books && B == EHPWH_TerrainType::Paper) ||
				(A == EHPWH_TerrainType::Paper && B == EHPWH_TerrainType::Books) ||
				(A == EHPWH_TerrainType::Books && B == EHPWH_TerrainType::Blocks) ||
				(A == EHPWH_TerrainType::Blocks && B == EHPWH_TerrainType::Books))
			{
				OutBiome = EHPWH_TerrainType::Paper;
				OutDensityScale = 0.52f;
			}
			else if ((A == EHPWH_TerrainType::Desk && B == EHPWH_TerrainType::Blocks) ||
				(A == EHPWH_TerrainType::Blocks && B == EHPWH_TerrainType::Desk))
			{
				OutBiome = EHPWH_TerrainType::Blocks;
				OutDensityScale = 0.55f;
			}
			else if (A == EHPWH_TerrainType::Desk || B == EHPWH_TerrainType::Desk)
			{
				OutBiome = EHPWH_TerrainType::Desk;
				OutDensityScale = 0.60f;
			}
		}
		return;
	}
}

FHPWH_CellRecord AHPWH_WorldDirector::GenerateRecord(const FIntPoint& Cell) const
{
	FHPWH_CellRecord Record;
	Record.Coordinate = Cell;
	Record.LayerSeed = LayerSeed;

	const float DensityNoise = HPWHValueNoise(Cell.X * 0.37f + 13.7f, Cell.Y * 0.37f - 9.3f,
		static_cast<uint32>(LayerSeed) ^ 0x9E3779B9u);

	EHPWH_TerrainType Biome = EHPWH_TerrainType::Empty;
	float BiomeDensityScale = 1.f;
	int32 BiomeZone = 0;
	FLinearColor BiomeTint = FLinearColor::White;
	EvaluateBiome(Cell, Biome, BiomeDensityScale, BiomeZone, BiomeTint);
	(void)BiomeZone;
	Record.Density = DensityNoise * BiomeDensityScale;
	Record.GroundTint = BiomeTint;

	const FVector World = CellCenter(Cell);
	const bool bOriginSafe = World.Size2D() < OriginSafeRadiusCm;

	// 空旷地块固定约占 50%，其余格子按 SDF 群系生成，形成成片地带与过渡缝。
	const float EmptyRoll = HPWHHashUnit(HPWHHash4(static_cast<uint32>(LayerSeed),
		static_cast<uint32>(Cell.X), static_cast<uint32>(Cell.Y), 0x454D50u));
	if (EmptyRoll < 0.50f)
	{
		Record.Terrain = EHPWH_TerrainType::Empty;
	}
	else
	{
		Record.Terrain = Biome;
	}

	if (bOriginSafe)
	{
		Record.Terrain = EmptyRoll < 0.5f ? EHPWH_TerrainType::Empty : EHPWH_TerrainType::Paper;
		Record.Density *= 0.25f;
	}

	if (POICells.IsValidIndex(0) && Cell == POICells[0])
	{
		Record.Terrain = EHPWH_TerrainType::Mirror;
		Record.Density = 0.f;
		Record.GroundTint = TerrainColor(Record.Terrain);
		Record.bHasPOI = true;
	}
	else if (POICells.IsValidIndex(1) && Cell == POICells[1])
	{
		Record.Terrain = EHPWH_TerrainType::Courtyard;
		Record.Density = 0.f;
		Record.GroundTint = TerrainColor(Record.Terrain);
		Record.bHasPOI = true;
	}
	else if (POICells.IsValidIndex(2) && Cell == POICells[2])
	{
		Record.Terrain = EHPWH_TerrainType::Laptop;
		Record.Density = 0.f;
		Record.GroundTint = TerrainColor(Record.Terrain);
		Record.bHasPOI = true;
	}
	else if (Cell == GetExitCell())
	{
		Record.Terrain = EHPWH_TerrainType::Empty;
		Record.Density = 0.f;
	}
	else if (IsPuzzleAnchorCell(Cell))
	{
		Record.Terrain = PuzzleTerrainForCell(Cell);
		Record.Density = 0.f;
		Record.GroundTint = TerrainColor(Record.Terrain);
		Record.bHasPOI = true;
	}
	else if (IsNearMirrorClearing(Cell))
	{
		Record.Terrain = EHPWH_TerrainType::Empty;
		Record.Density = 0.f;
	}

	return Record;
}

FLinearColor AHPWH_WorldDirector::TerrainColor(EHPWH_TerrainType Terrain) const
{
	FLinearColor Color = HPWH_TerrainColor(Terrain);
	if (Terrain == EHPWH_TerrainType::Mirror || Terrain == EHPWH_TerrainType::Courtyard ||
		Terrain == EHPWH_TerrainType::Laptop)
	{
		Color *= 1.15f;
	}
	return Color;
}

UMaterialInterface* AHPWH_WorldDirector::GetTerrainMaterial(EHPWH_TerrainType Terrain)
{
	const uint8 Key = static_cast<uint8>(Terrain);
	if (const TObjectPtr<UMaterialInstanceDynamic>* Found = TerrainMaterials.Find(Key))
	{
		return Found->Get();
	}
	if (!BaseMaterial)
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(BaseMaterial, this);
	if (Mid)
	{
		Mid->SetVectorParameterValue(TEXT("Color"), TerrainColor(Terrain));
		TerrainMaterials.Add(Key, Mid);
	}
	return Mid;
}

UMaterialInterface* AHPWH_WorldDirector::GetBlendedTerrainMaterial(const FLinearColor& Tint)
{
	// 颜色量化后缓存，避免每个格子都创建新的 MID。
	const uint32 Key = (static_cast<uint32>(FMath::Clamp(FMath::RoundToInt(Tint.R * 31.f), 0, 31)) << 10) |
		(static_cast<uint32>(FMath::Clamp(FMath::RoundToInt(Tint.G * 31.f), 0, 31)) << 5) |
		static_cast<uint32>(FMath::Clamp(FMath::RoundToInt(Tint.B * 31.f), 0, 31));
	if (const TObjectPtr<UMaterialInstanceDynamic>* Found = BlendedTerrainMaterials.Find(Key))
	{
		return Found->Get();
	}
	if (!BaseMaterial)
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(BaseMaterial, this);
	if (Mid)
	{
		Mid->SetVectorParameterValue(TEXT("Color"), Tint);
		BlendedTerrainMaterials.Add(Key, Mid);
	}
	return Mid;
}

UMaterialInterface* AHPWH_WorldDirector::GetObstacleMaterial(EHPWH_TerrainType Terrain)
{
	const uint8 Key = static_cast<uint8>(Terrain);
	if (const TObjectPtr<UMaterialInstanceDynamic>* Found = ObstacleMaterials.Find(Key))
	{
		return Found->Get();
	}
	if (!BaseMaterial)
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(BaseMaterial, this);
	if (Mid)
	{
		FLinearColor Color = TerrainColor(Terrain);
		Color = FLinearColor(Color.R * 0.62f, Color.G * 0.62f, Color.B * 0.62f, 1.f);
		Mid->SetVectorParameterValue(TEXT("Color"), Color);
		ObstacleMaterials.Add(Key, Mid);
	}
	return Mid;
}

void AHPWH_WorldDirector::RefreshAroundPlayer()
{
	auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	AHPWH_PlayerCharacter* Body = GM ? GM->GetPlayerBody() : nullptr;
	AHPWH_SoulAvatar* Soul = GM ? GM->GetSoulAvatar() : nullptr;

	APawn* Pawn = Soul ? static_cast<APawn*>(Soul) : (Body ? static_cast<APawn*>(Body) : nullptr);
	if (!Pawn)
	{
		if (const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			Pawn = PC->GetPawn();
		}
	}
	if (!Pawn)
	{
		return;
	}

	CenterCell = WorldToCell(Pawn->GetActorLocation());

	TSet<FIntPoint> Desired;
	auto CollectArea = [this, &Desired](const FVector& WorldLocation)
	{
		const FIntPoint Cell = WorldToCell(WorldLocation);
		for (int32 DX = -ActiveRadius; DX <= ActiveRadius; ++DX)
		{
			for (int32 DY = -ActiveRadius; DY <= ActiveRadius; ++DY)
			{
				Desired.Add(Cell + FIntPoint(DX, DY));
			}
		}
	};

	CollectArea(Pawn->GetActorLocation());
	if (Soul && Body)
	{
		// 灵魂离体时同时保持肉身周围可见，玩家才能看见并找回身体。
		CollectArea(Body->GetActorLocation());
	}

	// 每次窗口刷新先决定唯一的解密锚点，再按锚点生成地块。
	EnsurePuzzleAnchor(Desired);

	TArray<FIntPoint> ToRemove;
	for (const auto& Pair : LoadedCells)
	{
		if (!Desired.Contains(Pair.Key))
		{
			ToRemove.Add(Pair.Key);
		}
	}
	for (const FIntPoint& Cell : ToRemove)
	{
		DespawnCell(Cell);
	}

	for (const FIntPoint& Cell : Desired)
	{
		if (!LoadedCells.Contains(Cell))
		{
			SpawnCell(Cell);
		}
		if (FHPWH_CellRecord* Record = Records.Find(Cell))
		{
			Record->bVisited = true;
		}
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugGen")))
	{
		int32 EmptyCount = 0;
		int32 RecordCount = 0;
		int32 POICount = 0;
		int32 TerrainCounts[9] = { 0 };
		for (const FIntPoint& Cell : Desired)
		{
			if (const FHPWH_CellRecord* Record = Records.Find(Cell))
			{
				++RecordCount;
				TerrainCounts[static_cast<int32>(Record->Terrain)]++;
				if (Record->Terrain == EHPWH_TerrainType::Empty)
				{
					++EmptyCount;
				}
				if (Record->bHasPOI)
				{
					++POICount;
				}
			}
		}
		UE_LOG(LogTemp, Display,
			TEXT("HPWH: window center=(%d,%d) empty=%d/%d poi=%d anchors=%d paper=%d blocks=%d obstacle=%d desk=%d books=%d"),
			CenterCell.X, CenterCell.Y, EmptyCount, RecordCount, POICount, PuzzleAnchors.Num(),
			TerrainCounts[1], TerrainCounts[2], TerrainCounts[3], TerrainCounts[4], TerrainCounts[5]);
	}
}

void AHPWH_WorldDirector::SpawnCell(const FIntPoint& Cell)
{
	FHPWH_CellRecord* RecordPtr = Records.Find(Cell);
	if (!RecordPtr)
	{
		Records.Add(Cell, GenerateRecord(Cell));
		RecordPtr = Records.Find(Cell);
	}
	if (!RecordPtr)
	{
		return;
	}
	FHPWH_CellRecord& Record = *RecordPtr;

	TSubclassOf<AHPWH_CellPrefabBase> PrefabClass = nullptr;
	const bool bPOICell = Record.bHasPOI || Cell == GetExitCell();
	if (!bPOICell && TerrainPrefabs)
	{
		const int32 VariantSeed = static_cast<int32>(HPWHHash4(
			static_cast<uint32>(LayerSeed), static_cast<uint32>(Cell.X), static_cast<uint32>(Cell.Y), 0x505246u));
		PrefabClass = TerrainPrefabs->PickPrefab(Record.Terrain, VariantSeed);
	}
	const bool bSkipProcedural = PrefabClass && TerrainPrefabs && TerrainPrefabs->bReplaceProceduralObstacles;

	AHPWH_CellChunk* Chunk = GetWorld()->SpawnActor<AHPWH_CellChunk>(AHPWH_CellChunk::StaticClass(), CellCenter(Cell), FRotator::ZeroRotator);
	if (!Chunk)
	{
		return;
	}
	UMaterialInterface* GroundMaterial = GetTerrainMaterial(Record.Terrain);
	if (!bPOICell && !Record.GroundTint.Equals(FLinearColor::White, 0.01f))
	{
		GroundMaterial = GetBlendedTerrainMaterial(Record.GroundTint);
	}
	// 统一地板材质：普通格、器物格、八卦镜与四合院底部都使用导入地板。
	if (FloorMaterial)
	{
		GroundMaterial = FloorMaterial;
	}

	// 非关键器物格：整格地面改为 3×3 细分，按各自坐标采样群系色，边界带真正连续。
	TArray<UMaterialInterface*> GroundTileMaterials;
	if (!bPOICell && !FloorMaterial)
	{
		const int32 TileCount = AHPWH_CellChunk::GroundTileCount;
		const float Inv = 1.f / static_cast<float>(TileCount);
		GroundTileMaterials.Reserve(TileCount * TileCount);
		for (int32 Y = 0; Y < TileCount; ++Y)
		{
			for (int32 X = 0; X < TileCount; ++X)
			{
				EHPWH_TerrainType TileBiome = EHPWH_TerrainType::Empty;
				float TileDensity = 1.f;
				int32 TileZone = 0;
				FLinearColor TileTint = FLinearColor::White;
				EvaluateBiomeAt(FVector2D(Cell.X + (X + 0.5f) * Inv, Cell.Y + (Y + 0.5f) * Inv), Cell,
					TileBiome, TileDensity, TileZone, TileTint);
				GroundTileMaterials.Add(GetBlendedTerrainMaterial(TileTint));
			}
		}
	}
	Chunk->BuildCell(Record, CellSizeCm, GroundMaterial, GetObstacleMaterial(Record.Terrain), !bSkipProcedural,
		&GroundTileMaterials);
	LoadedCells.Add(Cell, Chunk);

	if (PrefabClass)
	{
		if (AHPWH_CellPrefabBase* Prefab = GetWorld()->SpawnActor<AHPWH_CellPrefabBase>(
			PrefabClass, CellCenter(Cell), FRotator::ZeroRotator))
		{
			Prefab->PlaceInCell(Cell, CellSizeCm);
			CellActors.FindOrAdd(Cell).Add(Prefab);
		}
	}

	if (Record.Terrain == EHPWH_TerrainType::Mirror && !POIActors.Contains(Cell))
	{
		if (AHPWH_MirrorPOI* POI = GetWorld()->SpawnActor<AHPWH_MirrorPOI>(AHPWH_MirrorPOI::StaticClass(), CellCenter(Cell), FRotator::ZeroRotator))
		{
			POI->InitializePOI(this, Cell, Record.bPuzzleSolved);
			POIActors.Add(Cell, POI);
		}
	}
	else if (Record.Terrain == EHPWH_TerrainType::Courtyard && !POIActors.Contains(Cell))
	{
		if (AHPWH_CourtyardPOI* POI = GetWorld()->SpawnActor<AHPWH_CourtyardPOI>(AHPWH_CourtyardPOI::StaticClass(), CellCenter(Cell), FRotator::ZeroRotator))
		{
			POI->InitializePOI(this, Cell, Record.bPuzzleSolved);
			POIActors.Add(Cell, POI);
		}
	}
	else if (Record.Terrain == EHPWH_TerrainType::Laptop && !POIActors.Contains(Cell))
	{
		if (AHPWH_LaptopPOI* POI = GetWorld()->SpawnActor<AHPWH_LaptopPOI>(AHPWH_LaptopPOI::StaticClass(), CellCenter(Cell), FRotator::ZeroRotator))
		{
			POI->InitializePOI(this, Cell, Record.bPuzzleSolved);
			POIActors.Add(Cell, POI);
		}
	}

	if (const TObjectPtr<AActor>* POIPtr = POIActors.Find(Cell))
	{
		UE_LOG(LogTemp, Display, TEXT("HPWH: POI %s at cell (%d,%d) terrain=%d solved=%d"),
			IsValid(*POIPtr) ? *(*POIPtr)->GetName() : TEXT("invalid"),
			Cell.X, Cell.Y, static_cast<int32>(Record.Terrain), Record.bPuzzleSolved ? 1 : 0);
	}

	SpawnEncounterMonsters(Record);
}

void AHPWH_WorldDirector::RestoreProgress(const TMap<FIntPoint, FHPWH_CellRecord>& InRecords,
	const TArray<FIntPoint>& InAnchors)
{
	TArray<FIntPoint> LoadedKeys;
	LoadedCells.GetKeys(LoadedKeys);
	for (const FIntPoint& Cell : LoadedKeys)
	{
		DespawnCell(Cell);
	}
	LoadedCells.Empty();

	for (auto It = CellActors.CreateIterator(); It; ++It)
	{
		for (const TWeakObjectPtr<AActor>& Actor : It.Value())
		{
			if (Actor.IsValid())
			{
				Actor->Destroy();
			}
		}
	}
	CellActors.Empty();
	for (auto It = POIActors.CreateIterator(); It; ++It)
	{
		if (IsValid(It.Value()))
		{
			It.Value()->Destroy();
		}
	}
	POIActors.Empty();

	Records = InRecords;
	PuzzleAnchors.Reset();
	for (const FIntPoint& Anchor : InAnchors)
	{
		PuzzleAnchors.Add(Anchor);
	}
	CenterCell = FIntPoint(999999, 999999);
	RefreshAroundPlayer();
	UE_LOG(LogTemp, Display, TEXT("HPWH: world progress restored records=%d anchors=%d"),
		Records.Num(), PuzzleAnchors.Num());
}

void AHPWH_WorldDirector::DespawnCell(const FIntPoint& Cell)
{
	if (TObjectPtr<AHPWH_CellChunk>* Chunk = LoadedCells.Find(Cell))
	{
		if (IsValid(*Chunk))
		{
			(*Chunk)->Destroy();
		}
		LoadedCells.Remove(Cell);
	}

	if (TArray<TWeakObjectPtr<AActor>>* Actors = CellActors.Find(Cell))
	{
		for (const TWeakObjectPtr<AActor>& Actor : *Actors)
		{
			if (Actor.IsValid())
			{
				Actor->Destroy();
			}
		}
		CellActors.Remove(Cell);
	}

	if (TObjectPtr<AActor>* POIPtr = POIActors.Find(Cell))
	{
		if (IsValid(*POIPtr))
		{
			if (AHPWH_POIBase* POI = Cast<AHPWH_POIBase>(POIPtr->Get()))
			{
				POI->ResetPuzzle();
			}
			POIPtr->Get()->Destroy();
		}
		POIActors.Remove(Cell);
	}
}

void AHPWH_WorldDirector::SpawnEncounterMonsters(const FHPWH_CellRecord& Record)
{
	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugNoSpawn")))
	{
		return;
	}
	FHPWH_CellRecord* MutableRecord = Records.Find(Record.Coordinate);
	if (!MutableRecord || MutableRecord->bHasPOI)
	{
		return;
	}
	if (Record.Coordinate == GetExitCell() || IsNearMirrorClearing(Record.Coordinate))
	{
		return;
	}
	if (CellCenter(Record.Coordinate).Size2D() < OriginSafeRadiusCm)
	{
		return;
	}

	if (!MutableRecord->bHasRolledEncounter)
	{
		MutableRecord->bHasRolledEncounter = true;
		const uint32 EncounterSeed = HPWHHash4(static_cast<uint32>(LayerSeed),
			static_cast<uint32>(Record.Coordinate.X), static_cast<uint32>(Record.Coordinate.Y), 0x454E43u);
		const float Roll = HPWHHashUnit(EncounterSeed);
		MutableRecord->EncounterCount = Roll < 0.20f ? 2 : 0;
		MutableRecord->bHasCoin = HPWHHashUnit(HPWHHash4(EncounterSeed, 0xC01Au, 0, 0)) < 0.07f;
	}

	if (TArray<TWeakObjectPtr<AActor>>* Existing = CellActors.Find(Record.Coordinate))
	{
		for (int32 I = Existing->Num() - 1; I >= 0; --I)
		{
			if (!(*Existing)[I].IsValid())
			{
				Existing->RemoveAt(I);
			}
		}
	}

	FVector PlayerLocation = FVector::ZeroVector;
	bool bHasPlayer = false;
	if (auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (const AHPWH_PlayerCharacter* Body = GM->GetPlayerBody())
		{
			PlayerLocation = Body->GetActorLocation();
			bHasPlayer = true;
		}
	}

	TArray<TWeakObjectPtr<AActor>>& Actors = CellActors.FindOrAdd(Record.Coordinate);
	int32 AliveNormal = 0;
	for (const TWeakObjectPtr<AActor>& Actor : Actors)
	{
		if (Actor.IsValid() && Actor->IsA<AHPWH_EnemyBase>())
		{
			++AliveNormal;
		}
	}
	const int32 Missing = FMath::Max(0, MutableRecord->EncounterCount - MutableRecord->KilledCount - AliveNormal);
	bool bPlayedSpawnSfx = false;

	for (int32 I = 0; I < Missing; ++I)
	{
		const uint32 KindSeed = HPWHHash4(static_cast<uint32>(LayerSeed),
			static_cast<uint32>(Record.Coordinate.X), static_cast<uint32>(Record.Coordinate.Y),
			static_cast<uint32>(MutableRecord->KilledCount + I) * 31u + 7u);
		const float KindRoll = HPWHHashUnit(KindSeed);

		UClass* EnemyClass = AHPWH_MeleeMonster::StaticClass();
		if (KindRoll > 0.55f && KindRoll <= 0.82f)
		{
			EnemyClass = AHPWH_RangedMonster::StaticClass();
		}
		else if (KindRoll > 0.82f)
		{
			EnemyClass = AHPWH_SoulSpider::StaticClass();
		}

		FRandomStream PositionStream(static_cast<int32>(KindSeed));
		FVector Location = CellCenter(Record.Coordinate);
		float BestDistance = -1.f;
		for (int32 Attempt = 0; Attempt < 8; ++Attempt)
		{
			const float Angle = PositionStream.FRandRange(0.f, 2.f * PI);
			const float Radius = PositionStream.FRandRange(CellSizeCm * 0.18f, CellSizeCm * 0.38f);
			const FVector Candidate = CellCenter(Record.Coordinate) +
				FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);
			const float DistanceToPlayer = bHasPlayer ? FVector::Dist2D(Candidate, PlayerLocation) : TNumericLimits<float>::Max();
			if (DistanceToPlayer > BestDistance)
			{
				BestDistance = DistanceToPlayer;
				Location = Candidate;
			}
			if (DistanceToPlayer >= MinMonsterSpawnDistanceCm)
			{
				break;
			}
		}
		Location.Z = 100.f;

		AHPWH_EnemyBase* Enemy = GetWorld()->SpawnActor<AHPWH_EnemyBase>(EnemyClass, Location, FRotator(0.f, PositionStream.FRandRange(0.f, 360.f), 0.f));
		if (Enemy)
		{
			Enemy->InitEnemy(this, Record.Coordinate, true);
			Actors.Add(Enemy);
			if (auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
			{
				// 同一格可能一次生成两只，只播一次出现音，避免叠音。
				if (!bPlayedSpawnSfx && bHasPlayer && FVector::Dist2D(Location, PlayerLocation) < 3000.f)
				{
					GM->PlaySfxAt(EHPWH_Sfx::EnemySpawn, Location, 0.45f);
					bPlayedSpawnSfx = true;
				}
			}
			UE_LOG(LogTemp, Display, TEXT("HPWH: spawn %s hp=%.0f at cell (%d,%d)"),
				*EnemyClass->GetName(), HPWH_MonsterMaxHealth(Record.Coordinate), Record.Coordinate.X, Record.Coordinate.Y);
		}
	}

	if (MutableRecord->bHasCoin)
	{
		bool bHasCoinActor = false;
		for (const TWeakObjectPtr<AActor>& Actor : Actors)
		{
			if (Actor.IsValid() && Actor->IsA<AHPWH_CoinMonster>())
			{
				bHasCoinActor = true;
				break;
			}
		}
		if (!bHasCoinActor)
		{
			const FVector Location = CellCenter(Record.Coordinate) + FVector(180.f, -140.f, 0.f);
			if (AHPWH_CoinMonster* Coin = GetWorld()->SpawnActor<AHPWH_CoinMonster>(AHPWH_CoinMonster::StaticClass(), Location, FRotator::ZeroRotator))
			{
				Actors.Add(Coin);
			}
		}
	}
}

void AHPWH_WorldDirector::NotifyEnemyKilled(const FIntPoint& Cell)
{
	if (FHPWH_CellRecord* Record = Records.Find(Cell))
	{
		Record->KilledCount = FMath::Min(Record->EncounterCount, Record->KilledCount + 1);
	}
}

void AHPWH_WorldDirector::NotifyPuzzleSolved(const FIntPoint& Cell)
{
	if (FHPWH_CellRecord* Record = Records.Find(Cell))
	{
		Record->bPuzzleSolved = true;
	}
}

void AHPWH_WorldDirector::NotifyCoreCollected(const FIntPoint& Cell)
{
	if (FHPWH_CellRecord* Record = Records.Find(Cell))
	{
		Record->bPuzzleSolved = true;
		Record->bCoreCollected = true;
	}
}

void AHPWH_WorldDirector::SpawnRemainingEnemies(const FIntPoint& Cell)
{
	if (const FHPWH_CellRecord* Record = Records.Find(Cell))
	{
		SpawnEncounterMonsters(*Record);
	}
}
