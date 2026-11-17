#include "UI/CC_ProgressionMenu.h"
#include "UI/CC_GameFont.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Components/InputKeySelector.h"
#include "Player/CC_PlayerController.h"
#include "Player/CC_PlayerState.h"
#include "Characters/CC_BaseCharacter.h"
#include "Progression/CC_SkillComponent.h"
#include "AbilitySystem/CC_AttributeSet.h"

UButton* UCC_ProgressionMenu::AddButton(UVerticalBox* Box,const FString& Label) {
 auto* B=WidgetTree->ConstructWidget<UButton>(); auto* T=WidgetTree->ConstructWidget<UTextBlock>();
 auto Font= CCGameFont::Kai(T->GetFont()); Font.Size=18; T->SetFont(Font); T->SetText(FText::FromString(Label)); T->SetMargin(FMargin(10,4)); B->AddChild(T); Box->AddChild(B); return B;
}
void UCC_ProgressionMenu::NativeOnInitialized() {
 Super::NativeOnInitialized(); SetIsFocusable(true);
 auto* Root=WidgetTree->ConstructWidget<UCanvasPanel>(); WidgetTree->RootWidget=Root;
 auto* Panel=WidgetTree->ConstructWidget<UBorder>(); Panel->SetBrushColor(FLinearColor(.02f,.02f,.025f,.97f)); Panel->SetPadding(FMargin(20));
 auto* PSlot=Root->AddChildToCanvas(Panel); PSlot->SetAnchors(FAnchors(.5f)); PSlot->SetAlignment(FVector2D(.5f)); PSlot->SetSize(FVector2D(1100,900));
 auto* Box=WidgetTree->ConstructWidget<UVerticalBox>(); Panel->SetContent(Box);
 auto* Title=WidgetTree->ConstructWidget<UTextBlock>(); Title->SetFont(CCGameFont::Kai(Title->GetFont())); Title->SetText(FText::FromString(TEXT("菜单 [Tab 返回游戏]"))); Box->AddChild(Title);
 AddButton(Box,TEXT("背包"))->OnClicked.AddDynamic(this,&ThisClass::Inventory);
 AddButton(Box,TEXT("进化"))->OnClicked.AddDynamic(this,&ThisClass::Evolution);
 AddButton(Box,TEXT("技能面板 / 装备技能"))->OnClicked.AddDynamic(this,&ThisClass::SkillsPage);
 AddButton(Box,TEXT("设置 / 键位与存档"))->OnClicked.AddDynamic(this,&ThisClass::SettingsPage);
 AddButton(Box,TEXT("退出界面 / 返回游戏"))->OnClicked.AddDynamic(this,&ThisClass::Close);
 Feedback=WidgetTree->ConstructWidget<UTextBlock>(); Feedback->SetFont(CCGameFont::Kai(Feedback->GetFont())); Feedback->SetAutoWrapText(true); Box->AddChild(Feedback);
 SkillControls=WidgetTree->ConstructWidget<UVerticalBox>(); Box->AddChild(SkillControls);
 SlotChoice=WidgetTree->ConstructWidget<UComboBoxString>();
 for (int32 I=0;I<4;++I) SlotChoice->AddOption(FString::Printf(TEXT("技能槽 %d"),I+1));
 SlotChoice->SetSelectedIndex(0); SkillControls->AddChild(SlotChoice);
 SkillChoice=WidgetTree->ConstructWidget<UComboBoxString>(); SkillControls->AddChild(SkillChoice);
 EquipButton=AddButton(SkillControls,TEXT("装备到所选槽位（重复技能交换槽位）")); EquipButton->OnClicked.AddDynamic(this,&ThisClass::Equip);
 AddButton(SkillControls,TEXT("恢复当前形态默认技能"))->OnClicked.AddDynamic(this,&ThisClass::ResetSkills);
 KeyControls=WidgetTree->ConstructWidget<UVerticalBox>(); Box->AddChild(KeyControls);
 KeyActionChoice=WidgetTree->ConstructWidget<UComboBoxString>();
 for (const TCHAR* Name:{TEXT("技能 1"),TEXT("技能 2"),TEXT("技能 3"),TEXT("技能 4"),TEXT("普通攻击（鼠标左键同时有效）"),TEXT("交互")}) KeyActionChoice->AddOption(Name);
 KeyControls->AddChild(KeyActionChoice); KeyActionChoice->OnSelectionChanged.AddDynamic(this,&ThisClass::KeyActionChanged);
 KeySelector=WidgetTree->ConstructWidget<UInputKeySelector>(); KeySelector->SetAllowGamepadKeys(false); KeySelector->SetAllowModifierKeys(false);KeySelector->SetEscapeKeys({EKeys::Tab});
 KeySelector->SetKeySelectionText(FText::FromString(TEXT("按新键，Tab 取消"))); KeySelector->OnKeySelected.AddDynamic(this,&ThisClass::KeySelected); KeyControls->AddChild(KeySelector); KeyActionChoice->SetSelectedIndex(0);
 AddButton(KeyControls,TEXT("恢复默认键位 J / 1 / 2 / 3 / 4 / E"))->OnClicked.AddDynamic(this,&ThisClass::ResetKeys);
 AddButton(KeyControls,TEXT("保存角色进度（经验 / 进化 / 技能装备）"))->OnClicked.AddDynamic(this,&ThisClass::SaveProgress);
 AddButton(KeyControls,TEXT("读取角色进度"))->OnClicked.AddDynamic(this,&ThisClass::LoadProgress);
 ContentScroll=WidgetTree->ConstructWidget<UScrollBox>(); Box->AddChildToVerticalBox(ContentScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
 auto* Body=WidgetTree->ConstructWidget<UVerticalBox>(); ContentScroll->AddChild(Body);
 Details=WidgetTree->ConstructWidget<UTextBlock>(); auto BodyFont= CCGameFont::Kai(Details->GetFont()); BodyFont.Size=18; Details->SetFont(BodyFont); Details->SetAutoWrapText(true); Details->SetMargin(FMargin(0,16)); Body->AddChild(Details);
 EvolveButton=AddButton(Body,TEXT("购买下一档进化")); EvolveButton->OnClicked.AddDynamic(this,&ThisClass::Evolve);

 if (auto* PS=GetOwningPlayerState<ACC_PlayerState>()) {
  Progression=PS->Progression; Progression->OnPurchaseFinished.AddUniqueDynamic(this,&ThisClass::PurchaseResult);
  Progression->OnSkillConfigurationFinished.AddUniqueDynamic(this,&ThisClass::SkillConfigurationResult);
  for (const auto& Skill:Progression->SkillCatalog) SkillChoice->AddOption(Skill ? Skill->DisplayName.ToString():TEXT("未配置"));
  SkillChoice->SetSelectedIndex(0);
 }
 Refresh();
}
void UCC_ProgressionMenu::NativeDestruct() { if (Progression) { Progression->OnPurchaseFinished.RemoveDynamic(this,&ThisClass::PurchaseResult); Progression->OnSkillConfigurationFinished.RemoveDynamic(this,&ThisClass::SkillConfigurationResult); } Super::NativeDestruct(); }
void UCC_ProgressionMenu::NativeTick(const FGeometry& G,float D) { Super::NativeTick(G,D); Refresh(); }
FReply UCC_ProgressionMenu::NativeOnPreviewKeyDown(const FGeometry& G,const FKeyEvent& E) {
 if (KeySelector && KeySelector->GetIsSelectingKey()) return Super::NativeOnPreviewKeyDown(G,E);
 if (E.GetKey()==EKeys::Tab) { if(!E.IsRepeat())Close(); return FReply::Handled(); } return Super::NativeOnPreviewKeyDown(G,E);
}
void UCC_ProgressionMenu::Inventory() { Page=0; ContentScroll->ScrollToStart(); Feedback->SetText(FText::GetEmpty()); Refresh(); }
void UCC_ProgressionMenu::Evolution() { Page=1; ContentScroll->ScrollToStart(); Feedback->SetText(FText::GetEmpty()); Refresh(); }
void UCC_ProgressionMenu::SkillsPage() { Page=2; ContentScroll->ScrollToStart(); Feedback->SetText(FText::GetEmpty()); Refresh(); }
void UCC_ProgressionMenu::SettingsPage() { Page=3; ContentScroll->ScrollToStart(); Feedback->SetText(FText::GetEmpty()); Refresh(); }
void UCC_ProgressionMenu::Evolve() { if (Progression) Progression->Evolve(); }
void UCC_ProgressionMenu::Close() { if (auto* PC=Cast<ACC_PlayerController>(GetOwningPlayer())) PC->ToggleProgressionMenu(); }
void UCC_ProgressionMenu::Refresh() {
 if (!Details || !Progression) return;
 EvolveButton->SetVisibility(Page==1 ? ESlateVisibility::Visible:ESlateVisibility::Collapsed);
 SkillControls->SetVisibility(Page==2 ? ESlateVisibility::Visible:ESlateVisibility::Collapsed);
 KeyControls->SetVisibility(Page==3 ? ESlateVisibility::Visible:ESlateVisibility::Collapsed);
 const int32 Cost=Progression->GetEvolutionCost(); EvolveButton->SetIsEnabled(Cost>=0 && Progression->Experience>=Cost); EquipButton->SetIsEnabled(Progression->CanConfigureSkills());
 FString Text;
 if (Page==0) Text=FString::Printf(TEXT("背包\n当前经验：%d\n经验用于购买进化与属性加成。"),Progression->Experience);
 else if (Page==1) {
  const auto* F=Progression->CurrentForm ? Progression->CurrentForm.Get():Progression->NativeForm.Get();
  const TCHAR* Tiers[]={TEXT("初级"),TEXT("中级"),TEXT("高级")};
  Text=FString::Printf(TEXT("进化\n当前：%s · %s\n经验余额：%d\n%s\n初级 → 中级：100 经验\n中级 → 高级：1000 经验"),F ? *F->DisplayName.ToString():TEXT("自然之力"),Tiers[FMath::Clamp(Progression->CurrentTier,0,2)],Progression->Experience,Cost<0 ? TEXT("已达最高档位或配置不可用"):*FString::Printf(TEXT("本次需要：%d 经验%s"),Cost,Progression->Experience<Cost ? TEXT("（经验不足）"):TEXT("")));
  if (F) for (int32 I=0;I<F->Stages.Num();++I) { const auto& S=F->Stages[I]; float HP=S.MaxHealth,ATK=S.Attack; if (S.CombatBalance) S.CombatBalance->Resolve(S.StatProfileId,HP,ATK); Text+=FString::Printf(TEXT("\n%s：生命 %.0f，攻击 %.2f"),Tiers[FMath::Clamp(I,0,2)],HP,ATK); }
 } else if (Page==2) {
  const auto* C=Cast<ACC_BaseCharacter>(GetOwningPlayerPawn()); const auto* AS=C ? Cast<UCC_AttributeSet>(C->GetAttributeSet()):nullptr; const float Attack=AS ? AS->GetAttack():15.f;
  Text=FString::Printf(TEXT("当前攻击力：%.2f\n手动装备在切换形态与进化后保留；恢复默认可使用形态技能组。\n冷却期间不能换装。设置中可保存 / 读取角色进度。\n当前装备：\n"),Attack);
  if (C) for (int32 I=0;I<C->Skills->CurrentSlots.Num();++I) { const auto& S=C->Skills->CurrentSlots[I]; Text+=FString::Printf(TEXT("槽 %d：%s\n"),I+1,S.Skill ? *S.Skill->DisplayName.ToString():TEXT("空")); }
  int32 Selected=SkillChoice->GetSelectedIndex();
  if (Progression->SkillCatalog.IsValidIndex(Selected)) if (const auto* S=Progression->SkillCatalog[Selected].Get()) Text+=TEXT("\n所选技能\n")+S->GetNumericDescription(Attack,1+Progression->GetSkillBonus(S->SkillId)).ToString();
  Text+=TEXT("\n\n技能库（可改名模板，滚轮浏览）\n");
  for (const auto& S:Progression->SkillCatalog) if (S) Text+=TEXT("\n")+S->GetNumericDescription(Attack,1+Progression->GetSkillBonus(S->SkillId)).ToString()+TEXT("\n");
 } else {
  Text=TEXT("选择动作，再点击按键框设置。\n移动、跳跃、镜头沿用模板；鼠标左键保留普攻，右键展开战技，Tab 返回。\n重复按键、移动键、菜单键不能覆盖。键位修改自动保存于本机。\n角色进度通过上方按钮保存 / 读取。\n");
  if (const auto* PC=Cast<ACC_PlayerController>(GetOwningPlayer())) for (int32 I=0;I<6;++I) Text+=KeyActionChoice->GetOptionAtIndex(I)+TEXT("：")+PC->GetCombatKey(I).GetDisplayName().ToString()+TEXT("\n");
 }
 if (Details->GetText().ToString()!=Text) Details->SetText(FText::FromString(Text));
}
void UCC_ProgressionMenu::PurchaseResult(ECC_PurchaseResult R) {
 const TCHAR* M=R==ECC_PurchaseResult::Success ? TEXT("进化成功"):R==ECC_PurchaseResult::InsufficientExperience ? TEXT("经验不足"):R==ECC_PurchaseResult::MaxLevel ? TEXT("已达高级"):TEXT("当前无法进化，请检查状态或配置。"); Feedback->SetText(FText::FromString(M)); Refresh();
}
void UCC_ProgressionMenu::Equip() { int32 I=SkillChoice->GetSelectedIndex(); if (Progression && Progression->SkillCatalog.IsValidIndex(I) && Progression->CanConfigureSkills()) { Feedback->SetText(FText::FromString(TEXT("正在请求装备……"))); Progression->EquipSkill(SlotChoice->GetSelectedIndex(),Progression->SkillCatalog[I]); } Refresh(); }
void UCC_ProgressionMenu::ResetSkills() { if (Progression) Progression->ResetSkills(); Refresh(); }
void UCC_ProgressionMenu::ResetKeys() { if (auto* PC=Cast<ACC_PlayerController>(GetOwningPlayer())) PC->ResetCombatKeys(); KeyActionChanged(TEXT(""),ESelectInfo::Direct); }
void UCC_ProgressionMenu::KeyActionChanged(FString Item,ESelectInfo::Type Type) { TGuardValue<bool> Guard(bUpdatingKeySelector,true); if (KeySelector) if (auto* PC=Cast<ACC_PlayerController>(GetOwningPlayer())) KeySelector->SetSelectedKey(FInputChord(PC->GetCombatKey(KeyActionChoice->GetSelectedIndex()))); }
void UCC_ProgressionMenu::KeySelected(FInputChord Chord) {
 if (bUpdatingKeySelector) return;
 if (auto* PC=Cast<ACC_PlayerController>(GetOwningPlayer())) { bool Success=!Chord.bCtrl && !Chord.bAlt && !Chord.bShift && !Chord.bCmd && PC->SetCombatKey(KeyActionChoice->GetSelectedIndex(),Chord.Key); Feedback->SetText(FText::FromString(Success ? TEXT("键位已保存"):TEXT("按键无效、已占用或保留给模板移动 / 菜单。"))); if (!Success) KeyActionChanged(TEXT(""),ESelectInfo::Direct); }
}
void UCC_ProgressionMenu::SaveProgress() { Feedback->SetText(FText::FromString(Progression && Progression->SaveProgress() ? TEXT("角色进度已保存"):TEXT("当前不能保存（仅单机支持本地存档）"))); }
void UCC_ProgressionMenu::LoadProgress() { Feedback->SetText(FText::FromString(Progression && Progression->LoadProgress() ? TEXT("角色进度已读取"):TEXT("没有有效存档或当前不能读取"))); Refresh(); }

void UCC_ProgressionMenu::SkillConfigurationResult(ECC_SkillConfigurationResult Result)
{
 const TCHAR* Messages[] = { TEXT("技能配置成功"), TEXT("当前角色状态不可配置"), TEXT("冷却期间不能换装"), TEXT("无效的技能槽位"), TEXT("技能未在可装备目录中"), TEXT("技能组配置无效") };
 if (Feedback) Feedback->SetText(FText::FromString(Messages[FMath::Clamp(int32(Result),0,5)]));
 Refresh();
}
