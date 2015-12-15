// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleInterface.h"
#include "Runtime/CoreUObject/Public/UObject/ObjectBase.h"
#include "Runtime/Engine/Classes/Engine/EngineTypes.h"
#include "Runtime/Engine/Classes/Engine/Texture.h"
#include "Runtime/Engine/Public/SceneTypes.h"

struct FFlattenMaterial
{
	FFlattenMaterial()
		: DiffuseSize(512, 512)
		, NormalSize(512, 512)
		, MetallicSize(0, 0)
		, RoughnessSize(0, 0)
		, SpecularSize(0, 0)
		//@third party code BEGIN SIMPLYGON
		, OpacitySize(0, 0)
		, EmissiveSize(0, 0)
		, AOSize(0, 0)
		, BlendMode(BLEND_Opaque)
		, EmissiveScale(1.0f)
		, bTwoSided(false)
		//@third party code END SIMPLYGON
	{}

	//@third party code BEGIN SIMPLYGON
	void ReleaseData()
	{
		DiffuseSamples.Empty();
		NormalSamples.Empty();
		MetallicSamples.Empty();
		RoughnessSamples.Empty();
		SpecularSamples.Empty();
		OpacitySamples.Empty();
		EmissiveSamples.Empty();
		AOSamples.Empty();
	}
	//@third party code END SIMPLYGON
		
	FGuid			MaterialId;
		
	FIntPoint		DiffuseSize;
	FIntPoint		NormalSize;
	FIntPoint		MetallicSize;	
	FIntPoint		RoughnessSize;	
	FIntPoint		SpecularSize;
	//@third party code BEGIN SIMPLYGON
	FIntPoint		OpacitySize;
	FIntPoint		EmissiveSize;
	FIntPoint		AOSize;

	bool			bTwoSided;
	EBlendMode		BlendMode;
	float			EmissiveScale;
	//@third party code END SIMPLYGON
			
	TArray<FColor>	DiffuseSamples;
	TArray<FColor>	NormalSamples;
	TArray<FColor>	MetallicSamples;
	TArray<FColor>	RoughnessSamples;
	TArray<FColor>	SpecularSamples;
	//@third party code BEGIN SIMPLYGON
	TArray<FColor>	OpacitySamples;
	TArray<FColor>	EmissiveSamples;
	TArray<FColor>	AOSamples;
	//@third party code END SIMPLYGON
};


//@third party code BEGIN SIMPLYGON
class ISimplygonMaterialProxyHook
{
public:
	/**
	 * Compile-time hooks
	 */
	virtual void AddReferencedTextures(TArray<UTexture*>& ReferencedTextures) = 0;
	virtual FMaterialCompiler* CreateCompilerProxy(FMaterialCompiler* InCompiler) = 0;
	virtual bool ShouldCache(EShaderPlatform Platform, const FShaderType* ShaderType, const FVertexFactoryType* VertexFactoryType) const = 0;
	virtual bool RequiresSynchronousCompilation() const = 0;
	/**
	 * Render-time hooks
	 */
	virtual bool GetVectorValue(const FName ParameterName, FLinearColor* OutValue, const FMaterialRenderContext& Context) const = 0;
	virtual bool GetScalarValue(const FName ParameterName, float* OutValue, const FMaterialRenderContext& Context) const = 0;
	virtual bool GetTextureValue(const FName ParameterName,const UTexture** OutValue, const FMaterialRenderContext& Context) const = 0;
};
//@third party code END SIMPLYGON


class UMaterialInterface;
class UMaterial;
class UTexture2D;
class UTextureRenderTarget2D;
class UWorld;
class ALandscapeProxy;
class ULandscapeComponent;
class FPrimitiveComponentId;

/**
 * Material utilities
 */
class FMaterialUtilities
{
public:
	/**
	 * Whether material utilities support exporting specified material blend mode and property 
	 */
	MATERIALUTILITIES_API static bool SupportsExport(EBlendMode InBlendMode, EMaterialProperty InMaterialProperty);

	/**
	 * Renders specified material property into texture
	 *
	 * @param InWorld				World object to use for material property rendering
	 * @param InMaterial			Target material
	 * @param InMaterialProperty	Material property to render
	 * @param InRenderTarget		Render target to render to
	 * @param OutBMP				Output array of rendered samples 
	 * @return						Whether operation was successful
	 */
	MATERIALUTILITIES_API static bool ExportMaterialProperty(UWorld* InWorld, UMaterialInterface* InMaterial, EMaterialProperty InMaterialProperty, UTextureRenderTarget2D* InRenderTarget, TArray<FColor>& OutBMP);

