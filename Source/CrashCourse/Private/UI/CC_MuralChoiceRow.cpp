#include "CC_MuralChoiceRow.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "UI/CC_GameFont.h"

void UCC_MuralChoiceRow::SetContent(const FString& Label, bool bShowIcon,
    UTexture2D* Texture, UMaterialInterface* Material)
{
    auto* Text = WidgetTree->ConstructWidget<UTextBlock>();
    FSlateFontInfo Font = CCGameFont::Kai(Text->GetFont());
    Font.Size = 20;
    Text->SetFont(Font);
    Text->SetColorAndOpacity(FLinearColor(.92f,.88f,.77f,1.f));
    Text->SetText(FText::FromString(Label));
    Text->SetAutoWrapText(true);

    if (!bShowIcon)
    {
        WidgetTree->RootWidget = Text;
        return;
    }

    auto* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
    WidgetTree->RootWidget = Row;
    auto* Size = WidgetTree->ConstructWidget<USizeBox>();
    Size->SetWidthOverride(32);
    Size->SetHeightOverride(32);
    Row->AddChildToHorizontalBox(Size)->SetPadding(FMargin(0,0,10,0));
    auto* Icon = WidgetTree->ConstructWidget<UImage>();
    Size->SetContent(Icon);
    if (Material)
        Icon->SetBrushFromMaterial(Material);
    else if (Texture)
        Icon->SetBrushFromTexture(Texture);
    Row->AddChildToHorizontalBox(Text)->SetVerticalAlignment(VAlign_Center);
}
