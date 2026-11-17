#pragma once

#include "CoreMinimal.h"
#include "HPWH_Types.generated.h"

UENUM(BlueprintType)
enum class EHPWH_TerrainType : uint8
{
	Empty UMETA(DisplayName = "空地"),
	Paper UMETA(DisplayName = "纸张聚集"),
	Blocks UMETA(DisplayName = "积木聚集"),
	Obstacle UMETA(DisplayName = "障碍物区块"),
	Desk UMETA(DisplayName = "案头工作区"),
	Books UMETA(DisplayName = "书本散落"),
	Mirror UMETA(DisplayName = "八卦镜空地"),
	Courtyard UMETA(DisplayName = "四合院孤置区"),
	Laptop UMETA(DisplayName = "笔记本电脑")
};

UENUM(BlueprintType)
enum class EHPWH_FloorState : uint8
{
	Exploring,
	PuzzleActive,
	CoreAvailable,
	CoreCollected,
	ExitRolling,
	FloorTransition,
	RunAwake
};

UENUM(BlueprintType)
enum class EHPWH_SoulState : uint8
{
	Embodied,
	SoulOut,
	ReturningToBody
};

UENUM(BlueprintType)
enum class EHPWH_MarkState : uint8
{
	Unmarked,
	Marked
};

UENUM(BlueprintType)
enum class EHPWH_LifeState : uint8
{
	Alive,
	Dead,
	Respawning
};

UENUM(BlueprintType)
enum class EHPWH_CrawlerState : uint8
{
	Dormant,
	Approaching,
	ChasingBody,
	ChasingSoul,
	Disrupted,
	Relocating
};

/** 原型音效编号，与 /Game/HPWH/Audio/SFX 下的资源一一对应。 */
UENUM(BlueprintType)
enum class EHPWH_Sfx : uint8
{
	Attack UMETA(DisplayName = "攻击"),
	SoulOut UMETA(DisplayName = "灵魂离体"),
	EnemySpawn UMETA(DisplayName = "怪物出现"),
	CrawlerAppear UMETA(DisplayName = "徘徊者出现"),
	CardAppear UMETA(DisplayName = "卡片出现"),
	PuzzleSolve UMETA(DisplayName = "解密成功"),
	Footstep UMETA(DisplayName = "脚步"),
	Win UMETA(DisplayName = "通关"),
	TileLaptop UMETA(DisplayName = "笔记本电流"),
	TileMirror UMETA(DisplayName = "八卦镜钟声"),
	TileCourtyard UMETA(DisplayName = "四合院乌鸦")
};

USTRUCT(BlueprintType)
struct FHPWH_CellRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FIntPoint Coordinate = FIntPoint::ZeroValue;
	UPROPERTY(BlueprintReadOnly) int32 LayerSeed = 0;
	UPROPERTY(BlueprintReadOnly) EHPWH_TerrainType Terrain = EHPWH_TerrainType::Empty;
	UPROPERTY(BlueprintReadOnly) FLinearColor GroundTint = FLinearColor::White;
	UPROPERTY(BlueprintReadOnly) float Density = 0.5f;
	UPROPERTY(BlueprintReadOnly) bool bVisited = false;
	UPROPERTY(BlueprintReadOnly) bool bHasRolledEncounter = false;
	UPROPERTY(BlueprintReadOnly) int32 EncounterCount = 0;
	UPROPERTY(BlueprintReadOnly) int32 KilledCount = 0;
	UPROPERTY(BlueprintReadOnly) bool bHasCoin = false;
	UPROPERTY(BlueprintReadOnly) bool bHasPOI = false;
	UPROPERTY(BlueprintReadOnly) bool bPuzzleSolved = false;
	UPROPERTY(BlueprintReadOnly) bool bCoreCollected = false;
};

/** 手机横屏的固定触摸布局，HUD 与输入共用，避免两边坐标不一致。 */
struct FHPWH_TouchLayout
{
	FVector2D JoystickCenter = FVector2D::ZeroVector;
	float JoystickRadius = 80.f;
	FVector2D AttackCenter = FVector2D::ZeroVector;
	float AttackRadius = 56.f;
	FVector2D InteractCenter = FVector2D::ZeroVector;
	float InteractRadius = 44.f;
};

inline FHPWH_TouchLayout HPWH_MakeTouchLayout(const FVector2D& ViewportSize)
{
	FHPWH_TouchLayout Layout;
	const float MinSide = FMath::Min(ViewportSize.X, ViewportSize.Y);
	Layout.JoystickRadius = MinSide * 0.10f;
	// 攻击键与移动键同尺寸，并以移动键的中心 Y 为标准水平对齐。
	Layout.AttackRadius = Layout.JoystickRadius;
	Layout.InteractRadius = MinSide * 0.058f;
	Layout.JoystickCenter = FVector2D(ViewportSize.X * 0.16f, ViewportSize.Y * 0.74f);
	Layout.AttackCenter = FVector2D(ViewportSize.X * 0.86f, Layout.JoystickCenter.Y);
	// 交互键紧贴攻击键上方，手指从攻击键拖上去即可交互。
	Layout.InteractCenter = Layout.AttackCenter + FVector2D(0.f, -(Layout.AttackRadius * 2.f + MinSide * 0.022f));
	return Layout;
}

/** 屏幕触摸位移 → 世界移动输入：屏幕 Y 轴向下为正，前进方向需要取反。 */
inline FVector2D HPWH_TouchToMoveInput(const FVector2D& ScreenDelta)
{
	return FVector2D(ScreenDelta.X, -ScreenDelta.Y);
}

