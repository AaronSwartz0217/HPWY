#include "UI/CC_LoreReader.h"

#include "Player/CC_PlayerController.h"
#include "UI/CC_GameFont.h"
#include "World/CC_LoreStand.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
const FLinearColor LoreInk(.085f,.047f,.026f);
const FLinearColor MutedInk(.23f,.137f,.075f);
const FLinearColor LoreSeal(.34f,.048f,.025f);
const FLinearColor BookBindingColor(.115f,.054f,.027f);
const FSlateRoundedBoxBrush Veil(FLinearColor(0.015f,0.011f,0.008f,.76f), 0.f);
const FSlateRoundedBoxBrush Cover(BookBindingColor, 9.f, FLinearColor(.32f,.17f,.06f), 1.f);
const FSlateRoundedBoxBrush LeftPaper(FLinearColor(.75f,.60f,.37f), 3.f);
const FSlateRoundedBoxBrush RightPaper(FLinearColor(.88f,.76f,.53f), 3.f);
const FSlateRoundedBoxBrush Gutter(FLinearColor(.22f,.115f,.052f), 0.f);
const FSlateRoundedBoxBrush LoreSelected(FLinearColor(.49f,.24f,.10f,.20f), 4.f);
const FSlateRoundedBoxBrush Blank(FLinearColor::Transparent, 0.f);
const FSlateRoundedBoxBrush ThoughtPanel(FLinearColor(.018f,.013f,.010f,.93f), 4.f,
    FLinearColor(.50f,.35f,.16f,.85f), 1.f);

FSlateFontInfo BookFont(int32 Size)
{
    return CCGameFont::Kai(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), Size));
}
}

