#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "../UI/CC_MenuStyle.h"
#include "Layout/ArrangedChildren.h"
#include "Widgets/SWidget.h"

namespace
{
bool FindLabelGeometry(const TSharedRef<SWidget>& Widget,const FGeometry& Geometry,
 const TSharedRef<SWidget>& Label,FVector2D& Size)
{
 if (Widget==Label) {Size=Geometry.GetLocalSize();return true;}
 FArrangedChildren Children(EVisibility::Visible);Widget->ArrangeChildren(Geometry,Children);
 for (int32 Index=0;Index<Children.Num();++Index)
  if (FindLabelGeometry(Children[Index].Widget,Children[Index].Geometry,Label,Size)) return true;
 return false;
}
}

// Exercises actual Slate font measurement and child allocation, without a game
// world, player, viewport, browser launch or save file access.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCC_MenuButtonLayout,"CrashCourse.UI.MenuButtonLayout",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCC_MenuButtonLayout::RunTest(const FString& Parameters)
{
 auto* Tree=NewObject<UWidgetTree>();
 struct FCase {const TCHAR* Label;int32 Font;FVector2D Size;bool Home;};
 const FCase Cases[]={
  {TEXT("开始游戏"),29,{432,76},true},
  {TEXT("宣传网站"),29,{432,76},true},
  {TEXT("退出游戏"),29,{432,76},true},
  {TEXT("01   休憩之地    · 当前所在"),21,{568,58},false},
  {TEXT("02   小镇    · 尚未抵达"),21,{568,58},false},
  {TEXT("03   山河社稷    · 可传送"),21,{568,58},false},
  {TEXT("04   万象天宫    · 当前所在"),21,{568,58},false},
  {TEXT("等待角色或技能就绪"),21,{568,56},false},
  {TEXT("切换小地图显示"),17,{244,52},false}
 };
 for (const auto& Case:Cases)
 {
  auto* Button=CCMenuStyle::Button(Tree,Case.Label,Case.Font);Tree->RootWidget=Button;
  auto* Label=CastChecked<UTextBlock>(Button->GetContent());
  if (Case.Home)
  {
   auto Style=CCMenuStyle::Action();Style.SetNormalPadding(FMargin(24,12));Button->SetStyle(Style);
   auto Font=Label->GetFont();Font.LetterSpacing=160;Label->SetFont(Font);
  }
  const auto Slate=Button->TakeWidget();const auto Text=Label->TakeWidget();
  // Change the live allocation repeatedly, including downscaling after upscaling.
  for (float Scale:{.5f,2.f/3.f,1.f,4.f/3.f,1.5f,.5f})
  {
   Slate->SlatePrepass(Scale);
   FVector2D Allotted;
   const FString Name=FString::Printf(TEXT("%s at scale %.3f"),Case.Label,Scale);
   if (!TestTrue(Name+TEXT(" has arranged text"),FindLabelGeometry(Slate,FGeometry::MakeRoot(Case.Size,FSlateLayoutTransform(Scale)),Text,Allotted))) continue;
   const FVector2D Desired=Text->GetDesiredSize();
   TestTrue(Name+TEXT(" entire label fits width"),Desired.X>0 && Desired.X<=Allotted.X+1.f);
   TestTrue(Name+TEXT(" entire label fits height"),Desired.Y>0 && Desired.Y<=Allotted.Y+1.f);
   TestTrue(Name+TEXT(" content uses available button width"),Allotted.X>=Case.Size.X-64.f);
  }
  Button->ReleaseSlateResources(true);
 }
 return true;
}
#endif
