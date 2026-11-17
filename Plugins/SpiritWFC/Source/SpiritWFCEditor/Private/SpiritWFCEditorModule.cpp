#include "Modules/ModuleManager.h"
#include "SWFCLayoutPainter.h"
#include "ToolMenus.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"

namespace
{
const FName PainterTab(TEXT("SpiritWFCLayoutPainter"));
}
class FSpiritWFCEditorModule : public IModuleInterface
{
  public:
	virtual void StartupModule() override
	{
		FGlobalTabmanager::Get()
		    ->RegisterNomadTabSpawner(
		        PainterTab, FOnSpawnTab::CreateLambda(
		                        [](const FSpawnTabArgs&)
		                        { return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(SWFCLayoutPainter)]; }))
		    .SetDisplayName(FText::FromString(TEXT("WFC 网格场景编辑器")))
		    .SetMenuType(ETabSpawnerMenuType::Hidden);
		UToolMenus::RegisterStartupCallback(
		    FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FSpiritWFCEditorModule::RegisterMenus));
	}
	virtual void ShutdownModule() override
	{
		UToolMenus::UnRegisterStartupCallback(this);
		UToolMenus::UnregisterOwner(this);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(PainterTab);
	}
	void RegisterMenus()
	{
		FToolMenuOwnerScoped Owner(this);
		auto& Section = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools")->FindOrAddSection("SpiritWFC");
		Section.AddMenuEntry(
		    "SpiritWFCLayoutPainter", FText::FromString(TEXT("WFC 网格场景编辑器")),
		    FText::FromString(TEXT("手绘道路、广场和建筑位置，再计算生成地图")), FSlateIcon(),
		    FUIAction(FExecuteAction::CreateLambda([]() { FGlobalTabmanager::Get()->TryInvokeTab(PainterTab); })));
	}
};
IMPLEMENT_MODULE(FSpiritWFCEditorModule, SpiritWFCEditor)
