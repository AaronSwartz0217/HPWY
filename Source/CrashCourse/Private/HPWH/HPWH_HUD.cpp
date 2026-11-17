#include "HPWH/HPWH_HUD.h"

#include "HPWH/HPWH_GameMode.h"
#include "HPWH/HPWH_Player.h"
#include "HPWH/HPWH_PlayerController.h"
#include "HPWH/HPWH_World.h"
#include "HPWH/HPWH_Crawler.h"
#include "HPWH/HPWH_POI.h"
#include "HPWH/HPWH_Upgrades.h"
#include "HPWH/HPWH_Menu.h"
#include "UI/CC_GameFont.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/CanvasPanel.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/GameplayStatics.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace
{
	// 与宣传网站一致的设定图色系：午夜蓝、冷光、黄铜与镜穗红。
	const FLinearColor HPWHColorInk(0.780f, 0.850f, 0.930f, 1.f);
	const FLinearColor HPWHColorInkDim(0.470f, 0.570f, 0.710f, 1.f);
	const FLinearColor HPWHColorMuted(0.210f, 0.290f, 0.400f, 1.f);
	const FLinearColor HPWHColorSoul(0.300f, 0.580f, 0.690f, 1.f);
	const FLinearColor HPWHColorBrass(0.480f, 0.320f, 0.130f, 1.f);
	const FLinearColor HPWHColorBrassSoft(0.760f, 0.630f, 0.390f, 1.f);
	const FLinearColor HPWHColorBgDeep(0.0012f, 0.0021f, 0.0043f, 1.f);
	const FLinearColor HPWHColorBgPanel(0.0033f, 0.0091f, 0.0195f, 1.f);
	const FLinearColor HPWHColorLine(0.350f, 0.510f, 0.700f, 1.f);
	const FLinearColor HPWHColorLightFill(0.760f, 0.870f, 0.960f, 1.f);
	const FLinearColor HPWHColorMirrorRed(0.420f, 0.045f, 0.050f, 1.f);

	inline FLinearColor HPWHAlpha(const FLinearColor& Color, float Alpha)
	{
		return FLinearColor(Color.R, Color.G, Color.B, Alpha);
	}

	FVector2D MeasureText(const FString& Text, const FSlateFontInfo& Font)
	{
		if (!FSlateApplication::IsInitialized())
		{
			return FVector2D(Text.Len() * Font.Size * 0.6f, Font.Size);
		}
		const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		return Measure->Measure(Text, Font);
	}
}

TSharedRef<SWidget> UHPWH_HUDWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		Root->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Root;
	}
	return Super::RebuildWidget();
}

void UHPWH_HUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	JoystickRingTexture = LoadObject<UTexture2D>(nullptr,
		TEXT("/Game/HPWH/UI/Sprites/T_HPWH_JoystickRing.T_HPWH_JoystickRing"));
	OrbTexture = LoadObject<UTexture2D>(nullptr,
		TEXT("/Game/HPWH/UI/Sprites/T_HPWH_Orb.T_HPWH_Orb"));
	MenuBackgroundTexture = LoadObject<UTexture2D>(nullptr,
		TEXT("/Game/HPWH/UI/Sprites/T_HPWH_MenuBg.T_HPWH_MenuBg"));
}

void UHPWH_HUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	InvalidateLayoutAndVolatility();
}

