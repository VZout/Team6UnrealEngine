// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleManager.h"
#include "Editor/PropertyEditor/Public/IDetailCustomNodeBuilder.h"

struct FFlattenMaterial;

class IMaterialLODSettingsLayout : public IDetailCustomNodeBuilder, public TSharedFromThis<IMaterialLODSettingsLayout>
{
public:
	IMaterialLODSettingsLayout() {}
	virtual ~IMaterialLODSettingsLayout() {}

	virtual const FSimplygonMaterialLODSettings& GetSettings() = 0;
	virtual void UpdateSettings(const FSimplygonMaterialLODSettings& InSettings) = 0;
	virtual const FSimplygonChannelCastingSettings& GetSettingsFromChannelWidget(uint8 ChannelIndex ) const = 0;

	virtual void SetAllowReuseUVs(bool InAllowReuseUVs) = 0;
	virtual void SetAllowMultiMaterial(bool InAllowMultiMaterial) = 0;
	virtual void SetForceActive(bool InForceActive) = 0;
};

class FSimplygonSettingsLODInfo
{
public:
	TSharedPtr<FSimplygonRemeshingSettings> RemeshingSettings;
	TSharedPtr<FMeshReductionSettings> ReductionSettings;
	int32 LODIndex;
	bool bIsReduction;
	bool bIsRemeshing;

	FSimplygonSettingsLODInfo()
		:RemeshingSettings(NULL)
		,ReductionSettings(NULL)
		,LODIndex(0)
		,bIsReduction(false)
		,bIsRemeshing(false)
	{

	}
};


/**
 * All data used for simplification of single StaticMesh pocked into a single structure.
 * This structure has constructor/destructor implemented inside SimplygonUtilities module, so
 * it should be allocated with ISimplygonUtilities::CreateMeshReductionData().
 * We are trying to avoid inlining constructor and destructor to work with forward declaration
 * of FFlattenMaterial type. This is also the reason of making all methods virtual
 * (SimplygonUtilities is a module, but not a library).
 */
struct FMeshMaterialReductionData
{
	bool bReleaseMesh;
	bool bHasNegativeScale;

	// Source mesh data. Either 'Mesh' or 'LODModel' (but not both) should be non-null.
	struct FRawMesh* Mesh;
	FStaticLODModel* LODModel;
	UStaticMesh* StaticMesh;
	UStaticMeshComponent* StaticMeshComponent;

	// Texture coordinate data
	/*
	 * TexcoordBounds used when mesh has material which don't require rendering with vertex attributes, but
	 * which has non-repeating pattern, so picture at UV range [1..2] will not repeat one at range [0..1].
	 * In this case system determines texture coordinate range used by mesh and performs rendering at full
	 * UV range instead of simple [0..1]. This range is stored in TexcoordBounds field.
	 */
	FBox2D TexcoordBounds;
	/*
	 * When system performs rendering of complex material, for example when material uses mesh vertex color to
	 * produce some effect, SimplygonUtilities will generate new non-overlapping UV for a mesh. This UV is stored
	 * in NewUVs field.
	 */
	TArray<FVector2D> NewUVs;

	// Input material data
	TIndirectArray<FFlattenMaterial> FlattenMaterials;
	TArray<UMaterialInterface*> NonFlattenMaterials;

	// Output material data
	/*
	 * Maps index of input material to index of output material.
	 */
	TArray<int32> OutputMaterialMap;
	/*
	 * Blend modes for each output material.
	 */
	TArray<EBlendMode> OutputBlendMode;
	/*
	 * TwoSided property for each output material.
	 */
	TArray<bool> OutputTwoSided;
	/*
	 * Emissive color scale for each output material.
	 */
	TArray<float> OutputEmissiveScale;

	/*
	 * Analyze blend modes of all materials and build OutputMaterialMap and OutputBlendMode.
	 */
	virtual void BuildOutputMaterialMap(const FSimplygonMaterialLODSettings& MaterialLODSettings, bool bAllowMultiMaterial);

	/*
	 * Convert NonFlattenMaterials to FlattenMaterials.
	 */
	virtual void BuildFlattenMaterials(const FSimplygonMaterialLODSettings& MaterialLODSettings, int32 TextureWidth, int32 TextureHeight);

	/*
	 * Determine maximal emissive level and adjust all materials to fit it. Result - all materials with emissive has identical emissive scale.
	 * Called automatically by BuildFlattenMaterials().
	 */
	virtual void AdjustEmissiveChannels();

