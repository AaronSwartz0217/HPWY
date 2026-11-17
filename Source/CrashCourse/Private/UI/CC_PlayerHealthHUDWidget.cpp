#include "UI/CC_PlayerHealthHUDWidget.h"
#include "UI/CC_GameFont.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Progression/CC_SkillComponent.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Player/CC_PlayerController.h"
#include "Player/CC_PlayerState.h"
#include "UI/CC_SkillGalleryData.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Rendering/DrawElements.h"
#include "Characters/CC_BaseCharacter.h"
#include "UI/CC_WidgetComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "World/CC_AdventureGameInstance.h"

namespace
{
 const FLinearColor PanelBackground(.018f,.018f,.018f,.56f);
 const FLinearColor PanelOutline(.52f,.52f,.52f,.32f);
 const FLinearColor PrimaryText(.94f,.94f,.94f);
 const FLinearColor SecondaryText(.75f,.75f,.75f);
 const FLinearColor HealthFill(.82f,.82f,.82f);
}

void UCC_PlayerHealthHUDWidget::NativeConstruct()
{
 Super::NativeConstruct();
 BindProgression();
 Refresh();
}
TSharedRef<SWidget> UCC_PlayerHealthHUDWidget::RebuildWidget()
{
 if(WidgetTree&&!WidgetTree->RootWidget)
 {
  auto* Root=WidgetTree->ConstructWidget<UCanvasPanel>();WidgetTree->RootWidget=Root;
  Root->SetVisibility(ESlateVisibility::HitTestInvisible);
  auto* Panel=WidgetTree->ConstructWidget<UBorder>();Panel->SetBrush(FSlateRoundedBoxBrush(PanelBackground,0.f,PanelOutline,1.f));Panel->SetPadding(FMargin(0));
  auto* Anchor=Root->AddChildToCanvas(Panel);Anchor->SetAnchors(FAnchors(1,1));Anchor->SetAlignment({1,1});Anchor->SetPosition({-40,-40});Anchor->SetSize({316,54});
  auto* Inner=WidgetTree->ConstructWidget<UCanvasPanel>();Panel->SetContent(Inner);
  auto Place=[Inner](UWidget* W,FVector2D P,FVector2D S){auto* Slot=Inner->AddChildToCanvas(W);Slot->SetPosition(P);Slot->SetSize(S);return Slot;};
  auto* Label=WidgetTree->ConstructWidget<UTextBlock>();Label->SetText(FText::FromString(TEXT("HP")));auto Font= CCGameFont::Kai(Label->GetFont());Font.Size=16;Label->SetFont(Font);Label->SetColorAndOpacity(SecondaryText);Place(Label,{16,6},{46,24});
  ValueText=WidgetTree->ConstructWidget<UTextBlock>();Font.Size=18;ValueText->SetFont(Font);ValueText->SetColorAndOpacity(PrimaryText);ValueText->SetJustification(ETextJustify::Right);Place(ValueText,{184,5},{116,26});
  HealthBar=WidgetTree->ConstructWidget<UProgressBar>();HealthBar->SetFillColorAndOpacity(HealthFill);HealthBar->SetIsMarquee(false);
  FProgressBarStyle Style;Style.BackgroundImage.TintColor=FSlateColor(FLinearColor(.08f,.08f,.08f,1));Style.FillImage.TintColor=FSlateColor(FLinearColor::White);HealthBar->SetWidgetStyle(Style);
  HealthBar->SetBorderPadding(FVector2D::ZeroVector);Place(HealthBar,{16,36},{284,6});
  FormText=WidgetTree->ConstructWidget<UTextBlock>();
  Font.Size=13;
  FormText->SetFont(Font);
  FormText->SetJustification(ETextJustify::Center);
  FormText->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
  FormText->SetColorAndOpacity(SecondaryText);
  Place(FormText,{62,7},{118,22});
  RefreshProgression();
  BuildSkillPanel(Root);
  SetExpanded(false);
  Refresh();
 }
 return Super::RebuildWidget();
}
void UCC_PlayerHealthHUDWidget::Unbind()
{
 SetExpanded(false);SelectedSlot=0;
 if(BoundProgression.IsValid())BoundProgression->OnChanged.RemoveDynamic(this,&ThisClass::RefreshProgression);
 BoundProgression.Reset();
 if(BoundASC.IsValid())
 {
  BoundASC->GetGameplayAttributeValueChangeDelegate(UCC_AttributeSet::GetHealthAttribute()).Remove(HealthHandle);
  BoundASC->GetGameplayAttributeValueChangeDelegate(UCC_AttributeSet::GetMaxHealthAttribute()).Remove(MaxHandle);
 }
 if(BoundCharacter.IsValid()&&BoundCharacter->HealthDisplay)BoundCharacter->HealthDisplay->SetVisibility(bPreviousWorldLabelVisibility);
 BoundASC.Reset();BoundCharacter.Reset();HealthHandle.Reset();MaxHandle.Reset();
}
void UCC_PlayerHealthHUDWidget::Refresh()
{
 Health=BoundASC.IsValid()?FMath::Max(0.f,BoundASC->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute())):0;
 Maximum=BoundASC.IsValid()?FMath::Max(0.f,BoundASC->GetNumericAttribute(UCC_AttributeSet::GetMaxHealthAttribute())):0;
 if(ValueText)ValueText->SetText(FText::FromString(Maximum>0?FString::Printf(TEXT("%.0f / %.0f"),Health,Maximum):TEXT("— / —")));
 if(HealthBar){HealthBar->SetPercent(Maximum>0?FMath::Clamp(Health/Maximum,0.f,1.f):0);HealthBar->SetFillColorAndOpacity(bRainbowHealth?FLinearColor::White:(Maximum>0&&Health/Maximum<=.25f?FLinearColor(.72f,.24f,.18f):HealthFill));}
}
void UCC_PlayerHealthHUDWidget::NativeTick(const FGeometry& G,float Dt)
{
 Super::NativeTick(G,Dt);
 BindProgression();
 if(bExpanded)
 {
  const auto* PC=Cast<ACC_PlayerController>(GetOwningPlayer());
  if(PC&&PC->IsGameplayInputBlocked())SetExpanded(false);
  else if((SkillRefreshElapsed+=Dt)>=.05f){SkillRefreshElapsed=0;RefreshCommandPage();}
 }
 RefreshItemCooldown();
 auto* Character=GetOwningPlayer()?Cast<ACC_BaseCharacter>(GetOwningPlayer()->GetPawn()):nullptr;
 auto* ASC=Character?Character->GetAbilitySystemComponent():nullptr;
 if(Character==BoundCharacter.Get()&&ASC==BoundASC.Get()){if(Character&&Character->HealthDisplay&&Character->HealthDisplay->IsVisible())Character->HealthDisplay->SetVisibility(false);return;}
 Unbind();BoundCharacter=Character;BoundASC=ASC;BindProgression();
 if(Character&&Character->HealthDisplay){bPreviousWorldLabelVisibility=Character->HealthDisplay->IsVisible();Character->HealthDisplay->SetVisibility(false);}
 if(ASC)
 {
  HealthHandle=ASC->GetGameplayAttributeValueChangeDelegate(UCC_AttributeSet::GetHealthAttribute()).AddWeakLambda(this,[this](const FOnAttributeChangeData&){Refresh();});
  MaxHandle=ASC->GetGameplayAttributeValueChangeDelegate(UCC_AttributeSet::GetMaxHealthAttribute()).AddWeakLambda(this,[this](const FOnAttributeChangeData&){Refresh();});
 }
 Refresh();
}
void UCC_PlayerHealthHUDWidget::NativeDestruct(){Unbind();Super::NativeDestruct();}