TSharedRef<SWidget> UCC_LoreReader::RebuildWidget()
{
    if (!Book || Book->Pages.IsEmpty())
        return SNullWidget::NullWidget;

    auto Index = SAssignNew(Contents, SScrollBox);
    PageButtons.Reset();
    FText LastSection;
    for (int32 Page = 0; Page < Book->Pages.Num(); ++Page)
    {
        const FCC_LorePage& Entry = Book->Pages[Page];
        if (!Entry.Section.IsEmpty() && !Entry.Section.EqualTo(LastSection))
        {
            Index->AddSlot().Padding(6,12,0,5)
            [
                SNew(STextBlock).Text(Entry.Section).Font(BookFont(15)).ColorAndOpacity(LoreSeal)
            ];
        }
        LastSection = Entry.Section;

        TSharedRef<SButton> Button = SNew(SButton)
            .ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
            .ContentPadding(0)
            .OnClicked_Lambda([this, Page] { SetPage(Page); return FReply::Handled(); })
            [
                SNew(SBorder)
                .BorderImage_Lambda([this, Page] { return Page == PageIndex ? &LoreSelected : &Blank; })
                .Padding(FMargin(9, 8))
                [
                    SNew(STextBlock).Text(Entry.Title).Font(BookFont(18)).ColorAndOpacity(LoreInk)
                        .AutoWrapText(true)
                ]
            ];
        PageButtons.Add(Button);
        Index->AddSlot().Padding(0,0,4,3)[Button];
    }

    auto LeftPage = SNew(SBorder).BorderImage(&LeftPaper).Padding(FMargin(23,24,20,18))
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(5,0,0,8)
        [
            SNew(STextBlock).Text(Book->Title).Font(BookFont(29)).ColorAndOpacity(LoreInk).AutoWrapText(true)
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(5,0,0,8)
        [
            SNew(STextBlock).Text(FText::FromString(TEXT("目 录"))).Font(BookFont(15)).ColorAndOpacity(MutedInk)
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(0,0,0,7)[SNew(SSeparator).ColorAndOpacity(MutedInk)]
        + SVerticalBox::Slot().FillHeight(1)[Index]
    ];

    auto FooterButton = [](const TCHAR* Label, FOnClicked Action, TAttribute<bool> Enabled = true)
    {
        return SNew(SButton)
            .ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
            .ContentPadding(FMargin(10,7))
            .OnClicked(Action)
            .IsEnabled(Enabled)
            [
                SNew(STextBlock).Text(FText::FromString(Label)).Font(BookFont(17)).ColorAndOpacity(LoreSeal)
            ];
    };
    auto RightPage = SNew(SBorder).BorderImage(&RightPaper).Padding(FMargin(30,20,28,18))
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(0,0,0,9)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text_Lambda([this]
                {
                    const FText& Section = Book->Pages[PageIndex].Section;
                    return Section.IsEmpty() ? Book->Title : Section;
                })
                .Font(BookFont(16)).ColorAndOpacity(LoreSeal)
            ]
            + SHorizontalBox::Slot().AutoWidth()
            [
                FooterButton(TEXT("合上"), FOnClicked::CreateWeakLambda(this,
                    [this] { CloseBook(); return FReply::Handled(); }))
            ]
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(0,0,0,16)
        [
            SNew(STextBlock)
                .Text_Lambda([this] { return Book->Pages[PageIndex].Title; })
                .Font(BookFont(31)).ColorAndOpacity(LoreInk).AutoWrapText(true)
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(0,0,0,16)[SNew(SSeparator).ColorAndOpacity(MutedInk)]
        + SVerticalBox::Slot().FillHeight(1)
        [
            SAssignNew(PageScroll, SScrollBox)
            + SScrollBox::Slot().Padding(0,0,15,0)
            [
                SNew(STextBlock)
                    .Text_Lambda([this] { return Book->Pages[PageIndex].Body; })
                    .Font(BookFont(21)).ColorAndOpacity(LoreInk).AutoWrapText(true).LineHeightPercentage(1.32f)
            ]
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(0,12,0,0)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth()
            [
                FooterButton(TEXT("上一页"), FOnClicked::CreateWeakLambda(this,
                    [this] { SetPage(PageIndex-1); return FReply::Handled(); }),
                    TAttribute<bool>::CreateLambda([this] { return PageIndex > 0; }))
            ]
            + SHorizontalBox::Slot().FillWidth(1).HAlign(HAlign_Center).VAlign(VAlign_Center)
            [
                SNew(STextBlock).Text_Lambda([this]
                {
                    return FText::FromString(FString::Printf(TEXT("%d / %d"), PageIndex+1, Book->Pages.Num()));
                }).Font(BookFont(16)).ColorAndOpacity(MutedInk)
            ]
            + SHorizontalBox::Slot().AutoWidth()
            [
                FooterButton(TEXT("下一页"), FOnClicked::CreateWeakLambda(this,
                    [this] { SetPage(PageIndex+1); return FReply::Handled(); }),
                    TAttribute<bool>::CreateLambda([this] { return PageIndex+1 < Book->Pages.Num(); }))
            ]
        ]
    ];

    auto BookSpread = SNew(SOverlay)
        + SOverlay::Slot()[SNew(SBorder).BorderImage(&Veil)]
        + SOverlay::Slot()
        [
            SNew(SConstraintCanvas)
            + SConstraintCanvas::Slot().Anchors(FAnchors(.12f,.10f,.88f,.90f)).Offset(FMargin(0))
            [
                SNew(SBorder).BorderImage(&Cover).Padding(9)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().FillWidth(.33f)[LeftPage]
                    + SHorizontalBox::Slot().AutoWidth()
                    [
                        SNew(SBox).WidthOverride(7)[SNew(SBorder).BorderImage(&Gutter)]
                    ]
                    + SHorizontalBox::Slot().FillWidth(.67f)[RightPage]
                ]
            ]
        ];
    bShowingThought = !Book->OpeningThoughts.IsEmpty();
    if (!bShowingThought)
        return BookSpread;

    auto Thought = SNew(SOverlay)
        + SOverlay::Slot()
        [
            SNew(SButton)
                .ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
                .IsFocusable(false)
                .ClickMethod(EButtonClickMethod::MouseDown)
                .OnClicked(FOnClicked::CreateUObject(this, &ThisClass::AdvanceOpeningThought))
        ]
        + SOverlay::Slot()
        [
            SNew(SConstraintCanvas).Visibility(EVisibility::HitTestInvisible)
            + SConstraintCanvas::Slot().Anchors(FAnchors(.20f,.69f,.80f,.91f)).Offset(FMargin(0))
            [
                SNew(SBorder).BorderImage(&ThoughtPanel).Padding(FMargin(24,13))
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight().Padding(0,0,0,7)
                    [
                        SNew(STextBlock).Text(FText::FromString(TEXT("尤里西斯 · 心声")))
                            .Font(BookFont(17)).ColorAndOpacity(FLinearColor(.88f,.68f,.36f))
                    ]
                    + SVerticalBox::Slot().FillHeight(1).VAlign(VAlign_Center)
                    [
                        SNew(STextBlock)
                            .Text_Lambda([this] { return Book->OpeningThoughts[ThoughtIndex]; })
                            .Font(BookFont(23)).ColorAndOpacity(FLinearColor(.93f,.89f,.80f))
                            .AutoWrapText(true)
                    ]
                    + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
                    [
                        SNew(STextBlock)
                            .Text_Lambda([this] {
                                return FText::FromString(ThoughtIndex + 1 < Book->OpeningThoughts.Num()
                                    ? TEXT("左键继续") : TEXT("左键翻开书册"));
                            })
                            .Font(BookFont(14)).ColorAndOpacity(FLinearColor(.77f,.65f,.45f))
                    ]
                ]
            ]
        ];
    return SAssignNew(ReaderStage, SWidgetSwitcher)
        + SWidgetSwitcher::Slot()[Thought]
        + SWidgetSwitcher::Slot()[BookSpread];
}

