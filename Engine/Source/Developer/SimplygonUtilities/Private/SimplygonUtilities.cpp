// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SimplygonUtilitiesPrivatePCH.h"
#include "SSimplygonMassiveLODWidget.h"
#include "SimplygonSettingsINI.h"

#include "SimplygonUtilitiesModule.h"

#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"
#include "Editor/LevelEditor/Public/LevelEditor.h"
#include "SDockTab.h"

#include "RawMesh.h"
#include "MeshUtilities.h"
#include "MaterialUtilities.h"

#define LOCTEXT_NAMESPACE "SimplygonUtilities"

const FName FSimplygonUtilities::SimplygonMassiveLODTabId( TEXT( "SimplygonMassiveLOD" ) );

bool FSimplygonUtilities::CurrentlyRendering = false;
TArray<UTextureRenderTarget2D*> FSimplygonUtilities::RTPool;

IMPLEMENT_MODULE( FSimplygonUtilities, SimplygonUtilities )

//#define MULTIMATERIAL_TEST		1


void FSimplygonUtilities::StartupModule()
{
	FModuleManager::LoadModuleChecked<IEditorStyleModule>("EditorStyle");

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(SimplygonMassiveLODTabId, FOnSpawnTab::CreateRaw( this, &FSimplygonUtilities::SpawnSimplygonMassiveLODTool ) )
		.SetDisplayName( LOCTEXT( "SimplygonMassiveLODMainTitle", "Massive LOD" ) )
		.SetGroup( WorkspaceMenu::GetMenuStructure().GetStructureRoot() )
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "SimplygonIcon.TabIcon"));
	FCoreUObjectDelegates::PreGarbageCollect.AddRaw(this, &FSimplygonUtilities::OnPreGarbageCollect);
}

void FSimplygonUtilities::ShutdownModule()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(SimplygonMassiveLODTabId);
	FCoreUObjectDelegates::PreGarbageCollect.RemoveAll(this);
	ClearRTPool();
}

void FSimplygonUtilities::OnPreGarbageCollect()
{
	ClearRTPool();
}

IMaterialLODSettingsLayout* FSimplygonUtilities::CreateMaterialLODSettingsLayout(int32 InLODIndex, bool InExtendReductionSettings)
{
	return new FMaterialLODSettingsLayout(InLODIndex, InExtendReductionSettings);
}

void FSimplygonUtilities::LoadSimplygonSettingsIni( const FString& FilePath,TArray<TSharedPtr<FSimplygonSettingsLODInfo>>& LODCOnfigInfo )
{
	FSimplygonSettingsIni iniFile;
	iniFile.Read(FilePath, LODCOnfigInfo);
}

TSharedRef<SDockTab> FSimplygonUtilities::SpawnSimplygonMassiveLODTool( const FSpawnTabArgs& InArgs )
{
	TSharedRef<SDockTab> SimplygonEditorTab = SNew( SDockTab )
		.Icon( FEditorStyle::GetBrush("SimplygonIcon.TabIcon") )
		.TabRole(ETabRole::NomadTab)
		[
			SNew( SSimplygonMassiveLODWidget ) 
		];

	return SimplygonEditorTab;
}

FMeshMaterialReductionData::FMeshMaterialReductionData()
	: bReleaseMesh(false)
	, bHasNegativeScale(false)
	, Mesh(nullptr)
	, LODModel(nullptr)
	, StaticMesh(nullptr)
	, StaticMeshComponent(nullptr)
{
	TexcoordBounds.Init();
}

FMeshMaterialReductionData::~FMeshMaterialReductionData()
{
	if (bReleaseMesh)
	{
		delete Mesh;
	}
}

FMeshMaterialReductionData* FSimplygonUtilities::NewMeshReductionData()
{
	return new FMeshMaterialReductionData();
}

