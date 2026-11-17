#include "UI/CC_MainMenuWidget.h"
#include "CC_MenuStyle.h"
#include "World/CC_AdventureGameInstance.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/NativeWidgetHost.h"
#include "Kismet/KismetSystemLibrary.h"
#include "HAL/PlatformProcess.h"

namespace
{
void HomePlace(UCanvasPanel* Canvas,UWidget* Widget,float X,float Y,float W,float H)
{
 auto* Layout=Canvas->AddChildToCanvas(Widget);Layout->SetPosition(FVector2D(X,Y));Layout->SetSize(FVector2D(W,H));
}
}
UCC_MainMenuWidget::UCC_MainMenuWidget(const FObjectInitializer& ObjectInitializer):Super(ObjectInitializer)
{
 ArtData=TSoftObjectPtr<UCC_MenuArtData>(FSoftObjectPath(TEXT("/Game/UI/MainMenu/DA_HomeArt.DA_HomeArt")));
}
TSharedRef<SWidget> UCC_MainMenuWidget::RebuildWidget()
{
 if (WidgetTree) BuildLayout();return Super::RebuildWidget();
}
void UCC_MainMenuWidget::BuildLayout()
{
 if (WidgetTree->RootWidget) return;
 LoadedArt=ArtData.LoadSynchronous();
 auto* Root=WidgetTree->ConstructWidget<UCanvasPanel>();WidgetTree->RootWidget=Root;
 auto* Ground=WidgetTree->ConstructWidget<UBorder>();Ground->SetBrushColor(FLinearColor(.012f,.016f,.014f,1));Ground->SetVisibility(ESlateVisibility::HitTestInvisible);
 auto* GroundLayout=Root->AddChildToCanvas(Ground);GroundLayout->SetAnchors(FAnchors(0,0,1,1));GroundLayout->SetOffsets(FMargin(0));
 auto* BackgroundFit=WidgetTree->ConstructWidget<UScaleBox>();BackgroundFit->SetStretch(EStretch::ScaleToFill);BackgroundFit->SetClipping(EWidgetClipping::ClipToBounds);BackgroundFit->SetVisibility(ESlateVisibility::HitTestInvisible);
 auto* BGLayout=Root->AddChildToCanvas(BackgroundFit);BGLayout->SetAnchors(FAnchors(0,0,1,1));BGLayout->SetOffsets(FMargin(0));
 auto* Background=WidgetTree->ConstructWidget<UImage>();Background->SetBrushFromTexture(LoadedArt ? LoadedArt->Background.LoadSynchronous() : nullptr,true);BackgroundFit->SetContent(Background);
 // Dim only the mural: the title and navigation are drawn after this layer.
 auto* Shade=WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("HomeBackgroundShade"));
 Shade->SetPadding(FMargin(0));Shade->SetBrushColor(FLinearColor(.004f,.004f,.005f,.65f));Shade->SetVisibility(ESlateVisibility::HitTestInvisible);
 auto* ShadeLayout=Root->AddChildToCanvas(Shade);ShadeLayout->SetAnchors(FAnchors(0,0,1,1));ShadeLayout->SetOffsets(FMargin(0));
 auto* Fit=WidgetTree->ConstructWidget<UScaleBox>();Fit->SetStretch(EStretch::ScaleToFit);
 auto* FitLayout=Root->AddChildToCanvas(Fit);FitLayout->SetAnchors(FAnchors(0,0,1,1));FitLayout->SetOffsets(FMargin(0));
 auto* Size=WidgetTree->ConstructWidget<USizeBox>();Size->SetWidthOverride(1920);Size->SetHeightOverride(1080);Fit->SetContent(Size);
 auto* Canvas=WidgetTree->ConstructWidget<UCanvasPanel>();Size->SetContent(Canvas);
 auto* Home=WidgetTree->ConstructWidget<UCanvasPanel>();HomePanel=Home;HomePlace(Canvas,Home,0,0,1920,1080);
 // A single centered axis keeps the wall visible and gives the title room to breathe.
 auto* LogoFit=WidgetTree->ConstructWidget<UScaleBox>();LogoFit->SetStretch(EStretch::ScaleToFit);LogoFit->SetVisibility(ESlateVisibility::HitTestInvisible);HomePlace(Home,LogoFit,660,170,600,246);
 auto* Logo=WidgetTree->ConstructWidget<UImage>();Logo->SetBrushFromTexture(LoadedArt ? LoadedArt->Logo.LoadSynchronous() : nullptr,true);Logo->SetColorAndOpacity(CCMenuStyle::Ivory);LogoFit->SetContent(Logo);
 auto MakeButton=[this,Home](const FString& Label,float Y,int32 FontSize) {
  auto* B=CCMenuStyle::Button(WidgetTree,Label,FontSize);FButtonStyle Style;
  const FSlateColorBrush Clear(FLinearColor::Transparent);
  Style.SetNormal(Clear);Style.SetHovered(Clear);Style.SetPressed(Clear);Style.SetDisabled(Clear);
  Style.SetNormalPadding(FMargin(16,10));Style.SetPressedPadding(FMargin(16,11,16,9));B->SetStyle(Style);
  auto* LabelWidget=CastChecked<UTextBlock>(B->GetContent());LabelWidget->SetJustification(ETextJustify::Center);auto Font=LabelWidget->GetFont();Font.LetterSpacing=240;LabelWidget->SetFont(Font);
  LabelWidget->SetColorAndOpacity(FLinearColor(.64f,.61f,.54f,1));
  HomePlace(Home,B,760,Y,400,64);return B;
 };
 StartButton=MakeButton(TEXT("开始游戏"),540,28);WebsiteButton=MakeButton(TEXT("宣传网站"),632,23);ExitButton=MakeButton(TEXT("退出游戏"),724,23);
 StartButton->OnClicked.AddDynamic(this,&ThisClass::StartGame);WebsiteButton->OnClicked.AddDynamic(this,&ThisClass::OpenWebsite);ExitButton->OnClicked.AddDynamic(this,&ThisClass::ExitGame);
 MessageText=CCMenuStyle::Text(WidgetTree,TEXT(""),17);MessageText->SetJustification(ETextJustify::Center);MessageText->SetColorAndOpacity(CCMenuStyle::Error);HomePlace(Home,MessageText,600,840,720,90);
 auto* LoadShade=WidgetTree->ConstructWidget<UNativeWidgetHost>();LoadingPanel=LoadShade;
 auto* LoadingLayout=Root->AddChildToCanvas(LoadShade);LoadingLayout->SetAnchors(FAnchors(0,0,1,1));LoadingLayout->SetOffsets(FMargin(0));LoadingLayout->SetZOrder(10);
 if(auto* Adventure=GetGameInstance<UCC_AdventureGameInstance>())LoadShade->SetContent(Adventure->CreateLoadingScreen());
 LoadingPanel->SetVisibility(ESlateVisibility::Collapsed);
 auto* Intro=WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("StartupLogoPanel"));Intro->SetBrushColor(FLinearColor::Black);Intro->SetPadding(FMargin(0));StartupLogoPanel=Intro;
 auto* IntroLayout=Root->AddChildToCanvas(Intro);IntroLayout->SetAnchors(FAnchors(0,0,1,1));IntroLayout->SetOffsets(FMargin(0));IntroLayout->SetZOrder(20);
 auto* IntroFit=WidgetTree->ConstructWidget<UScaleBox>();IntroFit->SetStretch(EStretch::ScaleToFit);Intro->SetContent(IntroFit);
 StartupLogoImage=WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("StartupLogoImage"));
 StartupLogoImage->SetBrushFromTexture(LoadedArt ? (LoadedArt->StartupLogo.IsNull() ? LoadedArt->Logo.LoadSynchronous() : LoadedArt->StartupLogo.LoadSynchronous()) : nullptr,true);
 IntroFit->SetContent(StartupLogoImage);Intro->SetVisibility(ESlateVisibility::Collapsed);
}
void UCC_MainMenuWidget::NativeConstruct()
{
 Super::NativeConstruct();SetIsFocusable(true);SetRenderOpacity(1.f);
 if (auto* Adventure=GetGameInstance<UCC_AdventureGameInstance>();Adventure && StartupLogoPanel && Adventure->ConsumeStartupLogo()) {
  bStartupSplashActive=true;StartupLogoStartedAt=0.;StartupLogoPanel->SetVisibility(ESlateVisibility::Visible);
  if (HomePanel) {HomePanel->SetIsEnabled(false);HomePanel->SetRenderOpacity(0.f);}
 }
 else if (StartButton) StartButton->SetUserFocus(GetOwningPlayer());
}
void UCC_MainMenuWidget::NativeTick(const FGeometry& Geometry,float DeltaSeconds)
{
 Super::NativeTick(Geometry,DeltaSeconds);
 if (bStartupSplashActive) {
  if (StartupLogoStartedAt<=0.) StartupLogoStartedAt=FPlatformTime::Seconds();
  const float Elapsed=static_cast<float>(FPlatformTime::Seconds()-StartupLogoStartedAt);
  StartupLogoImage->SetRenderOpacity(1.f-FMath::Clamp(Elapsed-2.f,0.f,1.f));
  if (Elapsed>=3.f) {bStartupSplashActive=false;StartupLogoPanel->SetVisibility(ESlateVisibility::Collapsed);if (HomePanel) HomePanel->SetIsEnabled(true);if (StartButton) StartButton->SetUserFocus(GetOwningPlayer());}
 }
 if (HomePanel && !bStartupSplashActive) HomePanel->SetRenderOpacity(FMath::Min(1.f,HomePanel->GetRenderOpacity()+DeltaSeconds*2.5f));
 const bool AnyHovered=(StartButton && StartButton->IsHovered()) || (WebsiteButton && WebsiteButton->IsHovered()) || (ExitButton && ExitButton->IsHovered());
 for (UButton* Button : {StartButton.Get(),WebsiteButton.Get(),ExitButton.Get()}) {
  if (!Button) continue;
  auto* Label=Cast<UTextBlock>(Button->GetContent());if (!Label) continue;
  const bool Active=AnyHovered ? Button->IsHovered() : Button->HasKeyboardFocus();
  const FLinearColor Target=Active ? CCMenuStyle::Ivory : Button==StartButton ? FLinearColor(.76f,.71f,.60f,1) : FLinearColor(.56f,.54f,.49f,1);
  Label->SetColorAndOpacity(FMath::Lerp(Label->GetColorAndOpacity().GetSpecifiedColor(),Target,FMath::Clamp(DeltaSeconds*10.f,0.f,1.f)));
 }
 auto* Adventure=GetGameInstance<UCC_AdventureGameInstance>();if (!Adventure || !StartButton) return;
 const bool Busy=Adventure->bTravelling;StartButton->SetIsEnabled(!Busy && !bStartupSplashActive);WebsiteButton->SetIsEnabled(!Busy && !bStartupSplashActive);ExitButton->SetIsEnabled(!Busy && !bStartupSplashActive);
 MessageText->SetText(Adventure->LastTravelError.IsEmpty() ? WebsiteStatus : Adventure->LastTravelError);LoadingPanel->SetVisibility(Busy ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}
void UCC_MainMenuWidget::StartGame(){if (!bStartupSplashActive) if (auto* Adventure=GetGameInstance<UCC_AdventureGameInstance>()) Adventure->StartGameFromHome();}
void UCC_MainMenuWidget::OpenWebsite()
{
 if (bStartupSplashActive) return;
 if (const auto* Adventure=GetGameInstance<UCC_AdventureGameInstance>();Adventure && Adventure->bTravelling) return;
 const FString URL=LoadedArt ? LoadedArt->WebsiteURL.TrimStartAndEnd() : FString();
 if ((!URL.StartsWith(TEXT("https://")) && !URL.StartsWith(TEXT("http://"))) || URL.Contains(TEXT("\n")) || URL.Contains(TEXT("\r")) || URL.Contains(TEXT(" ")))
 {WebsiteStatus=FText::FromString(TEXT("宣传网站地址尚未配置，请检查主页美术资产。"));return;}
 FString Error;FPlatformProcess::LaunchURL(*URL,nullptr,&Error);
 WebsiteStatus=Error.IsEmpty() ? FText::FromString(TEXT("已请求使用浏览器打开宣传网站。")) : FText::FromString(TEXT("浏览器打开失败，请检查网站地址。"));
}
void UCC_MainMenuWidget::ExitGame(){if (!bStartupSplashActive) UKismetSystemLibrary::QuitGame(this,GetOwningPlayer(),EQuitPreference::Quit,false);}
FReply UCC_MainMenuWidget::NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
 if (Event.GetKey()==EKeys::Enter && HasKeyboardFocus()) {StartGame();return FReply::Handled();}
 return Super::NativeOnKeyDown(Geometry,Event);
}