FReply UCC_LoreReader::AdvanceOpeningThought()
{
    if (!bShowingThought)
        return FReply::Handled();
    if (ThoughtIndex + 1 < Book->OpeningThoughts.Num())
    {
        ++ThoughtIndex;
        InvalidateLayoutAndVolatility();
    }
    else
    {
        bShowingThought = false;
        ReaderStage->SetActiveWidgetIndex(1);
        SetKeyboardFocus();
    }
    return FReply::Handled();
}

void UCC_LoreReader::SetPage(int32 Index)
{
    if (!Book || !Book->Pages.IsValidIndex(Index) || PageIndex == Index)
        return;
    PageIndex = Index;
    if (PageScroll)
        PageScroll->ScrollToStart();
    if (Contents && PageButtons.IsValidIndex(Index))
        Contents->ScrollDescendantIntoView(PageButtons[Index], false);
    InvalidateLayoutAndVolatility();
}

FReply UCC_LoreReader::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    const FKey Key = Event.GetKey();
    const auto* Controller = Cast<ACC_PlayerController>(GetOwningPlayer());
    if (Key == EKeys::Tab || Key == EKeys::Escape || (!bShowingThought && Controller && Key == Controller->GetCombatKey(5)))
    {
        if (!Event.IsRepeat())
            CloseBook();
        return FReply::Handled();
    }
    if (bShowingThought)
        return FReply::Handled();
    if (Key == EKeys::Left || Key == EKeys::PageUp || Key == EKeys::Right || Key == EKeys::PageDown)
    {
        SetPage(PageIndex + (Key == EKeys::Left || Key == EKeys::PageUp ? -1 : 1));
        return FReply::Handled();
    }
    return Super::NativeOnPreviewKeyDown(Geometry, Event);
}

void UCC_LoreReader::CloseBook()
{
    if (auto* Controller = Cast<ACC_PlayerController>(GetOwningPlayer()))
        Controller->CloseLoreReader();
}

void UCC_LoreReader::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);
    PageButtons.Reset();
    Contents.Reset();
    PageScroll.Reset();
    ReaderStage.Reset();
}