void FMeshMaterialReductionData::BuildOutputMaterialMap(const FSimplygonMaterialLODSettings& MaterialLODSettings, bool bAllowMultiMaterial)
{
	if (NonFlattenMaterials.Num() == 0)
	{
		UE_LOG(LogSimplygonUtilities, Error, TEXT("(Simplygon) BuildOutputMaterialMap failed - no input materials provided"));
		return;
	}

#if MULTIMATERIAL_TEST
	if (bAllowMultiMaterial)
	{
		// For testing: generate 1:1 output material map
		for (int32 SourceMaterialIndex = 0; SourceMaterialIndex < NonFlattenMaterials.Num(); SourceMaterialIndex++)
		{
			const UMaterialInterface* Material = NonFlattenMaterials[SourceMaterialIndex];
			if (Material)
			{
				OutputMaterialMap.Add(SourceMaterialIndex);
				OutputBlendMode.Add(Material->GetBlendMode());
				OutputTwoSided.Add(Material->IsTwoSided());
			}
			else
			{
				OutputMaterialMap.Add(0);
				OutputBlendMode.Add(BLEND_Opaque);
				OutputTwoSided.Add(false);
			}
		}
		return;
	}
#endif // MULTIMATERIAL_TEST

	// Check is opacity channel enabled in settings
	bool bHasOpacityEnabled = false;
	for (int32 ChannelIndex = 0; ChannelIndex < MaterialLODSettings.ChannelsToCast.Num(); ChannelIndex++)
	{
		const FSimplygonChannelCastingSettings& Channel = MaterialLODSettings.ChannelsToCast[ChannelIndex];
		if (Channel.bActive && Channel.MaterialChannel == ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_OPACITY)
		{
			bHasOpacityEnabled = true;
			break;
		}
	}

	// Find which blend mode groups are used
	bool bHasOpaqueBlend = false;
	bool bHasTranslucentBlend = false;
	EBlendMode MaxOpaqueBlend = BLEND_Opaque;
	EBlendMode MinTranslucentBlend = BLEND_MAX;
	EBlendMode MaxTranslucentBlend = BLEND_Opaque;

	for (int32 SourceMaterialIndex = 0; SourceMaterialIndex < NonFlattenMaterials.Num(); SourceMaterialIndex++)
	{
		const UMaterialInterface* Material = NonFlattenMaterials[SourceMaterialIndex];
		if (Material)
		{
			EBlendMode BlendMode = Material->GetBlendMode();
			if (IsTranslucentBlendMode(BlendMode))
			{
				bHasTranslucentBlend = true;
				if (BlendMode < MinTranslucentBlend)
				{
					MinTranslucentBlend = BlendMode;
				}
				if (BlendMode > MaxTranslucentBlend)
				{
					MaxTranslucentBlend = BlendMode;
				}
			}
			else
			{
				// either BLEND_Opaque or BLEND_Masked
				bHasOpaqueBlend = true;
				if (BlendMode > MaxOpaqueBlend)
				{
					MaxOpaqueBlend = BlendMode;
				}
			}
		}
	}

	OutputBlendMode.Empty();
	OutputTwoSided.Empty();

	if (!bHasOpacityEnabled)
	{
		// There's no point to bake translucent materials when opacity channel is not baked
		bHasTranslucentBlend = false;
		MaxOpaqueBlend = BLEND_Opaque;
	}

	if (!bAllowMultiMaterial ||												// not allowed by Simplygon
		!MaterialLODSettings.bAllowMultiMaterial ||							// disabled by user
		!bHasTranslucentBlend ||											// no translucency
		(!bHasOpaqueBlend && (MinTranslucentBlend == MaxTranslucentBlend)))	// no opacity, and single translucent mode
	{
		// Only single output material will be used.

		// Fill OutputMaterialMap with zeros
		OutputMaterialMap.Init(0, NonFlattenMaterials.Num());
		// Choose output material blend mode
		EBlendMode BlendMode = BLEND_Opaque;
		if (bHasOpaqueBlend && bHasTranslucentBlend)
		{
			// Both blend mode kinds are used, select BLEND_Masked
			BlendMode = BLEND_Masked;
		}
		else if (bHasTranslucentBlend)
		{
			// No opaque blends, can safely use translucency
			BlendMode = MinTranslucentBlend;
		}
		else
		{
			// Only opaque blend modes are used
			BlendMode = MaxOpaqueBlend;
		}
		OutputBlendMode.Add(BlendMode);
		// Choose TwoSided value
		bool bHasTwoSided = false;
		bool bHasSingleSided = false;
		for (int32 MaterialIndex = 0; MaterialIndex < NonFlattenMaterials.Num(); MaterialIndex++)
		{
			if (NonFlattenMaterials[MaterialIndex] && NonFlattenMaterials[MaterialIndex]->IsTwoSided())
			{
				bHasTwoSided = true;
			}
			else
			{
				bHasSingleSided = true;
			}
		}
		bool bTwoSided = bHasTwoSided && (!bHasSingleSided || MaterialLODSettings.bPreferTwoSideMaterials);
		OutputTwoSided.Add(bTwoSided);
		return;
	}

	// Multiple output materials could be used.
	// Analyze source material blend modes and decide if we want to merge them into a single material
	for (int32 SourceMaterialIndex = 0; SourceMaterialIndex < NonFlattenMaterials.Num(); SourceMaterialIndex++)
	{
		const UMaterialInterface* Material = NonFlattenMaterials[SourceMaterialIndex];
		if (Material)
		{
			EBlendMode BlendMode = Material->GetBlendMode();
			int32 OutputMaterialIndex;
			bool TwoSided = MaterialLODSettings.bPreferTwoSideMaterials ? Material->IsTwoSided() : false;
			for (OutputMaterialIndex = 0; OutputMaterialIndex < OutputBlendMode.Num(); OutputMaterialIndex++)
			{
				EBlendMode OtherBlendMode = OutputBlendMode[OutputMaterialIndex];
				if (OtherBlendMode == BlendMode)
				{
					// Found exact match
					break;
				}
				// BLEND_Masked and BLEND_Opaque are compatible
				if (OtherBlendMode == BLEND_Masked && BlendMode == BLEND_Opaque)
				{
					break;
				}
				if (OtherBlendMode == BLEND_Opaque && BlendMode == BLEND_Masked)
				{
					// replace existing blend mode with BLEND_Masked
					OutputBlendMode[OutputMaterialIndex] = BLEND_Masked;
					break;
				}
			}
			if (OutputMaterialIndex >= OutputBlendMode.Num())
			{
				// This blend mode was not found (for example if OutBlendModes is currently empty), add it.
				OutputBlendMode.Add(BlendMode);
				check(OutputBlendMode.Num() == OutputMaterialIndex+1);
			}
			OutputMaterialMap.Add(OutputMaterialIndex);
		}
		else
		{
			OutputMaterialMap.Add(0);
		}
	}

	// If no materials were processed, still need valid structures
	if (OutputMaterialMap.Num() && !OutputBlendMode.Num())
	{
		OutputBlendMode.Add(BLEND_Opaque);
		OutputTwoSided.Add(false);
		return;
	}

	// Select TwoSided property value for each of output materials
	for (int32 OutMaterialIndex = 0; OutMaterialIndex < OutputBlendMode.Num(); OutMaterialIndex++)
	{
		bool bHasTwoSided = false;
		bool bHasSingleSided = false;
		// Check all NonFlattenMaterials which will be mapped to current output material
		for (int32 InMaterialIndex = 0; InMaterialIndex < NonFlattenMaterials.Num(); InMaterialIndex++)
		{
			if (OutputMaterialMap[InMaterialIndex] == OutMaterialIndex)
			{
				if (NonFlattenMaterials[InMaterialIndex] && NonFlattenMaterials[InMaterialIndex]->IsTwoSided())
				{
					bHasTwoSided = true;
				}
				else
				{
					bHasSingleSided = true;
				}
			}
		}
		bool bTwoSided = bHasTwoSided && (!bHasSingleSided || MaterialLODSettings.bPreferTwoSideMaterials);
		OutputTwoSided.Add(bTwoSided);
	}
}

