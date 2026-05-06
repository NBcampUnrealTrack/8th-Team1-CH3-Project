// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class Sparta_H : ModuleRules
{
	public Sparta_H(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Slate", "SlateCore" });
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Systems/Public"));
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Systems/Private"));

	}
}