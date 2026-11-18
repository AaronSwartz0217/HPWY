#pragma once

#include "CoreMinimal.h"

enum class EHPWH_MenuPage : uint8
{
	Main,
	About,
	Settings
};

enum class EHPWH_GraphicsMode : uint8
{
	DLSS,
	TSR
};

/** 主界面布局：HUD 绘制与鼠标/触摸命中共用。 */
struct FHPWH_MenuLayout
{
	FVector2D ButtonSize = FVector2D(380.f, 66.f);
	FVector2D MainButtons[4];
	FVector2D BackCenter = FVector2D::ZeroVector;
	FVector2D SettingsRows[2];
	FVector2D SettingsRowSize = FVector2D(440.f, 66.f);
};

inline FHPWH_MenuLayout HPWH_MakeMenuLayout(const FVector2D& ViewportSize)
{
	FHPWH_MenuLayout Layout;
	Layout.ButtonSize = FVector2D(FMath::Clamp(ViewportSize.X * 0.22f, 280.f, 420.f),
		FMath::Clamp(ViewportSize.Y * 0.075f, 54.f, 78.f));
	Layout.SettingsRowSize = FVector2D(Layout.ButtonSize.X * 1.18f, Layout.ButtonSize.Y);
	for (int32 Index = 0; Index < 4; ++Index)
	{
		Layout.MainButtons[Index] = FVector2D(ViewportSize.X * 0.5f, ViewportSize.Y * (0.44f + Index * 0.095f));
	}
	for (int32 Index = 0; Index < 2; ++Index)
	{
		Layout.SettingsRows[Index] = FVector2D(ViewportSize.X * 0.5f, ViewportSize.Y * (0.46f + Index * 0.12f));
	}
	Layout.BackCenter = FVector2D(ViewportSize.X * 0.5f, ViewportSize.Y * 0.82f);
	return Layout;
}

inline bool HPWH_IsPointInMenuButton(const FVector2D& Point, const FVector2D& Center, const FVector2D& Size)
{
	return Point.X >= Center.X - Size.X * 0.5f && Point.X <= Center.X + Size.X * 0.5f &&
		Point.Y >= Center.Y - Size.Y * 0.5f && Point.Y <= Center.Y + Size.Y * 0.5f;
}

/** 游戏内顶部菜单、梦境档案面板与按钮布局。 */
struct FHPWH_InGameMenuLayout
{
	FVector2D ButtonSize = FVector2D(92.f, 34.f);
	FVector2D ButtonCenter = FVector2D::ZeroVector;
	FVector2D PanelPos = FVector2D::ZeroVector;
	FVector2D PanelSize = FVector2D(920.f, 560.f);
	FVector2D RowSize = FVector2D(176.f, 42.f);
	FVector2D Rows[3];
	FVector2D SettingsRowSize = FVector2D(360.f, 46.f);
	FVector2D SettingsRows[2];
	FVector2D BackCenter = FVector2D::ZeroVector;
};

inline FHPWH_InGameMenuLayout HPWH_MakeInGameMenuLayout(const FVector2D& ViewportSize)
{
	FHPWH_InGameMenuLayout Layout;
	Layout.ButtonCenter = FVector2D(ViewportSize.X * 0.5f, 28.f);
	// 全屏菜单：整页覆盖视口，排版参照强化选择界面。
	Layout.PanelPos = FVector2D::ZeroVector;
	Layout.PanelSize = ViewportSize;
	const float BottomY = ViewportSize.Y - FMath::Clamp(ViewportSize.Y * 0.06f, 48.f, 80.f);
	Layout.RowSize = FVector2D(FMath::Clamp(ViewportSize.X * 0.11f, 150.f, 210.f), 44.f);
	const float Spacing = Layout.RowSize.X + 18.f;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		Layout.Rows[Index] = FVector2D(ViewportSize.X * 0.5f + (Index - 1) * Spacing, BottomY);
	}
	for (int32 Index = 0; Index < 2; ++Index)
	{
		Layout.SettingsRows[Index] = FVector2D(ViewportSize.X * 0.5f,
			ViewportSize.Y * 0.44f + Index * ViewportSize.Y * 0.10f);
	}
	Layout.BackCenter = FVector2D(ViewportSize.X * 0.5f, Layout.SettingsRows[1].Y + 140.f);
	return Layout;
}

/** 异层传送门的单选红卡布局。 */
struct FHPWH_PortalCardLayout
{
	FVector2D CardSize = FVector2D(480.f, 330.f);
	FVector2D Center = FVector2D::ZeroVector;
	FVector2D ButtonCenter = FVector2D::ZeroVector;
	FVector2D ButtonSize = FVector2D(280.f, 56.f);
};

inline FHPWH_PortalCardLayout HPWH_MakePortalCardLayout(const FVector2D& ViewportSize)
{
	FHPWH_PortalCardLayout Layout;
	Layout.CardSize = FVector2D(FMath::Clamp(ViewportSize.X * 0.34f, 380.f, 520.f),
		FMath::Clamp(ViewportSize.Y * 0.46f, 300.f, 380.f));
	Layout.Center = FVector2D(ViewportSize.X * 0.5f, ViewportSize.Y * 0.5f);
	Layout.ButtonCenter = FVector2D(Layout.Center.X, Layout.Center.Y + Layout.CardSize.Y * 0.5f - 54.f);
	return Layout;
}
