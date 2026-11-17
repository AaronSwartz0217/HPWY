// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CrashCourse : ModuleRules
{
	public CrashCourse(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"EnhancedInput",
			"GameplayAbilities",
			"GameplayTasks",
			"GameplayTags",
			"AnimGraphRuntime",
			"Niagara",
			"UMG"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "AIModule", "NavigationSystem", "Slate", "SlateCore", "HTTP", "Json", "MoviePlayer", "SpiritWFC" });
		
		if (Target.bBuildEditor) PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "AnimGraph", "BlueprintGraph", "KismetCompiler", "NiagaraEditor" });
	}
}
