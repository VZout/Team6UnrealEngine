#pragma once

#include "SimplygonUtilities.h"

class IMaterialLODSettingsLayout;

/************************************************************************/
/* Simplygon Utilities Module                                           */
/************************************************************************/

struct FExportMaterialProxyCache
{
	// Material proxies for each property. Note: we're not handling all properties here,
	// so hold only up to MP_AmbientOcclusion inclusive.
	FMaterialRenderProxy* Proxies[MP_AmbientOcclusion+1];

	FExportMaterialProxyCache()
	{
		FMemory::Memzero(Proxies);
	}

	~FExportMaterialProxyCache()
	{
		Release();
	}

	void Release()
	{
		for (int32 PropertyIndex = 0; PropertyIndex < ARRAY_COUNT(Proxies); PropertyIndex++)
		{
			FMaterialRenderProxy* Proxy = Proxies[PropertyIndex];
			if (Proxy)
			{
				delete Proxy;
				Proxies[PropertyIndex] = nullptr;
			}
		}
	}
};

class FSimplygonUtilities : public ISimplygonUtilities
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	virtual IMaterialLODSettingsLayout* CreateMaterialLODSettingsLayout(int32 LODIndex, bool InExtendReductionSettings) override;

	virtual void LoadSimplygonSettingsIni(const FString& FilePath,TArray<TSharedPtr<FSimplygonSettingsLODInfo>>& LODCOnfigInfo) override;

	virtual FMeshMaterialReductionData* NewMeshReductionData() override;

	virtual void GetTextureCoordinateBoundsForRawMesh(
		const FRawMesh& InRawMesh,
		FBox2D& OutBounds) override;

	virtual void GetTextureCoordinateBoundsForSkeletalMesh(
		const FStaticLODModel& LODModel,
		FBox2D& OutBounds) override;

	virtual bool SimplygonGenerateUniqueUVs(
		FRawMesh& RawMesh,
		uint32 TexCoordIndex,
		float MaxDesiredStretch,
		uint32 DesiredTextureWidth,
		uint32 DesiredTextureHeight,
		uint32 DesiredGutterSpace
		) override;

	virtual bool SimplygonGenerateUniqueUVs(
		const FRawMesh& RawMesh,
		uint32 DesiredTextureWidth,
		uint32 DesiredTextureHeight,
		TArray<FVector2D>& OutTexCoords
		) override;

	virtual bool SimplygonGenerateUniqueUVs(
		const FStaticLODModel& LODModel,
		const FReferenceSkeleton& RefSkeleton,
		uint32 DesiredTextureWidth,
		uint32 DesiredTextureHeight,
		TArray<FVector2D>& OutTexCoords
		) override;

	virtual bool GenerateLODForStaticMesh(
		struct FRawMesh& OutReducedMesh,
		float& OutMaxDeviation,
		struct FRawMesh& InMesh,
		UStaticMesh* InStaticMesh,
		int32 LODIndex,
		const struct FMeshReductionSettings& ReductionSettings,
		const struct FSimplygonRemeshingSettings& RemeshingSettings) override;

	virtual bool GenerateMassiveLODMesh(
		const TArray<AActor*>& SourceActors,
		const struct FSimplygonRemeshingSettings& InMassiveLODSettings,
		FVector& OutProxyLocation,
		const FString& ProxyBasePackageName,
		TArray<UObject*>& OutAssetsToSync);

	virtual void FullyLoadMaterial(UMaterialInterface* Material) override;
	virtual void PurgeMaterialTextures(UMaterialInterface* Material) override;
	virtual void SaveMaterial(UMaterialInterface* Material) override;

	static FFlattenMaterial* CreateFlattenMaterial(
		const FSimplygonMaterialLODSettings& InMaterialLODSettings,
		int32 InTextureWidth,
		int32 InTextureHeight);

	static void SaveFlattenMaterial(const FFlattenMaterial& Material, const TCHAR* Filename);

	/**
	 * Combine colors of 2 flatten materials, put resulting material to DstMaterial. Code assumes that there's no pixels
	 * which has non-black values in both combined materials, i.e. materials are not overlapping.
	 */
	static void MergeFlattenMaterials(FFlattenMaterial& DstMaterial, const FFlattenMaterial& SrcMaterial);

	/**
	 * Convert Unreal material to FFlatterMaterial using simple square renderer.
	 */
	static bool ExportMaterial(
		UMaterialInterface* InMaterial,
		FFlattenMaterial& OutFlattenMaterial,
		FExportMaterialProxyCache* CompiledMaterial = nullptr);

	/**
	 * Convert Unreal material to FFlatterMaterial using simple square renderer, with taking into account TexCoord bounds.
	 */
	static bool ExportMaterial(
		UMaterialInterface* InMaterial,
		const FBox2D& InTexcoordBounds,
		FFlattenMaterial& OutFlattenMaterial,
		FExportMaterialProxyCache* CompiledMaterial = nullptr);

	/**
	 * Convert Unreal material to FFlattenMaterial using Skeletal or Static Mesh geometry for rendering more detailed materials.
	 */
	static bool ExportMaterial(
		UMaterialInterface* InMaterial,
		const FMeshMaterialReductionData* InMeshData,
		int32 InMaterialIndex,
		FFlattenMaterial& OutFlattenMaterial,
		FExportMaterialProxyCache* CompiledMaterial = nullptr);

	static void AnalyzeMaterial(
		UMaterialInterface* InMaterial,
		const FSimplygonMaterialLODSettings& InMaterialLODSettings,
		int32& OutNumTexCoords,
		bool& OutUseVertexColor,
		bool& OutHasNonRepeatingPattern,
		bool& OutUsePerActorData);

	/**
	 * Tries to optimize the samples in FFlattenMaterial (will set to const value if all samples are equal). Will use
	 * UV of the mesh to detect pixels which are not belong to any mesh triangle and those colors could be ignored.
	 */
	virtual void OptimizeFlattenMaterialForUV(FFlattenMaterial& Material, const TArray<FVector2D>& TexCoords) override;

private:
	//Spawn Massive LOD Tool
	TSharedRef<SDockTab> SpawnSimplygonMassiveLODTool( const FSpawnTabArgs& InArgs );

	static bool ExportMaterial(
		struct FMaterialData& InMaterialData,
		FFlattenMaterial& OutFlattenMaterial,
		FExportMaterialProxyCache* CompiledMaterial = nullptr);

	/**
	 * Render black-white texture which represents UV of some mesh. Black texel means it is used by one of triangles, black - unused.
	 */
	static void RenderUVMask(const TArray<FVector2D>& InTexCoords, const FIntPoint& InTargetSize, TArray<FColor>& OutSamples);

	static bool RenderMaterialPropertyToTexture(
		struct FMaterialData& InMaterialData,
		bool bInCompileOnly,
		EMaterialProperty InMaterialProperty,
		bool bInForceLinearGamma,
		EPixelFormat InPixelFormat,
		FIntPoint& InTargetSize,
		TArray<FColor>& OutSamples);

	static void FullyLoadMaterialStatic(UMaterialInterface* Material);

	void OnPreGarbageCollect();

	static UTextureRenderTarget2D* CreateRenderTarget(bool bInForceLinearGamma, EPixelFormat InPixelFormat, const FIntPoint& InTargetSize);

	static void ClearRTPool();

private:
	//Simplygon editor tab Id
	static const FName SimplygonMassiveLODTabId;

	static bool CurrentlyRendering;
	static TArray<UTextureRenderTarget2D*> RTPool;
};