void FMeshMaterialReductionData::BuildFlattenMaterials(const FSimplygonMaterialLODSettings& MaterialLODSettings, int32 TextureWidth, int32 TextureHeight)
{
	UMaterialInterface* DefaultMaterial = Cast<UMaterialInterface>(UMaterial::GetDefaultMaterial(MD_Surface));

	for (int32 MaterialIndex = 0; MaterialIndex < NonFlattenMaterials.Num(); MaterialIndex++)
	{
		UMaterialInterface* Material = NonFlattenMaterials[MaterialIndex];
		// Correctly handle NULL materials - they should take their slot in material table
		if (!Material)
		{
			UE_LOG(LogSimplygonUtilities, Warning, TEXT("(Simplygon) Material #%d does not exist. Adding default material instead."), MaterialIndex);
			Material = DefaultMaterial;
		}
		FMaterialResource* Resource = Material->GetMaterialResource(GMaxRHIFeatureLevel);
		if (!Resource)
		{
			UE_LOG(LogSimplygonUtilities, Warning, TEXT("(Simplygon) %s is missing material resource. Adding default material instead."), *Material->GetName());
			Material = DefaultMaterial;
		}

		FFlattenMaterial* FlattenMaterial = FSimplygonUtilities::CreateFlattenMaterial(MaterialLODSettings, TextureWidth, TextureHeight);
		FlattenMaterials.Add(FlattenMaterial);

		// Render FFlattenMaterial
		FSimplygonUtilities::ExportMaterial(
			Material,
			this,
			MaterialIndex,
			*FlattenMaterial);
	}

	AdjustEmissiveChannels();
}

