#include "UI/CC_MuralMenuWidget.h"
#include "CC_MuralChoiceRow.h"
#include "UI/CC_GameFont.h"
#include "UI/CC_MenuArtData.h"
#include "UI/CC_SkillGalleryWidget.h"
#include "UI/CC_OptionsWidget.h"
#include "UI/CC_PowerTokenWidget.h"
#include "CC_MenuStyle.h"
#include "Player/CC_PlayerController.h"
#include "Player/CC_PlayerState.h"
#include "Characters/CC_BaseCharacter.h"
#include "Progression/CC_SkillComponent.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/UniformGridPanel.h"
#include "Engine/Texture2D.h"
#include "Brushes/SlateColorBrush.h"

namespace {
const FLinearColor Gold(.78f,.68f,.48f,1.f), Ivory(.9f,.87f,.79f,1.f), Muted(.57f,.57f,.57f,1.f);
void Place(UCanvasPanel* Canvas,UWidget* Widget,float X,float Y,float W,float H) {
 auto* LayoutSlot=Canvas->AddChildToCanvas(Widget); LayoutSlot->SetPosition(FVector2D(X,Y)); LayoutSlot->SetSize(FVector2D(W,H));
}
FButtonStyle ActionStyle(bool Selected=false) {
 FButtonStyle S;
 S.SetNormal(FSlateColorBrush(Selected ? FLinearColor(.035f,.035f,.035f,.90f) : FLinearColor(0,0,0,.76f)));
 S.SetHovered(FSlateColorBrush(FLinearColor(.055f,.055f,.055f,.90f)));
 S.SetPressed(FSlateColorBrush(FLinearColor(.025f,.025f,.025f,.96f)));
 S.SetDisabled(FSlateColorBrush(FLinearColor(0,0,0,.48f)));
 S.SetNormalPadding(FMargin(12,9)); S.SetPressedPadding(FMargin(12,10,12,8));
 return S;
}
FSlateBrush ImageBrush(UTexture2D* Texture) {
 FSlateBrush B; B.DrawAs=ESlateBrushDrawType::Image; B.SetResourceObject(Texture);
 if (Texture) B.ImageSize=FVector2D(Texture->GetSizeX(),Texture->GetSizeY());
 return B;
}
FString UpgradeLabel(const UCC_UpgradeDefinition* U) {
 if (!U) return TEXT("未配置");
 if (U->DisplayName.ToString()!=U->UpgradeId.ToString()) return U->DisplayName.ToString();
 const TCHAR* Labels[]={TEXT("生命上限"),TEXT("法力上限"),TEXT("攻击力"),TEXT("防御力"),TEXT("移动速度"),TEXT("技能等级")};
 return Labels[FMath::Clamp(int32(U->Type),0,5)];
}
}
UTextBlock* UCC_MuralMenuWidget::AddText(UVerticalBox* Box,const FString& Value,int32 Size) {
 auto* T=WidgetTree->ConstructWidget<UTextBlock>(); auto Font= CCGameFont::Kai(T->GetFont()); Font.Size=Size; T->SetFont(Font);
 T->SetColorAndOpacity(Ivory); T->SetAutoWrapText(true); T->SetText(FText::FromString(Value));
 if (Box) { auto* LayoutSlot=Box->AddChildToVerticalBox(T); LayoutSlot->SetPadding(FMargin(0,0,0,14)); }
 return T;
}
UButton* UCC_MuralMenuWidget::AddAction(UVerticalBox* Box,const FString& Value) {
 auto* B=WidgetTree->ConstructWidget<UButton>(); B->SetStyle(ActionStyle());
 auto* T=AddText(nullptr,Value,20); T->SetAutoWrapText(false); T->SetJustification(ETextJustify::Center); T->SetColorAndOpacity(Gold); B->AddChild(T);
 auto* LayoutSlot=Box->AddChildToVerticalBox(B); LayoutSlot->SetPadding(FMargin(0,0,0,10));
 return B;
}
UComboBoxString* UCC_MuralMenuWidget::AddChoice(UVerticalBox* Box) {
 auto* C=WidgetTree->ConstructWidget<UComboBoxString>();
 FComboBoxStyle Style=C->GetWidgetStyle(); Style.ComboButtonStyle.ButtonStyle=ActionStyle();
 C->SetWidgetStyle(Style); C->SetMaxListHeight(420);
 auto* LayoutSlot=Box->AddChildToVerticalBox(C); LayoutSlot->SetPadding(FMargin(0,0,0,14));
 C->OnGenerateWidgetEvent.BindDynamic(this,&ThisClass::GenerateChoice);
 C->OnSelectionChanged.AddDynamic(this,&ThisClass::ChoiceChanged); return C;
}
UWidget* UCC_MuralMenuWidget::GenerateChoice(FString Item)
{
    auto* Row = CreateWidget<UCC_MuralChoiceRow>(this);
    if (Row)
        Row->SetContent(Item);
    return Row;
}
void UCC_MuralMenuWidget::NativeOnInitialized() {
 Super::NativeOnInitialized(); SetIsFocusable(true);
 auto* Root=WidgetTree->ConstructWidget<UCanvasPanel>(); WidgetTree->RootWidget=Root;
 auto* Background=WidgetTree->ConstructWidget<UImage>(); if (ArtData) Background->SetBrushFromTexture(ArtData->Background.LoadSynchronous());
 auto* BGSlot=Root->AddChildToCanvas(Background); BGSlot->SetAnchors(FAnchors(0,0,1,1)); BGSlot->SetOffsets(FMargin(0));
 Background->SetVisibility(ESlateVisibility::HitTestInvisible);
 auto* Scale=WidgetTree->ConstructWidget<UScaleBox>(); Scale->SetStretch(EStretch::ScaleToFit);
 auto* ScaleSlot=Root->AddChildToCanvas(Scale); ScaleSlot->SetAnchors(FAnchors(0,0,1,1)); ScaleSlot->SetOffsets(FMargin(0));
 auto* Size=WidgetTree->ConstructWidget<USizeBox>(); Size->SetWidthOverride(1920); Size->SetHeightOverride(1080); Scale->SetContent(Size);
 auto* Canvas=WidgetTree->ConstructWidget<UCanvasPanel>(); Size->SetContent(Canvas);
 auto AddArt=[this,Canvas](UTexture2D* Texture,float X,float Y,float W,float H) {
  auto* Fit=WidgetTree->ConstructWidget<UScaleBox>(); Fit->SetStretch(EStretch::ScaleToFit);
  auto* I=WidgetTree->ConstructWidget<UImage>(); I->SetBrushFromTexture(Texture,true); Fit->SetContent(I);
  Fit->SetVisibility(ESlateVisibility::HitTestInvisible); Place(Canvas,Fit,X,Y,W,H); return I;
 };
 LogoImage=AddArt(ArtData ? ArtData->Logo.LoadSynchronous() : nullptr,54,54,410,168);
 PortraitImage=AddArt(nullptr,1100,100,640,930);
 TitleImage=AddArt(nullptr,1730,70,142,430);
 ShapeCaption=AddText(nullptr,TEXT(""),21); ShapeCaption->SetColorAndOpacity(Gold); ShapeCaption->SetJustification(ETextJustify::Center);
 Place(Canvas,ShapeCaption,1120,1030,600,35);
 const TCHAR* NavLabels[]={TEXT("背包"),TEXT("技能"),TEXT("交易"),TEXT("设置")};
 for (int32 I=0;I<4;++I) {
  auto* B=WidgetTree->ConstructWidget<UButton>(); B->SetStyle(ActionStyle()); NavigationButtons.Add(B);
  if (!ArtData || !ArtData->Navigation.IsValidIndex(I)) B->AddChild(AddText(nullptr,NavLabels[I],32));
  Place(Canvas,B,58,292+I*135,350,109);
 }
 NavigationButtons[0]->OnClicked.AddDynamic(this,&ThisClass::Inventory); NavigationButtons[1]->OnClicked.AddDynamic(this,&ThisClass::SkillsPage);
 NavigationButtons[2]->OnClicked.AddDynamic(this,&ThisClass::Trade); NavigationButtons[3]->OnClicked.AddDynamic(this,&ThisClass::Settings);
 PageTitle=AddText(nullptr,TEXT("行囊"),38); PageTitle->SetColorAndOpacity(Gold); Place(Canvas,PageTitle,446,208,550,60);
 PageSubtitle=AddText(nullptr,TEXT(""),18); PageSubtitle->SetColorAndOpacity(Muted); Place(Canvas,PageSubtitle,448,274,760,55);
 auto* Panel=WidgetTree->ConstructWidget<UBorder>(); ContentPanel=Panel; Panel->SetBrushColor(FLinearColor(0,0,0,.78f)); Panel->SetPadding(FMargin(26)); Place(Canvas,Panel,426,310,1004,720);
 auto* Content=WidgetTree->ConstructWidget<UVerticalBox>(); Panel->SetContent(Content);
 Feedback=AddText(Content,TEXT(""),17); Feedback->SetColorAndOpacity(Gold);
 Scroll=WidgetTree->ConstructWidget<UScrollBox>(); Scroll->SetConsumeMouseWheel(EConsumeMouseWheel::Always);
 Content->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
 auto* Body=WidgetTree->ConstructWidget<UVerticalBox>(); Scroll->AddChild(Body);
 for (int32 I=0;I<4;++I) { auto* Box=WidgetTree->ConstructWidget<UVerticalBox>(); Body->AddChild(Box); PageBoxes.Add(Box); }
 // Inventory: amounts and current combat values are rendered from authoritative data.
 WalletText=AddText(PageBoxes[0],TEXT("经验  0"),26); WalletText->SetColorAndOpacity(Gold);
 RemoteShopShortcut=AddAction(PageBoxes[0],TEXT("远程商店"));RemoteShopShortcut->OnClicked.AddDynamic(this,&ThisClass::OpenRemoteShop);
 RemoteShopHint=AddText(PageBoxes[0],TEXT("在小镇归灯客栈购买远程商店通行契。"),17);
 AddText(PageBoxes[0],TEXT("力 量 信 物"),25)->SetColorAndOpacity(Gold);
 auto* BagColumns=WidgetTree->ConstructWidget<UHorizontalBox>();PageBoxes[0]->AddChildToVerticalBox(BagColumns)->SetPadding(FMargin(0,0,0,14));
 auto* GridSize=WidgetTree->ConstructWidget<USizeBox>();GridSize->SetWidthOverride(466);BagColumns->AddChildToHorizontalBox(GridSize)->SetPadding(FMargin(0,0,18,0));
 BagGrid=WidgetTree->ConstructWidget<UUniformGridPanel>();BagGrid->SetSlotPadding(FMargin(5));GridSize->SetContent(BagGrid);
 auto BagCard=CCMenuStyle::Frame(WidgetTree,ArtData,FMargin(24));BagColumns->AddChildToHorizontalBox(BagCard.Root)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
 auto* BagActions=WidgetTree->ConstructWidget<UVerticalBox>();BagCard.Body->SetContent(BagActions);
 BagSelectionTitle=AddText(BagActions,TEXT("选择一件信物"),25);BagSelectionTitle->SetColorAndOpacity(Gold);
 BagUseButton=AddAction(BagActions,TEXT("使用 · 转换形态")); BagUseButton->OnClicked.AddDynamic(this,&ThisClass::UseBagPower);
 BagEvolveButton=AddAction(BagActions,TEXT("进化")); BagEvolveButton->OnClicked.AddDynamic(this,&ThisClass::EvolveBagPower);
 BagDetails=AddText(BagActions,TEXT("点击左侧信物，查看能力与可用操作。"),17);
 AddAction(PageBoxes[0],TEXT("购买永久属性加成"))->OnClicked.AddDynamic(this,&ThisClass::Trade);
 // Skill equipment: four independent slots plus the existing catalog.
 for (int32 I=0;I<4;++I) {
  auto* B=WidgetTree->ConstructWidget<UButton>(); B->SetStyle(ActionStyle());
  auto* T=AddText(nullptr,TEXT(""),18); T->SetAutoWrapText(false); B->AddChild(T); SlotButtons.Add(B); SlotTexts.Add(T);
  auto* S=PageBoxes[1]->AddChildToVerticalBox(B); S->SetPadding(FMargin(0,0,0,7));
 }
 SlotButtons[0]->OnClicked.AddDynamic(this,&ThisClass::SlotOne); SlotButtons[1]->OnClicked.AddDynamic(this,&ThisClass::SlotTwo);
 SlotButtons[2]->OnClicked.AddDynamic(this,&ThisClass::SlotThree); SlotButtons[3]->OnClicked.AddDynamic(this,&ThisClass::SlotFour);
 SkillChoice=AddChoice(PageBoxes[1]);
 EquipButton=AddAction(PageBoxes[1],TEXT("装入所选技能槽")); EquipButton->OnClicked.AddDynamic(this,&ThisClass::Equip);
 ResetSkillsButton=AddAction(PageBoxes[1],TEXT("恢复当前形态技能组")); ResetSkillsButton->OnClicked.AddDynamic(this,&ThisClass::ResetSkills);
 // Shop combines evolution with permanent attribute purchases under the supplied trade artwork.
 BuildItemShop();
 GrowthShopSection=WidgetTree->ConstructWidget<UVerticalBox>();PageBoxes[2]->AddChildToVerticalBox(GrowthShopSection);
 AddText(GrowthShopSection,TEXT("力量进修 · 消耗经验"),23)->SetColorAndOpacity(Gold);
 RouteChoice=AddChoice(GrowthShopSection);
 SwitchButton=AddAction(GrowthShopSection,TEXT("切换至所选形态")); SwitchButton->OnClicked.AddDynamic(this,&ThisClass::SwitchRoute);
 EvolveButton=AddAction(GrowthShopSection,TEXT("购买当前形态下一档")); EvolveButton->OnClicked.AddDynamic(this,&ThisClass::Evolve);
 ShopUpgradeHeading=AddText(PageBoxes[2],TEXT("永久锻炼 · 消耗经验"),23);ShopUpgradeHeading->SetColorAndOpacity(Gold);
 UpgradeChoice=AddChoice(PageBoxes[2]); PurchaseButton=AddAction(PageBoxes[2],TEXT("购买所选加成")); PurchaseButton->OnClicked.AddDynamic(this,&ThisClass::Purchase);
 // A dedicated panel keeps six live key selectors, map controls and save actions together.
 OptionsPanel=CreateWidget<UCC_OptionsWidget>(GetOwningPlayer(),UCC_OptionsWidget::StaticClass());
 if (OptionsPanel) {OptionsPanel->ArtData=ArtData;PageBoxes[3]->AddChildToVerticalBox(OptionsPanel);}
 Details=AddText(Body,TEXT(""),20); Details->SetLineHeightPercentage(1.25f);
 auto* BackBox=WidgetTree->ConstructWidget<UVerticalBox>(); Place(Canvas,BackBox,986,212,250,58);
 OverviewButton=AddAction(BackBox,TEXT("返回主菜单")); OverviewButton->OnClicked.AddDynamic(this,&ThisClass::Overview);
 if (ArtData && ArtData->SkillGallery) {
  SkillGalleryWidget=CreateWidget<UCC_SkillGalleryWidget>(GetOwningPlayer(),UCC_SkillGalleryWidget::StaticClass());
  SkillGalleryWidget->InitializeGallery(ArtData->SkillGallery); Place(Canvas,SkillGalleryWidget,0,0,1920,1080);
 }
 ShowPage(ECC_MuralMenuPage::Overview);
}
void UCC_MuralMenuWidget::NativeConstruct() { Super::NativeConstruct(); BindSources(); RefreshData(); }
void UCC_MuralMenuWidget::ReleaseSources() {
 if(Progression)Progression->OnItemActionFinished.RemoveDynamic(this,&ThisClass::ItemResult);
 if (Progression) { Progression->OnChanged.RemoveDynamic(this,&ThisClass::SourceChanged); Progression->OnPurchaseFinished.RemoveDynamic(this,&ThisClass::PurchaseResult); Progression->OnSkillConfigurationFinished.RemoveDynamic(this,&ThisClass::SkillResult); }
 if (Skills) Skills->OnLoadoutChanged.RemoveDynamic(this,&ThisClass::SourceChanged);
 if (auto* PC=Cast<ACC_PlayerController>(GetOwningPlayer())) PC->OnCombatKeysChanged.RemoveDynamic(this,&ThisClass::SourceChanged);
 Progression=nullptr; Skills=nullptr; bListsInitialized=false;
}
void UCC_MuralMenuWidget::BindSources() {
 auto* PS=GetOwningPlayerState<ACC_PlayerState>(); auto* C=Cast<ACC_BaseCharacter>(GetOwningPlayerPawn());
 if (Progression==(PS ? PS->Progression.Get() : nullptr) && Skills==(C ? C->Skills.Get() : nullptr)) return;
 ReleaseSources(); Progression=PS ? PS->Progression.Get() : nullptr; Skills=C ? C->Skills.Get() : nullptr;
 if(Progression)Progression->OnItemActionFinished.AddUniqueDynamic(this,&ThisClass::ItemResult);
 if (Progression) { Progression->OnChanged.AddUniqueDynamic(this,&ThisClass::SourceChanged); Progression->OnPurchaseFinished.AddUniqueDynamic(this,&ThisClass::PurchaseResult); Progression->OnSkillConfigurationFinished.AddUniqueDynamic(this,&ThisClass::SkillResult); }
 if (Skills) Skills->OnLoadoutChanged.AddUniqueDynamic(this,&ThisClass::SourceChanged);
 if (auto* PC=Cast<ACC_PlayerController>(GetOwningPlayer())) PC->OnCombatKeysChanged.AddUniqueDynamic(this,&ThisClass::SourceChanged);
 PopulateLists();
}
void UCC_MuralMenuWidget::NativeDestruct() { ReleaseSources(); Super::NativeDestruct(); }
void UCC_MuralMenuWidget::NativeTick(const FGeometry& G,float D) { Super::NativeTick(G,D); RefreshElapsed+=D; if (RefreshElapsed>=.1f) { RefreshElapsed=0; BindSources(); RefreshData(); } }
FReply UCC_MuralMenuWidget::NativeOnPreviewKeyDown(const FGeometry& G,const FKeyEvent& E) {
 if (OptionsPanel && OptionsPanel->IsSelectingKey()) return Super::NativeOnPreviewKeyDown(G,E);
 if (E.GetKey()==EKeys::Tab) { if(!E.IsRepeat())Close(); return FReply::Handled(); }
 return Super::NativeOnPreviewKeyDown(G,E);
}
void UCC_MuralMenuWidget::PopulateLists() {
 if (!Progression || !SkillChoice || bListsInitialized) return;
 bListsInitialized=true; SkillChoice->ClearOptions(); RouteChoice->ClearOptions(); UpgradeChoice->ClearOptions(); RouteOptions.Empty();
 int32 Index=0;
 for (const auto& S:Progression->SkillCatalog) SkillChoice->AddOption(FString::Printf(TEXT("%02d  %s"),++Index,S ? *S->DisplayName.ToString() : TEXT("未配置")));
 auto AddRoute=[this](UCC_FormDefinition* F) { if (!F || !F->bAvailable || RouteOptions.Contains(F)) return; RouteOptions.Add(F); RouteChoice->AddOption(F->DisplayName.ToString()); };
 AddRoute(Progression->NativeForm); for (const auto& F:Progression->ArtifactForms) AddRoute(F);
 SkillChoice->SetSelectedIndex(0); UpgradeChoice->SetSelectedIndex(0); RefreshBag();
 PopulateItems();
 const int32 Active=RouteOptions.IndexOfByKey(Progression->GetActiveForm()); RouteChoice->SetSelectedIndex(Active>=0 ? Active : 0);
}
void UCC_MuralMenuWidget::RefreshNavigation() {
 for (int32 I=0;I<NavigationButtons.Num();++I) {
  if (!ArtData || !ArtData->Navigation.IsValidIndex(I)) { NavigationButtons[I]->SetStyle(ActionStyle(I==int32(Page))); continue; }
  auto& A=ArtData->Navigation[I]; auto* Normal=A.Normal.LoadSynchronous(); auto* Active=A.Active.LoadSynchronous();
  FButtonStyle S; S.SetNormal(ImageBrush(I==int32(Page) ? Active : Normal)); S.SetHovered(ImageBrush(Active)); S.SetPressed(ImageBrush(Active));
  S.SetNormalPadding(FMargin(0)); S.SetPressedPadding(FMargin(0)); NavigationButtons[I]->SetStyle(S);
 }
}
void UCC_MuralMenuWidget::ShowPage(ECC_MuralMenuPage InPage) {
 Page=InPage; if (Scroll) Scroll->ScrollToStart();
 const bool GalleryPage=Page==ECC_MuralMenuPage::Skills && SkillGalleryWidget;
 if (SkillGalleryWidget) SkillGalleryWidget->SetVisibility(GalleryPage ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
 for (UWidget* ArtWidget : {static_cast<UWidget*>(PortraitImage.Get()),static_cast<UWidget*>(TitleImage.Get()),static_cast<UWidget*>(ShapeCaption.Get())}) if (ArtWidget) ArtWidget->SetVisibility(GalleryPage ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
 if (LogoImage && LogoImage->GetParent()) if (auto* L=Cast<UCanvasPanelSlot>(LogoImage->GetParent()->Slot)) { L->SetPosition(GalleryPage ? FVector2D(54,16) : FVector2D(54,54)); L->SetSize(GalleryPage ? FVector2D(300,122) : FVector2D(410,168)); }
 for (int32 I=0;I<NavigationButtons.Num();++I) if (auto* N=Cast<UCanvasPanelSlot>(NavigationButtons[I]->Slot)) { N->SetPosition(GalleryPage ? FVector2D(54,200+I*140) : FVector2D(58,292+I*135)); N->SetSize(GalleryPage ? FVector2D(390,120) : FVector2D(350,109)); }
 const bool Full=Page!=ECC_MuralMenuPage::Overview && !GalleryPage;
 for (UWidget* Widget : {static_cast<UWidget*>(ContentPanel.Get()),static_cast<UWidget*>(PageTitle.Get()),static_cast<UWidget*>(PageSubtitle.Get()),static_cast<UWidget*>(OverviewButton.Get())}) if (Widget) Widget->SetVisibility(Full ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
 if (PortraitImage && PortraitImage->GetParent()) if (auto* S=Cast<UCanvasPanelSlot>(PortraitImage->GetParent()->Slot)) { S->SetPosition(Full ? FVector2D(1450,280) : FVector2D(1100,100)); S->SetSize(Full ? FVector2D(360,700) : FVector2D(640,930)); }
 if (ShapeCaption) if (auto* S=Cast<UCanvasPanelSlot>(ShapeCaption->Slot)) { S->SetPosition(Full ? FVector2D(1430,1002) : FVector2D(1120,1030)); S->SetSize(Full ? FVector2D(400,62) : FVector2D(600,35)); }
 const TCHAR* Titles[]={TEXT("行囊"),TEXT("技能"),TEXT("交易"),TEXT("设置")};
 const TCHAR* Subtitles[]={TEXT("力量信物 · 随身道具"),TEXT("四式随身 · 术随形变"),TEXT("铜钱买药 · 经验进化与强化"),TEXT("键位 · 舆图 · 角色存档")};
 if (PageTitle) PageTitle->SetText(FText::FromString(Titles[FMath::Clamp(int32(Page),0,3)]));
 if (PageSubtitle) PageSubtitle->SetText(FText::FromString(Subtitles[FMath::Clamp(int32(Page),0,3)]));
 for (int32 I=0;I<PageBoxes.Num();++I) PageBoxes[I]->SetVisibility(I==int32(Page) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
 if (Feedback) Feedback->SetText(FText::GetEmpty()); RefreshNavigation(); RefreshData();
}
void UCC_MuralMenuWidget::RefreshArtwork() {
 if (!Progression || !PortraitImage || !TitleImage) return;
 const auto* Form=Progression->GetActiveForm(); if (!Form) return;
 const int32 Tier=FMath::Clamp(Progression->CurrentTier,0,2);
 if (DisplayedFormId!=Form->FormId || DisplayedTier!=Tier) {
  DisplayedFormId=Form->FormId; DisplayedTier=Tier;
  const FCC_FormStage* Stage=Form->Stages.IsValidIndex(Tier) ? &Form->Stages[Tier] : nullptr;
  DisplayedPortrait=Stage && !Stage->MenuPortrait.IsNull() ? Stage->MenuPortrait.LoadSynchronous() : ArtData ? ArtData->FallbackPortrait.LoadSynchronous() : nullptr;
  DisplayedTitle=Stage && !Stage->MenuTitle.IsNull() ? Stage->MenuTitle.LoadSynchronous() : ArtData ? ArtData->FallbackTitle.LoadSynchronous() : nullptr;
  PortraitImage->SetBrushFromTexture(DisplayedPortrait,true); TitleImage->SetBrushFromTexture(DisplayedTitle,true);
  const TCHAR* Tiers[]={TEXT("初级"),TEXT("中级"),TEXT("高级")};
  const FString Name=Stage && !Stage->MenuDisplayName.IsEmpty() ? Stage->MenuDisplayName.ToString() : Form->DisplayName.ToString();
  ShapeCaption->SetText(FText::FromString(Name+TEXT("  /  ")+Form->DisplayName.ToString()+(Form->bCanEvolve ? FString(TEXT(" · "))+Tiers[Tier] : TEXT(" · 初始形态"))));
 }
}
UCC_FormDefinition* UCC_MuralMenuWidget::GetSelectedRoute() const {
 return RouteChoice && RouteOptions.IsValidIndex(RouteChoice->GetSelectedIndex()) ? RouteOptions[RouteChoice->GetSelectedIndex()].Get() : nullptr;
}
UCC_SkillDefinition* UCC_MuralMenuWidget::GetSelectedSkill() const {
 const int32 I=SkillChoice ? SkillChoice->GetSelectedIndex() : INDEX_NONE;
 return Progression && Progression->SkillCatalog.IsValidIndex(I) ? Progression->SkillCatalog[I].Get() : nullptr;
}
void UCC_MuralMenuWidget::RefreshData() {
 if(bUpdatingItems)return;
 RefreshItems();
 if(SkillGalleryWidget) SkillGalleryWidget->RefreshFromPlayer();
 if (!Details) return;
 if (!Progression) { Details->SetText(FText::FromString(TEXT("正在读取角色状态……"))); return; }
 RefreshArtwork(); RefreshBag();
 auto* C=Cast<ACC_BaseCharacter>(GetOwningPlayerPawn()); const auto* AS=C ? Cast<UCC_AttributeSet>(C->GetAttributeSet()) : nullptr;
 auto* PC=Cast<ACC_PlayerController>(GetOwningPlayer());
 WalletText->SetText(FText::FromString(FString::Printf(TEXT("经验   %d    铜币   %d"),Progression->Experience,Progression->CopperCoins)));
 FString Text;
 const auto* Form=Progression->GetActiveForm();
 if (Page==ECC_MuralMenuPage::Overview) { Details->SetText(FText::GetEmpty()); return; }
 if (Page==ECC_MuralMenuPage::Inventory) {
  auto* Selected=SelectedBagForm.Get();
  const bool Alive=C && C->IsAlive();
  BagUseButton->SetIsEnabled(Alive && Selected && Selected!=Form);
  CastChecked<UTextBlock>(BagUseButton->GetContent())->SetText(FText::FromString(Selected==Form ? TEXT("正在使用") : TEXT("使用 · 转换形态")));
  const int32 EvolutionCost=Progression->GetFormEvolutionCost(Selected);
  BagEvolveButton->SetIsEnabled(Alive && Selected && EvolutionCost>=0 && Progression->Experience>=EvolutionCost);
  CastChecked<UTextBlock>(BagEvolveButton->GetContent())->SetText(FText::FromString(EvolutionCost>=0 ? FString::Printf(TEXT("进化 · %d 经验"),EvolutionCost) : Selected && Selected->bCanEvolve ? TEXT("已达最高档") : TEXT("随经验自然强化")));
  BagSelectionTitle->SetText(Selected ? Selected->GetInventoryName() : FText::FromString(TEXT("选择一件信物")));
  const TCHAR* Tiers[]={TEXT("初级"),TEXT("中级"),TEXT("高级")};
  const int32 OwnedTier=FMath::Clamp(Progression->GetOwnedFormTier(Selected),0,2);
  if (Selected) {
   Text=Selected==Form ? TEXT("正在使用此信物的力量\n\n") : TEXT("已获得的永久信物\n\n");
   if (Selected->bCanEvolve) {
    Text+=FString::Printf(TEXT("已解锁档位：%s\n"),Tiers[OwnedTier]);
    FCC_FormStage Current; if (Progression->GetStagePreview(Selected,OwnedTier,Current)) Text+=FString::Printf(TEXT("当前档基础生命 %.0f    攻击 %.2f\n"),Current.MaxHealth,Current.Attack);
    FCC_FormStage Next;
    if (OwnedTier<2 && Progression->GetStagePreview(Selected,OwnedTier+1,Next)) {
     Text+=FString::Printf(TEXT("\n进化至%s：需要 %d 经验\n下档基础生命 %.0f    攻击 %.2f\n"),Tiers[OwnedTier+1],Next.Cost,Next.MaxHealth,Next.Attack);
     if (Progression->Experience<Next.Cost) Text+=TEXT("当前经验不足。\n");
     else if (Selected!=Form) Text+=TEXT("可直接进化此信物，不改变当前形态。\n");
    } else Text+=TEXT("已进化至最高档。\n");
   } else if (Selected->FormId==TEXT("InitialInk")) {
    FCC_FormStage Ink;Progression->GetStagePreview(Selected,0,Ink);
    Text+=FString::Printf(TEXT("初始自带 · 随经验强化\n每持有1经验，生命和攻击增加%g%%。\n当前倍率 %.2f 倍\n基础生命 %.0f    攻击 %.0f\n\n数值四舍五入，花费经验后同步降低。\n"),Progression->InitialInkBonusPerExperience*100.0,Progression->GetExperienceGrowthMultiplier(Selected),Ink.MaxHealth,Ink.Attack);
   } else Text+=TEXT("此信物没有档位进化。\n");
  }
  Text+=TEXT("\n信物永久保留，进化后保留档位。\n点击格子只浏览；使用后才转换形态。\n");
  if (AS) Text+=FString::Printf(TEXT("\n当前角色实际属性\n生命 %.0f / %.0f\n法力 %.0f / %.0f\n攻击 %.2f    防御 %.2f\n移速 %.0f\n"),AS->GetHealth(),AS->GetMaxHealth(),AS->GetMana(),AS->GetMaxMana(),AS->GetAttack(),AS->GetDefense(),AS->GetMoveSpeed());
  if (BagDetails) BagDetails->SetText(FText::FromString(Text));Text.Empty();
 }
 else if (Page==ECC_MuralMenuPage::Skills) {
  if (Skills) for (int32 I=0;I<4;++I) {
   SlotButtons[I]->SetStyle(ActionStyle(I==SelectedSlot)); SlotTexts[I]->SetColorAndOpacity(I==SelectedSlot ? Gold : Ivory);
   const FCC_SkillSlot* S=Skills->CurrentSlots.IsValidIndex(I) ? &Skills->CurrentSlots[I] : nullptr;
   const float CD=Skills->GetCooldownRemaining(I);
   SlotTexts[I]->SetText(FText::FromString(FString::Printf(TEXT("%s  [%s]  %s%s"),I==SelectedSlot ? TEXT("◆") : TEXT("◇"),PC ? *PC->GetCombatKey(I).GetDisplayName().ToString() : TEXT("?"),S && S->Skill ? *S->Skill->DisplayName.ToString() : TEXT("尚未就绪"),CD>0 ? *FString::Printf(TEXT("  %.1fs"),CD) : TEXT(""))));
  }
  const bool Ready=Progression->CanConfigureSkills(); EquipButton->SetIsEnabled(Ready && GetSelectedSkill()); ResetSkillsButton->SetIsEnabled(Ready);
  if (const auto* S=GetSelectedSkill()) {
   int32 Level=1+Progression->GetSkillBonus(S->SkillId);
   if (Skills) for (const auto& EquippedSlot:Skills->CurrentSlots) if (EquippedSlot.Skill==S) { Level=EquippedSlot.Level+Progression->GetSkillBonus(S->SkillId); break; }
   Text=S->GetNumericDescription(AS ? AS->GetAttack() : 0,Level).ToString();
  }
  Text+=TEXT("\n\n同一技能移到另一槽将交换位置。\n技能与普攻冷却期间不能换装。");
 }
 else if (Page==ECC_MuralMenuPage::Trade) {
  auto* Selected=GetSelectedRoute(); int32 OwnedTier=INDEX_NONE;
  if (Selected==Progression->NativeForm) OwnedTier=Progression->NativeTier;
  else for (const auto& F:Progression->Forms) if (F.Form==Selected) OwnedTier=F.Tier;
  SwitchButton->SetIsEnabled(Selected && OwnedTier>=0 && Selected!=Form);
  const int32 Cost=Progression->GetEvolutionCost();
  EvolveButton->SetIsEnabled(Selected==Form && Cost>=0 && Progression->Experience>=Cost);
  if (Selected && (SelectedTownShop==ECC_TownShopId::Books||(bRemoteBrowsing&&SelectedTownShop==ECC_TownShopId::None&&Progression->bRemoteShopUnlocked))) {
   Text=Selected->DisplayName.ToString()+TEXT("\n");
   if (!Selected->bCanEvolve) {
    FCC_FormStage Ink;if (Progression->GetStagePreview(Selected,0,Ink)) Text+=FString::Printf(TEXT("初墨之力随持有经验强化；每点增加%g%%生命和攻击，四舍五入。\n当前基础生命 %.0f，攻击 %.0f。\n"),Progression->InitialInkBonusPerExperience*100.0,Ink.MaxHealth,Ink.Attack);
   }
   else {
    const TCHAR* Tiers[]={TEXT("初级"),TEXT("中级"),TEXT("高级")};
    for (int32 I=0;I<3;++I) { FCC_FormStage S; if (Progression->GetStagePreview(Selected,I,S)) Text+=FString::Printf(TEXT("\n%s  %s\n生命 %.0f    攻击 %.2f\n%s\n"),Tiers[I],OwnedTier==I ? TEXT("· 当前档位") : TEXT(""),S.MaxHealth,S.Attack,I==0 ? TEXT("通过器物获取") : *FString::Printf(TEXT("进化费用 %d 经验"),S.Cost)); }
    Text+=OwnedTier<0 ? TEXT("\n尚未解锁，请先与对应器物交互。\n") : Selected!=Form ? TEXT("\n切换到本形态后可购买下一档进化。\n") : Cost<0 ? TEXT("\n已达最高档位。\n") : FString::Printf(TEXT("\n本次进化需要 %d 经验。\n"),Cost);
   }
  }
  if (auto* Upgrade=GetSelectedUpgrade()) {
   const int32 Level=Progression->GetUpgradeLevel(Upgrade); const bool CanBuy=Upgrade->Prices.IsValidIndex(Level);
   PurchaseButton->SetIsEnabled(Progression->CanAccessTownShop(CCTownShops::ForUpgrade(Upgrade)) && CanBuy && Progression->Experience>=Upgrade->Prices[Level]);
   Text+=TEXT("\n────────────\n")+UpgradeLabel(Upgrade)+FString::Printf(TEXT("  已购 %d 级\n"),Level);
   Text+=CanBuy ? FString::Printf(TEXT("下一次需要 %d 经验\n"),Upgrade->Prices[Level]) : TEXT("已购至最高级\n");
   Text+=Upgrade->Type==ECC_UpgradeType::SkillLevel ? TEXT("每次增加对应技能1级。\n") : FString::Printf(TEXT("每级增加 %.2f，跨形态保留。\n"),Upgrade->BonusPerLevel);
  }
 }
 if (Details->GetText().ToString()!=Text) Details->SetText(FText::FromString(Text));
}
void UCC_MuralMenuWidget::SetFeedback(const FString& Message,bool bError) { if (Feedback) { Feedback->SetColorAndOpacity(bError ? FLinearColor(.86f,.51f,.4f) : Gold); Feedback->SetText(FText::FromString(Message)); } }
void UCC_MuralMenuWidget::SourceChanged() { PopulateItems(); RefreshData(); }
void UCC_MuralMenuWidget::Overview() { ShowPage(ECC_MuralMenuPage::Overview); }
void UCC_MuralMenuWidget::Inventory() { ShowPage(ECC_MuralMenuPage::Inventory); }
void UCC_MuralMenuWidget::SkillsPage() {
 if (SkillGalleryWidget && Progression) {
  const FName Id=Progression->GetActiveForm() ? Progression->GetActiveForm()->FormId : NAME_None;
  SkillGalleryWidget->SelectCategory(Id==TEXT("Artifact_01") ? 1 : Id==TEXT("Artifact_02") ? 2 : Id==TEXT("Native") ? 3 : 0);
 }
 ShowPage(ECC_MuralMenuPage::Skills);
}
void UCC_MuralMenuWidget::Trade() {
 if(Progression&&Progression->GetLocalTownShop()!=ECC_TownShopId::None)OpenTownShop(Progression->GetLocalTownShop());
 else if(Progression&&Progression->bRemoteShopUnlocked)OpenRemoteShop();
 else {SelectedTownShop=ECC_TownShopId::None;PopulateItems();ShowPage(ECC_MuralMenuPage::Trade);}
}
void UCC_MuralMenuWidget::Settings() { ShowPage(ECC_MuralMenuPage::Settings); if (OptionsPanel) OptionsPanel->RefreshOptions(); }
void UCC_MuralMenuWidget::Close() { if (auto* PC=Cast<ACC_PlayerController>(GetOwningPlayer())) PC->CloseProgressionMenu(); }
void UCC_MuralMenuWidget::SelectSkillSlot(int32 I) { if (I<0 || I>3) return; SelectedSlot=I; RefreshData(); }
void UCC_MuralMenuWidget::SlotOne() { SelectSkillSlot(0); }
void UCC_MuralMenuWidget::SlotTwo() { SelectSkillSlot(1); }
void UCC_MuralMenuWidget::SlotThree() { SelectSkillSlot(2); }
void UCC_MuralMenuWidget::SlotFour() { SelectSkillSlot(3); }
void UCC_MuralMenuWidget::ChoiceChanged(FString Item,ESelectInfo::Type Type) { RefreshData(); }
void UCC_MuralMenuWidget::Equip() { if (Progression && GetSelectedSkill()) { SetFeedback(TEXT("正在请求装备……")); Progression->EquipSkill(SelectedSlot,GetSelectedSkill()); } }
void UCC_MuralMenuWidget::ResetSkills() { if (Progression) { SetFeedback(TEXT("正在恢复技能组……")); Progression->ResetSkills(); } }
void UCC_MuralMenuWidget::SwitchRoute() { if (Progression && GetSelectedRoute()) { PendingPurchase=TEXT("切换形态"); Progression->SwitchForm(GetSelectedRoute()==Progression->NativeForm ? nullptr : GetSelectedRoute()); } }
void UCC_MuralMenuWidget::Evolve() { if (Progression && GetSelectedRoute()==Progression->GetActiveForm()) { PendingPurchase=TEXT("进化"); Progression->Evolve(); } }
void UCC_MuralMenuWidget::Purchase() {if(Progression)if(auto* Upgrade=GetSelectedUpgrade()){PendingPurchase=TEXT("购买加成");Progression->Purchase(Upgrade);}}
void UCC_MuralMenuWidget::PurchaseResult(ECC_PurchaseResult R) {
 const TCHAR* Fail[]={TEXT("成功"),TEXT("经验不足"),TEXT("已达最高档位"),TEXT("配置不可用"),TEXT("当前角色状态不可操作"),TEXT("请先通过器物解锁此形态")};
 SetFeedback(R==ECC_PurchaseResult::Success ? PendingPurchase+TEXT("成功") : Fail[FMath::Clamp(int32(R),0,5)],R!=ECC_PurchaseResult::Success); RefreshData();
}
void UCC_MuralMenuWidget::SkillResult(ECC_SkillConfigurationResult R) {
 if(SkillGalleryWidget) SkillGalleryWidget->HandleEquipmentResult(R);
 if(R==ECC_SkillConfigurationResult::NotUnlocked) { SetFeedback(TEXT("尚未解锁对应流派，请先获取元素之力"),true); RefreshData(); return; }
 const TCHAR* Messages[]={TEXT("技能配置成功"),TEXT("当前角色状态不可配置"),TEXT("冷却期间不能换装"),TEXT("只支持四个技能槽"),TEXT("技能未在可装备目录中"),TEXT("技能组配置无效")};
 SetFeedback(Messages[FMath::Clamp(int32(R),0,5)],R!=ECC_SkillConfigurationResult::Success); RefreshData();
}

void UCC_MuralMenuWidget::RefreshBag() {
 if (!BagGrid || !Progression) return;
 TArray<TObjectPtr<UCC_FormDefinition>> Expected;
 if (Progression->NativeForm) Expected.Add(Progression->NativeForm);
 for (const auto& P:Progression->Forms) if (P.Form && P.Form->bAvailable && !Expected.Contains(P.Form)) Expected.Add(P.Form);
 BagOptions=MoveTemp(Expected);
 if (!BagOptions.Contains(SelectedBagForm)) SelectedBagForm=BagOptions.Contains(Progression->GetActiveForm()) ? Progression->GetActiveForm() : BagOptions.IsEmpty() ? nullptr : BagOptions[0].Get();
 const int32 Required=FMath::Max(9,FMath::DivideAndRoundUp(BagOptions.Num(),3)*3);
 if (Required!=BagCells.Num()) {
  BagGrid->ClearChildren();BagCells.Empty();
  for (int32 I=0;I<Required;++I) {
   auto* Cell=CreateWidget<UCC_PowerTokenWidget>(GetOwningPlayer(),UCC_PowerTokenWidget::StaticClass());
   if (!Cell) break;
   Cell->OnSelected.AddDynamic(this,&ThisClass::SelectBagForm);BagGrid->AddChildToUniformGrid(Cell,I/3,I%3);BagCells.Add(Cell);
  }
 }
 for (int32 I=0;I<BagCells.Num();++I) {
  auto* Form=BagOptions.IsValidIndex(I) ? BagOptions[I].Get() : nullptr;
  BagCells[I]->SetEntry(Form,Form ? Progression->GetOwnedFormTier(Form) : 0,Form && Form==Progression->GetActiveForm(),Form && Form==SelectedBagForm,ArtData);
 }
}
void UCC_MuralMenuWidget::SelectBagForm(UCC_FormDefinition* Form) {
 if (!Form || !BagOptions.Contains(Form)) return;
 SelectedBagForm=Form;RefreshData();
}
void UCC_MuralMenuWidget::UseBagPower() {
 if (!Progression || !SelectedBagForm || !BagOptions.Contains(SelectedBagForm)) return;
 auto* F=SelectedBagForm.Get(); PendingPurchase=TEXT("使用元素之力");
 Progression->SwitchForm(F==Progression->NativeForm ? nullptr : F);
}

void UCC_MuralMenuWidget::EvolveBagPower() {
 if (!Progression || !SelectedBagForm || !BagOptions.Contains(SelectedBagForm)) return;
 PendingPurchase=TEXT("进化"); Progression->EvolveForm(SelectedBagForm);
}
