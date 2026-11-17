#include "UI/CC_FirstPersonScanWidget.h"
#include "UI/CC_GameFont.h"
#include "Interaction/CC_PlayerScanComponent.h"
#include "Interaction/CC_ScanData.h"
#include "Player/CC_PlayerController.h"
#include "Characters/CC_PlayerCharacter.h"
#include "World/CC_LevitationPlatform.h"
#include "World/CC_TownShop.h"
#include "World/CC_LoreStand.h"
#include "World/CC_InteractiveDoubleDoor.h"
#include "Progression/CC_ExperiencePack.h"
#include "Progression/CC_ArtifactComponent.h"
#include "Progression/CC_GameplayData.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Rendering/DrawElements.h"
#include "WFCMapGenerator.h"
TSharedRef<SWidget> UCC_FirstPersonScanWidget::RebuildWidget(){
 if(WidgetTree&&!WidgetTree->RootWidget){
  auto* Root=WidgetTree->ConstructWidget<UCanvasPanel>();Root->SetVisibility(ESlateVisibility::HitTestInvisible);WidgetTree->RootWidget=Root;
  Card=WidgetTree->ConstructWidget<UBorder>();Card->SetPadding(FMargin(0));Card->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0,0,0,.76f),0.f,FLinearColor(.65f,.48f,.24f,.60f),1.f));
  auto* Layout=Root->AddChildToCanvas(Card);Layout->SetAnchors(FAnchors(.5f,.5f));Layout->SetAlignment({.5f,0});Layout->SetPosition({0,36});Layout->SetSize({440,128});
  auto* Inner=WidgetTree->ConstructWidget<UCanvasPanel>();Card->SetContent(Inner);
  auto Text=[this,Inner](int32 Size,FLinearColor Color,FVector2D P,FVector2D S){auto* T=WidgetTree->ConstructWidget<UTextBlock>();auto Font= CCGameFont::Kai(T->GetFont());Font.Size=Size;T->SetFont(Font);T->SetColorAndOpacity(Color);auto* L=Inner->AddChildToCanvas(T);L->SetPosition(P);L->SetSize(S);return T;};
  Title=Text(20,FLinearColor(.87f,.75f,.51f),{16,10},{408,30});
  Body=Text(15,FLinearColor(.83f,.81f,.74f),{16,46},{408,44});Body->SetAutoWrapText(true);Body->SetClipping(EWidgetClipping::ClipToBounds);
  Hint=Text(13,FLinearColor(.69f,.64f,.53f),{16,100},{408,22});Card->SetVisibility(ESlateVisibility::Collapsed);
  HeavenGuide=WidgetTree->ConstructWidget<UBorder>();
  HeavenGuide->SetPadding(FMargin(13,7));
  HeavenGuide->SetBrush(FSlateRoundedBoxBrush(FLinearColor(.02f,.025f,.023f,.85f),4.f));
  HeavenGuideText=WidgetTree->ConstructWidget<UTextBlock>();
  auto GuideFont=CCGameFont::Kai(HeavenGuideText->GetFont());
  GuideFont.Size=15;
  HeavenGuideText->SetFont(GuideFont);
  HeavenGuideText->SetColorAndOpacity(FLinearColor(.9f,.83f,.63f));
  HeavenGuide->SetContent(HeavenGuideText);
  auto* GuideSlot=Root->AddChildToCanvas(HeavenGuide);
  GuideSlot->SetAnchors(FAnchors(.5f,0.f));
  GuideSlot->SetAlignment({.5f,0.f});
  GuideSlot->SetPosition({0.f,25.f});
  GuideSlot->SetAutoSize(true);
  HeavenGuide->SetVisibility(ESlateVisibility::Collapsed);
 }
 return Super::RebuildWidget();
}
void UCC_FirstPersonScanWidget::NativeTick(const FGeometry& G,float Delta){
 Super::NativeTick(G,Delta);DisplayedTarget.Reset();bInteractionReady=false;
 auto* PC=Cast<ACC_PlayerController>(GetOwningPlayer());auto* Scan=PC?PC->Scanner.Get():nullptr;auto* Character=PC?Cast<ACC_PlayerCharacter>(PC->GetPawn()):nullptr;
 if(!Card)return;
 const auto* Heaven=PC?Cast<AWFCMapGenerator>(PC->GetHeavenSceneTarget()):nullptr;
 const bool bMaze=Heaven&&Heaven->DisplayedSettings.LayoutPreset==EWFCLayoutPreset::Maze;
 const bool bGuideVisible=Heaven&&Character&&Character->IsAlive()&&!PC->IsGameplayInputBlocked();
 HeavenGuide->SetVisibility(bGuideVisible?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
 if(bGuideVisible)
 {
  const FString InteractionKey=PC->GetCombatKey(5).GetDisplayName().ToString();
  const FString Guide = !bMaze
   ? FString::Printf(TEXT("万象天宫 · 都市    [%s] 重建都市    [L] 进入迷宫"), *InteractionKey)
   : PC->IsHeavenCityUnlocked()
    ? FString::Printf(TEXT("都市建造已解锁    [%s] 重排迷宫    [L] 建造都市"), *InteractionKey)
    : FString::Printf(TEXT("触碰黄色区块解锁都市建造    [%s] 重排迷宫"), *InteractionKey);
  HeavenGuideText->SetText(FText::FromString(Guide));
 }
 auto* InteractionTarget=PC?PC->FindInteractionTarget():nullptr;
 auto* NearbyShop=Cast<ACC_TownShop>(InteractionTarget);
 auto* NearbyDoor=Cast<ACC_InteractiveDoubleDoor>(InteractionTarget);
 auto* NearbyBook=Cast<ACC_LoreStand>(InteractionTarget);
 const bool bCanObserve = PC && Character && Character->IsAlive() && !PC->IsGameplayInputBlocked();
 if(!bCanObserve || !Character->IsFirstPerson()) ObservationTarget.Reset();
 if(!bCanObserve||(!Character->IsFirstPerson()&&!NearbyShop&&!NearbyDoor&&!NearbyBook)){Card->SetVisibility(ESlateVisibility::Collapsed);return;}
 const bool First=Character->IsFirstPerson();AActor* Target=First&&Scan?Scan->GetTarget():nullptr;
 if(NearbyShop)Target=NearbyShop;
 if(NearbyDoor)Target=NearbyDoor;
 if(NearbyBook)Target=NearbyBook;
 if(!NearbyShop&&!NearbyBook){if(auto* Scene=PC->GetHeavenSceneTarget())Target=Scene;else if(!Target)Target=PC->FindInteractionTarget();}
 auto* Info=Target&&Scan?Scan->ResolveInfoForActor(Target):nullptr;
 if(!IsValid(Target)){ObservationTarget.Reset();Card->SetVisibility(ESlateVisibility::Collapsed);return;}
 if(First)
 {
  const double Now = GetWorld()->GetTimeSeconds();
  const int32 Revision = Target == Heaven ? Heaven->RuntimeRevision : 0;
  if(ObservationTarget.Get() != Target || ObservationRevision != Revision)
  {
   ObservationTarget = Target;
   ObservationRevision = Revision;
   ObservationEndsAt = Now + 2.;
  }
  if(Now >= ObservationEndsAt){Card->SetVisibility(ESlateVisibility::Collapsed);return;}
 }
 DisplayedTarget=Target;
 // Only first-person observation mode displays scan or interaction information.
 bInteractionReady=NearbyBook?NearbyBook->CanRead(Character):NearbyShop?NearbyShop->CanInteract(Character):PC->GetHeavenSceneTarget()==Target?Cast<AWFCMapGenerator>(Target)->CanRegenerateRandomScene():First?(Scan&&Scan->GetTarget()==Target?Scan->CanInteractWithTarget():PC->FindInteractionTarget()==Target):true;
 Card->SetVisibility(ESlateVisibility::HitTestInvisible);
 auto* Layout=Cast<UCanvasPanelSlot>(Card->Slot);Layout->SetAnchors(First?FAnchors(.5f,.5f):FAnchors(.5f,1));Layout->SetAlignment({.5f,First?0.f:1.f});Layout->SetPosition({0,First?36.f:-40.f});Layout->SetSize({440,First&&Info?128.f:76.f});
 FText Name=Info?Info->DisplayName:FText::FromString(TEXT("可交互物体"));FText Action=Info?Info->ActionText:FText();
 if(!Info){
  if(auto* Door=Cast<ACC_InteractiveDoubleDoor>(Target)){Name=FText::FromString(TEXT("双开门"));Action=FText::FromString(Door->bOpen?TEXT("关门"):TEXT("开门"));}
  else if(auto* Shop=Cast<ACC_TownShop>(Target)){Name=Shop->GetShopName();Action=FText::FromString(TEXT("进店交易"));}
  else if(Cast<AWFCMapGenerator>(Target)){Name=FText::FromString(TEXT("万象天宫"));Action=FText::FromString(TEXT("随机重组整个场景"));}
  else if(Cast<ACC_LevitationPlatform>(Target)){Name=FText::FromString(TEXT("浮空石台"));Action=FText::FromString(TEXT("重组浮台"));}
  else if(Cast<ACC_ExperiencePack>(Target)){Name=FText::FromString(TEXT("经验灵物"));Action=FText::FromString(TEXT("汲取灵光"));}
  else if(auto* Artifact=Target->FindComponentByClass<UCC_ArtifactComponent>()){Name=Artifact->Form?Artifact->Form->DisplayName:FText::FromString(TEXT("物魄信物"));Action=FText::FromString(TEXT("唤醒信物"));}
 }
 if(NearbyBook)
 {
  Name=NearbyBook->Book->Title;
  Action=FText::FromString(TEXT("翻阅"));
 }
 if(bMaze&&Target==Heaven)
 {
  Name=FText::FromString(TEXT("万象天工 · 方块迷宫"));
  Action=FText::FromString(TEXT("重排迷宫"));
 }
 Title->SetText(Name);Body->SetVisibility(First&&Info?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);if(Info)Body->SetText(bMaze&&Target==Heaven?FText::FromString(TEXT("从入口庭院出发，踏上金色地砖即可抵达出口。")):Info->Description);
 Cast<UCanvasPanelSlot>(Hint->Slot)->SetPosition({16,First&&Info?100.f:44.f});
 const FBox Bounds=Target->GetComponentsBoundingBox(false);const FVector Point=Bounds.IsValid?Bounds.GetClosestPointTo(Character->GetActorLocation()):Target->GetActorLocation();
 const float Distance=First&&Scan&&Scan->GetTarget()==Target?Scan->GetTargetDistance():FVector::Distance(Point,Character->GetActorLocation());
 FString Value=Cast<AWFCMapGenerator>(Target)?TEXT("全场景"):FString::Printf(TEXT("%.1f 米"),Distance/100.f);
 if(bInteractionReady)Value+=FString::Printf(TEXT("  ·  [%s] %s"),*PC->GetCombatKey(5).GetDisplayName().ToString(),Action.IsEmpty()?TEXT("交互"):*Action.ToString());
 else if(Cast<AWFCMapGenerator>(Target))Value+=TEXT("  ·  重组准备中");
 else if(Scan&&Scan->HasInteraction())Value+=TEXT("  ·  靠近后可交互");
 Hint->SetText(FText::FromString(Value));
}
int32 UCC_FirstPersonScanWidget::NativePaint(const FPaintArgs& Args,const FGeometry& G,const FSlateRect& Cull,FSlateWindowElementList& Elements,int32 Layer,const FWidgetStyle& Style,bool bParentEnabled)const{
 const int32 Top=Super::NativePaint(Args,G,Cull,Elements,Layer,Style,bParentEnabled)+1;
 const auto* PC=Cast<ACC_PlayerController>(GetOwningPlayer());const auto* Scan=PC?PC->Scanner.Get():nullptr;if(!Scan||!Scan->IsScanActive())return Top;
 const FVector2D Center=G.GetLocalSize()*.5f;FLinearColor Color=(Scan->GetInfo()||bInteractionReady)?FLinearColor(.91f,.69f,.30f,.95f):FLinearColor(.9f,.88f,.79f,.85f);Color*=Style.GetColorAndOpacityTint();
 auto Line=[&](FVector2D A,FVector2D B){TArray<FVector2D> Points{Center+A,Center+B};FSlateDrawElement::MakeLines(Elements,Top,G.ToPaintGeometry(),Points,ESlateDrawEffect::None,Color,true,1.25f);};
 Line({-10,0},{-5,0});Line({5,0},{10,0});Line({0,-10},{0,-5});Line({0,5},{0,10});Line({-.6f,0},{.6f,0});
 return Top;
}

AActor* UCC_FirstPersonScanWidget::GetDisplayedTarget()const{
 const auto* PC=Cast<ACC_PlayerController>(GetOwningPlayer());const auto* C=PC?Cast<ACC_PlayerCharacter>(PC->GetPawn()):nullptr;
 return C&&(C->IsFirstPerson()||Cast<ACC_TownShop>(DisplayedTarget.Get())||Cast<ACC_InteractiveDoubleDoor>(DisplayedTarget.Get())||Cast<ACC_LoreStand>(DisplayedTarget.Get()))&&C->IsAlive()&&!PC->IsGameplayInputBlocked()?DisplayedTarget.Get():nullptr;
}
bool UCC_FirstPersonScanWidget::IsInteractionReady()const{return GetDisplayedTarget()!=nullptr&&bInteractionReady;}
