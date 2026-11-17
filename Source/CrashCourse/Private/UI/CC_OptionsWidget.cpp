#include "UI/CC_OptionsWidget.h"
#include "CC_MenuStyle.h"
#include "Player/CC_PlayerController.h"
#include "Player/CC_PlayerState.h"
#include "Progression/CC_ProgressionComponent.h"
#include "UI/CC_WorldNavigationWidget.h"
#include "World/CC_AdventureGameInstance.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/InputKeySelector.h"
#include "Components/SizeBox.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

namespace
{
const TCHAR* GraphicsSection=TEXT("SpiritTales.Graphics");
const TCHAR* UpscalerKey=TEXT("Upscaler");

bool SupportsDLSS()
{
	// Resolve the optional plugin at runtime so other builds can still use TSR.
	UClass* Library=FindObject<UClass>(nullptr,TEXT("/Script/DLSSBlueprint.DLSSLibrary"));
	UFunction* Function=Library ? Library->FindFunctionByName(TEXT("IsDLSSSupported")) : nullptr;
	const FBoolProperty* Return=Function ? CastField<FBoolProperty>(Function->GetReturnProperty()) : nullptr;
	if (!Return) return false;
	FStructOnScope Params(Function);
	Library->GetDefaultObject()->ProcessEvent(Function,Params.GetStructMemory());
	return Return->GetPropertyValue_InContainer(Params.GetStructMemory());
}

bool IsDLSSEnabled()
{
	const IConsoleVariable* Variable=IConsoleManager::Get().FindConsoleVariable(TEXT("r.NGX.DLSS.Enable"));
	return Variable && Variable->GetInt()!=0 && SupportsDLSS();
}

void ApplyUpscaler(bool bDLSS)
{
	IConsoleManager& Console=IConsoleManager::Get();
	if (IConsoleVariable* Variable=Console.FindConsoleVariable(TEXT("r.AntiAliasingMethod")))
		Variable->Set(4,ECVF_SetByConsole);
	if (IConsoleVariable* Variable=Console.FindConsoleVariable(TEXT("r.DynamicRes.OperationMode")))
		Variable->Set(0,ECVF_SetByConsole);
	if (IConsoleVariable* Variable=Console.FindConsoleVariable(TEXT("r.ScreenPercentage")))
		Variable->Set(bDLSS ? 66.6667f : 80.0f,ECVF_SetByConsole);
	if (IConsoleVariable* Variable=Console.FindConsoleVariable(TEXT("r.NGX.DLSS.Enable")))
		Variable->Set(bDLSS ? 1 : 0,ECVF_SetByConsole);
}
}

void UCC_OptionsWidget::ApplySavedUpscaler()
{
	FString Value;
	if (GConfig && GConfig->GetString(GraphicsSection,UpscalerKey,Value,GGameUserSettingsIni))
		ApplyUpscaler(Value==TEXT("DLSS") && SupportsDLSS());
}

void UCC_OptionsWidget::RefreshUpscaler()
{
	if (!UpscalerStatus || !DLSSButton) return;
	const bool bSupported=SupportsDLSS();
	DLSSButton->SetIsEnabled(bSupported);
	UpscalerStatus->SetText(FText::FromString(IsDLSSEnabled() ? TEXT("当前：DLSS Quality") :
		bSupported ? TEXT("当前：UE 超分辨率") : TEXT("当前：UE 超分辨率\n本机未启用或不支持 DLSS")));
}

void UCC_OptionsWidget::SelectUpscaler(bool bDLSS)
{
	if (bDLSS && !SupportsDLSS()) return;
	ApplyUpscaler(bDLSS);
	if (GConfig)
	{
		GConfig->SetString(GraphicsSection,UpscalerKey,bDLSS ? TEXT("DLSS") : TEXT("TSR"),GGameUserSettingsIni);
		GConfig->Flush(false,GGameUserSettingsIni);
	}
	RefreshUpscaler();
}

void UCC_OptionsWidget::SelectTSR(){SelectUpscaler(false);}
void UCC_OptionsWidget::SelectDLSS(){SelectUpscaler(true);}