void FMeshMaterialReductionData::AdjustEmissiveChannels()
{
	// Iterate over all output materials.
	for (int32 OutputMaterialIndex = 0; OutputMaterialIndex < GetOutputMaterialCount(); OutputMaterialIndex++)
	{
		// Collect all input materials and determine maximal emissive scale
		TArray<FFlattenMaterial*> InputMaterials;
		float MaxEmissiveScale = 0.0f;
		for (int32 InputMaterialIndex = 0; InputMaterialIndex < GetInputMaterialCount(); InputMaterialIndex++)
		{
			if (OutputMaterialMap[InputMaterialIndex] == OutputMaterialIndex)
			{
				FFlattenMaterial& Material = FlattenMaterials[InputMaterialIndex];
				if (Material.EmissiveSamples.Num())
				{
					if (Material.EmissiveScale > MaxEmissiveScale)
					{
						MaxEmissiveScale = Material.EmissiveScale;
					}
					InputMaterials.Add(&Material);
				}
			}
		}

		OutputEmissiveScale.Add(MaxEmissiveScale);

		if (InputMaterials.Num() <= 1 || MaxEmissiveScale <= 0.001f)
		{
			// Nothing to do here.
			continue;
		}
		// Rescale all materials.
		for (int32 MaterialIndex = 0; MaterialIndex < InputMaterials.Num(); MaterialIndex++)
		{
			FFlattenMaterial* Material = InputMaterials[MaterialIndex];
			float Scale = Material->EmissiveScale / MaxEmissiveScale;
			if (FMath::Abs(Scale - 1.0f) < 0.01f)
			{
				// Difference is not noticeable for this material, or this material has maximal emissive level.
				continue;
			}
			// Rescale emissive data.
			for (int32 PixelIndex = 0; PixelIndex < Material->EmissiveSamples.Num(); PixelIndex++)
			{
				FColor& C = Material->EmissiveSamples[PixelIndex];
				C.R = FMath::RoundToInt(C.R * Scale);
				C.G = FMath::RoundToInt(C.G * Scale);
				C.B = FMath::RoundToInt(C.B * Scale);
			}
		}
	}
}

bool FSimplygonUtilities::SimplygonGenerateUniqueUVs(
	FRawMesh& RawMesh,
	uint32 TexCoordIndex,
	float MaxDesiredStretch,
	uint32 DesiredTextureWidth,
	uint32 DesiredTextureHeight,
	uint32 DesiredGutterSpace
	)
{
	bool bSuccess = false;

	ISimplygonModule& SimplygonModule = FModuleManager::LoadModuleChecked<ISimplygonModule>("SimplygonMeshReduction");
	
	IMeshMaterialReduction* SimplygonInterface = SimplygonModule.GetMeshMaterialReductionInterface();
	if (!SimplygonInterface)
	{
		return false;
	}

	bSuccess = SimplygonInterface->GenerateUniqueUVs(RawMesh, TexCoordIndex, MaxDesiredStretch, DesiredTextureWidth, DesiredTextureHeight, DesiredGutterSpace);

	return bSuccess;
}