	FORCEINLINE int32 GetInputMaterialCount() const
	{
		return NonFlattenMaterials.Num();
	}

	FORCEINLINE int32 GetOutputMaterialCount() const
	{
		return OutputBlendMode.Num();
	}

	// Constructor and destructor
	FMeshMaterialReductionData();
	virtual ~FMeshMaterialReductionData();

	FORCEINLINE void Setup(FRawMesh* InMesh, bool InReleaseMesh = false)
	{
		Mesh = InMesh;
		bReleaseMesh = InReleaseMesh;
	}

	FORCEINLINE void Setup(FStaticLODModel* InLODModel)
	{
		LODModel = InLODModel;
	}
};

/**
 * The public interface to this module
 */
class ISimplygonUtilities : public IModuleInterface
{

public:

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline ISimplygonUtilities& Get()
	{
		return FModuleManager::LoadModuleChecked< ISimplygonUtilities >( "SimplygonUtilities" );
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded( "SimplygonUtilities" );
	}

	virtual IMaterialLODSettingsLayout* CreateMaterialLODSettingsLayout(int32 InLODIndex, bool InAllowReuseUVs = false) = 0;

	virtual void LoadSimplygonSettingsIni( const FString& FilePath,TArray<TSharedPtr<FSimplygonSettingsLODInfo>>& LODCOnfigInfo ) = 0;

	virtual FMeshMaterialReductionData* NewMeshReductionData() = 0;

	FORCEINLINE FMeshMaterialReductionData* CreateMeshReductionData(FRawMesh* InMesh, bool InReleaseMesh = false)
	{
		FMeshMaterialReductionData* Data = NewMeshReductionData();
		Data->Setup(InMesh, InReleaseMesh);
		return Data;
	}

	FORCEINLINE FMeshMaterialReductionData* CreateMeshReductionData(FStaticLODModel* InLODModel)
	{
		FMeshMaterialReductionData* Data = NewMeshReductionData();
		Data->Setup(InLODModel);
		return Data;
	}

	FORCEINLINE static float ConvertScreenSizeToDeviation(const float BoundingSphereRadius, const float SgScreenSize, const float UnitScale = 1.0f)
	{
		return UnitScale*(2.0f*BoundingSphereRadius) / SgScreenSize;
	}

	virtual void GetTextureCoordinateBoundsForRawMesh( const FRawMesh& InRawMesh, FBox2D& OutBounds) = 0;
	virtual void GetTextureCoordinateBoundsForSkeletalMesh(const FStaticLODModel& LODModel, FBox2D& OutBounds) = 0;

	virtual bool SimplygonGenerateUniqueUVs(
		FRawMesh& RawMesh,
		uint32 TexCoordIndex,
		float MaxDesiredStretch,
		uint32 DesiredTextureWidth,
		uint32 DesiredTextureHeight,
		uint32 DesiredGutterSpace
		) = 0;

	virtual bool SimplygonGenerateUniqueUVs(
		const FRawMesh& RawMesh,
		uint32 DesiredTextureWidth,
		uint32 DesiredTextureHeight,
		TArray<FVector2D>& OutTexCoords
		) = 0;

	virtual bool SimplygonGenerateUniqueUVs(
		const FStaticLODModel& LODModel,
		const FReferenceSkeleton& RefSkeleton,
		uint32 DesiredTextureWidth,
		uint32 DesiredTextureHeight,
		TArray<FVector2D>& OutTexCoords
		) = 0;

	virtual bool GenerateLODForStaticMesh(
		struct FRawMesh& OutReducedMesh,
		float& OutMaxDeviation,
		struct FRawMesh& InMesh,
		UStaticMesh* InStaticMesh,
		int32 LODIndex,
		const struct FMeshReductionSettings& ReductionSettings,
		const struct FSimplygonRemeshingSettings& RemeshingSettings) = 0;

	virtual bool GenerateMassiveLODMesh(
		const TArray<AActor*>& SourceActors,
		const struct FSimplygonRemeshingSettings& InMassiveLODSettings,
		FVector& OutProxyLocation,
		const FString& ProxyBasePackageName,
		TArray<UObject*>& OutAssetsToSync) = 0;

	virtual void OptimizeFlattenMaterialForUV(FFlattenMaterial& Material, const TArray<FVector2D>& TexCoords) = 0;

	virtual void FullyLoadMaterial(UMaterialInterface* Material) = 0;
	virtual void PurgeMaterialTextures(UMaterialInterface* Material) = 0;
	virtual void SaveMaterial(UMaterialInterface* Material) = 0;
};