int32 UHPWH_HUDWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 BaseLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled) + 1;
	int32 Layer = BaseLayer;

	const FVector2D Size = AllottedGeometry.GetLocalSize();
	if (Size.X < 4.f || Size.Y < 4.f)
	{
		return Layer;
	}
	++PaintCount;
	if (PaintCount == 1 || PaintCount == 60)
	{
		UE_LOG(LogTemp, Display, TEXT("HPWH: HUD painted %d times at %.0fx%.0f"), PaintCount, Size.X, Size.Y);
	}

	const AHPWH_PlayerController* PC = Cast<AHPWH_PlayerController>(GetOwningPlayer());
	AHPWH_GameMode* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	const AHPWH_PlayerCharacter* Body = GM ? GM->GetPlayerBody() : nullptr;

	auto Rect = [&](const FVector2D& Position, const FVector2D& BoxSize, const FLinearColor& Color, float Radius = 0.f)
	{
		const FPaintGeometry Geometry = AllottedGeometry.ToPaintGeometry(
			FVector2f(static_cast<float>(BoxSize.X), static_cast<float>(BoxSize.Y)),
			FSlateLayoutTransform(FVector2f(static_cast<float>(Position.X), static_cast<float>(Position.Y))));
		if (Radius <= 0.5f)
		{
			FSlateDrawElement::MakeBox(OutDrawElements, Layer, Geometry,
				FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, Color);
		}
		else
		{
			FSlateRoundedBoxBrush Brush(Color, Radius);
			FSlateDrawElement::MakeBox(OutDrawElements, Layer, Geometry, &Brush, ESlateDrawEffect::None, Color);
		}
	};

	auto Text = [&](const FString& Content, const FVector2D& Position, float FontSize, const FLinearColor& Color,
		bool bCenter = false, bool bShadow = false)
	{
		if (Content.IsEmpty())
		{
			return;
		}
		const FSlateFontInfo Font = CCGameFont::Kai(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), FMath::RoundToInt(FontSize)));
		const FVector2D Extent = MeasureText(Content, Font);
		FVector2D DrawPosition = Position;
		if (bCenter)
		{
			DrawPosition.X -= Extent.X * 0.5f;
		}
		if (bShadow)
		{
			const FPaintGeometry ShadowGeometry = AllottedGeometry.ToPaintGeometry(
				FVector2f(static_cast<float>(Extent.X + 8.f), static_cast<float>(Extent.Y + 8.f)),
				FSlateLayoutTransform(FVector2f(static_cast<float>(DrawPosition.X + 2.f), static_cast<float>(DrawPosition.Y + 2.f))));
			FSlateDrawElement::MakeText(OutDrawElements, Layer, ShadowGeometry, Content, Font,
				ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, 0.8f));
		}
		const FPaintGeometry TextGeometry = AllottedGeometry.ToPaintGeometry(
			FVector2f(static_cast<float>(Extent.X + 8.f), static_cast<float>(Extent.Y + 8.f)),
			FSlateLayoutTransform(FVector2f(static_cast<float>(DrawPosition.X), static_cast<float>(DrawPosition.Y))));
		FSlateDrawElement::MakeText(OutDrawElements, Layer, TextGeometry, Content, Font, ESlateDrawEffect::None, Color);
	};

	auto Image = [&](UTexture2D* Texture, const FVector2D& Center, float Diameter, const FLinearColor& Tint) -> bool
	{
		if (!Texture || Diameter <= 1.f)
		{
			return false;
		}
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
		Brush.DrawAs = ESlateBrushDrawType::Image;
		const FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry(
			FVector2f(Diameter, Diameter),
			FSlateLayoutTransform(FVector2f(static_cast<float>(Center.X - Diameter * 0.5f),
				static_cast<float>(Center.Y - Diameter * 0.5f))));
		FSlateDrawElement::MakeBox(OutDrawElements, Layer, PaintGeometry, &Brush, ESlateDrawEffect::None, Tint);
		return true;
	};

	auto FullImage = [&](UTexture2D* Texture, const FLinearColor& Tint) -> bool
	{
		if (!Texture)
		{
			return false;
		}
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
		Brush.DrawAs = ESlateBrushDrawType::Image;
		const FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry(
			FVector2f(static_cast<float>(Size.X), static_cast<float>(Size.Y)),
			FSlateLayoutTransform(FVector2f(0.f, 0.f)));
		FSlateDrawElement::MakeBox(OutDrawElements, Layer, PaintGeometry, &Brush, ESlateDrawEffect::None, Tint);
		return true;
	};

	// 白色剪影图标：与网站牌库同源的 5 类符号，强化卡与 buff 面板共用。
	auto Icon = [&](EHPWH_UpgradeIcon Kind, const FVector2D& Center, float Diameter, const FLinearColor& Color)
	{
		const float D = FMath::Max(10.f, Diameter);
		const FPaintGeometry Geo = AllottedGeometry.ToPaintGeometry();
		const float Thickness = FMath::Max(1.2f, D * 0.055f);
		auto Map = [&](float U, float V)
		{
			return FVector2f(static_cast<float>(Center.X + (U - 0.5f) * D),
				static_cast<float>(Center.Y + (V - 0.5f) * D));
		};
		auto Stroke = [&](TArray<FVector2D> Points, bool bClosed)
		{
			TArray<FVector2f> Line;
			Line.Reserve(Points.Num() + 1);
			for (const FVector2D& Point : Points)
			{
				Line.Add(Map(static_cast<float>(Point.X), static_cast<float>(Point.Y)));
			}
			if (bClosed && Line.Num() > 1)
			{
				const FVector2f FirstPoint = Line[0];
				Line.Add(FirstPoint);
			}
			FSlateDrawElement::MakeLines(OutDrawElements, Layer, Geo, MoveTemp(Line),
				ESlateDrawEffect::None, Color, true, Thickness);
		};
		auto Circle = [&](float CX, float CY, float Radius, int32 Segments)
		{
			TArray<FVector2D> Points;
			Points.Reserve(Segments + 1);
			for (int32 Index = 0; Index <= Segments; ++Index)
			{
				const float Angle = 2.f * PI * static_cast<float>(Index) / static_cast<float>(Segments);
				Points.Add(FVector2D(CX + FMath::Cos(Angle) * Radius, CY + FMath::Sin(Angle) * Radius));
			}
			Stroke(MoveTemp(Points), false);
		};
		auto Spark = [&](float CX, float CY, float Radius)
		{
			Stroke({ FVector2D(CX, CY - Radius), FVector2D(CX + Radius * 0.32f, CY - Radius * 0.32f),
				FVector2D(CX + Radius, CY), FVector2D(CX + Radius * 0.32f, CY + Radius * 0.32f),
				FVector2D(CX, CY + Radius), FVector2D(CX - Radius * 0.32f, CY + Radius * 0.32f),
				FVector2D(CX - Radius, CY), FVector2D(CX - Radius * 0.32f, CY - Radius * 0.32f) }, true);
		};

		switch (Kind)
		{
		case EHPWH_UpgradeIcon::Attack:
			Circle(0.63f, 0.35f, 0.16f, 18);
			Stroke({ FVector2D(0.46f, 0.49f), FVector2D(0.18f, 0.77f) }, false);
			Stroke({ FVector2D(0.35f, 0.40f), FVector2D(0.13f, 0.62f) }, false);
			Stroke({ FVector2D(0.52f, 0.63f), FVector2D(0.30f, 0.85f) }, false);
			Spark(0.86f, 0.14f, 0.07f);
			Spark(0.82f, 0.72f, 0.05f);
			break;
		case EHPWH_UpgradeIcon::Life:
			Stroke({ FVector2D(0.5f, 0.30f), FVector2D(0.67f, 0.14f), FVector2D(0.85f, 0.24f),
				FVector2D(0.89f, 0.44f), FVector2D(0.78f, 0.62f), FVector2D(0.5f, 0.87f),
				FVector2D(0.22f, 0.62f), FVector2D(0.11f, 0.44f), FVector2D(0.15f, 0.24f),
				FVector2D(0.33f, 0.14f) }, true);
			Stroke({ FVector2D(0.24f, 0.47f), FVector2D(0.38f, 0.47f), FVector2D(0.45f, 0.34f),
				FVector2D(0.54f, 0.57f), FVector2D(0.61f, 0.45f), FVector2D(0.76f, 0.45f) }, false);
			Stroke({ FVector2D(0.87f, 0.08f), FVector2D(0.87f, 0.18f) }, false);
			Stroke({ FVector2D(0.82f, 0.13f), FVector2D(0.92f, 0.13f) }, false);
			break;
		case EHPWH_UpgradeIcon::Range:
			Circle(0.5f, 0.5f, 0.38f, 26);
			Circle(0.5f, 0.5f, 0.21f, 20);
			Circle(0.5f, 0.5f, 0.055f, 8);
			Stroke({ FVector2D(0.5f, 0.02f), FVector2D(0.5f, 0.10f) }, false);
			Stroke({ FVector2D(0.5f, 0.90f), FVector2D(0.5f, 0.98f) }, false);
			Stroke({ FVector2D(0.02f, 0.5f), FVector2D(0.10f, 0.5f) }, false);
			Stroke({ FVector2D(0.90f, 0.5f), FVector2D(0.98f, 0.5f) }, false);
			Stroke({ FVector2D(0.14f, 0.14f), FVector2D(0.20f, 0.20f) }, false);
			Stroke({ FVector2D(0.80f, 0.80f), FVector2D(0.86f, 0.86f) }, false);
			Stroke({ FVector2D(0.86f, 0.14f), FVector2D(0.80f, 0.20f) }, false);
			Stroke({ FVector2D(0.20f, 0.80f), FVector2D(0.14f, 0.86f) }, false);
			break;
		case EHPWH_UpgradeIcon::Soul:
			Stroke({ FVector2D(0.5f, 0.05f), FVector2D(0.63f, 0.22f), FVector2D(0.74f, 0.36f),
				FVector2D(0.79f, 0.52f), FVector2D(0.76f, 0.69f), FVector2D(0.64f, 0.83f),
				FVector2D(0.5f, 0.89f), FVector2D(0.36f, 0.83f), FVector2D(0.24f, 0.69f),
				FVector2D(0.21f, 0.52f), FVector2D(0.26f, 0.36f), FVector2D(0.37f, 0.22f) }, true);
			Stroke({ FVector2D(0.5f, 0.40f), FVector2D(0.60f, 0.52f), FVector2D(0.62f, 0.65f),
				FVector2D(0.56f, 0.75f), FVector2D(0.5f, 0.77f), FVector2D(0.44f, 0.75f),
				FVector2D(0.38f, 0.65f), FVector2D(0.40f, 0.52f) }, true);
			Stroke({ FVector2D(0.86f, 0.34f), FVector2D(0.91f, 0.48f), FVector2D(0.87f, 0.60f) }, false);
			Circle(0.88f, 0.16f, 0.035f, 8);
			break;
		case EHPWH_UpgradeIcon::Fate:
			Stroke({ FVector2D(0.06f, 0.5f), FVector2D(0.18f, 0.36f), FVector2D(0.34f, 0.26f),
				FVector2D(0.5f, 0.23f), FVector2D(0.66f, 0.26f), FVector2D(0.82f, 0.36f),
				FVector2D(0.94f, 0.5f), FVector2D(0.82f, 0.64f), FVector2D(0.66f, 0.74f),
				FVector2D(0.5f, 0.77f), FVector2D(0.34f, 0.74f), FVector2D(0.18f, 0.64f) }, true);
			Circle(0.5f, 0.5f, 0.16f, 18);
			Circle(0.5f, 0.5f, 0.065f, 8);
			Stroke({ FVector2D(0.5f, 0.05f), FVector2D(0.5f, 0.16f) }, false);
			Stroke({ FVector2D(0.27f, 0.10f), FVector2D(0.33f, 0.20f) }, false);
			Stroke({ FVector2D(0.73f, 0.10f), FVector2D(0.67f, 0.20f) }, false);
			break;
		}
	};

	// 主界面
	if (GM && GM->IsMainMenuActive())
	{
		const bool bHasMenuBackground = FullImage(MenuBackgroundTexture, FLinearColor(0.92f, 0.96f, 1.f, 1.f));
		Rect(FVector2D(0.f, 0.f), Size, HPWHAlpha(HPWHColorBgDeep, bHasMenuBackground ? 0.24f : 0.985f));
		Rect(FVector2D(0.f, Size.Y * 0.105f), FVector2D(Size.X, 1.5f),
			HPWHAlpha(HPWHColorBrass, 0.22f));
		Rect(FVector2D(0.f, Size.Y * 0.895f), FVector2D(Size.X, 1.5f),
			HPWHAlpha(HPWHColorBrass, 0.14f));

		if (GM->GetMenuPage() == EHPWH_MenuPage::Main)
		{
			Text(TEXT("幻魄物语"), FVector2D(Size.X * 0.5f, Size.Y * 0.15f), 74.f,
				HPWHColorInk, true, true);
			Text(TEXT("在梦里找回自己的影子"), FVector2D(Size.X * 0.5f, Size.Y * 0.26f), 22.f,
				HPWHColorMuted, true, true);
		}
		else
		{
			Text(TEXT("幻魄物语"), FVector2D(Size.X * 0.5f, Size.Y * 0.09f), 24.f,
				HPWHColorMuted, true, true);
		}

		const FHPWH_MenuLayout Menu = HPWH_MakeMenuLayout(Size);
		auto MenuButton = [&](const FString& Label, const FVector2D& Center, const FVector2D& ButtonSize, bool bSelected)
		{
			// 进入界面只保留文字，不绘制底板与边框。
			const float LabelSize = FMath::Clamp(ButtonSize.Y * 0.42f, 24.f, 30.f);
			const FString FullLabel = bSelected ? Label + TEXT("   ·   当前") : Label;
			Text(FullLabel, FVector2D(Center.X, Center.Y - LabelSize * 0.62f), LabelSize,
				bSelected ? HPWHColorBrassSoft : HPWHColorInk, true, true);
		};

		if (GM->GetMenuPage() == EHPWH_MenuPage::Main)
		{
			if (GM->HasSaveGame())
			{
				MenuButton(TEXT("继续游戏"), Menu.MainButtons[0], Menu.ButtonSize, false);
				MenuButton(TEXT("开始游戏"), Menu.MainButtons[1], Menu.ButtonSize, false);
				MenuButton(TEXT("关于我们"), Menu.MainButtons[2], Menu.ButtonSize, false);
				MenuButton(TEXT("设置"), Menu.MainButtons[3], Menu.ButtonSize, false);
			}
			else
			{
				MenuButton(TEXT("开始游戏"), Menu.MainButtons[0], Menu.ButtonSize, false);
				MenuButton(TEXT("关于我们"), Menu.MainButtons[1], Menu.ButtonSize, false);
				MenuButton(TEXT("设置"), Menu.MainButtons[2], Menu.ButtonSize, false);
			}
			Text(TEXT("点击选择    Esc 返回"), FVector2D(Size.X * 0.5f, Size.Y * 0.87f), 18.f,
				HPWHColorMuted, true);
		}
		else if (GM->GetMenuPage() == EHPWH_MenuPage::About)
		{
			Text(TEXT("关于我们"), FVector2D(Size.X * 0.5f, Size.Y * 0.20f), 44.f,
				HPWHColorInk, true, true);
			const TArray<FString> Lines = {
				TEXT("《幻魄物语》是一款发生在普通人梦中的"),
				TEXT("俯视角探索解谜与轻 Roguelite 原型。"),
				TEXT("在案头旧物之间寻找异层传送门，"),
				TEXT("同时躲避被惊醒的匍匐者。"),
				TEXT(""),
				TEXT("当前版本：2026-11-17 玩法原型")
			};
			for (int32 Index = 0; Index < Lines.Num(); ++Index)
			{
				Text(Lines[Index], FVector2D(Size.X * 0.5f, Size.Y * 0.32f + Index * 34.f), 21.f,
					HPWHColorInkDim, true);
			}
			MenuButton(TEXT("返回"), Menu.BackCenter, Menu.ButtonSize, false);
		}
		else
		{
			Text(TEXT("设置"), FVector2D(Size.X * 0.5f, Size.Y * 0.20f), 44.f,
				HPWHColorInk, true, true);
			Text(TEXT("画面抗锯齿"), FVector2D(Size.X * 0.5f, Size.Y * 0.34f), 22.f,
				HPWHColorInkDim, true);
			const bool bDlss = GM->GetGraphicsMode() == EHPWH_GraphicsMode::DLSS;
			MenuButton(TEXT("DLSS"), Menu.SettingsRows[0], Menu.SettingsRowSize, bDlss);
			MenuButton(TEXT("TSR"), Menu.SettingsRows[1], Menu.SettingsRowSize, !bDlss);
			Text(TEXT("DLSS 需要支持 NVIDIA DLSS 的显卡；TSR 为通用时域超分。"),
				FVector2D(Size.X * 0.5f, Size.Y * 0.65f), 18.f,
				HPWHColorMuted, true);
			MenuButton(TEXT("返回"), Menu.BackCenter, Menu.ButtonSize, false);
		}

		return Layer + 1;
	}

	if (PC && Body)
	{
		const FHPWH_TouchLayout Touch = HPWH_MakeTouchLayout(Size);

		if (Body->IsFrightened())
		{
			const FLinearColor Vignette(0.42f, 0.02f, 0.02f, 0.35f);
			const float Band = Size.Y * 0.10f;
			Rect(FVector2D(0.f, 0.f), FVector2D(Size.X, Band), Vignette);
			Rect(FVector2D(0.f, Size.Y - Band), FVector2D(Size.X, Band), Vignette);
			Rect(FVector2D(0.f, 0.f), FVector2D(Size.X * 0.08f, Size.Y), Vignette);
			Rect(FVector2D(Size.X * 0.92f, 0.f), FVector2D(Size.X * 0.08f, Size.Y), Vignette);
		}

		// 左上：坐标、等级、受击与经验数值。
		FIntPoint PlayerCell(0, 0);
		if (GM)
		{
			if (AHPWH_WorldDirector* Director = GM->GetDirector())
			{
				PlayerCell = Director->WorldToCell(Body->GetActorLocation());
			}
		}
		FString ProgressText = FString::Printf(TEXT("坐标 (%d, %d)    等级 %d    解密 %d/3    受击 %d"),
			PlayerCell.X, PlayerCell.Y, GM ? GM->GetPlayerLevel() : 1,
			GM ? GM->GetSolvedPuzzleTypeCount() : 0, Body->GetHitCount());
		Text(ProgressText, FVector2D(24.f, 18.f), 21.f, HPWHColorInk, false, true);
		FString XpText = FString::Printf(TEXT("经验 %d / %d"),
			GM ? GM->GetExperience() : 0, GM ? GM->GetExperienceToNextLevel() : 10);
		if (GM && GM->GetPendingUpgradeChoices() > 0)
		{
			XpText += FString::Printf(TEXT("    待选 %d"), GM->GetPendingUpgradeChoices());
		}
		Text(XpText, FVector2D(24.f, 46.f), 14.f, HPWHColorMuted, false, true);

		// 右上百分比
		const FString PercentText = FString::Printf(TEXT("%d%%"), GM ? GM->GetDisplayPercent() : 5);
		const FSlateFontInfo PercentFont = CCGameFont::Kai(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 46));
		const FVector2D PercentExtent = MeasureText(PercentText, PercentFont);
		Text(PercentText, FVector2D(Size.X - 34.f - PercentExtent.X, 16.f), 46.f, HPWHColorBrassSoft, false, true);

		// 中央消息
		if (GM)
		{
			const FString Message = GM->GetActiveMessage();
			if (!Message.IsEmpty())
			{
				const bool bPriority = GM->IsPriorityMessageActive();
				const float MessageSize = bPriority ? 46.f : 30.f;
				const float MessageY = GM->IsInGameMenuOpen() ? Size.Y * 0.74f : Size.Y * 0.26f;
				Text(Message, FVector2D(Size.X * 0.5f, MessageY), MessageSize,
					bPriority ? HPWHColorInk : HPWHColorInkDim, true, true);
			}
		}

		// 灵魂离体提示：给出肉身距离与屏幕方向，靠近后提示交互。
		if (GM && GM->GetSoulState() == EHPWH_SoulState::SoulOut && !GM->IsInGameMenuOpen())
		{
			FString SoulText = TEXT("灵魂离体 · 找到肉身才能回体");
			const AHPWH_SoulAvatar* Soul = GM->GetSoulAvatar();
			FVector2D BodyScreen = FVector2D::ZeroVector;
			bool bBodyProjected = false;
			if (PC && Body)
			{
				bBodyProjected = PC->ProjectWorldLocationToScreen(
					Body->GetActorLocation() + FVector(0.f, 0.f, 140.f), BodyScreen);
			}
			if (Soul && Body)
			{
				const FVector Delta = Body->GetActorLocation() - Soul->GetActorLocation();
				const float DistanceMeters = Delta.Size2D() / 100.f;
				if (DistanceMeters <= 3.f)
				{
					SoulText = TEXT("肉身就在附近 · 交互回体");
				}
				else
				{
					// 有屏幕投影时按屏幕方向判断，保证提示与玩家看到的画面一致。
					float DirectionAngle = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
					if (bBodyProjected)
					{
						const float ScreenDX = static_cast<float>(BodyScreen.X - Size.X * 0.5f);
						const float ScreenDY = static_cast<float>(BodyScreen.Y - Size.Y * 0.5f);
						DirectionAngle = FMath::RadiansToDegrees(FMath::Atan2(ScreenDX, -ScreenDY));
					}
					DirectionAngle = FMath::Fmod(DirectionAngle + 360.f, 360.f);
					const TCHAR* Direction = TEXT("上方");
					if (DirectionAngle < 22.5f || DirectionAngle >= 337.5f)
					{
						Direction = TEXT("上方");
					}
					else if (DirectionAngle < 67.5f)
					{
						Direction = TEXT("右上方");
					}
					else if (DirectionAngle < 112.5f)
					{
						Direction = TEXT("右方");
					}
					else if (DirectionAngle < 157.5f)
					{
						Direction = TEXT("右下方");
					}
					else if (DirectionAngle < 202.5f)
					{
						Direction = TEXT("下方");
					}
					else if (DirectionAngle < 247.5f)
					{
						Direction = TEXT("左下方");
					}
					else if (DirectionAngle < 292.5f)
					{
						Direction = TEXT("左方");
					}
					else
					{
						Direction = TEXT("左上方");
					}
					SoulText = FString::Printf(TEXT("灵魂离体 · 肉身 %.1f m · %s"), DistanceMeters, Direction);
				}
			}
			Text(SoulText, FVector2D(Size.X * 0.5f, 74.f), 22.f, HPWHColorSoul, true, true);

			// 灵魂状态边缘光：与惊吓的红色暗角区分。
			const FLinearColor SoulVignette(0.18f, 0.52f, 0.62f, 0.10f);
			const float EdgeBand = Size.Y * 0.055f;
			Rect(FVector2D(0.f, 0.f), FVector2D(Size.X, EdgeBand), SoulVignette);
			Rect(FVector2D(0.f, Size.Y - EdgeBand), FVector2D(Size.X, EdgeBand), SoulVignette);
			Rect(FVector2D(0.f, 0.f), FVector2D(Size.X * 0.045f, Size.Y), SoulVignette);
			Rect(FVector2D(Size.X * 0.955f, 0.f), FVector2D(Size.X * 0.045f, Size.Y), SoulVignette);

			// 肉身在视野内时画出屏幕标记；离开视野只保留顶部距离与方向提示。
			if (bBodyProjected && Soul && Body &&
				BodyScreen.X > 100.f && BodyScreen.X < Size.X - 100.f &&
				BodyScreen.Y > 130.f && BodyScreen.Y < Size.Y - 130.f)
			{
				const FLinearColor MarkerColor(0.62f, 0.88f, 1.f, 0.95f);
				Rect(BodyScreen - FVector2D(38.f, 7.f), FVector2D(76.f, 20.f),
					FLinearColor(0.f, 0.f, 0.f, 0.38f), 4.f);
				Rect(BodyScreen - FVector2D(12.f, 1.f), FVector2D(24.f, 2.f), MarkerColor);
				Rect(BodyScreen - FVector2D(1.f, 11.f), FVector2D(2.f, 22.f), MarkerColor);
				Rect(BodyScreen - FVector2D(1.f, 1.f), FVector2D(2.f, 2.f), MarkerColor);
				const float BodyDistance = FVector::Dist2D(
					Body->GetActorLocation(), Soul->GetActorLocation()) / 100.f;
				Text(FString::Printf(TEXT("肉身 %.1f m"), BodyDistance),
					BodyScreen + FVector2D(0.f, 7.f), 13.f, MarkerColor, true, true);
			}
		}

		// 解密倒计时：只在谜题进行中显示，剩余 3 秒转黄铜色提醒。
		AHPWH_POIBase* ActivePuzzle = nullptr;
		for (TActorIterator<AHPWH_POIBase> It(GetWorld()); It; ++It)
		{
			if (It->IsPuzzleRunning())
			{
				ActivePuzzle = *It;
				break;
			}
		}
		if (ActivePuzzle)
		{
			const float Limit = ActivePuzzle->GetPuzzleTimeLimit();
			const float Remaining = ActivePuzzle->GetPuzzleRemaining();
			const float Ratio = Limit > 0.f ? FMath::Clamp(Remaining / Limit, 0.f, 1.f) : 0.f;
			const FLinearColor TimerColor = Remaining <= 3.f ? HPWHColorBrassSoft : HPWHColorSoul;
			Text(FString::Printf(TEXT("解密 %.1f s"), Remaining),
				FVector2D(Size.X * 0.5f, 88.f), 16.f, TimerColor, true, true);
			const FVector2D BarSize(320.f, 4.f);
			const FVector2D BarPos(Size.X * 0.5f - BarSize.X * 0.5f, 112.f);
			Rect(BarPos, BarSize, HPWHAlpha(HPWHColorLine, 0.22f), 2.f);
			Rect(BarPos, FVector2D(BarSize.X * Ratio, BarSize.Y), HPWHAlpha(TimerColor, 0.9f), 2.f);
		}

		// 交互提示：纯文字，不带按键前缀。
		const FString Prompt = PC->GetInteractPrompt();
		if (!Prompt.IsEmpty())
		{
			Text(Prompt, FVector2D(Size.X * 0.5f, Size.Y - 72.f), 21.f, HPWHColorInk, true);
		}

		// 手机触摸控件：本地抠图环 + 实体球；按钮不显示文字。
		const FVector2D KnobOffset = PC->GetJoystickVisual() * (Touch.JoystickRadius * 0.55f);
		const bool bDrewRing = Image(JoystickRingTexture, Touch.JoystickCenter,
			Touch.JoystickRadius * 2.22f, FLinearColor::White);
		const bool bDrewKnob = Image(OrbTexture, Touch.JoystickCenter + KnobOffset,
			Touch.JoystickRadius * 0.72f, FLinearColor::White);
		if (!bDrewRing)
		{
			Rect(Touch.JoystickCenter - FVector2D(Touch.JoystickRadius, Touch.JoystickRadius),
				FVector2D(Touch.JoystickRadius * 2.f, Touch.JoystickRadius * 2.f),
				HPWHAlpha(HPWHColorSoul, 0.12f), Touch.JoystickRadius);
		}
		if (!bDrewKnob)
		{
			Rect(Touch.JoystickCenter - FVector2D(28.f, 28.f) + KnobOffset, FVector2D(56.f, 56.f),
				HPWHAlpha(HPWHColorInk, 0.28f), 28.f);
		}

		const FVector2D AttackCenter = Touch.AttackCenter + PC->GetAttackVisualOffset();
		const bool bDrewAttack = Image(OrbTexture, AttackCenter,
			Touch.AttackRadius * 2.0f, FLinearColor::White);
		if (!bDrewAttack)
		{
			Rect(AttackCenter - FVector2D(Touch.AttackRadius, Touch.AttackRadius),
				FVector2D(Touch.AttackRadius * 2.f, Touch.AttackRadius * 2.f),
				HPWHAlpha(HPWHColorMirrorRed, 0.30f), Touch.AttackRadius);
		}

	}

	// 底部通栏经验条：3px，随屏幕宽度延伸。
	if (GM && GM->GetExperienceToNextLevel() > 0)
	{
		const float BarHeight = 3.f;
		const float XpRatio = FMath::Clamp(
			static_cast<float>(GM->GetExperience()) / static_cast<float>(GM->GetExperienceToNextLevel()), 0.f, 1.f);
		const FVector2D BarPos(0.f, Size.Y - BarHeight);
		Rect(BarPos, FVector2D(Size.X, BarHeight), HPWHAlpha(HPWHColorBgDeep, 0.95f));
		Rect(BarPos, FVector2D(Size.X * XpRatio, BarHeight), FLinearColor(0.42f, 0.72f, 0.86f, 1.f));
		Rect(BarPos, FVector2D(Size.X, 1.f), HPWHAlpha(HPWHColorSoul, 0.42f));
	}

	// 游戏内顶部菜单
	if (GM && !GM->IsUpgradeSelectionActive())
	{
		const FHPWH_InGameMenuLayout InGame = HPWH_MakeInGameMenuLayout(Size);
		const FVector2D MenuPos = InGame.ButtonCenter - InGame.ButtonSize * 0.5f;
		Rect(MenuPos, InGame.ButtonSize, HPWHAlpha(HPWHColorBgPanel, 0.82f), 8.f);
		const FLinearColor MenuBorder = GM->IsInGameMenuOpen()
			? HPWHAlpha(HPWHColorSoul, 0.9f)
			: HPWHAlpha(HPWHColorLine, 0.55f);
		Rect(MenuPos, FVector2D(InGame.ButtonSize.X, 1.5f), MenuBorder, 1.f);
		Rect(MenuPos + FVector2D(0.f, InGame.ButtonSize.Y - 1.5f), FVector2D(InGame.ButtonSize.X, 1.5f), MenuBorder, 1.f);
		Rect(MenuPos, FVector2D(1.5f, InGame.ButtonSize.Y), MenuBorder, 1.f);
		Rect(MenuPos + FVector2D(InGame.ButtonSize.X - 1.5f, 0.f), FVector2D(1.5f, InGame.ButtonSize.Y), MenuBorder, 1.f);
		Text(TEXT("菜单"), FVector2D(InGame.ButtonCenter.X, InGame.ButtonCenter.Y - 11.f), 15.f,
			HPWHColorInk, true, true);

		if (GM->IsInGameMenuOpen())
		{
			Rect(FVector2D(0.f, 0.f), Size, HPWHAlpha(HPWHColorBgDeep, 0.76f));
			const bool bSettings = GM->IsInGameSettingsOpen();
			const FVector2D PanelPos = InGame.PanelPos;
			const FVector2D PanelSize = InGame.PanelSize;
			Rect(PanelPos + FVector2D(0.f, 6.f), PanelSize, FLinearColor(0.f, 0.f, 0.f, 0.45f), 16.f);
			Rect(PanelPos, PanelSize, HPWHAlpha(HPWHColorBgDeep, 0.99f), 16.f);
			Rect(PanelPos, FVector2D(PanelSize.X, 1.5f), HPWHAlpha(HPWHColorBrass, 0.45f), 1.f);
			Rect(PanelPos + FVector2D(0.f, PanelSize.Y - 1.5f), FVector2D(PanelSize.X, 1.5f),
				HPWHAlpha(HPWHColorLine, 0.22f), 1.f);
			Text(bSettings ? TEXT("设置") : TEXT("梦境档案"),
				FVector2D(Size.X * 0.5f, PanelPos.Y + 12.f), 24.f, HPWHColorInk, true, true);

			auto RowButton = [&](const FString& Label, const FVector2D& Center, const FVector2D& RowSize, bool bSelected)
			{
				const FVector2D Pos = Center - RowSize * 0.5f;
				Rect(Pos, RowSize, bSelected
					? HPWHAlpha(HPWHColorLightFill, 0.96f)
					: HPWHAlpha(HPWHColorBgPanel, 0.96f), 9.f);
				const FLinearColor Border = bSelected
					? HPWHAlpha(HPWHColorSoul, 0.9f)
					: HPWHAlpha(HPWHColorLine, 0.4f);
				Rect(Pos, FVector2D(RowSize.X, 1.2f), Border, 1.f);
				Rect(Pos + FVector2D(0.f, RowSize.Y - 1.2f), FVector2D(RowSize.X, 1.2f), Border, 1.f);
				Text(Label, FVector2D(Center.X, Center.Y - 11.f), 18.f,
					bSelected ? HPWHColorBgDeep : HPWHColorInk, true, true);
				if (bSelected)
				{
					Text(TEXT("当前"), FVector2D(Center.X + RowSize.X * 0.5f - 42.f, Center.Y - 8.f), 12.f,
						HPWHColorBgDeep, false, true);
				}
			};

			if (bSettings)
			{
				const bool bDlss = GM->GetGraphicsMode() == EHPWH_GraphicsMode::DLSS;
				Text(TEXT("画面抗锯齿"), FVector2D(Size.X * 0.5f, PanelPos.Y + 116.f), 16.f,
					HPWHColorInkDim, true);
				RowButton(TEXT("DLSS"), InGame.SettingsRows[0], InGame.SettingsRowSize, bDlss);
				RowButton(TEXT("TSR"), InGame.SettingsRows[1], InGame.SettingsRowSize, !bDlss);
				Text(TEXT("DLSS 需要支持 NVIDIA DLSS 的显卡；TSR 为通用时域超分。"),
					FVector2D(Size.X * 0.5f, PanelPos.Y + 302.f), 12.f, HPWHColorMuted, true);
				RowButton(TEXT("返回"), InGame.BackCenter, InGame.RowSize, false);
			}
			else
			{
				const float PadX = 26.f;
				const float Top = PanelPos.Y + 52.f;
				const float BottomLimit = InGame.Rows[0].Y - InGame.RowSize.Y * 0.5f - 14.f;
				const float LeftW = FMath::Clamp(PanelSize.X * 0.30f, 218.f, 300.f);

				// 左：角色血量与当前状态
				const FVector2D LeftPos(PanelPos.X + PadX, Top);
				const FVector2D LeftSize(LeftW, BottomLimit - Top);
				Rect(LeftPos, LeftSize, HPWHAlpha(HPWHColorBgPanel, 0.72f), 10.f);
				Rect(LeftPos, FVector2D(LeftSize.X, 1.f), HPWHAlpha(HPWHColorLine, 0.28f), 1.f);

				const float L = LeftPos.X + 16.f;
				float Y = LeftPos.Y + 12.f;
				Text(TEXT("初墨"), FVector2D(L, Y), 21.f, HPWHColorInk, false, true);
				const bool bSoulOut = GM->GetSoulState() == EHPWH_SoulState::SoulOut;
				const FString StateText = bSoulOut ? TEXT("灵魂离体") : TEXT("肉身");
				const FSlateFontInfo StateFont = CCGameFont::Kai(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 12));
				Text(StateText, FVector2D(LeftPos.X + LeftSize.X - 16.f - MeasureText(StateText, StateFont).X, Y + 7.f),
					12.f, bSoulOut ? HPWHColorSoul : HPWHColorMuted, false, true);
				Y += 34.f;

				const float Health = Body ? Body->GetHealth() : 0.f;
				const float MaxHealth = Body ? FMath::Max(1.f, Body->GetMaxHealth()) : 1.f;
				Text(FString::Printf(TEXT("生命  %d / %d"), FMath::RoundToInt(Health), FMath::RoundToInt(MaxHealth)),
					FVector2D(L, Y), 13.f, HPWHColorInkDim, false, true);
				Y += 22.f;
				const FVector2D HpBarSize(LeftSize.X - 32.f, 7.f);
				Rect(FVector2D(L, Y), HpBarSize, HPWHAlpha(HPWHColorLine, 0.18f), 3.f);
				Rect(FVector2D(L, Y), FVector2D(HpBarSize.X * FMath::Clamp(Health / MaxHealth, 0.f, 1.f), HpBarSize.Y),
					FLinearColor(0.62f, 0.10f, 0.12f, 0.95f), 3.f);
				Y += 22.f;

				const FHPWH_RunStats& Stats = GM->GetRunStats();
				const int32 StatCount = 10;
				const float LineH = FMath::Clamp((LeftPos.Y + LeftSize.Y - 8.f - Y) / StatCount, 14.f, 21.f);
				const float StatFontSize = LineH < 17.f ? 11.f : 12.f;
				auto StatLine = [&](const FString& Label, const FString& Value)
				{
					Text(Label, FVector2D(L, Y), StatFontSize, HPWHColorMuted);
					const FSlateFontInfo StatFont = CCGameFont::Kai(
						FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), FMath::RoundToInt(StatFontSize)));
					const float ValueWidth = MeasureText(Value, StatFont).X;
					Text(Value, FVector2D(LeftPos.X + LeftSize.X - 16.f - ValueWidth, Y), StatFontSize,
						HPWHColorInkDim, false, true);
					Y += LineH;
				};
				StatLine(TEXT("通关率"), FString::Printf(TEXT("%d%%"), GM->GetDisplayPercent()));
				StatLine(TEXT("层数"), FString::Printf(TEXT("%d"), GM->GetLayerIndex()));
				StatLine(TEXT("等级"), FString::Printf(TEXT("%d"), GM->GetPlayerLevel()));
				StatLine(TEXT("已解谜"), FString::Printf(TEXT("%d / 3"), GM->GetSolvedPuzzleTypeCount()));
				StatLine(TEXT("受击"), FString::Printf(TEXT("%d"), Body ? Body->GetHitCount() : 0));
				StatLine(TEXT("攻击加成"), FString::Printf(TEXT("+%d"), FMath::RoundToInt(Stats.AttackBonus)));
				StatLine(TEXT("移速加成"), FString::Printf(TEXT("+%d%%"), FMath::RoundToInt(Stats.MoveSpeedBonus * 100.f)));
				StatLine(TEXT("子弹冷却"), FString::Printf(TEXT("%.2f s"), FMath::Max(0.2f, 2.f - Stats.BulletCooldownReduction)));
				StatLine(TEXT("弹数"), FString::Printf(TEXT("×%d"), Stats.ProjectileCount));
				StatLine(TEXT("交互范围"), FString::Printf(TEXT("+%d%%"), FMath::RoundToInt(Stats.InteractRangeBonus * 100.f)));

				// 右：buff 包，按类型合并并显示最高稀有度描边
				const FVector2D GridPos(PanelPos.X + PadX + LeftW + 22.f, Top);
				const FVector2D GridSize(PanelPos.X + PanelSize.X - PadX - GridPos.X, BottomLimit - Top);
				Rect(GridPos, GridSize, HPWHAlpha(HPWHColorBgPanel, 0.55f), 10.f);
				Rect(GridPos, FVector2D(GridSize.X, 1.f), HPWHAlpha(HPWHColorLine, 0.28f), 1.f);

				const TArray<FHPWH_OwnedUpgrade>& Owned = GM->GetOwnedUpgrades();
				Text(FString::Printf(TEXT("已持有强化   %d"), Owned.Num()),
					FVector2D(GridPos.X + 16.f, GridPos.Y + 12.f), 14.f, HPWHColorInkDim, false, true);
				const FString Legend = TEXT("描边颜色对应稀有度");
				const FSlateFontInfo LegendFont = CCGameFont::Kai(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 10));
				Text(Legend, FVector2D(GridPos.X + GridSize.X - 14.f - MeasureText(Legend, LegendFont).X, GridPos.Y + 15.f),
					10.f, HPWHColorMuted, false);

				if (Owned.Num() == 0)
				{
					Text(TEXT("还没有获得强化"), FVector2D(GridPos.X + GridSize.X * 0.5f, GridPos.Y + GridSize.Y * 0.42f),
						16.f, HPWHColorMuted, true);
					Text(TEXT("升级或完成解密后，三选一的卡牌会收进这里。"),
						FVector2D(GridPos.X + GridSize.X * 0.5f, GridPos.Y + GridSize.Y * 0.42f + 28.f),
						12.f, HPWHColorMuted, true);
				}
				else
				{
					const float Gap = 9.f;
					const float AreaW = GridSize.X - 28.f;
					const float AreaH = GridSize.Y - 58.f;
					const int32 Cols = FMath::Clamp(FMath::FloorToInt((AreaW + Gap) / 78.f), 3, 7);
					float Tile = FMath::Min(70.f, (AreaW - (Cols - 1) * Gap) / Cols);
					const int32 Rows = FMath::CeilToInt(static_cast<float>(Owned.Num()) / static_cast<float>(Cols));
					float LabelSpace = 12.f;
					if (Rows * (Tile + LabelSpace) > AreaH)
					{
						Tile = FMath::Max(26.f, AreaH / Rows - LabelSpace);
						if (Tile < 40.f)
						{
							LabelSpace = 6.f;
							Tile = FMath::Max(24.f, AreaH / Rows - LabelSpace);
						}
					}
					const float RowH = Tile + LabelSpace;
					const float StartX = GridPos.X + 14.f;
					const float StartY = GridPos.Y + 46.f;

					for (int32 Index = 0; Index < Owned.Num(); ++Index)
					{
						const FHPWH_OwnedUpgrade& Entry = Owned[Index];
						const FLinearColor RarityColor = HPWH_RarityColor(Entry.Rarity);
						const int32 Col = Index % Cols;
						const int32 Row = Index / Cols;
						const FVector2D TilePos(StartX + Col * (Tile + Gap), StartY + Row * RowH);
						Rect(TilePos, FVector2D(Tile, Tile), HPWHAlpha(HPWHColorBgDeep, 0.9f), 7.f);
						const float BW = FMath::Max(1.2f, Tile * 0.035f);
						Rect(TilePos, FVector2D(Tile, BW), RarityColor, 1.f);
						Rect(TilePos + FVector2D(0.f, Tile - BW), FVector2D(Tile, BW), RarityColor, 1.f);
						Rect(TilePos, FVector2D(BW, Tile), RarityColor, 1.f);
						Rect(TilePos + FVector2D(Tile - BW, 0.f), FVector2D(BW, Tile), RarityColor, 1.f);
						Icon(HPWH_UpgradeIconFor(Entry.Type), TilePos + FVector2D(Tile * 0.5f, Tile * 0.40f),
							Tile * 0.46f, FLinearColor(1.f, 1.f, 1.f, 0.9f));
						if (Tile >= 40.f)
						{
							FString Name = HPWH_UpgradeTitle(Entry.Type);
							if (Entry.Count > 1)
							{
								Name += FString::Printf(TEXT(" ×%d"), Entry.Count);
							}
							Text(Name, FVector2D(TilePos.X + Tile * 0.5f, TilePos.Y + Tile - 16.f), 9.f,
								HPWHColorInkDim, true);
						}
					}
				}

				RowButton(TEXT("返回主页"), InGame.Rows[0], InGame.RowSize, false);
				RowButton(TEXT("设置"), InGame.Rows[1], InGame.RowSize, false);
				RowButton(TEXT("存档"), InGame.Rows[2], InGame.RowSize, false);
				Text(TEXT("Esc 关闭菜单    1 / 2 / 3 选择"),
					FVector2D(Size.X * 0.5f, PanelPos.Y + PanelSize.Y + 6.f), 12.f, HPWHColorMuted, true);
			}
		}
	}

	// 异层传送门单选红卡
	if (GM && GM->IsPortalChoiceActive())
	{
		++Layer;
		Rect(FVector2D(0.f, 0.f), Size, HPWHAlpha(HPWHColorBgDeep, 0.82f));

		const FHPWH_PortalCardLayout Portal = HPWH_MakePortalCardLayout(Size);
		const FVector2D CardPos = Portal.Center - Portal.CardSize * 0.5f;
		const FLinearColor Red = HPWHColorMirrorRed;
		const FLinearColor RedSoft(0.92f, 0.62f, 0.58f, 1.f);

		Rect(CardPos - FVector2D(10.f, 10.f), Portal.CardSize + FVector2D(20.f, 20.f),
			FLinearColor(Red.R, Red.G, Red.B, 0.18f), 22.f);
		Rect(CardPos, Portal.CardSize, FLinearColor(0.035f, 0.030f, 0.040f, 0.99f), 16.f);
		const float Border = 5.f;
		Rect(CardPos, FVector2D(Portal.CardSize.X, Border), Red, 3.f);
		Rect(CardPos + FVector2D(0.f, Portal.CardSize.Y - Border), FVector2D(Portal.CardSize.X, Border), Red, 3.f);
		Rect(CardPos, FVector2D(Border, Portal.CardSize.Y), Red, 3.f);
		Rect(CardPos + FVector2D(Portal.CardSize.X - Border, 0.f), FVector2D(Border, Portal.CardSize.Y), Red, 3.f);

		const bool bWin = GM->IsPortalChoiceSuccess();
		Text(bWin ? TEXT("通关") : TEXT("通关概率翻倍"),
			FVector2D(Portal.Center.X, CardPos.Y + 58.f), 52.f,
			bWin ? HPWHColorBrassSoft : RedSoft, true, true);
		Text(bWin ? TEXT("你从梦中醒来。") : TEXT("门后是更深的一层。"),
			FVector2D(Portal.Center.X, CardPos.Y + 132.f), 21.f,
			HPWHColorInkDim, true, true);
		Text(bWin ? TEXT("这一局结束了。") : TEXT("进入下一层后，坐标回到原点。"),
			FVector2D(Portal.Center.X, CardPos.Y + 166.f), 14.f,
			HPWHColorMuted, true);

		const FVector2D ButtonPos = Portal.ButtonCenter - Portal.ButtonSize * 0.5f;
		Rect(ButtonPos, Portal.ButtonSize, FLinearColor(0.12f, 0.030f, 0.045f, 0.98f), 10.f);
		Rect(ButtonPos, FVector2D(Portal.ButtonSize.X, 2.f), Red, 1.f);
		Rect(ButtonPos + FVector2D(0.f, Portal.ButtonSize.Y - 2.f), FVector2D(Portal.ButtonSize.X, 2.f), Red, 1.f);
		Text(bWin ? TEXT("回到主页") : TEXT("进入下一层"),
			FVector2D(Portal.ButtonCenter.X, Portal.ButtonCenter.Y - 13.f), 23.f,
			HPWHColorInk, true, true);
	}

	// 三选一强化界面
	if (GM && GM->IsUpgradeSelectionActive())
	{
		++Layer;
		Rect(FVector2D(0.f, 0.f), Size, HPWHAlpha(HPWHColorBgDeep, 0.78f));

		const FHPWH_UpgradeCardLayout Layout = HPWH_MakeUpgradeCardLayout(Size);
		const TArray<FHPWH_UpgradeOption>& Options = GM->GetUpgradeOptions();
		for (int32 Index = 0; Index < Options.Num() && Index < 3; ++Index)
		{
			const FHPWH_UpgradeOption& Option = Options[Index];
			const FVector2D Center = Layout.Centers[Index];
			const FVector2D Size2D = Layout.CardSize;
			const FVector2D Pos = Center - Size2D * 0.5f;
			const FLinearColor BorderColor = HPWH_RarityColor(Option.Rarity);

			Rect(Pos - FVector2D(8.f, 8.f), Size2D + FVector2D(16.f, 16.f),
				FLinearColor(BorderColor.R, BorderColor.G, BorderColor.B, 0.18f), 18.f);
			Rect(Pos, Size2D, HPWHAlpha(HPWHColorBgPanel, 0.97f), 14.f);

			const float BorderWidth = 5.f;
			Rect(Pos, FVector2D(Size2D.X, BorderWidth), BorderColor, 3.f);
			Rect(Pos + FVector2D(0.f, Size2D.Y - BorderWidth), FVector2D(Size2D.X, BorderWidth), BorderColor, 3.f);
			Rect(Pos, FVector2D(BorderWidth, Size2D.Y), BorderColor, 3.f);
			Rect(Pos + FVector2D(Size2D.X - BorderWidth, 0.f), FVector2D(BorderWidth, Size2D.Y), BorderColor, 3.f);

			Text(HPWH_RarityName(Option.Rarity), FVector2D(Center.X, Pos.Y + 16.f), 17.f, BorderColor, true, true);
			Icon(HPWH_UpgradeIconFor(Option.Type), FVector2D(Center.X, Pos.Y + 78.f), 54.f,
				FLinearColor(1.f, 1.f, 1.f, 0.94f));
			Text(Option.Title, FVector2D(Center.X, Pos.Y + 108.f), 34.f, HPWHColorInk, true, true);
			Rect(FVector2D(Center.X - 60.f, Pos.Y + 150.f), FVector2D(120.f, 2.f),
				FLinearColor(BorderColor.R, BorderColor.G, BorderColor.B, 0.65f), 1.f);

			TArray<FString> Lines;
			Option.Description.ParseIntoArrayLines(Lines);
			const float LineTop = Pos.Y + Size2D.Y * 0.43f;
			for (int32 LineIndex = 0; LineIndex < Lines.Num(); ++LineIndex)
			{
				Text(Lines[LineIndex], FVector2D(Center.X, LineTop + LineIndex * 32.f), 21.f,
					HPWHColorInkDim, true);
			}

			Text(TEXT("点击选择"), FVector2D(Center.X, Pos.Y + Size2D.Y - 48.f), 16.f,
				HPWHColorMuted, true);
		}

		// 每组三选一可刷新一次。
		const bool bCanReroll = GM->CanRerollUpgrade();
		const FVector2D RerollPos = Layout.RerollCenter - Layout.RerollSize * 0.5f;
		Rect(RerollPos, Layout.RerollSize, bCanReroll
			? HPWHAlpha(HPWHColorBgPanel, 0.94f)
			: HPWHAlpha(HPWHColorBgPanel, 0.5f), 9.f);
		const FLinearColor RerollBorder = bCanReroll
			? HPWHAlpha(HPWHColorBrassSoft, 0.8f)
			: HPWHAlpha(HPWHColorLine, 0.22f);
		Rect(RerollPos, FVector2D(Layout.RerollSize.X, 1.2f), RerollBorder, 1.f);
		Rect(RerollPos + FVector2D(0.f, Layout.RerollSize.Y - 1.2f),
			FVector2D(Layout.RerollSize.X, 1.2f), RerollBorder, 1.f);
		Text(bCanReroll ? TEXT("刷新 ×1    R") : TEXT("本组已刷新"),
			FVector2D(Layout.RerollCenter.X, Layout.RerollCenter.Y - 11.f), 15.f,
			bCanReroll ? HPWHColorInk : HPWHColorMuted, true, true);

		Text(FString::Printf(TEXT("等级 %d   选择一张强化"), GM->GetPlayerLevel()),
			FVector2D(Size.X * 0.5f, Size.Y * 0.09f), 38.f, HPWHColorInk, true, true);
		Text(FString::Printf(TEXT("经验 %d / %d    1 / 2 / 3 选择    R 刷新"),
			GM->GetExperience(), GM->GetExperienceToNextLevel()),
			FVector2D(Size.X * 0.5f, Size.Y * 0.16f), 20.f, HPWHColorMuted, true, true);
	}

	return Layer + 1;
}
