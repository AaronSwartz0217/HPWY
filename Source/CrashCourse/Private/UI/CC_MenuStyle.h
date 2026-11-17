#pragma once
#include "UI/CC_GameFont.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/CC_MenuArtData.h"
#include "UI/CC_SkillGalleryData.h"

// Shared native presentation for the map and options; textures remain in DA_MenuArt.
namespace CCMenuStyle
{
inline const FLinearColor Gold(.82f,.72f,.51f,1.f);
inline const FLinearColor Ivory(.92f,.88f,.77f,1.f);
inline const FLinearColor Muted(.57f,.57f,.57f,1.f);
inline const FLinearColor Error(.88f,.52f,.38f,1.f);

inline UTextBlock* Text(UWidgetTree* Tree,const FString& Value,int32 Size=18)
{
	auto* Label=Tree->ConstructWidget<UTextBlock>();
	auto Font= CCGameFont::Kai(Label->GetFont());Font.Size=Size;Label->SetFont(Font);
	Label->SetText(FText::FromString(Value));Label->SetColorAndOpacity(Ivory);
	Label->SetAutoWrapText(true);Label->SetLineHeightPercentage(1.2f);
	return Label;
}
inline void SingleLine(UTextBlock* Label)
{
	// Auto-wrap plus a centered, desired-width button slot can retain a narrow
	// wrap width after a scaled prepass. Fixed-height controls must stay one line.
	Label->SetAutoWrapText(false);Label->SetWrapTextAt(0.f);
	Label->SetLineHeightPercentage(1.f);
	Label->SetClipping(EWidgetClipping::ClipToBounds);
	Label->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
}
inline FButtonStyle Action(bool Selected=false)
{
	FButtonStyle Style;
	Style.SetNormal(FSlateColorBrush(Selected ? FLinearColor(.035f,.035f,.035f,.90f) : FLinearColor(0,0,0,.72f)));
	Style.SetHovered(FSlateColorBrush(FLinearColor(.055f,.055f,.055f,.90f)));
	Style.SetPressed(FSlateColorBrush(FLinearColor(.025f,.025f,.025f,.96f)));
	Style.SetDisabled(FSlateColorBrush(FLinearColor(0,0,0,.48f)));
	Style.SetNormalPadding(FMargin(14,6));Style.SetPressedPadding(FMargin(14,7,14,5));
	return Style;
}
inline UButton* Button(UWidgetTree* Tree,const FString& Value,int32 FontSize=18)
{
	auto* Result=Tree->ConstructWidget<UButton>();Result->SetStyle(Action());
	auto* Label=Text(Tree,Value,FontSize);Label->SetJustification(ETextJustify::Center);
	SingleLine(Label);Label->SetColorAndOpacity(Gold);Result->SetContent(Label);
	auto* Slot=CastChecked<UButtonSlot>(Label->Slot);
	Slot->SetHorizontalAlignment(HAlign_Fill);Slot->SetVerticalAlignment(VAlign_Center);
	return Result;
}
struct FFrame { UBorder* Root; UBorder* Body; };
inline FFrame Frame(UWidgetTree* Tree,UCC_MenuArtData* Art,FMargin Padding=FMargin(20))
{
	auto* Root=Tree->ConstructWidget<UBorder>();Root->SetPadding(FMargin(1));
	Root->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0,0,0,.84f),0.f,FLinearColor(.82f,.72f,.51f,.32f),1.f));
	auto* Body=Tree->ConstructWidget<UBorder>();Body->SetPadding(Padding);
	Body->SetBrush(FSlateColorBrush(FLinearColor::Transparent));
	Root->SetContent(Body);return {Root,Body};
}
}
