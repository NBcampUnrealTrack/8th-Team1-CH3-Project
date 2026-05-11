// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class Sparta_H : ModuleRules
{
	public Sparta_H(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"EnhancedInput", 
			"UMG", 
			"Slate", 
			"SlateCore", 
			"AIModule",
			"Niagara",
			"NavigationSystem"
		});

		// 모든 주요 폴더의 Public 경로를 포함하여 헤더 찾기를 쉽게 만듭니다.
		PublicIncludePaths.AddRange(new string[]
		{
			ModuleDirectory,
			Path.Combine(ModuleDirectory, "Characters/Public"),
			Path.Combine(ModuleDirectory, "Enemy/Public"),
			Path.Combine(ModuleDirectory, "Framework/Public"),
			Path.Combine(ModuleDirectory, "Systems/Public"),
			Path.Combine(ModuleDirectory, "UI/Public"),
			Path.Combine(ModuleDirectory, "Weapon/Public")
		});

		PrivateIncludePaths.AddRange(new string[]
		{
			Path.Combine(ModuleDirectory, "Characters/Private"),
			Path.Combine(ModuleDirectory, "Enemy/Private"),
			Path.Combine(ModuleDirectory, "Framework/Private"),
			Path.Combine(ModuleDirectory, "Systems/Private"),
			Path.Combine(ModuleDirectory, "UI/Private"),
			Path.Combine(ModuleDirectory, "Weapon/Private")
		});
	}
}
