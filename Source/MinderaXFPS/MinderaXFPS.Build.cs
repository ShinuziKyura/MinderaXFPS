// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MinderaXFPS : ModuleRules
{
	public MinderaXFPS(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new[]
			{
				"AIModule",
				"Core", 
				"CoreUObject",
				"Engine", 
				"EnhancedInput",
				"InputCore",
				"NavigationSystem",
			}
		);
	}
}
