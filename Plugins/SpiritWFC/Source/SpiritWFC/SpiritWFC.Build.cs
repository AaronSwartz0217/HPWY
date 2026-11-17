using UnrealBuildTool;
using System.IO;

public class SpiritWFC : ModuleRules
{
 public SpiritWFC(ReadOnlyTargetRules Target) : base(Target)
 {
  PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
  PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
  RuntimeDependencies.Add(Path.Combine(PluginDirectory, "ThirdPartyNotices", "Marian42-LICENSE.txt"), StagedFileType.NonUFS);
 }
}
