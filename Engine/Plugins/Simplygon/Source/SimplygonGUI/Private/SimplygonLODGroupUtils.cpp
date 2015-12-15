#include "SimplygonGUIPrivatePCH.h"
#include "SimplygonLODGroupUtils.h"
#include "TargetPlatform.h"
#include "StaticMeshResources.h"

#define LOCTEXT_NAMESPACE "SimplygonPlugin"

void FSimplygonLODGroupUtils::ApplyLODGroupToStaticMesh(UStaticMesh* StaticMesh, const int32 LODGroupIndex)
{
	if (!StaticMesh)
		return;

	TArray<FName> LODGroupNames;
	LODGroupNames.Reset();
	UStaticMesh::GetLODGroups(LODGroupNames);
	FName NewGroup = LODGroupNames[LODGroupIndex];

	if (StaticMesh->LODGroup != NewGroup)
	{
		StaticMesh->Modify();
		StaticMesh->LODGroup = NewGroup;

		const ITargetPlatform* Platform = GetTargetPlatformManagerRef().GetRunningTargetPlatform();
		check(Platform);
		const FStaticMeshLODGroup& GroupSettings = Platform->GetStaticMeshLODSettings().GetLODGroup(NewGroup);

		// Set the number of LODs to the default.
		int32 LODCount = GroupSettings.GetDefaultNumLODs();
		if (StaticMesh->SourceModels.Num() > LODCount)
		{
			int32 NumToRemove = StaticMesh->SourceModels.Num() - LODCount;
			StaticMesh->SourceModels.RemoveAt(LODCount, NumToRemove);
		}
		while (StaticMesh->SourceModels.Num() < LODCount)
		{
			new(StaticMesh->SourceModels) FStaticMeshSourceModel();
		}
		check(StaticMesh->SourceModels.Num() == LODCount);

		// Set reduction settings to the defaults.
		for (int32 LODIndex = 0; LODIndex < LODCount; ++LODIndex)
		{
			StaticMesh->SourceModels[LODIndex].ReductionSettings = GroupSettings.GetDefaultSettings(LODIndex);
		}
		StaticMesh->bAutoComputeLODScreenSize = true;
		StaticMesh->LightMapResolution = GroupSettings.GetDefaultLightMapResolution();

		StaticMesh->PostEditChange();
	}
}

#undef LOCTEXT_NAMESPACE