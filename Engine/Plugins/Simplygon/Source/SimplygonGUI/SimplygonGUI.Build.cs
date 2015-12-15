// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class SimplygonGUI : ModuleRules
	{
		public SimplygonGUI(TargetInfo Target)
		{
			PublicIncludePaths.AddRange(
				new string[] {
					// ... add public include paths required here ...
					"SimplygonGUI/Public",
				}
				);

			PrivateIncludePaths.AddRange(
				new string[] {
					"SimplygonGUI/Private",
					// ... add other private include paths required here ...
				}
				);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject", 
					"Engine", 
					"Slate",
					"SlateCore",
					"EditorStyle",
					"TargetPlatform",
					// ... add other public dependencies that you statically link with here ...
				}
				);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					// ... add private dependencies that you statically link with here ...
					"Core",
					"CoreUObject",
					"InputCore",
					"Engine",
					"RenderCore",
					"UnrealEd",
					"LevelEditor",
					"ContentBrowser",
				}
				);

			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
				}
				);

			PrivateIncludePathModuleNames.AddRange(
				new string[] {
				}
				);
		}
	}
}