inline FLinearColor HPWH_TerrainColor(EHPWH_TerrainType Terrain)
{
	switch (Terrain)
	{
	case EHPWH_TerrainType::Paper: return FLinearColor(0.66f, 0.63f, 0.56f, 1.f);
	case EHPWH_TerrainType::Blocks: return FLinearColor(0.52f, 0.28f, 0.24f, 1.f);
	case EHPWH_TerrainType::Obstacle: return FLinearColor(0.22f, 0.23f, 0.27f, 1.f);
	case EHPWH_TerrainType::Desk: return FLinearColor(0.33f, 0.20f, 0.12f, 1.f);
	case EHPWH_TerrainType::Books: return FLinearColor(0.20f, 0.32f, 0.26f, 1.f);
	case EHPWH_TerrainType::Mirror: return FLinearColor(0.42f, 0.56f, 0.58f, 1.f);
	case EHPWH_TerrainType::Courtyard: return FLinearColor(0.38f, 0.34f, 0.27f, 1.f);
	case EHPWH_TerrainType::Laptop: return FLinearColor(0.20f, 0.26f, 0.36f, 1.f);
	default: return FLinearColor(0.10f, 0.10f, 0.13f, 1.f);
	}
}

inline uint32 HPWHHash4(uint32 A, uint32 B, uint32 C, uint32 D)
{
	uint32 H = A * 374761393u + B * 668265263u + C * 2147483647u + D * 1274126177u;
	H = (H ^ (H >> 13)) * 1274126177u;
	return H ^ (H >> 16);
}

inline float HPWHHashUnit(uint32 Hash)
{
	return static_cast<float>(Hash & 0x00FFFFFFu) / 16777216.f;
}

inline float HPWHValueNoise(float X, float Y, uint32 Seed)
{
	const int32 X0 = FMath::FloorToInt(X);
	const int32 Y0 = FMath::FloorToInt(Y);
	const float FX = FMath::SmoothStep(0.f, 1.f, X - X0);
	const float FY = FMath::SmoothStep(0.f, 1.f, Y - Y0);

	const float V00 = HPWHHashUnit(HPWHHash4(static_cast<uint32>(X0), static_cast<uint32>(Y0), Seed, 17u));
	const float V10 = HPWHHashUnit(HPWHHash4(static_cast<uint32>(X0 + 1), static_cast<uint32>(Y0), Seed, 17u));
	const float V01 = HPWHHashUnit(HPWHHash4(static_cast<uint32>(X0), static_cast<uint32>(Y0 + 1), Seed, 17u));
	const float V11 = HPWHHashUnit(HPWHHash4(static_cast<uint32>(X0 + 1), static_cast<uint32>(Y0 + 1), Seed, 17u));

	return FMath::Lerp(FMath::Lerp(V00, V10, FX), FMath::Lerp(V01, V11, FX), FY);
}

/** 世界坐标 → 格子坐标；格子 N 覆盖 [N*Size, (N+1)*Size)，与 Floor 规则一致。 */
inline FIntPoint HPWH_WorldToCell(const FVector& WorldLocation, float CellSizeCm)
{
	if (CellSizeCm <= KINDA_SMALL_NUMBER)
	{
		return FIntPoint::ZeroValue;
	}
	return FIntPoint(
		FMath::FloorToInt(static_cast<float>(WorldLocation.X) / CellSizeCm),
		FMath::FloorToInt(static_cast<float>(WorldLocation.Y) / CellSizeCm));
}

/** 格子几何中心；预制体、POI、地面方块都以此为中心放置。 */
inline FVector HPWH_CellCenter(const FIntPoint& Cell, float CellSizeCm)
{
	return FVector((Cell.X + 0.5f) * CellSizeCm, (Cell.Y + 0.5f) * CellSizeCm, 0.f);
}

/** 角色基础移速；怪物按倍率分类与灵魂提速都以此为基准。 */
inline constexpr float HPWH_PlayerBaseSpeed = 480.f;
/** 灵魂离体后的速度倍率：基础移速的 120%。 */
inline constexpr float HPWH_SoulSpeedMultiplier = 1.2f;
/** 普通怪物统一攻击力。 */
inline constexpr float HPWH_MonsterAttackDamage = 5.f;

/** 普通怪生命：初始 10，每 5 格增加 5，按切比雪夫格子环计算。 */
inline float HPWH_MonsterMaxHealth(const FIntPoint& Cell)
{
	const int32 Ring = FMath::Max(FMath::Abs(Cell.X), FMath::Abs(Cell.Y));
	return 10.f + 5.f * static_cast<float>(Ring / 5);
}

/** 把世界坐标吸附到 0.1 米精度（10cm），用于匍匐者的虚拟位置。 */
inline FVector HPWH_SnapDecimeter(const FVector& Location)
{
	return FVector(
		FMath::RoundToFloat(static_cast<float>(Location.X) / 10.f) * 10.f,
		FMath::RoundToFloat(static_cast<float>(Location.Y) / 10.f) * 10.f,
		FMath::RoundToFloat(static_cast<float>(Location.Z) / 10.f) * 10.f);
}

/** 解密区块线：绝对值 5、15、30，之后每 30 格一条横线或竖线。 */
inline bool HPWH_IsPuzzleLineCell(const FIntPoint& Cell, int32 Interval = 30)
{
	const int32 AbsX = FMath::Abs(Cell.X);
	const int32 AbsY = FMath::Abs(Cell.Y);
	auto IsLine = [Interval](int32 Value)
	{
		if (Value == 0)
		{
			return false;
		}
		if (Value == 5 || Value == 15)
		{
			return true;
		}
		return Value >= 30 && Interval > 0 && Value % Interval == 0;
	};
	return IsLine(AbsX) || IsLine(AbsY);
}