	/**
	 * Renders specified material property into texture
	 *
	 * @param InWorld				World object to use for material property rendering
	 * @param InMaterial			Target material
	 * @param InMaterialProperty	Material property to render
	 * @param OutBMP				Output array of rendered samples 
	 * @return						Whether operation was successful
	 */

	MATERIALUTILITIES_API static bool ExportMaterialProperty(UWorld* InWorld, UMaterialInterface* InMaterial, EMaterialProperty InMaterialProperty,  FIntPoint& OutSize, TArray<FColor>& OutBMP);

	/**
	 * Flattens specified material
	 *
	 * @param InWorld				World object to use for material rendering
	 * @param InMaterial			Target material
	 * @param OutFlattenMaterial	Output flattened material
	 * @return						Whether operation was successful
	 */
	MATERIALUTILITIES_API static bool ExportMaterial(UWorld* InWorld, UMaterialInterface* InMaterial, FFlattenMaterial& OutFlattenMaterial);

	/**
	 * Flattens specified landscape material
	 *
	 * @param InLandscape			Target landscape
	 * @param HiddenPrimitives		Primitives to hide while rendering scene to texture
	 * @param OutFlattenMaterial	Output flattened material
	 * @return						Whether operation was successful
	 */
	MATERIALUTILITIES_API static bool ExportLandscapeMaterial(ALandscapeProxy* InLandscape, const TSet<FPrimitiveComponentId>& HiddenPrimitives, FFlattenMaterial& OutFlattenMaterial);
	
	/**
 	 * Generates a texture from an array of samples 
	 *
	 * @param Outer					Outer for the material and texture objects, if NULL new packages will be created for each asset
	 * @param AssetLongName			Long asset path for the new texture
	 * @param Size					Resolution of the texture to generate (must match the number of samples)
	 * @param Samples				Color data for the texture
	 * @param CompressionSettings	Compression settings for the new texture
	 * @param LODGroup				LODGroup for the new texture
	 * @param Flags					ObjectFlags for the new texture
	 * @param bSRGB					Whether to set the bSRGB flag on the new texture
	 * @param SourceGuidHash		(optional) Hash (stored as Guid) to use part of the texture source's DDC key.
	 * @return						The new texture.
	 */
	MATERIALUTILITIES_API static UTexture2D* CreateTexture(UPackage* Outer, const FString& AssetLongName, FIntPoint Size, const TArray<FColor>& Samples, TextureCompressionSettings CompressionSettings, TextureGroup LODGroup, EObjectFlags Flags, bool bSRGB, const FGuid& SourceGuidHash = FGuid());

	/**
	 * Creates UMaterial object from a flatten material
	 *
	 * @param Outer					Outer for the material and texture objects, if NULL new packages will be created for each asset
	 * @param BaseName				BaseName for the material and texture objects, should be a long package name in case Outer is not specified
	 * @param Flags					Object flags for the material and texture objects.
	 * @param OutGeneratedAssets	List of generated assets - material, textures
	 * @return						Returns a pointer to the constructed UMaterial object.
	 */
	MATERIALUTILITIES_API static UMaterial* CreateMaterial(const FFlattenMaterial& InFlattenMaterial, UPackage* InOuter, const FString& BaseName, EObjectFlags Flags, TArray<UObject*>& OutGeneratedAssets);

	/**
	* Creates bakes textures for a ULandscapeComponent
	*
	* @param LandscapeComponent		The component to bake textures for
	* @return						Whether operation was successful
	*/
	MATERIALUTILITIES_API static bool ExportBaseColor(ULandscapeComponent* LandscapeComponent, int32 TextureSize, TArray<FColor>& OutSamples);

	//@third party code BEGIN SIMPLYGON
	MATERIALUTILITIES_API static FMaterialRenderProxy* CreateExportMaterialProxy(UMaterialInterface* InMaterial, EMaterialProperty InMaterialProperty, ISimplygonMaterialProxyHook* InProxyHook = nullptr);
	MATERIALUTILITIES_API static UMaterialInterface* SgCreateMaterial(const FFlattenMaterial& InFlattenMaterial, UPackage* InOuter, const FString& BaseName, EObjectFlags Flags);
	//@third party code END SIMPLYGON
};