namespace
{
 constexpr float CommandWidth = 288.f;
 const FVector2D CommandFrameAnchor(40.f,-76.f);
 const FVector2D CommandFramePadding(8.f,6.f);
 constexpr float CommandFrameHeight = 212.f;
 constexpr float SelectedRowOffset = -6.f;
 constexpr float CommandHeight = 38.f;
 constexpr float CommandStep = 42.f;
 constexpr int32 VisibleRows = 4;
 constexpr float ListTop = 32.f;
 constexpr float ListHeight = (VisibleRows-1)*CommandStep+CommandHeight;
 const TCHAR* const CommandCategories[] = {TEXT("技能"),TEXT("道具"),TEXT("变身"),TEXT("传送")};

 UCanvasPanelSlot* PlaceCommandWidget(UCanvasPanel* Canvas, UWidget* Widget, FVector2D Position, FVector2D Size)
 {
  auto* Slot = Canvas->AddChildToCanvas(Widget);
  Slot->SetPosition(Position);
  Slot->SetSize(Size);
  return Slot;
 }
}

UCC_PlayerHealthHUDWidget::FCommandRow UCC_PlayerHealthHUDWidget::BuildCommandRow()
{
 FCommandRow Row;
 Row.Frame = WidgetTree->ConstructWidget<UBorder>();
 Row.Frame->SetPadding(FMargin(0));
 auto* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
 Row.Frame->SetContent(Canvas);

 Row.Icon = WidgetTree->ConstructWidget<UImage>();
 Row.Icon->SetVisibility(ESlateVisibility::Hidden);
 PlaceCommandWidget(Canvas, Row.Icon, {16,6}, {26,26});

 Row.Name = WidgetTree->ConstructWidget<UTextBlock>();
 auto Font = CCGameFont::Kai(Row.Name->GetFont());
 Font.Size = 16;
 Row.Name->SetFont(Font);
 Row.Name->SetColorAndOpacity(PrimaryText);
 Row.Name->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
 PlaceCommandWidget(Canvas, Row.Name, {50,8}, {158,22});

 Row.State = WidgetTree->ConstructWidget<UTextBlock>();
 Font.Size = 12;
 Row.State->SetFont(Font);
 Row.State->SetColorAndOpacity(SecondaryText);
 Row.State->SetJustification(ETextJustify::Right);
 PlaceCommandWidget(Canvas, Row.State, {210,10}, {56,18});

 Row.Cooldown = WidgetTree->ConstructWidget<UProgressBar>();
 FProgressBarStyle Style;
 Style.FillImage=FSlateColorBrush(FLinearColor(0.f,0.f,0.f,.20f));
 Style.BackgroundImage.DrawAs=ESlateBrushDrawType::NoDrawType;
 Row.Cooldown->SetWidgetStyle(Style);
 Row.Cooldown->SetFillColorAndOpacity(FLinearColor::White);
 Row.Cooldown->SetBorderPadding(FVector2D::ZeroVector);
 Row.Cooldown->SetBarFillType(EProgressBarFillType::RightToLeft);
 PlaceCommandWidget(Canvas, Row.Cooldown, {1,1}, {CommandWidth-14,CommandHeight-2});
 return Row;
}

