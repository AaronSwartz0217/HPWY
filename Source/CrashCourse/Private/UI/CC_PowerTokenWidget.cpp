#include "UI/CC_PowerTokenWidget.h"
#include "CC_MenuStyle.h"
#include "Progression/CC_GameplayData.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Brushes/SlateRoundedBoxBrush.h"

void UCC_PowerTokenWidget::NativeOnInitialized()
{
 Super::NativeOnInitialized();
 auto* Size=WidgetTree->ConstructWidget<USizeBox>();Size->SetWidthOverride(142);Size->SetHeightOverride(172);WidgetTree->RootWidget=Size;
 CellButton=WidgetTree->ConstructWidget<UButton>();CellButton->SetStyle(CCMenuStyle::Action());Size->SetContent(CellButton);
 auto* Canvas=WidgetTree->ConstructWidget<UCanvasPanel>();CellButton->SetContent(Canvas);
 auto Place=[Canvas](UWidget* Widget,float X,float Y,float W,float H){auto* Layout=Canvas->AddChildToCanvas(Widget);Layout->SetPosition(FVector2D(X,Y));Layout->SetSize(FVector2D(W,H));};
 // Button padding is zero so the 142px frame matches the clickable cell exactly.
 auto Style=CCMenuStyle::Action();Style.SetNormalPadding(FMargin(0));Style.SetPressedPadding(FMargin(0));CellButton->SetStyle(Style);
 FrameImage=WidgetTree->ConstructWidget<UImage>();FrameImage->SetVisibility(ESlateVisibility::HitTestInvisible);Place(FrameImage,0,0,142,142);
 TokenSeal=WidgetTree->ConstructWidget<UBorder>();TokenSeal->SetPadding(FMargin(0));TokenSeal->SetVisibility(ESlateVisibility::HitTestInvisible);TokenSeal->SetRenderTransformAngle(45);Place(TokenSeal,43,35,56,56);
 TokenMark=CCMenuStyle::Text(WidgetTree,TEXT(""),31);TokenMark->SetJustification(ETextJustify::Center);TokenMark->SetVisibility(ESlateVisibility::HitTestInvisible);Place(TokenMark,27,41,88,48);
 IconImage=WidgetTree->ConstructWidget<UImage>();IconImage->SetVisibility(ESlateVisibility::Collapsed);Place(IconImage,24,18,94,94);
 NameLabel=CCMenuStyle::Text(WidgetTree,TEXT(""),18);CCMenuStyle::SingleLine(NameLabel);NameLabel->SetJustification(ETextJustify::Center);NameLabel->SetVisibility(ESlateVisibility::HitTestInvisible);Place(NameLabel,5,111,132,28);
 StateLabel=CCMenuStyle::Text(WidgetTree,TEXT("空位"),15);CCMenuStyle::SingleLine(StateLabel);StateLabel->SetJustification(ETextJustify::Center);StateLabel->SetColorAndOpacity(CCMenuStyle::Muted);StateLabel->SetVisibility(ESlateVisibility::HitTestInvisible);Place(StateLabel,0,146,142,24);
 CellButton->OnClicked.AddDynamic(this,&ThisClass::SelectEntry);
}
void UCC_PowerTokenWidget::SetEntry(UCC_FormDefinition* Form,int32 Tier,bool bActive,bool bSelected,UCC_MenuArtData* Art)
{
 EntryForm=Form;if (!CellButton) return;
 UTexture2D* Frame=Art && Art->SkillGallery ? (bSelected ? Art->SkillGallery->CardSelected : Art->SkillGallery->CardNormal).LoadSynchronous() : nullptr;
 FSlateBrush Brush;Brush.SetResourceObject(Frame);Brush.DrawAs=ESlateBrushDrawType::Image;
 Brush.SetUVRegion(FBox2f(FVector2f(.085f,.085f),FVector2f(.915f,.915f)));FrameImage->SetBrush(Brush);
 FrameImage->SetColorAndOpacity(bSelected ? CCMenuStyle::Gold : Form ? FLinearColor(.64f,.62f,.51f,1) : FLinearColor(.35f,.38f,.33f,.5f));
 CellButton->SetIsEnabled(Form!=nullptr);
 NameLabel->SetText(Form ? Form->GetInventoryName() : FText::GetEmpty());
 TokenMark->SetText(FText::GetEmpty());TokenSeal->SetVisibility(Form ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
 if (!Form) {IconImage->SetVisibility(ESlateVisibility::Collapsed);StateLabel->SetText(FText::FromString(TEXT("空位")));return;}
 FString Mark=TEXT("印");FLinearColor Color(.22f,.24f,.28f,1);
 const bool InkDrop=Form->FormId==TEXT("InitialInk");
 if (InkDrop) Mark.Empty();
 else if (Form->FormId==TEXT("Artifact_01")) {Mark=TEXT("铜");Color=FLinearColor(.07f,.28f,.58f,1);}
 else if (Form->FormId==TEXT("Artifact_02")) {Mark=TEXT("陶");Color=FLinearColor(.54f,.29f,.08f,1);}
 else if (Form->FormId==TEXT("Native")) {Mark=TEXT("竹");Color=FLinearColor(.06f,.34f,.13f,1);}
 TokenSeal->SetBrushColor(Color);TokenMark->SetText(FText::FromString(Mark));TokenMark->SetColorAndOpacity(CCMenuStyle::Gold);
 if (InkDrop) {
  // A square with one pointed corner, rotated 45 degrees, forms a native ink drop.
  TokenSeal->SetBrush(FSlateRoundedBoxBrush(FLinearColor(.025f,.03f,.035f,1),FVector4(2,28,28,28),CCMenuStyle::Gold,1.5f));
  TokenSeal->SetBrushColor(FLinearColor::White);
 } else {TokenSeal->SetBrush(FSlateColorBrush(FLinearColor::White));TokenSeal->SetBrushColor(Color);}
 UTexture2D* Icon=Form->InventoryIcon.LoadSynchronous();IconImage->SetBrushFromTexture(Icon);
 IconImage->SetVisibility(Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
 TokenMark->SetVisibility(Icon || InkDrop ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
 TokenSeal->SetVisibility(Icon ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
 const TCHAR* Tiers[]={TEXT("初级"),TEXT("中级"),TEXT("高级")};
 StateLabel->SetText(FText::FromString(FString(bActive ? TEXT("使用中 · ") : TEXT(""))+(InkDrop ? TEXT("墨滴") : Form->bCanEvolve ? Tiers[FMath::Clamp(Tier,0,2)] : TEXT("初始"))));
 StateLabel->SetColorAndOpacity(bActive ? CCMenuStyle::Gold : CCMenuStyle::Muted);
 CellButton->SetToolTipText(FText::FromString(FString(InkDrop ? TEXT("墨滴 · ") : TEXT(""))+Form->GetInventoryName().ToString()+TEXT("\n点击查看信物，选择使用或进化。")));
}
void UCC_PowerTokenWidget::SelectEntry(){if (EntryForm) OnSelected.Broadcast(EntryForm);}