bool FSimplygonUtilities::SimplygonGenerateUniqueUVs(
	const FRawMesh& RawMesh,
	uint32 DesiredTextureWidth,
	uint32 DesiredTextureHeight,
	TArray<FVector2D>& OutTexCoords
	)
{
	// Note: there's another possible approach to generate UV coordinates: see FLayoutUV. It is used by editor to generate lightmap UVs.
	bool bSuccess = false;

	ISimplygonModule& SimplygonModule = FModuleManager::LoadModuleChecked<ISimplygonModule>("SimplygonMeshReduction");

	IMeshMaterialReduction* SimplygonInterface = SimplygonModule.GetMeshMaterialReductionInterface();
	if (!SimplygonInterface)
	{
		return false;
	}
	
	bSuccess = SimplygonInterface->GenerateUniqueUVs(RawMesh, 0.8f, DesiredTextureWidth, DesiredTextureHeight, 3.0f, OutTexCoords);

	return bSuccess;
}

bool FSimplygonUtilities::SimplygonGenerateUniqueUVs(
	const FStaticLODModel& LODModel,
	const FReferenceSkeleton& RefSkeleton,
	uint32 DesiredTextureWidth,
	uint32 DesiredTextureHeight,
	TArray<FVector2D>& OutTexCoords
	)
{
	// Note: there's another possible approach to generate UV coordinates: see FLayoutUV. It is used by editor to generate lightmap UVs.
	bool bSuccess = false;

	ISimplygonModule& SimplygonModule = FModuleManager::LoadModuleChecked<ISimplygonModule>("SimplygonMeshReduction");

	IMeshMaterialReduction* SimplygonInterface = SimplygonModule.GetMeshMaterialReductionInterface();
	if (!SimplygonInterface)
	{
		return false;
	}

	bSuccess = SimplygonInterface->GenerateUniqueUVs(LODModel, RefSkeleton, 0.8f, DesiredTextureWidth, DesiredTextureHeight, 3.0f, OutTexCoords);

	return bSuccess;
}


