// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class SimplygonMeshReduction : ModuleRules
{
	public SimplygonMeshReduction(TargetInfo Target)
	{
		BinariesSubFolder = "NotForLicensees";

		PublicIncludePaths.Add("Developer/SimplygonMeshReduction/Public");
		PrivateDependencyModuleNames.Add("Core");
		PrivateDependencyModuleNames.Add("CoreUObject");
		PrivateDependencyModuleNames.Add("Engine");
		PrivateDependencyModuleNames.Add("RenderCore");
		AddThirdPartyPrivateStaticDependencies(Target, "Simplygon");
		PrivateDependencyModuleNames.Add("RawMesh");
		PrivateIncludePathModuleNames.Add("MeshUtilities");
        PrivateIncludePathModuleNames.Add("MaterialUtilities");
        PrivateDependencyModuleNames.Add("MeshBoneReduction");
		//@third party code BEGIN SIMPLYGON
		PrivateIncludePaths.Add("Developer/SimplygonMeshReduction/Private");
		AddThirdPartyPrivateDynamicDependencies(Target, "PropertyEditor");
		PrivateDependencyModuleNames.Add("UnrealEd"); //For MaterialExporters etc
		PublicDependencyModuleNames.Add("RHI");
        PrivateDependencyModuleNames.Add("MaterialUtilities");
		//@third party code END SIMPLYGON

	}
}
