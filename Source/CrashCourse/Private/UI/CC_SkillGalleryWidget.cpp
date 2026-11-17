#include "UI/CC_SkillGalleryWidget.h"
#include "UI/CC_GameFont.h"
#include "UI/CC_SkillGalleryData.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/ScrollBox.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Player/CC_PlayerController.h"
#include "Player/CC_PlayerState.h"
#include "Characters/CC_BaseCharacter.h"
#include "Progression/CC_SkillComponent.h"
#include "Brushes/SlateColorBrush.h"
namespace {
const FLinearColor GalleryGold(.87f,.78f,.57f,1.f), GalleryDim(.48f,.47f,.40f,1.f);
void GalleryPlace(UCanvasPanel* Parent,UWidget* Child,float X,float Y,float W,float H) {
 auto* Layout=Parent->AddChildToCanvas(Child); Layout->SetPosition({X,Y}); Layout->SetSize({W,H});
}
void GalleryTexture(UImage* Image,UTexture2D* Texture,bool Frame=false) {
 FSlateBrush B; B.SetResourceObject(Texture); B.DrawAs=ESlateBrushDrawType::Image;
 if (Frame) B.SetUVRegion(FBox2f(FVector2f(.085f,.085f),FVector2f(.915f,.915f)));
 Image->SetBrush(B);
}
void GalleryIcon(UImage* Image,UTexture2D* Texture) {
 GalleryTexture(Image,Texture); auto B=Image->GetBrush(); B.SetUVRegion(FBox2f(FVector2f(.026f,.026f),FVector2f(.974f,.974f))); Image->SetBrush(B);
}
}
void UCC_SkillGalleryWidget::NativeOnInitialized() {
 Super::NativeOnInitialized(); Root=WidgetTree->ConstructWidget<UCanvasPanel>(); WidgetTree->RootWidget=Root;
 Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible); SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}
