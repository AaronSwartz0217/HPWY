using UnrealBuildTool;
public class SpiritWFCEditor : ModuleRules
{
 public SpiritWFCEditor(ReadOnlyTargetRules Target) : base(Target)
 {
  PCHUsage=PCHUsageMode.UseExplicitOrSharedPCHs;
  PrivateDependencyModuleNames.AddRange(new[]{"Core","CoreUObject","Engine","SpiritWFC","Slate","SlateCore",
   "UnrealEd","ToolMenus","PropertyEditor","AssetRegistry","InputCore","AssetTools","AppFramework"});
 }
}