void UCC_PlayerHealthHUDWidget::StyleCommandRow(UBorder* Frame, bool bSelected)
{
 const FLinearColor Background = bSelected ? FLinearColor(.30f,.30f,.30f,.58f) : FLinearColor(.018f,.018f,.018f,.10f);
 const FLinearColor Outline = bSelected ? FLinearColor(.83f,.83f,.83f,.85f) : PanelOutline;
 Frame->SetBrush(FSlateRoundedBoxBrush(Background, 0.f, Outline, 1.f));
 Frame->SetRenderTranslation({bSelected?SelectedRowOffset:0.f,0.f});
}

void UCC_PlayerHealthHUDWidget::BuildSkillPanel(UCanvasPanel* Root)
{
 Gallery = LoadObject<UCC_SkillGalleryData>(nullptr, TEXT("/Game/UI/MuralMenu/DA_SkillGallery.DA_SkillGallery"));
 auto* Panel = WidgetTree->ConstructWidget<UCanvasPanel>();
 SkillPanel = Panel;
 auto* Frame = WidgetTree->ConstructWidget<UBorder>();
 Frame->SetBrush(FSlateRoundedBoxBrush(PanelBackground, 0.f, PanelOutline, 1.f));
 Frame->SetPadding(FMargin(CommandFramePadding.X,CommandFramePadding.Y));
 Frame->SetContent(Panel);
 auto* Anchor = PlaceCommandWidget(Root, Frame, CommandFrameAnchor, {CommandWidth+2*CommandFramePadding.X,CommandFrameHeight});
 Anchor->SetAnchors(FAnchors(0,1));
 Anchor->SetAlignment({0,1});

 auto* Title = WidgetTree->ConstructWidget<UTextBlock>();
 CommandTitle = Title;
 auto Font = CCGameFont::Kai(Title->GetFont());
 Font.Size = 16;
 Title->SetFont(Font);
 Title->SetColorAndOpacity(PrimaryText);
 Title->SetText(FText::FromString(TEXT("对策")));
 auto* Header=WidgetTree->ConstructWidget<UBorder>();
 Header->SetBrush(FSlateRoundedBoxBrush(FLinearColor(.15f,.15f,.15f,.40f),0.f,PanelOutline,1.f));
 Header->SetPadding(FMargin(10,3));
 Header->SetContent(Title);
 PlaceCommandWidget(Panel, Header, {0,0}, {CommandWidth,28});

 ShownDefinitions.SetNum(4);
 Cooldowns.Init(0.f,4);
 for (int32 Index = 0; Index < VisibleRows; ++Index)
 {
  const auto Row = BuildCommandRow();
  PlaceCommandWidget(Panel, Row.Frame, {0.f,ListTop + Index * CommandStep}, {CommandWidth-12,CommandHeight});
  Rows.Add(Row.Frame);
  Icons.Add(Row.Icon);
  Names.Add(Row.Name);
  States.Add(Row.State);
  CooldownBars.Add(Row.Cooldown);
 }
 ScrollTrack=WidgetTree->ConstructWidget<UBorder>();
 ScrollTrack->SetBrush(FSlateRoundedBoxBrush(FLinearColor(.45f,.45f,.45f,.26f),1.f));
 PlaceCommandWidget(Panel,ScrollTrack,{CommandWidth-6,ListTop},{3,ListHeight});
 ScrollThumb=WidgetTree->ConstructWidget<UBorder>();
 ScrollThumb->SetBrush(FSlateRoundedBoxBrush(FLinearColor(.78f,.78f,.78f,.82f),1.f));
 PlaceCommandWidget(Panel,ScrollThumb,{CommandWidth-6,ListTop},{3,ListHeight});

 CommandBadge=WidgetTree->ConstructWidget<UBorder>();
 CommandBadge->SetBrush(FSlateRoundedBoxBrush(PanelBackground,0.f,PanelOutline,1.f));
 CommandBadge->SetPadding(FMargin(12,0));
 CommandBadge->SetVerticalAlignment(VAlign_Center);
 auto* BadgeText=WidgetTree->ConstructWidget<UTextBlock>();
 BadgeText->SetText(FText::FromString(TEXT("对策")));
 auto BadgeFont= CCGameFont::Kai(BadgeText->GetFont());
 BadgeFont.Size=13;
 BadgeText->SetFont(BadgeFont);
 BadgeText->SetColorAndOpacity(PrimaryText);
 CommandBadge->SetContent(BadgeText);
 auto* BadgeSlot=PlaceCommandWidget(Root, CommandBadge, {40,-40}, {228,28});
 BadgeSlot->SetAnchors(FAnchors(0,1));BadgeSlot->SetAlignment({0,1});

 DescriptionFrame=WidgetTree->ConstructWidget<UBorder>();
 DescriptionFrame->SetBrush(FSlateRoundedBoxBrush(PanelBackground,0.f,PanelOutline,1.f));
 DescriptionFrame->SetPadding(FMargin(10,5));
 DescriptionText=WidgetTree->ConstructWidget<UTextBlock>();
 Font.Size=12;
 DescriptionText->SetFont(Font);
 DescriptionText->SetAutoWrapText(false);
 DescriptionText->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
 DescriptionFrame->SetClipping(EWidgetClipping::ClipToBounds);
 DescriptionFrame->SetContent(DescriptionText);
 auto* DescriptionSlot=PlaceCommandWidget(Root, DescriptionFrame, {40,-40}, {CommandWidth+16,26});
 DescriptionSlot->SetAnchors(FAnchors(0,1));DescriptionSlot->SetAlignment({0,1});

}