UTextBlock* UCC_SkillGalleryWidget::Text(UCanvasPanel* Parent,const FString& V,float X,float Y,float W,float H,int32 Size) {
 auto* T=WidgetTree->ConstructWidget<UTextBlock>(); auto Font= CCGameFont::Kai(T->GetFont()); Font.Size=Size; Font.LetterSpacing=50; T->SetFont(Font);
 T->SetText(FText::FromString(V)); T->SetColorAndOpacity(GalleryGold); T->SetShadowColorAndOpacity(FLinearColor(0,0,0,.8f)); T->SetShadowOffset({1,1});
 T->SetVisibility(ESlateVisibility::HitTestInvisible); GalleryPlace(Parent,T,X,Y,W,H); return T;
}
UImage* UCC_SkillGalleryWidget::Art(UCanvasPanel* Parent,UTexture2D* T,float X,float Y,float W,float H,bool Frame) {
 auto* I=WidgetTree->ConstructWidget<UImage>(); GalleryTexture(I,T,Frame); I->SetVisibility(ESlateVisibility::HitTestInvisible); GalleryPlace(Parent,I,X,Y,W,H); return I;
}
UButton* UCC_SkillGalleryWidget::Hit(UCanvasPanel* Parent,float X,float Y,float W,float H) {
 auto* B=WidgetTree->ConstructWidget<UButton>(); FButtonStyle Style;
 FSlateColorBrush Clear(FLinearColor::Transparent); Style.SetNormal(Clear); Style.SetHovered(Clear); Style.SetPressed(Clear); Style.SetDisabled(Clear); B->SetStyle(Style);
 GalleryPlace(Parent,B,X,Y,W,H); return B;
}
void UCC_SkillGalleryWidget::InitializeGallery(UCC_SkillGalleryData* Data) {
 if (bBuilt || !Data || Data->Entries.IsEmpty()) return; bBuilt=true; PresentationData=Data;
 for (int32 I=0;I<4;++I) {
  auto* T=Data->CategoryTabs.IsValidIndex(I) ? Data->CategoryTabs[I].LoadSynchronous() : nullptr;
  Art(Root,T,465+I*300,24,294,78);
  auto* Label=Text(Root,Data->Categories.IsValidIndex(I) ? Data->Categories[I].ToString() : TEXT(""),465+I*300,42,294,44,27); Label->SetJustification(ETextJustify::Center); CategoryLabels.Add(Label);
  auto* Mark=Art(Root,nullptr,513+I*300,94,198,2); Mark->SetColorAndOpacity(GalleryGold); CategoryMarks.Add(Mark);
  CategoryButtons.Add(Hit(Root,465+I*300,24,294,78));
 }
 CategoryButtons[0]->OnClicked.AddDynamic(this,&ThisClass::Category0); CategoryButtons[1]->OnClicked.AddDynamic(this,&ThisClass::Category1);
 CategoryButtons[2]->OnClicked.AddDynamic(this,&ThisClass::Category2); CategoryButtons[3]->OnClicked.AddDynamic(this,&ThisClass::Category3);
 auto* TopLine=Art(Root,nullptr,480,125,590,1); TopLine->SetColorAndOpacity(GalleryDim);
 CategoryTitle=Text(Root,TEXT(""),480,136,590,64,38); CategoryTitle->SetJustification(ETextJustify::Center);
 auto* BottomLine=Art(Root,nullptr,480,200,590,1); BottomLine->SetColorAndOpacity(GalleryDim);
 GridScroll=WidgetTree->ConstructWidget<UScrollBox>(); GridScroll->SetConsumeMouseWheel(EConsumeMouseWheel::Always); GridScroll->SetScrollbarThickness({3,3}); GalleryPlace(Root,GridScroll,480,220,600,594);
 auto* GridSize=WidgetTree->ConstructWidget<USizeBox>(); GridSize->SetWidthOverride(590); GridSize->SetHeightOverride(900); GridScroll->AddChild(GridSize);
 auto* Grid=WidgetTree->ConstructWidget<UCanvasPanel>(); GridSize->SetContent(Grid);
 for (int32 I=0;I<6;++I) {
  const float X=18+(I%2)*295, Y=(I/2)*300;
  CardIcons.Add(Art(Grid,nullptr,X+7,Y+7,231,231));
  CardFrames.Add(Art(Grid,Data->CardNormal.LoadSynchronous(),X,Y,245,245,true));
  CardButtons.Add(Hit(Grid,X,Y,245,292));
  auto* Label=Text(Grid,TEXT(""),X-17,Y+252,280,40,25); Label->SetJustification(ETextJustify::Center); CardLabels.Add(Label);
 }
 CardButtons[0]->OnClicked.AddDynamic(this,&ThisClass::Card0); CardButtons[1]->OnClicked.AddDynamic(this,&ThisClass::Card1);
 CardButtons[2]->OnClicked.AddDynamic(this,&ThisClass::Card2); CardButtons[3]->OnClicked.AddDynamic(this,&ThisClass::Card3);
 CardButtons[4]->OnClicked.AddDynamic(this,&ThisClass::Card4); CardButtons[5]->OnClicked.AddDynamic(this,&ThisClass::Card5);
 auto* FooterLine=Art(Root,nullptr,480,834,600,1); FooterLine->SetColorAndOpacity(GalleryGold);
 Text(Root,TEXT("◇  快捷技能"),480,845,370,45,28); EquipmentHint=Text(Root,TEXT(""),928,855,150,30,16); EquipmentHint->SetColorAndOpacity(GalleryDim);
 PreviewSlots.Init(INDEX_NONE,4);
 for (int32 I=0;I<4;++I) {
  const float X=494+I*146;
  auto* Key=Text(Root,FString::Printf(TEXT("%02d"),I+1),X,892,132,24,17); Key->SetJustification(ETextJustify::Center); SlotKeys.Add(Key);
  SlotIcons.Add(Art(Root,nullptr,X+6,923,120,120));
  SlotFrames.Add(Art(Root,Data->SlotNormal.LoadSynchronous(),X,917,132,132,true));
  auto* Empty=Text(Root,TEXT("◇"),X,951,132,60,39); Empty->SetJustification(ETextJustify::Center); SlotEmpty.Add(Empty);
  auto* B=Hit(Root,X,917,132,132);
  if (I==0) B->OnClicked.AddDynamic(this,&ThisClass::Slot0); if (I==1) B->OnClicked.AddDynamic(this,&ThisClass::Slot1);
  if (I==2) B->OnClicked.AddDynamic(this,&ThisClass::Slot2); if (I==3) B->OnClicked.AddDynamic(this,&ThisClass::Slot3);
 }
 auto* Back=WidgetTree->ConstructWidget<UBorder>(); Back->SetBrush(FSlateColorBrush(FLinearColor(0,0,0,.84f))); Back->SetVisibility(ESlateVisibility::HitTestInvisible); GalleryPlace(Root,Back,1125,134,672,883);
 Art(Root,Data->DetailPanel.LoadSynchronous(),1105,113,720,931);
 DetailTitle=Text(Root,TEXT(""),1160,143,600,62,44);
 DetailCategory=Text(Root,TEXT(""),1166,214,600,38,23);
 Hero=Art(Root,nullptr,1300,250,330,330);
 auto* Divider=Art(Root,nullptr,1160,591,600,1); Divider->SetColorAndOpacity(GalleryGold);
 for (int32 I=0;I<4;++I) {
  StatLabels.Add(Text(Root,TEXT(""),1160,606+I*46,242,37,24));
  auto* V=Text(Root,TEXT(""),1410,606+I*46,350,37,23); V->SetJustification(ETextJustify::Right); StatValues.Add(V);
  auto* L=Art(Root,nullptr,1160,649+I*46,600,1); L->SetColorAndOpacity(FLinearColor(.3f,.28f,.19f,.6f));
 }
 auto* DescScroll=WidgetTree->ConstructWidget<UScrollBox>(); DescScroll->SetScrollbarThickness({3,3}); GalleryPlace(Root,DescScroll,1160,797,600,140);
 Description=WidgetTree->ConstructWidget<UTextBlock>(); auto Font= CCGameFont::Kai(Description->GetFont()); Font.Size=23; Description->SetFont(Font); Description->SetColorAndOpacity(GalleryGold); Description->SetAutoWrapText(true); Description->SetLineHeightPercentage(1.25f); DescScroll->AddChild(Description);
 // Keep the equip frame transparent so the detail panel shows through its alpha.
 Text(Root,TEXT("◇"),1178,961,50,55,34); Text(Root,TEXT("◇"),1701,961,50,55,34);
 auto* EquipFrame=Art(Root,Data->CategoryTabs.IsValidIndex(0) ? Data->CategoryTabs[0].LoadSynchronous() : nullptr,1210,952,500,75); EquipFrame->SetColorAndOpacity(GalleryGold);
 auto* EquipLabel=Text(Root,TEXT("装备技能"),1210,960,500,58,32); EquipLabel->SetJustification(ETextJustify::Center);
 EquipPreviewButton=Hit(Root,1210,948,500,80); EquipPreviewButton->SetToolTipText(FText::FromString(TEXT("装备到所选真实技能槽，1～4释放；同技能换槽自动交换"))); EquipPreviewButton->OnClicked.AddDynamic(this,&ThisClass::PreviewEquip);
 EquipmentStatus=Text(Root,TEXT(""),1130,1036,675,34,17);
 Text(Root,TEXT("恢复当前形态技能组"),494,1052,430,26,16);
 Hit(Root,494,1050,430,30)->OnClicked.AddDynamic(this,&ThisClass::ResetEquipment);
 SelectCategory(1); RefreshFromPlayer();
}
void UCC_SkillGalleryWidget::SelectCategory(int32 Index) {
 if (!PresentationData || !PresentationData->Categories.IsValidIndex(Index)) return;
 Category=Index; CategoryEntries.Empty();
 for (int32 I=0;I<PresentationData->Entries.Num();++I) if (PresentationData->Entries[I].Category==Index) CategoryEntries.Add(I);
 GridScroll->SetScrollBarVisibility(CategoryEntries.Num()>4 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
 CategoryTitle->SetText(PresentationData->Categories[Index]);
 for (int32 I=0;I<CardButtons.Num();++I) {
  const bool Visible=CategoryEntries.IsValidIndex(I);
  CardButtons[I]->SetVisibility(Visible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
  CardIcons[I]->SetVisibility(Visible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
  CardFrames[I]->SetVisibility(Visible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
  CardLabels[I]->SetVisibility(Visible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
  if (Visible) { const auto& R=PresentationData->Entries[CategoryEntries[I]]; GalleryIcon(CardIcons[I],R.Icon.LoadSynchronous()); auto* PS=GetOwningPlayerState<ACC_PlayerState>(); const bool Unlocked=PS && PS->Progression->IsSkillUnlocked(R.ExecutableSkill); CardIcons[I]->SetColorAndOpacity(Unlocked ? FLinearColor::White : FLinearColor(.3f,.3f,.3f,1)); CardLabels[I]->SetText(Unlocked ? R.DisplayName : FText::FromString(R.DisplayName.ToString()+TEXT(" · 未解锁"))); }
 }
 if (auto* Size=Cast<USizeBox>(GridScroll->GetChildAt(0))) Size->SetHeightOverride(FMath::Max(0.f,FMath::DivideAndRoundUp(CategoryEntries.Num(),2)*300.f-6.f));
 GridScroll->ScrollToStart();
 RefreshFromPlayer();
 SelectEntry(CategoryEntries.IsValidIndex(Index==1 ? 2 : 0) ? CategoryEntries[Index==1 ? 2 : 0] : INDEX_NONE);
}
void UCC_SkillGalleryWidget::SelectEntry(int32 Index) {
 if (!PresentationData || !PresentationData->Entries.IsValidIndex(Index) || PresentationData->Entries[Index].Category!=Category) return;
 Entry=Index; RefreshSelection();
}
void UCC_SkillGalleryWidget::SelectPreviewSlot(int32 Index) { if (Index>=0 && Index<4) { PreviewSlot=Index; RefreshSelection(); } }
void UCC_SkillGalleryWidget::PreviewEquip() {
 if(!PresentationData || !PresentationData->Entries.IsValidIndex(Entry)) return;
 auto* PS=GetOwningPlayerState<ACC_PlayerState>(); auto* P=PS ? PS->Progression.Get() : nullptr;
 auto* Skill=PresentationData->Entries[Entry].ExecutableSkill.Get();
 if(!P || !Skill) { EquipmentStatus->SetText(FText::FromString(TEXT("技能尚未配置执行资产"))); return; }
 if(!P->IsSkillUnlocked(Skill)) { HandleEquipmentResult(ECC_SkillConfigurationResult::NotUnlocked); return; }
 EquipmentStatus->SetText(FText::FromString(TEXT("正在装备……"))); P->EquipSkill(PreviewSlot,Skill);
}
void UCC_SkillGalleryWidget::RefreshSelection() {
 if (!PresentationData || !PresentationData->Entries.IsValidIndex(Entry)) return;
 auto* PS=GetOwningPlayerState<ACC_PlayerState>(); auto* P=PS ? PS->Progression.Get() : nullptr;
 const auto* SelectedSkill=PresentationData->Entries[Entry].ExecutableSkill.Get();
 const bool Unlocked=P && P->IsSkillUnlocked(SelectedSkill);
 if(EquipPreviewButton) EquipPreviewButton->SetIsEnabled(Unlocked && P->CanConfigureSkills());
 if(EquipmentHint) EquipmentHint->SetText(FText::FromString(Unlocked ? TEXT("实际四槽") : TEXT("流派未解锁")));
 const auto& R=PresentationData->Entries[Entry]; DetailTitle->SetText(R.DisplayName); DetailCategory->SetText(PresentationData->Categories[R.Category]); Description->SetText(R.ExecutableSkill && !R.ExecutableSkill->Description.IsEmpty() ? R.ExecutableSkill->Description : R.Description); GalleryIcon(Hero,R.Icon.LoadSynchronous());
 for (int32 I=0;I<4;++I) {
  CategoryLabels[I]->SetColorAndOpacity(I==Category ? GalleryGold : GalleryDim); CategoryMarks[I]->SetVisibility(I==Category ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
  StatLabels[I]->SetText(R.StatLabels.IsValidIndex(I) ? R.StatLabels[I] : FText::GetEmpty()); StatValues[I]->SetText(R.StatValues.IsValidIndex(I) ? R.StatValues[I] : FText::GetEmpty());
  const int32 SlotEntry=PreviewSlots[I]; const bool Filled=PresentationData->Entries.IsValidIndex(SlotEntry);
  GalleryIcon(SlotIcons[I],Filled ? PresentationData->Entries[SlotEntry].Icon.LoadSynchronous() : nullptr);
  SlotIcons[I]->SetVisibility(Filled ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed); SlotEmpty[I]->SetVisibility(Filled ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
  GalleryTexture(SlotFrames[I],(I==PreviewSlot ? PresentationData->SlotSelected : PresentationData->SlotNormal).LoadSynchronous(),true);
  SlotFrames[I]->SetColorAndOpacity(I==PreviewSlot ? GalleryGold : FLinearColor(.65f,.63f,.53f,1));
 }
 for (int32 I=0;I<CardFrames.Num();++I) {
  const bool Selected=CategoryEntries.IsValidIndex(I) && CategoryEntries[I]==Entry;
  GalleryTexture(CardFrames[I],(Selected ? PresentationData->CardSelected : PresentationData->CardNormal).LoadSynchronous(),true);
  CardFrames[I]->SetColorAndOpacity(Selected ? GalleryGold : FLinearColor(.62f,.60f,.51f,1));
 }
}
void UCC_SkillGalleryWidget::Category0() { SelectCategory(0); }
void UCC_SkillGalleryWidget::Category1() { SelectCategory(1); }
void UCC_SkillGalleryWidget::Category2() { SelectCategory(2); }
void UCC_SkillGalleryWidget::Category3() { SelectCategory(3); }
void UCC_SkillGalleryWidget::Card0() { if (CategoryEntries.IsValidIndex(0)) SelectEntry(CategoryEntries[0]); }
void UCC_SkillGalleryWidget::Card1() { if (CategoryEntries.IsValidIndex(1)) SelectEntry(CategoryEntries[1]); }
void UCC_SkillGalleryWidget::Card2() { if (CategoryEntries.IsValidIndex(2)) SelectEntry(CategoryEntries[2]); }
void UCC_SkillGalleryWidget::Card3() { if (CategoryEntries.IsValidIndex(3)) SelectEntry(CategoryEntries[3]); }
void UCC_SkillGalleryWidget::Card4() { if (CategoryEntries.IsValidIndex(4)) SelectEntry(CategoryEntries[4]); }
void UCC_SkillGalleryWidget::Card5() { if (CategoryEntries.IsValidIndex(5)) SelectEntry(CategoryEntries[5]); }
void UCC_SkillGalleryWidget::Slot0() { SelectPreviewSlot(0); }
void UCC_SkillGalleryWidget::Slot1() { SelectPreviewSlot(1); }
void UCC_SkillGalleryWidget::Slot2() { SelectPreviewSlot(2); }
void UCC_SkillGalleryWidget::Slot3() { SelectPreviewSlot(3); }

void UCC_SkillGalleryWidget::RefreshFromPlayer() {
 if(!PresentationData) return;
 auto* PC=Cast<ACC_PlayerController>(GetOwningPlayer()); auto* C=PC ? Cast<ACC_BaseCharacter>(PC->GetPawn()) : nullptr;
 auto* PS=GetOwningPlayerState<ACC_PlayerState>(); auto* P=PS ? PS->Progression.Get() : nullptr;
 if(!P || !C) return;
 for(int32 I=0;I<4;++I) {
  const auto* S=C->Skills->CurrentSlots.IsValidIndex(I) ? C->Skills->CurrentSlots[I].Skill.Get() : nullptr;
  PreviewSlots[I]=PresentationData->Entries.IndexOfByPredicate([S](const FCC_SkillGalleryEntry& E){return S && E.ExecutableSkill==S;});
  if(SlotKeys.IsValidIndex(I)) SlotKeys[I]->SetText(PC->GetCombatKey(I).GetDisplayName());
  if(SlotEmpty.IsValidIndex(I)) {
   const bool Legacy=S && PreviewSlots[I]==INDEX_NONE;
   SlotEmpty[I]->SetText(Legacy ? S->DisplayName : FText::FromString(TEXT("◇")));
   auto Font= CCGameFont::Kai(SlotEmpty[I]->GetFont());Font.Size=Legacy ? 12 : 39;SlotEmpty[I]->SetFont(Font);SlotEmpty[I]->SetAutoWrapText(Legacy);
   SlotFrames[I]->SetToolTipText(S ? FText::FromString(S->DisplayName.ToString()+TEXT("\n")+S->Description.ToString()) : FText::GetEmpty());
  }
 }
 for(int32 I=0;I<CategoryEntries.Num() && I<CardIcons.Num();++I) {
  const auto& Row=PresentationData->Entries[CategoryEntries[I]];const bool Open=P->IsSkillUnlocked(Row.ExecutableSkill);
  CardIcons[I]->SetColorAndOpacity(Open ? FLinearColor::White : FLinearColor(.3f,.3f,.3f,1));
  CardLabels[I]->SetText(Open ? Row.DisplayName : FText::FromString(Row.DisplayName.ToString()+TEXT(" · 未解锁")));
 }
 RefreshSelection();
}
void UCC_SkillGalleryWidget::HandleEquipmentResult(ECC_SkillConfigurationResult R) {
 if(EquipmentStatus) {
  const TCHAR* M[]={TEXT("装备成功，1～4使用实际技能"),TEXT("当前状态无法装备"),TEXT("技能冷却期间不能换装"),TEXT("无效槽位"),TEXT("技能不在可装备目录"),TEXT("技能配置无效"),TEXT("尚未解锁此流派，请先拾取对应元素之力")};
  EquipmentStatus->SetText(FText::FromString(M[FMath::Clamp(int32(R),0,6)]));
 }
 RefreshFromPlayer();
}

void UCC_SkillGalleryWidget::ResetEquipment() { if(auto* PS=GetOwningPlayerState<ACC_PlayerState>()) PS->Progression->ResetSkills(); }
