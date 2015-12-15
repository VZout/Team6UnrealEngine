// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class MaterialUtilities : ModuleRules
{
	public MaterialUtilities(TargetInfo Target)
	{
        PrivateDependencyModuleNames.AddRange(
			new string [] {
				"Core",
				"CoreUObject",
				"Engine",
				"RenderCore",
                "RHI",
                "Landscape",
				//@third party code BEGIN SIMPLYGON
				"ShaderCore",
                "SimplygonUtilities",
				"UnrealEd" // for CreateMaterialTemplate
				//@third party code END SIMPLYGON
			}
		);

        PrivateIncludePathModuleNames.AddRange(
            new string[] {
                "Landscape",
				//@third party code BEGIN SIMPLYGON
                "SimplygonUtilities"
				//@third party code END SIMPLYGON
			}
        );

        CircularlyReferencedDependentModules.AddRange(
            new string[] {
                "Landscape",
                "SimplygonUtilities"
            }
        );
	}
}