void UCC_PlayerHealthHUDWidget::SetExpanded(bool bOpen)
{
 if(!bOpen){if(HeldSkills.IsValid())HeldSkills->CancelPendingInput();ReleaseSelectedSkill();}
 bExpanded=bOpen;
 if(SkillPanel)SkillPanel->GetParent()->SetVisibility(bOpen?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
 CommandPage=ECommandPage::Main;
 CategoryIndex=SelectedSlot=ScrollOffset=0;
 CommandFeedback=FText::GetEmpty();
 if(CommandBadge)CommandBadge->SetVisibility(bOpen?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
 if(DescriptionFrame)DescriptionFrame->SetVisibility(bOpen?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
 if(bOpen)RefreshCommandPage();
}
void UCC_PlayerHealthHUDWidget::ToggleCommandMenu()
{
 if(!bExpanded || CommandPage==ECommandPage::Main)
 {
  SetExpanded(!bExpanded);
  return;
 }
 if(HeldSkills.IsValid())HeldSkills->CancelPendingInput();
 ReleaseSelectedSkill();
 CommandPage=ECommandPage::Main;
 SelectedSlot=CategoryIndex;
 ScrollOffset=0;
 CommandFeedback=FText::GetEmpty();
 RefreshCommandPage();
}

void UCC_PlayerHealthHUDWidget::CycleSkill(int32 Direction)
{
 const int32 Count=GetCommandCount();
 if(!bExpanded || Direction==0 || Count==0)return;
 if(HeldSkills.IsValid())HeldSkills->CancelPendingInput();
 ReleaseSelectedSkill();
 const int32 Step=Direction>0?1:-1;
 SelectedSlot=(FMath::Clamp(SelectedSlot,0,Count-1)+Step+Count)%Count;
 CommandFeedback=FText::GetEmpty();
 RefreshCommandPage();
}

bool UCC_PlayerHealthHUDWidget::PressSelectedSkill(FKey Source)
{
 if(!bExpanded || HeldSlot!=INDEX_NONE)return false;
 auto* PC=Cast<ACC_PlayerController>(GetOwningPlayer());
 auto* Character=PC?Cast<ACC_BaseCharacter>(PC->GetPawn()):nullptr;
 if(!Character || !Character->IsAlive() || PC->IsGameplayInputBlocked())return false;
 if(CommandPage!=ECommandPage::Skills)return ActivateCommandChoice();
 if(!Character->Skills)return false;
 CommandFeedback=FText::GetEmpty();
 if(!Character->Skills->ActivateSlot(SelectedSlot))
 {
  RefreshSkills();
  if(States.IsValidIndex(SelectedSlot))
  {
   const FText Reason=States[SelectedSlot]->GetText();
   CommandFeedback=Reason.IsEmpty() || Reason.EqualTo(FText::FromString(TEXT("就绪")))
    ? FText::FromString(TEXT("暂时无法施放")) : Reason;
   SetCommandDescription(CommandFeedback);
  }
  return false;
 }
 HeldSkills=Character->Skills;
 HeldSlot=SelectedSlot;
 HeldInputKey=Source;
 RefreshSkills();
 return true;
}
void UCC_PlayerHealthHUDWidget::ReleaseSelectedSkill(FKey Source)
{
 if(Source.IsValid()&&HeldInputKey.IsValid()&&Source!=HeldInputKey)return;
 if(HeldSkills.IsValid()&&HeldSlot!=INDEX_NONE)HeldSkills->ReleaseSlot(HeldSlot);
 HeldSkills.Reset();HeldSlot=INDEX_NONE;HeldInputKey=FKey();
}
void UCC_PlayerHealthHUDWidget::RefreshSkills()
{
 auto* PC=Cast<ACC_PlayerController>(GetOwningPlayer());auto* C=PC?Cast<ACC_BaseCharacter>(PC->GetPawn()):nullptr;
 auto* Skills=C?C->Skills.Get():nullptr;auto* ASC=C?C->GetAbilitySystemComponent():nullptr;
 auto* PS=PC?PC->GetPlayerState<ACC_PlayerState>():nullptr;
 for(int32 I=0;I<Rows.Num();++I)
 {
  auto* Skill=Skills&&Skills->CurrentSlots.IsValidIndex(I)?Skills->CurrentSlots[I].Skill.Get():nullptr;
  const bool Selected=I==SelectedSlot;
  StyleCommandRow(Rows[I], Selected);
  Names[I]->SetText(FText::FromString(FString::Printf(TEXT("[%s] %s"),PC?*PC->GetCombatKey(I).GetDisplayName().ToString():TEXT("?"),Skill?*Skill->DisplayName.ToString():TEXT("空槽"))));
  float CooldownDuration=0.f;
  Cooldowns[I]=Skills?Skills->GetCooldownState(I,CooldownDuration):0.f;
  FString State=TEXT("未装备");
  if(Skill)
  {
   State=TEXT("就绪");
   if(PS&&PS->Progression&&!PS->Progression->IsSkillUnlocked(Skill))State=TEXT("尚未解锁");
   else if(Skills->GetQueuedSkillSlot()==I)State=TEXT("已预输入");
   else if(Cooldowns[I]>0)State=TEXT("冷却中");
   else if(!ASC||ASC->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute())<=0)State=TEXT("无法施放");
   else if(ASC->GetNumericAttribute(UCC_AttributeSet::GetManaAttribute())<Skill->ManaCost)State=TEXT("法力不足");
   else if(Skills->IsStunned()&&Skill->DesignMechanic!=ECC_DesignMechanic::Invulnerable)State=TEXT("受控中");
   else if(Skill->Behavior==ECC_SkillBehavior::Dash && Skills->IsRooted())State=TEXT("定身中");
   else if(Skill->DesignMechanic==ECC_DesignMechanic::Charge && Skills->IsSlotHeld(I))State=TEXT("蓄力中");
   else if(Skills->HasBlockingSkillAction())State=TEXT("动作衔接中");
  }
  States[I]->SetText(FText::FromString(State));
  CooldownBars[I]->SetPercent(CooldownDuration>0.f?FMath::Clamp(Cooldowns[I]/CooldownDuration,0.f,1.f):0.f);
  if(ShownDefinitions[I]!=Skill)
  {
   ShownDefinitions[I]=Skill;UTexture2D* Texture=nullptr;
   if(Skill&&Gallery)for(const auto& Entry:Gallery->Entries)if(Entry.ExecutableSkill==Skill){Texture=Entry.Icon.LoadSynchronous();break;}
   Icons[I]->SetBrushFromTexture(Texture);Icons[I]->SetVisibility(Texture?ESlateVisibility::HitTestInvisible:ESlateVisibility::Hidden);
  }
  Icons[I]->SetVisibility(Icons[I]->GetBrush().GetResourceObject()?ESlateVisibility::HitTestInvisible:ESlateVisibility::Hidden);
 }
}


int32 UCC_PlayerHealthHUDWidget::NativePaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& Cull,FSlateWindowElementList& Elements,int32 Layer,const FWidgetStyle& Style,bool bParentEnabled)const
{
 const int32 Top = Super::NativePaint(Args,Geometry,Cull,Elements,Layer,Style,bParentEnabled) + 1;
 if(!bExpanded)return Top;
 const FVector2D FirstRow=FVector2D(0.f,Geometry.GetLocalSize().Y-CommandFrameHeight)+CommandFrameAnchor+CommandFramePadding+FVector2D(0.f,ListTop);
 auto RowOrigin=[&](int32 Index)
 {
  return FirstRow+FVector2D(Index+ScrollOffset==SelectedSlot?SelectedRowOffset:0.f,Index*CommandStep);
 };
 auto DrawCorner = [&](int32 Index, bool bSelected)
 {
  const FVector2D Origin=RowOrigin(Index);
  const FVector2D Corner[] = {{3,14},{3,3},{14,3},{14,7},{7,7},{7,11}};
  TArray<FVector2D> Points;
  for (const FVector2D& Point : Corner)Points.Add(Origin+Point);
  const FLinearColor Accent=bSelected?FLinearColor(.90f,.90f,.90f,.88f):FLinearColor(.64f,.64f,.64f,.44f);
  FSlateDrawElement::MakeLines(Elements,Top,Geometry.ToPaintGeometry(),Points,ESlateDrawEffect::None,Accent*Style.GetColorAndOpacityTint(),true,1.f);
 };
 if(bExpanded)
 {
  for(int32 Index=0;Index<Rows.Num();++Index)
  {
   if(Rows[Index]->GetVisibility()!=ESlateVisibility::Collapsed)
    DrawCorner(Index, Index+ScrollOffset==SelectedSlot);
  }
 }
 if(bExpanded && CommandPage==ECommandPage::Forms)
 {
  const auto Forms=GetFormChoices();
  for(int32 Index=0;Index<Rows.Num();++Index)
  {
   const int32 Entry=Index+ScrollOffset;
   if(!Forms.IsValidIndex(Entry))break;
   const FVector2D IconOrigin=RowOrigin(Index)+FVector2D(18,8);

   const FName Id=Forms[Entry]->FormId;
   const bool bBronze=Id==TEXT("Artifact_01");
   const bool bClay=Id==TEXT("Artifact_02");
   const bool bNature=Id==TEXT("Native");
   const bool bInk=Id==TEXT("InitialInk");
   if(!bBronze && !bClay && !bNature && !bInk)continue;

   const FLinearColor Seal=bBronze?FLinearColor(.29f,.56f,.32f):bClay?FLinearColor(.54f,.30f,.19f):FLinearColor(.55f,.57f,.56f);
   const FLinearColor Mark=bBronze?FLinearColor(.96f,.73f,.30f):bClay?FLinearColor(.92f,.54f,.50f):bNature?FLinearColor(.51f,.80f,.38f):FLinearColor(.025f,.025f,.025f);
   auto Line=[&](std::initializer_list<FVector2D> Path,FLinearColor Color,float Thickness=1.25f)
   {
    TArray<FVector2D> Points;
    for(const auto& Point:Path)Points.Add(IconOrigin+Point);
    FSlateDrawElement::MakeLines(Elements,Top,Geometry.ToPaintGeometry(),Points,ESlateDrawEffect::None,Color*Style.GetColorAndOpacityTint(),true,Thickness);
   };
   Line({{2,8},{2,2},{20,2},{20,13}},Seal);
   Line({{20,16},{20,20},{2,20},{2,11}},Seal);
   if(bBronze)
   {
    Line({{5,9},{17,9},{15,14},{7,14},{5,9},{6,6},{8,6},{8,9}},Mark);
    Line({{14,9},{14,6},{16,6},{17,9}},Mark);
    Line({{8,14},{8,17},{7,17}},Mark);
    Line({{14,14},{14,17},{15,17}},Mark);
   }
   else if(bClay)
   {
    Line({{8,6},{14,6},{14,8},{17,11},{17,14},{14,17},{8,17},{5,14},{5,11},{8,8},{8,6}},Mark);
    Line({{7,12},{10,11},{12,13},{15,12}},Mark);
   }
   else if(bNature)
   {
    Line({{6,16},{5,12},{8,8},{13,6},{17,5},{16,10},{14,14},{10,16},{6,16}},Mark);
    Line({{5,18},{13,10},{13,7}},Mark);
    Line({{9,14},{13,14}},Seal);
   }
   else
   {
    Line({{11,5},{7,10},{6,13},{8,16},{11,17},{14,16},{16,13},{15,10},{11,5}},Seal,2.2f);
    Line({{11,5},{7,10},{6,13},{8,16},{11,17},{14,16},{16,13},{15,10},{11,5}},Mark,1.2f);
   }
  }
 }
 return Top;
}

void UCC_PlayerHealthHUDWidget::BindProgression()
{
 auto* PC=GetOwningPlayer();auto* PS=PC?PC->GetPlayerState<ACC_PlayerState>():nullptr;
 auto* Progression=PS?PS->Progression.Get():nullptr;
 if(Progression==BoundProgression.Get())return;
 if(BoundProgression.IsValid())BoundProgression->OnChanged.RemoveDynamic(this,&ThisClass::RefreshProgression);
 BoundProgression=Progression;
 if(Progression)Progression->OnChanged.AddUniqueDynamic(this,&ThisClass::RefreshProgression);
 RefreshProgression();
}
void UCC_PlayerHealthHUDWidget::RefreshProgression()
{
 RefreshItemCooldown();
 auto* P=BoundProgression.Get();DisplayedExperience=P?FMath::Max(0,P->Experience):0;DisplayedCoins=P?FMath::Max(0,P->CopperCoins):0;
 const bool bWantRainbow=P && P->HasRainbowHealth();
 if(HealthBar && bWantRainbow!=bRainbowHealth)
 {
  if(bWantRainbow && !RainbowHealthMaterial) RainbowHealthMaterial=RainbowHealthMaterialAsset.LoadSynchronous();
  bRainbowHealth=bWantRainbow && RainbowHealthMaterial;
  FProgressBarStyle HealthStyle=HealthBar->GetWidgetStyle();
  HealthStyle.FillImage.SetResourceObject(bRainbowHealth?RainbowHealthMaterial.Get():nullptr);
  HealthStyle.FillImage.DrawAs=ESlateBrushDrawType::Image;
  HealthStyle.FillImage.TintColor=FSlateColor(FLinearColor::White);
  HealthBar->SetWidgetStyle(HealthStyle);
  Refresh();
 }
 ExperienceGoal=P?FMath::Max(0,P->GetEvolutionCost()):0;
 if(FormText)
 {
  const auto* Form=P?P->GetActiveForm():nullptr;
  FormText->SetText(Form?Form->DisplayName:FText::GetEmpty());
 }
}

TArray<UCC_FormDefinition*> UCC_PlayerHealthHUDWidget::GetFormChoices() const
{
 TArray<UCC_FormDefinition*> Choices;
 if(const auto* Progression=BoundProgression.Get())
 {
  if(Progression->NativeForm)Choices.Add(Progression->NativeForm);
  for(const auto& Entry:Progression->Forms)
  {
   if(Entry.Form && Entry.Form->bAvailable)Choices.AddUnique(Entry.Form);
  }
 }
 return Choices;
}

int32 UCC_PlayerHealthHUDWidget::GetCommandCount() const
{
 switch(CommandPage)
 {
 case ECommandPage::Items: return GetItemChoices().Num();
 case ECommandPage::Forms: return GetFormChoices().Num();
 case ECommandPage::Travel:
  if(const auto* Adventure=GetGameInstance<UCC_AdventureGameInstance>())return Adventure->Regions.Num();
  return 0;
 default: return VisibleRows;
 }
}

void UCC_PlayerHealthHUDWidget::SetCommandDescription(const FText& Detail)
{
 FString Summary=Detail.ToString();
 int32 Break=INDEX_NONE;
 if(Summary.FindChar(TEXT('。'),Break))Summary.LeftInline(Break+1);
 Summary.ReplaceInline(TEXT("\r"),TEXT(" "));
 Summary.ReplaceInline(TEXT("\n"),TEXT(" "));
 DescriptionText->SetText(FText::FromString(Summary));
}

void UCC_PlayerHealthHUDWidget::RefreshCommandPage()
{
 if(!DescriptionText || Rows.Num()!=VisibleRows)return;
 const bool bMain=CommandPage==ECommandPage::Main;
 const bool bSkills=CommandPage==ECommandPage::Skills;
 const bool bItems=CommandPage==ECommandPage::Items;
 const bool bForms=CommandPage==ECommandPage::Forms;
 const auto* Adventure=GetGameInstance<UCC_AdventureGameInstance>();
 const auto* Progression=BoundProgression.Get();
 const auto Items=bItems?GetItemChoices():TArray<UCC_ConsumableDefinition*>();
 const auto Forms=bForms?GetFormChoices():TArray<UCC_FormDefinition*>();
 const int32 Count=bItems?Items.Num():bForms?Forms.Num():GetCommandCount();
 SelectedSlot=FMath::Clamp(SelectedSlot,0,FMath::Max(0,Count-1));
 const int32 MaxOffset=FMath::Max(0,Count-VisibleRows);
 ScrollOffset=FMath::Clamp(ScrollOffset,0,MaxOffset);
 if(SelectedSlot<ScrollOffset)ScrollOffset=SelectedSlot;
 if(SelectedSlot>=ScrollOffset+VisibleRows)ScrollOffset=SelectedSlot-VisibleRows+1;

 const auto Visible=ESlateVisibility::HitTestInvisible;
 const auto Hidden=ESlateVisibility::Collapsed;
 ScrollTrack->SetVisibility(MaxOffset>0?Visible:Hidden);
 ScrollThumb->SetVisibility(MaxOffset>0?Visible:Hidden);
 if(MaxOffset>0)
 {
  const float ThumbHeight=FMath::Max(12.f,ListHeight*VisibleRows/Count);
  auto* ThumbLayout=CastChecked<UCanvasPanelSlot>(ScrollThumb->Slot);
  ThumbLayout->SetPosition({CommandWidth-6,ListTop+(ListHeight-ThumbHeight)*ScrollOffset/MaxOffset});
  ThumbLayout->SetSize({3,ThumbHeight});
 }
 CommandTitle->SetText(FText::FromString(bMain?TEXT("对策"):CommandCategories[static_cast<int32>(CommandPage)-1]));
 FText Detail=FText::FromString(TEXT("请选择对策"));
 if(bSkills)
 {
  for(const auto& Frame:Rows)Frame->SetVisibility(Visible);
  RefreshSkills();
  const auto* Skill=ShownDefinitions[SelectedSlot].Get();
  Detail=Skill?Skill->Description:FText::FromString(TEXT("未装备技能"));
 }
 else
 {
  for(int32 Index=0;Index<VisibleRows;++Index)
  {
   const int32 Entry=ScrollOffset+Index;
   Rows[Index]->SetVisibility(Visible);
   StyleCommandRow(Rows[Index],Entry<Count && Entry==SelectedSlot);
   Names[Index]->SetText(FText::GetEmpty());
   States[Index]->SetText(FText::GetEmpty());
   Icons[Index]->SetVisibility(Hidden);
   ShownDefinitions[Index]=nullptr;
   CooldownBars[Index]->SetPercent(0.f);
   if(Entry>=Count)continue;
   if(bMain)Names[Index]->SetText(FText::FromString(CommandCategories[Entry]));
   else if(bItems)
   {
    const auto* Item=Items[Entry];
    Names[Index]->SetText(Item->DisplayName);
    States[Index]->SetText(FText::FromString(FString::Printf(TEXT("×%d"),Progression->GetItemCount(Item))));
    if(auto* Material=Item->IconMaterial.LoadSynchronous())
    {
     if(Icons[Index]->GetBrush().GetResourceObject()!=Material)Icons[Index]->SetBrushFromMaterial(Material);
     Icons[Index]->SetVisibility(Visible);
    }
    else if(auto* Texture=Item->Icon.LoadSynchronous())
    {
     if(Icons[Index]->GetBrush().GetResourceObject()!=Texture)Icons[Index]->SetBrushFromTexture(Texture);
     Icons[Index]->SetVisibility(Visible);
    }
    CooldownBars[Index]->SetPercent(FMath::Clamp(Progression->GetItemCooldownRemaining()/FMath::Max(15.f,Progression->ItemCooldownSeconds),0.f,1.f));
    if(Entry==SelectedSlot)Detail=Item->Description;
   }
   else if(bForms)
   {
    Names[Index]->SetText(Forms[Entry]->DisplayName);
    if(Progression->GetActiveForm()==Forms[Entry])States[Index]->SetText(FText::FromString(TEXT("当前")));
    Detail=FText::FromString(TEXT("切换到选中的形态"));
   }
   else
   {
    const auto& Region=Adventure->Regions[Entry];
    Names[Index]->SetText(Region.DisplayName);
    if(!Adventure->IsRegionUnlocked(Region.Id))States[Index]->SetText(FText::FromString(TEXT("未解锁")));
    else if(const auto* Scroll=Progression?Progression->FindTravelScroll(Region.Id):nullptr)
     States[Index]->SetText(FText::FromString(FString::Printf(TEXT("×%d"),Progression->GetItemCount(Scroll))));
    CooldownBars[Index]->SetPercent(FMath::Clamp(Adventure->GetTravelCooldownRemaining()/30.f,0.f,1.f));
    Detail=FText::FromString(TEXT("释放传送球，离开范围会取消"));
   }
  }
  if(Count==0)Detail=FText::FromString(bItems?TEXT("没有可用道具"):TEXT("没有可用选项"));
 }
 SetCommandDescription(CommandFeedback.IsEmpty()?Detail:CommandFeedback);
}

bool UCC_PlayerHealthHUDWidget::ActivateCommandChoice()
{
 if(CommandPage==ECommandPage::Main)
 {
  CategoryIndex=SelectedSlot;
  CommandPage=static_cast<ECommandPage>(CategoryIndex+1);
  if(CommandPage==ECommandPage::Skills)
  {
   for(auto& Icon:Icons)Icon->SetBrushFromTexture(nullptr);
  }
  SelectedSlot=ScrollOffset=0;
  RefreshCommandPage();
  return true;
 }
 const int32 Choice=SelectedSlot;
 auto* Progression=BoundProgression.Get();
 if(CommandPage==ECommandPage::Items)
 {
  const auto Items=GetItemChoices();
  if(!Items.IsValidIndex(Choice) || Progression->GetItemCooldownRemaining()>0.f)return false;
  Progression->UseItem(Items[Choice]);
  RefreshCommandPage();
  return true;
 }
 if(CommandPage==ECommandPage::Forms)
 {
  const auto Forms=GetFormChoices();
  if(!Forms.IsValidIndex(Choice))return false;
  auto* Form=Forms[Choice];
  Progression->SwitchForm(Form==Progression->NativeForm?nullptr:Form);
  SetExpanded(false);
  return true;
 }
 auto* Adventure=GetGameInstance<UCC_AdventureGameInstance>();
 if(!Adventure || !Adventure->Regions.IsValidIndex(Choice))return false;
 if(!Adventure->RequestFastTravel(Adventure->Regions[Choice].Id))
 {
  CommandFeedback=Adventure->LastTravelError;
  SetCommandDescription(CommandFeedback);
  return false;
 }
 SetExpanded(false);
 return true;
}
