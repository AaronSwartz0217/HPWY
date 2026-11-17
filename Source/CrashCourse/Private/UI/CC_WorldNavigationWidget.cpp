#include "UI/CC_WorldNavigationWidget.h"
#include "CC_MenuStyle.h"
#include "World/CC_AdventureGameInstance.h"
#include "World/CC_MinimapCapture.h"
#include "Player/CC_PlayerController.h"
#include "Characters/CC_BaseCharacter.h"
#include "Player/CC_PlayerState.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/NativeWidgetHost.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Rendering/DrawElements.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
const FName RegionIds[]={TEXT("Rest"),TEXT("Town"),TEXT("Arena"),TEXT("Heaven"),TEXT("Palace")};
void NavigationPlace(UCanvasPanel* Canvas,UWidget* Widget,float X,float Y,float W,float H)
{
 auto* Layout=Canvas->AddChildToCanvas(Widget);Layout->SetPosition(FVector2D(X,Y));Layout->SetSize(FVector2D(W,H));
}
FString RegionDescription(FName Id)
{
 if (Id==TEXT("Rest")) return TEXT("安身歇息之所，也是旅途的起点。\n默认解锁，角色倒下后将回到此地。");
 if (Id==TEXT("Town")) return TEXT("古风小镇，沿旧传送装置开启新的旅程。\n先从休憩之地触碰传送装置抵达，方可快捷传送。");
 if (Id==TEXT("Heaven")) return TEXT("万象铺陈，天宫初成。\n默认解锁，可在此调整网格街区、建筑与材质。");
 if (Id==TEXT("Palace")) return TEXT("宫殿场景搭建中。\n使用宫殿传送卷轴前往。");
 return TEXT("山河之间的历练之地。\n默认解锁，可在此尝试变身、进化与各式技能。");
}
}
UCC_WorldNavigationWidget::UCC_WorldNavigationWidget(const FObjectInitializer& ObjectInitializer):Super(ObjectInitializer)
{
 static ConstructorHelpers::FObjectFinder<UCC_MenuArtData> Art(TEXT("/Game/UI/MuralMenu/DA_MenuArt.DA_MenuArt"));ArtData=Art.Object;
}
TSharedRef<SWidget> UCC_WorldNavigationWidget::RebuildWidget()
{
 // Creating RootWidget in NativeConstruct is too late for the initial Slate root.
 if (WidgetTree) BuildLayout();
 return Super::RebuildWidget();
}
void UCC_WorldNavigationWidget::NativeConstruct()
{
 Super::NativeConstruct();SetIsFocusable(true);
}
void UCC_WorldNavigationWidget::BuildLayout()
{
 if (WidgetTree->RootWidget) return;
 auto* Root=WidgetTree->ConstructWidget<UCanvasPanel>();Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);WidgetTree->RootWidget=Root;
 auto* Scale=WidgetTree->ConstructWidget<UScaleBox>();Scale->SetStretch(EStretch::ScaleToFit);Scale->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
 auto* ScaleLayout=Root->AddChildToCanvas(Scale);ScaleLayout->SetAnchors(FAnchors(0,0,1,1));ScaleLayout->SetOffsets(FMargin(0));
 auto* DesignSize=WidgetTree->ConstructWidget<USizeBox>();DesignSize->SetWidthOverride(1920);DesignSize->SetHeightOverride(1080);Scale->SetContent(DesignSize);
 auto* Canvas=WidgetTree->ConstructWidget<UCanvasPanel>();Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);DesignSize->SetContent(Canvas);
 // Keep the minimap render target square.
 // Anchor the map to the viewport, outside the centered 16:9 travel layout.
 auto* MapScale=WidgetTree->ConstructWidget<UScaleBox>();MapScale->SetStretch(EStretch::ScaleToFit);MapScale->SetVisibility(ESlateVisibility::HitTestInvisible);
 auto* MapLayout=Root->AddChildToCanvas(MapScale);MapLayout->SetAnchors(FAnchors(0,0));MapLayout->SetPosition({40,40});MapLayout->SetSize({216,216});
 auto* MapSize=WidgetTree->ConstructWidget<USizeBox>();MapSize->SetWidthOverride(334);MapSize->SetHeightOverride(334);MapScale->SetContent(MapSize);
 // A single square map with a fine bronze frame and player direction marker.
 auto* MapFrame=WidgetTree->ConstructWidget<UBorder>();MapFrame->SetPadding(FMargin(0));
 MapFrame->SetBrush(FSlateRoundedBoxBrush(FLinearColor(.02f,.017f,.014f,.56f),0.f,FLinearColor(.60f,.43f,.20f,.42f),1.f));
 MapPanel=MapScale;MapSize->SetContent(MapFrame);MapPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
 auto* MapCanvas=WidgetTree->ConstructWidget<UCanvasPanel>();MapFrame->SetContent(MapCanvas);MapDecoration=MapCanvas;
 MapImage=WidgetTree->ConstructWidget<UImage>();MapImage->SetClipping(EWidgetClipping::ClipToBounds);NavigationPlace(MapCanvas,MapImage,10,10,314,314);
 // Map-only HUD: no title strip, compass label, or shortcut footer.
 // Retain the internal region label for the shared travel refresh lifecycle.
 RegionTitle=CCMenuStyle::Text(WidgetTree,TEXT(""),23);RegionTitle->SetVisibility(ESlateVisibility::Collapsed);
 Heading=CCMenuStyle::Text(WidgetTree,TEXT("▲"),26);Heading->SetJustification(ETextJustify::Center);Heading->SetColorAndOpacity(CCMenuStyle::Gold);NavigationPlace(MapCanvas,Heading,148,148,38,38);
 auto Travel=CCMenuStyle::Frame(WidgetTree,ArtData,FMargin(0));TravelPanel=Travel.Root;NavigationPlace(Canvas,Travel.Root,1260,48,620,702);
 auto* TravelCanvas=WidgetTree->ConstructWidget<UCanvasPanel>();Travel.Body->SetContent(TravelCanvas);
 auto Label=[this,TravelCanvas](const FString& Value,float X,float Y,float W,float H,int32 FontSize=18) {
  auto* T=CCMenuStyle::Text(WidgetTree,Value,FontSize);CCMenuStyle::SingleLine(T);NavigationPlace(TravelCanvas,T,X,Y,W,H);return T;
 };
 Label(TEXT("行 旅"),26,22,300,44,30)->SetColorAndOpacity(CCMenuStyle::Gold);
 Label(TEXT("择一处山河，再启程"),26,68,450,27,16)->SetColorAndOpacity(CCMenuStyle::Muted);
 auto* Close=CCMenuStyle::Button(WidgetTree,TEXT("×"),24);auto CloseStyle=CCMenuStyle::Action();CloseStyle.SetNormalPadding(FMargin(0));CloseStyle.SetPressedPadding(FMargin(0));Close->SetStyle(CloseStyle);NavigationPlace(TravelCanvas,Close,540,22,52,44);Close->OnClicked.AddDynamic(this,&ThisClass::CloseTravel);
 for (int32 I=0;I<UE_ARRAY_COUNT(RegionIds);++I)
 {
  auto* B=CCMenuStyle::Button(WidgetTree,TEXT(""),21);NavigationPlace(TravelCanvas,B,26,106+I*68,568,58);
  TravelButtons.Add(B);TravelLabels.Add(CastChecked<UTextBlock>(B->GetContent()));
 }
 TravelButtons[0]->OnClicked.AddDynamic(this,&ThisClass::TravelRest);TravelButtons[1]->OnClicked.AddDynamic(this,&ThisClass::TravelTown);TravelButtons[2]->OnClicked.AddDynamic(this,&ThisClass::TravelArena);
 TravelButtons[3]->OnClicked.AddDynamic(this,&ThisClass::TravelHeaven);
 TravelButtons[4]->OnClicked.AddDynamic(this,&ThisClass::TravelPalace);
 DestinationTitle=Label(TEXT(""),28,452,564,38,22);DestinationTitle->SetColorAndOpacity(CCMenuStyle::Gold);
 DestinationDescription=Label(TEXT(""),28,496,564,78,17);DestinationDescription->SetAutoWrapText(true);DestinationDescription->SetLineHeightPercentage(1.2f);DestinationDescription->SetColorAndOpacity(CCMenuStyle::Muted);
 Feedback=Label(TEXT(""),28,578,564,34,15);Feedback->SetAutoWrapText(true);Feedback->SetColorAndOpacity(CCMenuStyle::Error);
 ConfirmButton=CCMenuStyle::Button(WidgetTree,TEXT("选择目的地"),21);ConfirmLabel=CastChecked<UTextBlock>(ConfirmButton->GetContent());NavigationPlace(TravelCanvas,ConfirmButton,26,622,568,56);ConfirmButton->OnClicked.AddDynamic(this,&ThisClass::ConfirmTravel);
 TravelPanel->SetVisibility(ESlateVisibility::Collapsed);
 // The loading shade covers the actual viewport, including ultra-wide margins.
 auto* Shade=WidgetTree->ConstructWidget<UNativeWidgetHost>();LoadingPanel=Shade;
 auto* LoadingLayout=Root->AddChildToCanvas(Shade);LoadingLayout->SetAnchors(FAnchors(0,0,1,1));LoadingLayout->SetOffsets(FMargin(0));LoadingLayout->SetZOrder(100);
 if(auto* Adventure=GetGameInstance<UCC_AdventureGameInstance>())Shade->SetContent(Adventure->CreateLoadingScreen());
 LoadingPanel->SetVisibility(ESlateVisibility::Collapsed);
}
void UCC_WorldNavigationWidget::InitializeNavigation(ACC_MinimapCapture* Camera)
{
 Minimap=Camera;
 if (auto* Adventure=GetGameInstance<UCC_AdventureGameInstance>()) bMapVisible=Adventure->bMinimapVisible;
 if (MapPanel) MapPanel->SetVisibility(bMapVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
 if (Minimap) Minimap->SetCaptureEnabled(bMapVisible);
 if (MapImage && Minimap && Minimap->RenderTarget)
 {
  FSlateBrush Brush;Brush.DrawAs=ESlateBrushDrawType::Image;Brush.SetResourceObject(Minimap->RenderTarget);Brush.ImageSize=FVector2D(256,256);MapImage->SetBrush(Brush);
 }
 RefreshPresentation();OnNavigationVisibilityChanged(bMapVisible,bTravelOpen);
}
void UCC_WorldNavigationWidget::ToggleMinimap()
{
 bMapVisible=!bMapVisible;
 if (auto* Adventure=GetGameInstance<UCC_AdventureGameInstance>()) Adventure->bMinimapVisible=bMapVisible;
 if (MapPanel) MapPanel->SetVisibility(bMapVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
 if (Minimap) Minimap->SetCaptureEnabled(bMapVisible);
 OnNavigationVisibilityChanged(bMapVisible,bTravelOpen);
}
void UCC_WorldNavigationWidget::SetTravelOpen(bool bOpen)
{
 bTravelOpen=bOpen;if (TravelPanel) TravelPanel->SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
 if (bOpen)
 {
  if (const auto* Adventure=GetGameInstance<UCC_AdventureGameInstance>())
   if (SelectedRegion.IsNone() || SelectedRegion==Adventure->CurrentRegion)
    for (FName Id:RegionIds) if (Id!=Adventure->CurrentRegion && Adventure->IsRegionUnlocked(Id)) {SelectedRegion=Id;break;}
  RefreshPresentation();
 }
 OnNavigationVisibilityChanged(bMapVisible,bTravelOpen);
}
void UCC_WorldNavigationWidget::SelectRegion(FName Id)
{
 for (FName Known:RegionIds) if (Id==Known) {SelectedRegion=Id;RefreshPresentation();return;}
}
void UCC_WorldNavigationWidget::RefreshPresentation()
{
 auto* Adventure=GetGameInstance<UCC_AdventureGameInstance>();if (!Adventure || !RegionTitle) return;
 if (const auto* Region=Adventure->FindRegion(Adventure->CurrentRegion)) RegionTitle->SetText(Region->DisplayName);
 for (int32 I=0;I<TravelButtons.Num();++I)
 {
  const auto* Region=Adventure->FindRegion(RegionIds[I]);const bool Current=Adventure->CurrentRegion==RegionIds[I];
  const bool Unlocked=Adventure->IsRegionUnlocked(RegionIds[I]);
  TravelButtons[I]->SetIsEnabled(Region && !Adventure->bTravelling);TravelButtons[I]->SetStyle(CCMenuStyle::Action(SelectedRegion==RegionIds[I]));
  TravelLabels[I]->SetColorAndOpacity(Unlocked ? CCMenuStyle::Gold : CCMenuStyle::Muted);
  TravelLabels[I]->SetText(FText::FromString(FString::Printf(TEXT("%02d   %s    · %s"),I+1,Region ? *Region->DisplayName.ToString() : TEXT("暂无地区"),Current ? TEXT("当前所在") : Unlocked ? TEXT("可传送") : TEXT("尚未抵达"))));
 }
 const auto* Destination=Adventure->FindRegion(SelectedRegion);const bool Unlocked=Adventure->IsRegionUnlocked(SelectedRegion),Current=Adventure->CurrentRegion==SelectedRegion;
 DestinationTitle->SetText(Destination ? Destination->DisplayName : FText::FromString(TEXT("选择目的地")));
 DestinationDescription->SetText(FText::FromString(Destination ? RegionDescription(SelectedRegion) : TEXT("从上方选择一处已解锁的地区。")));
 auto* PC=Cast<ACC_PlayerController>(GetOwningPlayer());auto* PS=PC ? PC->GetPlayerState<ACC_PlayerState>() : nullptr;
 auto* Character=PC ? Cast<ACC_BaseCharacter>(PC->GetPawn()) : nullptr;
 const bool Ready=Character && Character->IsAlive() && PS && PS->Progression && PS->Progression->CanConfigureSkills();
 const float Cooldown=Adventure->GetTravelCooldownRemaining();const bool Pending=Adventure->IsPortalDeparturePending(),HasScroll=Adventure->HasTravelScroll(SelectedRegion);
 ConfirmButton->SetIsEnabled(Destination&&Unlocked&&!Current&&Ready&&!Adventure->bTravelling&&!Pending&&Cooldown<=0&&HasScroll);
 ConfirmLabel->SetText(FText::FromString(!Destination?TEXT("选择目的地"):Current?TEXT("已在此地"):!Unlocked?TEXT("尚未解锁"):!HasScroll?TEXT("缺少卷轴 · 旧纸摊购买"):Pending?TEXT("传送球凝聚中 · 留在附近"):Cooldown>0?FString::Printf(TEXT("传送冷却 %.0f 秒"),FMath::CeilToFloat(Cooldown)):!Ready?TEXT("等待角色或技能就绪"):TEXT("释放传送球 · 等待8秒")));
 FString Details=Destination?RegionDescription(SelectedRegion):TEXT("从上方选择一处已解锁的地区。");
 int32 LineBreak=INDEX_NONE;if(Details.FindChar('\n',LineBreak))Details=Details.Left(LineBreak);
 Details+=TEXT("\n传送球凝聚8秒，离开范围取消；每30秒可释放一次。");
 if(SelectedRegion==TEXT("Arena")||SelectedRegion==TEXT("Heaven")||SelectedRegion==TEXT("Palace"))Details+=TEXT("成功抵达消耗1张对应卷轴，取消不消耗。");
 DestinationDescription->SetText(FText::FromString(Details));
 Feedback->SetText(Adventure->LastTravelError);
}
void UCC_WorldNavigationWidget::NativeTick(const FGeometry& Geometry,float DeltaSeconds)
{
 Super::NativeTick(Geometry,DeltaSeconds);
 auto* PC=Cast<ACC_PlayerController>(GetOwningPlayer());auto* Adventure=GetGameInstance<UCC_AdventureGameInstance>();if (!PC || !Adventure) return;
 const bool ShowMap=bMapVisible && !PC->IsProgressionMenuOpen() && !Adventure->bTravelling;
 if (Minimap) Minimap->SetCaptureEnabled(ShowMap);
 if (MapPanel) MapPanel->SetVisibility(ShowMap ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
 // Designer layouts own their presentation and receive the same camera/state events.
 if (!RegionTitle) return;
 if (Minimap && Minimap->RenderTarget && MapImage->GetBrush().GetResourceObject()!=Minimap->RenderTarget.Get()) InitializeNavigation(Minimap);
 if (APawn* Pawn=PC->GetPawn()) Heading->SetRenderTransformAngle(Pawn->GetActorRotation().Yaw);
 RefreshElapsed+=DeltaSeconds;if (RefreshElapsed>=.15f) {RefreshElapsed=0;RefreshPresentation();}
 LoadingPanel->SetVisibility(Adventure->bTravelling ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}
FReply UCC_WorldNavigationWidget::NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
 if (Event.GetKey()==EKeys::M) {ToggleMinimap();return FReply::Handled();}
 if (Event.GetKey()==EKeys::Tab) {if(!Event.IsRepeat())if (auto* PC=Cast<ACC_PlayerController>(GetOwningPlayer())) PC->ToggleProgressionMenu();return FReply::Handled();}
 return Super::NativeOnKeyDown(Geometry,Event);
}
void UCC_WorldNavigationWidget::NativeDestruct(){if (bTravelOpen) CloseTravel();Super::NativeDestruct();}
void UCC_WorldNavigationWidget::ConfirmTravel(){if (auto* Adventure=GetGameInstance<UCC_AdventureGameInstance>()) Adventure->RequestFastTravel(SelectedRegion);RefreshPresentation();}
void UCC_WorldNavigationWidget::CloseTravel(){if (auto* PC=Cast<ACC_PlayerController>(GetOwningPlayer())) PC->SetTravelMenuOpen(false);}
void UCC_WorldNavigationWidget::TravelRest(){SelectRegion(TEXT("Rest"));}
void UCC_WorldNavigationWidget::TravelTown(){SelectRegion(TEXT("Town"));}
void UCC_WorldNavigationWidget::TravelArena(){SelectRegion(TEXT("Arena"));}
void UCC_WorldNavigationWidget::TravelHeaven(){SelectRegion(TEXT("Heaven"));}
void UCC_WorldNavigationWidget::TravelPalace(){SelectRegion(TEXT("Palace"));}


int32 UCC_WorldNavigationWidget::NativePaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& Cull,FSlateWindowElementList& Elements,int32 Layer,const FWidgetStyle& Style,bool bParentEnabled)const
{
 const int32 Top=Super::NativePaint(Args,Geometry,Cull,Elements,Layer,Style,bParentEnabled)+1;
 if(!MapDecoration||!MapPanel||MapPanel->GetVisibility()==ESlateVisibility::Collapsed||MapPanel->GetVisibility()==ESlateVisibility::Hidden)return Top;
 const auto& G=MapDecoration->GetCachedGeometry();const FVector2D Size=G.GetLocalSize();if(Size.IsNearlyZero())return Top;
 auto Line=[&](std::initializer_list<FVector2D> Points,float Alpha,float Width=1.f)
 {
  TArray<FVector2D> Path;Path.Reserve(static_cast<int32>(Points.size()));for(const auto& P:Points)Path.Add(Geometry.AbsoluteToLocal(G.LocalToAbsolute(P)));
  FLinearColor Color(.77f,.55f,.25f,Alpha);Color*=Style.GetColorAndOpacityTint();
  FSlateDrawElement::MakeLines(Elements,Top,Geometry.ToPaintGeometry(),Path,ESlateDrawEffect::None,Color,true,Width);
 };
 for(int32 X=0;X<2;++X)for(int32 Y=0;Y<2;++Y)
 {
  const FVector2D C(X?Size.X-3:3,Y?Size.Y-3:3);const float SX=X?-1.f:1.f,SY=Y?-1.f:1.f;
  Line({C+FVector2D(0,27*SY),C,C+FVector2D(27*SX,0)},.9f,1.2f);
  Line({C+FVector2D(5*SX,20*SY),C+FVector2D(5*SX,5*SY),C+FVector2D(20*SX,5*SY),C+FVector2D(20*SX,13*SY),C+FVector2D(12*SX,13*SY),C+FVector2D(12*SX,9*SY)},.7f,.9f);
  Line({C+FVector2D(31*SX,0),C+FVector2D(43*SX,0),C+FVector2D(47*SX,4*SY),C+FVector2D(62*SX,4*SY)},.4f,.8f);
 }
 // Paired cloud curls continue the combat HUD's understated bronze border.
 const float Mid=Size.X*.5f;
 for(float Sign:{-1.f,1.f})
 {
  Line({{Mid+Sign*7,5},{Mid+Sign*18,5},{Mid+Sign*23,10},{Mid+Sign*36,10},{Mid+Sign*40,6},{Mid+Sign*40,3},{Mid+Sign*31,3}},.62f,.9f);
 }
 Line({{Mid-3,5},{Mid,2},{Mid+3,5},{Mid,8},{Mid-3,5}},.85f,.9f);
 return Top;
}
