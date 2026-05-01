// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Sparta_H : ModuleRules
{
	public Sparta_H(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
	}
}