bool FSimplygonUtilities::GenerateLODForStaticMesh(
	struct FRawMesh& OutReducedMesh,
	float& OutMaxDeviation,
	struct FRawMesh& InMesh,
	UStaticMesh* InStaticMesh,
	int32 LODIndex,
	const struct FMeshReductionSettings& ReductionSettings,
	const struct FSimplygonRemeshingSettings& RemeshingSettings)
{
	ISimplygonModule& SimplygonModule = FModuleManager::LoadModuleChecked<ISimplygonModule>("SimplygonMeshReduction");
	IMeshMaterialReduction* SimplygonInterface = SimplygonModule.GetMeshMaterialReductionInterface();
	IMeshReduction* SimplygonBasicInterface = SimplygonModule.GetMeshReductionInterface();

	if (!SimplygonInterface || !SimplygonBasicInterface)
	{
		return false;
	}

	UE_LOG(LogSimplygonUtilities, Log, TEXT("(Simplygon) Generating LOD%d"), LODIndex);

	TArray<FMeshMaterialReductionData*>			InputDataArray;
	FFlattenMaterial		OutFlattenMaterial;
	FSimplygonMaterialLODSettings				MaterialSettings;
	bool bBakedNewMaterial = false;

	FMeshMaterialReductionData& InputData = *CreateMeshReductionData(&InMesh, false);
	InputDataArray.Add(&InputData);

	if (SimplygonBasicInterface && ReductionSettings.bActive)
		MaterialSettings = ReductionSettings.MaterialLODSettings;
	else if (SimplygonInterface && RemeshingSettings.bActive)
		MaterialSettings = RemeshingSettings.MaterialLODSettings;

	// Find out how many sections are in the mesh.
	int32 MaxMaterialIndex = -1;
	for (int32 FaceIndex = 0; FaceIndex < InMesh.FaceMaterialIndices.Num(); FaceIndex++)
	{
		MaxMaterialIndex = FMath::Max<int32>(InMesh.FaceMaterialIndices[FaceIndex], MaxMaterialIndex);
	}

	//Find out if we are doing a cascaded or not
	int32 CascadedLODIndex = ReductionSettings.bActive ? ReductionSettings.BaseLODModel : 0;

	if (MaterialSettings.bActive)
	{
		//Get the materials those are used by the mesh
		for (int32 Index = 0; Index <= MaxMaterialIndex; ++Index)
		{
			// Get section info map using the correct LOD and material index.
			FMeshSectionInfo Info = InStaticMesh->SectionInfoMap.Get(CascadedLODIndex, Index);

			//Get the material from the static mesh using section info
			//and add it to the input material array
			UMaterialInterface* MaterialToAdd = InStaticMesh->GetMaterial(Info.MaterialIndex);
			InputData.NonFlattenMaterials.Add(MaterialToAdd);
		}

		InputData.BuildOutputMaterialMap(MaterialSettings, ReductionSettings.bActive);

		uint32 TextureWidth = MaterialSettings.GetTextureResolutionFromEnum(MaterialSettings.TextureWidth);
		uint32 TextureHeight = MaterialSettings.GetTextureResolutionFromEnum(MaterialSettings.TextureHeight);
		GetTextureCoordinateBoundsForRawMesh(InMesh, InputData.TexcoordBounds);
		if (!MaterialSettings.bReuseExistingCharts || RemeshingSettings.bActive)
		{
			SimplygonGenerateUniqueUVs(InMesh, TextureWidth, TextureHeight, InputData.NewUVs);
		}

		InputData.BuildFlattenMaterials(MaterialSettings, TextureWidth, TextureHeight);
	}

	TIndirectArray<FFlattenMaterial> OutFlattenMaterials;

	//QUICK FIX : Added ReductionSettings.PercentTriangles <= 1.0f check to make sure engliten trarget meshes are not reduced for those customer who are 
	//also using enlighten along Simplygon Integration.
	if (SimplygonBasicInterface && ReductionSettings.bActive && ReductionSettings.PercentTriangles <= 1.0f)
	{
		if (MaterialSettings.bActive)
		{
			SimplygonInterface->ReduceWithMaterialLOD(OutReducedMesh,
				OutMaxDeviation,
				InputData,
				ReductionSettings,
				OutFlattenMaterials);
			bBakedNewMaterial = true;
		}
		else
		{
			SimplygonBasicInterface->Reduce(OutReducedMesh, OutMaxDeviation, InMesh, ReductionSettings);
		}
	}
	else if (SimplygonInterface && RemeshingSettings.bActive)
	{
		bBakedNewMaterial = true;
		// Create single FFlattenMaterial instance and add it to OutFlattenMaterials array. This is needed to unify
		// code below - we'll iterate over OutFlattenMaterials array to build output materials.
		FFlattenMaterial* OutFlattenMaterial = new (OutFlattenMaterials) FFlattenMaterial;
		SimplygonInterface->ProxyLOD(InputDataArray,
			RemeshingSettings,
			OutReducedMesh,
			*OutFlattenMaterial);

		//Update deviation
		OutMaxDeviation = ISimplygonUtilities::ConvertScreenSizeToDeviation(InStaticMesh->GetBounds().SphereRadius, (float)RemeshingSettings.ScreenSize, 100.0f);
	}

	if (MaterialSettings.bActive)
	{
		// Remove all UVs except first 2 (main UV and lightmap UV) - these UV channels could be kept
		// from older LOD model version, with wrong vertex count
		for (int32 UVIndex = 2; UVIndex < MAX_MESH_TEXTURE_COORDS; UVIndex++)
		{
			OutReducedMesh.WedgeTexCoords[UVIndex].Empty();
		}
	}

	// Remove SectionInfoMap entries for current LOD
	if (LODIndex > 0)
	{
		int32 SectionsToClean = InStaticMesh->Materials.Num();
		for (int32 SectionIndex = 0; SectionIndex < SectionsToClean; SectionIndex++)
		{
			InStaticMesh->SectionInfoMap.Remove(LODIndex, SectionIndex);
		}
	}

	//Need to remove the old baked material if it exists
	FString BakedMaterialExt = FString::Printf(TEXT("_SgBakedMaterial_LOD%d"), LODIndex); //Should be a global in SimplygonUtilities
	for (int32 MaterialIndex = 0; MaterialIndex < InStaticMesh->Materials.Num(); ++MaterialIndex)
	{
		UMaterialInterface* Material = InStaticMesh->GetMaterial(MaterialIndex);
		if (!Material || !Material->GetName().Contains(BakedMaterialExt))
		{
			continue;
		}
		// Remove the material from UStaticMesh::Materials array
		InStaticMesh->Materials.RemoveAt(MaterialIndex);
		// Update affected indices in SectionInfoMap
		for (TMap<uint32,FMeshSectionInfo>::TIterator It(InStaticMesh->SectionInfoMap.Map); It; ++It)
		{
			FMeshSectionInfo& SectionInfo = It.Value();
			if (SectionInfo.MaterialIndex >= MaterialIndex)
			{
				SectionInfo.MaterialIndex--;
			}
		}
	}

	if (bBakedNewMaterial)
	{
		for (int32 OutMaterialIndex = 0; OutMaterialIndex < OutFlattenMaterials.Num(); OutMaterialIndex++)
		{
			const FFlattenMaterial& OutFlattenMaterial = OutFlattenMaterials[OutMaterialIndex];
			FString AbsoluteBakedMaterialName = FPackageName::GetLongPackagePath(InStaticMesh->GetOutermost()->GetName());
			AbsoluteBakedMaterialName += TEXT("/SimplygonMaterials/") + InStaticMesh->GetName() + BakedMaterialExt;
			if (OutFlattenMaterials.Num() > 1)
			{
				// Append material index to name
				AbsoluteBakedMaterialName += FString::Printf(TEXT("_%d"), OutMaterialIndex);
			}

			//Create baked material
			UMaterialInterface* BakedMaterial = FMaterialUtilities::SgCreateMaterial(OutFlattenMaterial, NULL, AbsoluteBakedMaterialName, RF_Public | RF_Standalone);

			//Save the package silently
			SaveMaterial(BakedMaterial);

			// flag the property (Materials) we're modifying so that not all of the object is rebuilt.
			UProperty* ChangedProperty = NULL;
			ChangedProperty = FindField<UProperty>(UStaticMesh::StaticClass(), "Materials");
			check(ChangedProperty);
			InStaticMesh->PreEditChange(ChangedProperty);

			//slot index will always be 0 for a remeshed asset
			int32 SlotIndex = OutMaterialIndex;
			FMeshSectionInfo Info = InStaticMesh->SectionInfoMap.Get(LODIndex, SlotIndex);

			if (LODIndex == 0)
			{
				check(Info.MaterialIndex == SlotIndex);
				check(InStaticMesh->Materials.IsValidIndex(SlotIndex));
				InStaticMesh->Materials[SlotIndex] = BakedMaterial;
			}
			else
			{
				int32 NewMaterialIndex = InStaticMesh->Materials.Add(BakedMaterial);
				Info.MaterialIndex = NewMaterialIndex;
				InStaticMesh->SectionInfoMap.Set(LODIndex, SlotIndex, Info);
			}
		}
	}
	else
	{
		// Current LOD mesh uses parent's material. Should create SectionInfo data for newly
		// generated LOD. This is only required when parent LOD model is not BaseLOD - this
		// case is correctly processed by SectionInfoMap structure itself.
		if (LODIndex > 0 && CascadedLODIndex > 0)
		{
			// Material baking is disabled, so material count will not be changed
			for (int32 Index = 0; Index <= MaxMaterialIndex; ++Index)
			{
				// Get section info map using the correct LOD and material index.
				FMeshSectionInfo Info = InStaticMesh->SectionInfoMap.Get(CascadedLODIndex, Index);
				// Store the same infor for new LOD.
				InStaticMesh->SectionInfoMap.Set(LODIndex, Index, Info);
			}
		}
	}

	//Cleanup
	for (int32 Index = 0; Index < InputDataArray.Num(); ++Index)
	{
		delete InputDataArray[Index]; // really, there should be only 1 element in this array
	}

	UE_LOG(LogSimplygonUtilities, Log, TEXT("(Simplygon) LOD%d generated successfully"), LODIndex);
	return true;
}


bool FSimplygonUtilities::GenerateMassiveLODMesh(
	const TArray<AActor*>& SourceActors,
	const struct FSimplygonRemeshingSettings& InMassiveLODSettings,
	FVector& OutProxyLocation,
	const FString& ProxyBasePackageName,
	TArray<UObject*>& OutAssetsToSync)
{
	return FMassiveLODSettingsLayout::GenerateMassiveLODMesh(
		SourceActors,
		InMassiveLODSettings,
		OutProxyLocation,
		ProxyBasePackageName,
		OutAssetsToSync);
}

#undef LOCTEXT_NAMESPACE