void UCC_OptionsWidget::BuildLayout()
{
	if (WidgetTree->RootWidget) return;
	auto* Columns=WidgetTree->ConstructWidget<UHorizontalBox>();WidgetTree->RootWidget=Columns;
	auto Left=CCMenuStyle::Frame(WidgetTree,ArtData,FMargin(24));
	auto* LeftSlot=Columns->AddChildToHorizontalBox(Left.Root);LeftSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));LeftSlot->SetPadding(FMargin(0,0,18,0));
	auto* Controls=WidgetTree->ConstructWidget<UVerticalBox>();Left.Body->SetContent(Controls);
	auto AddText=[this](UVerticalBox* Parent,const FString& Value,int32 FontSize=18) {
		auto* Label=CCMenuStyle::Text(WidgetTree,Value,FontSize);Parent->AddChildToVerticalBox(Label)->SetPadding(FMargin(0,0,0,12));return Label;
	};
	AddText(Controls,TEXT("按 键 配 置"),26)->SetColorAndOpacity(CCMenuStyle::Gold);
	AddText(Controls,TEXT("点击右侧键位框，再按下新的按键。"),16)->SetColorAndOpacity(CCMenuStyle::Muted);
	const TCHAR* Labels[]={TEXT("技能槽一"),TEXT("技能槽二"),TEXT("技能槽三"),TEXT("技能槽四"),TEXT("普通攻击"),TEXT("拾取 / 交互")};
	for (int32 I=0;I<6;++I)
	{
		auto* Row=WidgetTree->ConstructWidget<UHorizontalBox>();Controls->AddChildToVerticalBox(Row)->SetPadding(FMargin(0,0,0,10));
		auto* Name=CCMenuStyle::Text(WidgetTree,Labels[I],20);
		auto* NameSlot=Row->AddChildToHorizontalBox(Name);NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));NameSlot->SetVerticalAlignment(VAlign_Center);
		auto* Key=WidgetTree->ConstructWidget<UInputKeySelector>();Key->SetAllowGamepadKeys(false);Key->SetAllowModifierKeys(false);Key->SetEscapeKeys({EKeys::Tab});
		Key->SetButtonStyle(CCMenuStyle::Action());Key->SetKeySelectionText(FText::FromString(TEXT("请按键…")));
		Key->SetNoKeySpecifiedText(FText::FromString(TEXT("未设置")));
		auto Font=Key->GetTextStyle().Font;Font.Size=19;
		Key->SetTextStyle(FTextBlockStyle().SetFont(Font).SetColorAndOpacity(CCMenuStyle::Gold));
		auto* Size=WidgetTree->ConstructWidget<USizeBox>();Size->SetWidthOverride(164);Size->SetHeightOverride(44);Size->SetContent(Key);Row->AddChildToHorizontalBox(Size);
		KeySelectors.Add(Key);
	}
	KeySelectors[0]->OnKeySelected.AddDynamic(this,&ThisClass::Key0);KeySelectors[1]->OnKeySelected.AddDynamic(this,&ThisClass::Key1);
	KeySelectors[2]->OnKeySelected.AddDynamic(this,&ThisClass::Key2);KeySelectors[3]->OnKeySelected.AddDynamic(this,&ThisClass::Key3);
	KeySelectors[4]->OnKeySelected.AddDynamic(this,&ThisClass::Key4);KeySelectors[5]->OnKeySelected.AddDynamic(this,&ThisClass::Key5);
	auto* Reset=CCMenuStyle::Button(WidgetTree,TEXT("恢复默认键位"));Controls->AddChildToVerticalBox(Reset)->SetPadding(FMargin(0,8,0,12));Reset->OnClicked.AddDynamic(this,&ThisClass::ResetKeys);
	AddText(Controls,TEXT("移动  W A S D     跳跃  空格\nShift  轻按闪避，长按疾跑（最多5秒）\n视角  鼠标移动\n右键打开对策或返回；滚轮选择，左键或 Enter 确认。\n收起对策后，左键为普攻，数字 1～5 可快捷使用。\n菜单  Tab     地图  M"),16)->SetColorAndOpacity(CCMenuStyle::Muted);
	Status=AddText(Controls,TEXT("键位修改后自动保存到本机。"),16);Status->SetColorAndOpacity(CCMenuStyle::Gold);
	auto* RightSize=WidgetTree->ConstructWidget<USizeBox>();RightSize->SetWidthOverride(292);Columns->AddChildToHorizontalBox(RightSize);
	auto* Right=WidgetTree->ConstructWidget<UVerticalBox>();RightSize->SetContent(Right);
	auto Graphics=CCMenuStyle::Frame(WidgetTree,ArtData,FMargin(20));
	Right->AddChildToVerticalBox(Graphics.Root)->SetPadding(FMargin(0,0,0,12));
	auto* GraphicsList=WidgetTree->ConstructWidget<UVerticalBox>();Graphics.Body->SetContent(GraphicsList);
	AddText(GraphicsList,TEXT("画 质"),24)->SetColorAndOpacity(CCMenuStyle::Gold);
	UpscalerStatus=AddText(GraphicsList,TEXT("读取超分辨率…"),15);
	auto* UpscalerRow=WidgetTree->ConstructWidget<UHorizontalBox>();GraphicsList->AddChildToVerticalBox(UpscalerRow);
	auto* TSRButton=CCMenuStyle::Button(WidgetTree,TEXT("TSR 80%"),15);
	DLSSButton=CCMenuStyle::Button(WidgetTree,TEXT("DLSS Quality"),15);
	UpscalerRow->AddChildToHorizontalBox(TSRButton)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UpscalerRow->AddChildToHorizontalBox(DLSSButton)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	TSRButton->OnClicked.AddDynamic(this,&ThisClass::SelectTSR);
	DLSSButton->OnClicked.AddDynamic(this,&ThisClass::SelectDLSS);
	auto Map=CCMenuStyle::Frame(WidgetTree,ArtData,FMargin(24));Right->AddChildToVerticalBox(Map.Root)->SetPadding(FMargin(0,0,0,18));
	auto* MapList=WidgetTree->ConstructWidget<UVerticalBox>();Map.Body->SetContent(MapList);
	AddText(MapList,TEXT("舆 图"),26)->SetColorAndOpacity(CCMenuStyle::Gold);
	MapStatus=AddText(MapList,TEXT("读取地图状态…"),17);
	MapButton=CCMenuStyle::Button(WidgetTree,TEXT("切换小地图显示"),17);MapList->AddChildToVerticalBox(MapButton)->SetPadding(FMargin(0,0,0,12));MapButton->OnClicked.AddDynamic(this,&ThisClass::ToggleMap);
	AddText(MapList,TEXT("M 显示或隐藏右下角地图。\n关闭菜单后按 N 选择已解锁地区。\n小镇需先经传送装置实际抵达。"),16)->SetColorAndOpacity(CCMenuStyle::Muted);
	auto Saves=CCMenuStyle::Frame(WidgetTree,ArtData,FMargin(24));Right->AddChildToVerticalBox(Saves.Root);
	auto* SaveList=WidgetTree->ConstructWidget<UVerticalBox>();Saves.Body->SetContent(SaveList);
	AddText(SaveList,TEXT("角 色 存 档"),26)->SetColorAndOpacity(CCMenuStyle::Gold);
	SaveStatus=AddText(SaveList,TEXT("读取存档状态…"),16);
	SaveButton=CCMenuStyle::Button(WidgetTree,TEXT("保存当前进度"),17);SaveList->AddChildToVerticalBox(SaveButton)->SetPadding(FMargin(0,0,0,10));SaveButton->OnClicked.AddDynamic(this,&ThisClass::SaveProgress);
	LoadButton=CCMenuStyle::Button(WidgetTree,TEXT("读取已存进度"),17);SaveList->AddChildToVerticalBox(LoadButton)->SetPadding(FMargin(0,0,0,12));LoadButton->OnClicked.AddDynamic(this,&ThisClass::LoadProgress);
	AddText(SaveList,TEXT("读取将覆盖当前角色的未保存进度。\n启动恢复存档；切图前后自动保存。\n打开菜单时，场景仍继续运行。"),15)->SetColorAndOpacity(CCMenuStyle::Muted);
}
void UCC_OptionsWidget::BindController()
{
	auto* PC=Cast<ACC_PlayerController>(GetOwningPlayer());
	if (BoundController.Get()==PC) return;
	if (BoundController.IsValid()) BoundController->OnCombatKeysChanged.RemoveDynamic(this,&ThisClass::RefreshOptions);
	BoundController=PC;
	if (PC) PC->OnCombatKeysChanged.AddUniqueDynamic(this,&ThisClass::RefreshOptions);
}
TSharedRef<SWidget> UCC_OptionsWidget::RebuildWidget()
{
	// The Slate root is selected before NativeConstruct; create the UMG tree first.
	if (WidgetTree) BuildLayout();
	return Super::RebuildWidget();
}
void UCC_OptionsWidget::NativeConstruct(){Super::NativeConstruct();BindController();RefreshOptions();RefreshUpscaler();}
void UCC_OptionsWidget::NativeDestruct()
{
	if (BoundController.IsValid()) BoundController->OnCombatKeysChanged.RemoveDynamic(this,&ThisClass::RefreshOptions);
	BoundController.Reset();Super::NativeDestruct();
}
void UCC_OptionsWidget::NativeTick(const FGeometry& Geometry,float DeltaSeconds)
{
	Super::NativeTick(Geometry,DeltaSeconds);RefreshElapsed+=DeltaSeconds;
	if (RefreshElapsed>=.25f) {RefreshElapsed=0;BindController();RefreshOptions();}
}
bool UCC_OptionsWidget::IsSelectingKey() const
{
	for (const auto& Selector:KeySelectors) if (Selector && Selector->GetIsSelectingKey()) return true;
	return false;
}
UInputKeySelector* UCC_OptionsWidget::GetKeySelector(int32 Action) const{return KeySelectors.IsValidIndex(Action) ? KeySelectors[Action].Get() : nullptr;}
void UCC_OptionsWidget::RefreshOptions()
{
	auto* PC=Cast<ACC_PlayerController>(GetOwningPlayer());if (!PC || !Status) return;
	TGuardValue<bool> Guard(bUpdatingKeys,true);
	for (int32 I=0;I<KeySelectors.Num();++I)
		if (!KeySelectors[I]->GetIsSelectingKey() && KeySelectors[I]->GetSelectedKey().Key!=PC->GetCombatKey(I)) KeySelectors[I]->SetSelectedKey(FInputChord(PC->GetCombatKey(I)));
	auto* NavigationWidget=PC->GetWorldNavigationWidget();MapButton->SetIsEnabled(NavigationWidget!=nullptr);
	MapStatus->SetText(FText::FromString(!NavigationWidget ? TEXT("当前场景未启用地图") : NavigationWidget->IsMinimapVisible() ? TEXT("小地图：显示") : TEXT("小地图：隐藏")));
	auto* PS=PC->GetPlayerState<ACC_PlayerState>();const auto* Adventure=GetGameInstance<UCC_AdventureGameInstance>();
	const bool CanSave=PS && PS->Progression && GetWorld()->GetNetMode()==NM_Standalone && (!Adventure || !Adventure->bTravelling);
	SaveButton->SetIsEnabled(CanSave);LoadButton->SetIsEnabled(CanSave && UGameplayStatics::DoesSaveGameExist(TEXT("SpiritTales"),0));
	SaveStatus->SetText(FText::FromString(GetWorld()->GetNetMode()!=NM_Standalone ? TEXT("角色存档仅支持当前单机模式") : UGameplayStatics::DoesSaveGameExist(TEXT("SpiritTales"),0) ? TEXT("本机已有角色存档") : TEXT("本机尚无角色存档")));
}
void UCC_OptionsWidget::Message(const FString& Value,bool bError)
{
	FeedbackMessage=FText::FromString(Value);
	if (Status) {Status->SetColorAndOpacity(bError ? CCMenuStyle::Error : CCMenuStyle::Gold);Status->SetText(FeedbackMessage);}
	OnOptionsMessage(FeedbackMessage,bError);
}
void UCC_OptionsWidget::ApplyKey(int32 Action,FInputChord Chord)
{
	if (bUpdatingKeys) return;
	RebindAction(Action,Chord);
}
bool UCC_OptionsWidget::RebindAction(int32 Action,FInputChord Chord)
{
	auto* PC=Cast<ACC_PlayerController>(GetOwningPlayer());
	const bool OK=PC && !Chord.bCtrl && !Chord.bAlt && !Chord.bShift && !Chord.bCmd && PC->SetCombatKey(Action,Chord.Key);
	Message(OK ? TEXT("键位已应用并保存。") : TEXT("该键已占用或为保留按键，请重新选择。"),!OK);RefreshOptions();
	return OK;
}
void UCC_OptionsWidget::Key0(FInputChord C){ApplyKey(0,C);} void UCC_OptionsWidget::Key1(FInputChord C){ApplyKey(1,C);}
void UCC_OptionsWidget::Key2(FInputChord C){ApplyKey(2,C);} void UCC_OptionsWidget::Key3(FInputChord C){ApplyKey(3,C);}
void UCC_OptionsWidget::Key4(FInputChord C){ApplyKey(4,C);} void UCC_OptionsWidget::Key5(FInputChord C){ApplyKey(5,C);}
void UCC_OptionsWidget::ResetKeys(){if (auto* PC=Cast<ACC_PlayerController>(GetOwningPlayer())) PC->ResetCombatKeys();Message(TEXT("已恢复默认键位。"));RefreshOptions();}
void UCC_OptionsWidget::ToggleMap(){if (auto* PC=Cast<ACC_PlayerController>(GetOwningPlayer())) PC->ToggleMinimap();RefreshOptions();}
void UCC_OptionsWidget::SaveProgress()
{
	auto* PS=GetOwningPlayerState<ACC_PlayerState>();const bool OK=PS && PS->Progression && PS->Progression->SaveProgress();
	Message(OK ? TEXT("角色进度已保存。") : TEXT("保存失败，请确认角色已就绪且处于单机模式。"),!OK);RefreshOptions();
}
void UCC_OptionsWidget::LoadProgress()
{
	auto* PS=GetOwningPlayerState<ACC_PlayerState>();const bool OK=PS && PS->Progression && PS->Progression->LoadProgress();
	Message(OK ? TEXT("角色进度已读取。") : TEXT("读取失败：没有有效存档、角色未就绪或技能仍在冷却。"),!OK);RefreshOptions();
}
