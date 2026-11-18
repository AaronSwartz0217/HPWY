#include "SCC_LoadingScreen.h"
#include "UI/CC_GameFont.h"
#include "Engine/Texture2D.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void SCC_LoadingScreen::Construct(const FArguments& Args)
{
 const FLinearColor LoadingGold(.72f,.55f,.29f,1),LoadingMuted(.48f,.45f,.38f,1);
 LogoBrush.DrawAs=ESlateBrushDrawType::Image;
 LogoBrush.SetResourceObject(Args._Logo);
 LogoBrush.ImageSize=Args._Logo?FVector2D(Args._Logo->GetSizeX(),Args._Logo->GetSizeY()):FVector2D(600,246);
 ProgressStyle.SetBackgroundImage(FSlateRoundedBoxBrush(FLinearColor(.065f,.058f,.043f,1),3.f));
 ProgressStyle.SetFillImage(FSlateRoundedBoxBrush(LoadingGold,3.f));
 FSlateBrush Marquee=FCoreStyle::Get().GetWidgetStyle<FProgressBarStyle>("ProgressBar").MarqueeImage;
 Marquee.TintColor=LoadingGold;
 ProgressStyle.SetMarqueeImage(Marquee);
 const auto Progress=Args._Progress;
 const auto Opening=Args._Opening;
 ChildSlot
 [
  SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
  .BorderBackgroundColor(FLinearColor(.009f,.011f,.012f,1)).Padding(0)
  [
   SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
   [
    SNew(SBox).WidthOverride(1920).HeightOverride(1080)
    [
     SNew(SConstraintCanvas)
     +SConstraintCanvas::Slot().Anchors(FAnchors(.5f,.4f)).Alignment(FVector2D(.5f,.5f)).Offset(FMargin(0,0,600,246))
     [
      SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
      [
       SNew(SImage).Image(&LogoBrush).ColorAndOpacity(FLinearColor(.87f,.81f,.67f,1))
       .Visibility(Args._Logo?EVisibility::Visible:EVisibility::Collapsed)
      ]
     ]
     +SConstraintCanvas::Slot().Anchors(FAnchors(.5f,.6f)).Alignment(FVector2D(.5f,0)).AutoSize(true)
     [
      SNew(SBox).WidthOverride(640)
      [
       SNew(SVerticalBox)
       +SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0,0,0,22)
       [SNew(STextBlock).Text(Args._Title).Font(CCGameFont::Kai(FCoreStyle::GetDefaultFontStyle("Regular",22))).ColorAndOpacity(FLinearColor(.75f,.70f,.59f,1))]
       +SVerticalBox::Slot().AutoHeight()
       [
        SNew(SBox).HeightOverride(8)
        [SNew(SProgressBar).Style(&ProgressStyle).BarFillStyle(EProgressBarFillStyle::Scale).BorderPadding(FVector2D::ZeroVector)
         .FillColorAndOpacity(FLinearColor::White).Percent_Lambda([Progress,Opening](){return Opening.Get()?TOptional<float>():Progress.Get();})]
       ]
       +SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0,18,0,0)
       [SNew(STextBlock).Font(CCGameFont::Kai(FCoreStyle::GetDefaultFontStyle("Regular",16))).ColorAndOpacity(LoadingMuted)
        .Text_Lambda([Progress,Opening](){
         const auto Value=Progress.Get();
         return FText::FromString(Opening.Get()?TEXT("正在初始化场景，请稍候…"):Value.IsSet()?
          FString::Printf(TEXT("资源读取 · %d%%"),FMath::RoundToInt(Value.GetValue()*100)):TEXT("正在读取场景资源…"));
        })]
      ]
     ]
    ]
   ]
  ]
 ];
}

#if WITH_DEV_AUTOMATION_TESTS
#include "World/CC_AdventureGameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "Containers/Ticker.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

// Render the exact shared screen without travelling or writing player progress.
static FAutoConsoleCommandWithWorld LoadingPreviewCommand(TEXT("CC.LoadingPreview"),TEXT("Capture the shared loading presentation and exit this preview process."),
 FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World){
  if(!World||World->WorldType!=EWorldType::Game||World->GetNetMode()!=NM_Standalone||!FPaths::ProjectSavedDir().Contains(TEXT("LoadingPreviewUser")))return;
  auto* GI=World->GetGameInstance<UCC_AdventureGameInstance>();
  if(!GI||!World->GetGameViewport())return;
  World->GetGameViewport()->AddViewportWidgetContent(GI->CreateLoadingScreen(FText::FromString(TEXT("前往 · 小镇"))),1000);
  FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float){
   FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Loading/LoadingScreen.png"),true,false);
   FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float){FPlatformMisc::RequestExit(false);return false;}),1.f);
   return false;
  }),2.f);
 }));
#endif
