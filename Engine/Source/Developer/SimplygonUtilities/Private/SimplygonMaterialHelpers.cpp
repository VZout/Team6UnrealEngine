#include "SimplygonUtilitiesPrivatePCH.h"
#include "EngineModule.h"
#include "LocalVertexFactory.h"
#include "MeshBatch.h"
#include "RendererInterface.h"
#include "RawMesh.h"
#include "MaterialUtilities.h"
#include "MaterialCompiler.h"
#include "ShaderCompiler.h"   // for GShaderCompilingManager
#include "ThumbnailHelpers.h" // for FClassThumbnailScene
#include "PrimitiveUniformShaderParameters.h" // GIdentityPrimitiveUniformBuffer
#include "ShaderParameterUtils.h"
#include "LightMap.h"

#include "SimplygonUtilitiesModule.h"

//#define SHOW_WIREFRAME_MESH 1 // should use "reuse UVs" for better visualization effect
//#define VISUALIZE_DILATION 1
//#define SAVE_INTERMEDIATE_TEXTURES		TEXT("C:/TEMP")
#define USE_LOCALVERTEXFACTORY		1		// for debugging

// Could use ERHIFeatureLevel::Num, but this will cause assertion when baking material on startup.
#define MATERIAL_FEATURE_LEVEL		ERHIFeatureLevel::SM5

//PRAGMA_DISABLE_OPTIMIZATION

static FName SimplygonActorWorldPositionName(TEXT("Simplygon_ActorWorldPosition"));
static FName SimplygonAOMaskTexture(TEXT("Simplygon_AOMaskTexture"));

static UTexture2D* GetBlackTexture()
{
	return LoadObject<UTexture2D>(NULL, TEXT("/Engine/EngineResources/Black.Black"), NULL, LOAD_None, NULL);
}

/*
 * Proxy material compiler. Reference: FLightmassMaterialCompiler.
 */
struct FSimplygonMaterialCompiler : public FProxyMaterialCompiler
{
	FSimplygonMaterialCompiler(FMaterialCompiler* InCompiler)
		: FProxyMaterialCompiler(InCompiler)
		, BlackTexture(nullptr)
	{}

	virtual int32 WorldPosition(EWorldPositionIncludedOffsets WorldPositionIncludedOffsets) override
	{
		return Compiler->Constant3(0.0f,0.0f,0.0f);
	}

	virtual int32 ObjectWorldPosition() override
	{
		return Compiler->Constant3(0.0f,0.0f,0.0f);
	}

	virtual int32 ObjectRadius() override
	{
		return Compiler->Constant(500);
	}

	virtual int32 ObjectBounds() override
	{
		return Compiler->Constant3(0,0,0);
	}

	virtual int32 DistanceCullFade() override
	{
		return Compiler->Constant(1.0f);
	}

	virtual int32 ActorWorldPosition() override
	{
		return Compiler->ForceCast(
			Compiler->VectorParameter(SimplygonActorWorldPositionName, FLinearColor(0.0f, 0.0f, 0.0f)),
			MCT_Float3);
	}

	virtual int32 CameraVector() override
	{
		return Compiler->Constant3(0.0f,0.0f,1.0f);
	}

	virtual int32 LightVector() override
	{
		return Compiler->Constant3(1.0f,0.0f,0.0f);
	}

	virtual int32 ReflectionVector() override
	{
		return Compiler->Constant3(0.0f,0.0f,-1.0f);
	}

	virtual int32 AtmosphericFogColor(int32 WorldPosition) override
	{
		return INDEX_NONE;
	}

	// Note: any MaterialExpression is passed to compiler through CallExpression() method. As reference, see
	// FHLSLMaterialTranslator::CallExpression(). It is possible to fully override particular nodes with overriding that method.

	// Missing FProxyMaterialCompiler functions
	virtual int32 ReflectionAboutCustomWorldNormal(int32 CustomWorldNormal, int32 bNormalizeCustomWorldNormal) override
	{
		return Compiler->Constant3(0.0f,0.0f,-1.0f);
	}

	virtual int32 ParticleRelativeTime() override
	{
		return Compiler->Constant(0.0f);
	}

	virtual int32 ParticleMotionBlurFade() override
	{
		return Compiler->Constant(1.0f);
	}

	virtual int32 ParticleDirection() override
	{
		return Compiler->Constant3(0.0f, 0.0f, 0.0f);
	}

	virtual int32 ParticleSpeed() override
	{
		return Compiler->Constant(0.0f);
	}
	
	virtual int32 ParticleSize() override
	{
		return Compiler->Constant2(0.0f,0.0f);
	}

	virtual int32 AccessCollectionParameter(UMaterialParameterCollection* ParameterCollection, int32 ParameterIndex, int32 ComponentIndex) override
	{
		if (!ParameterCollection || ParameterIndex == -1)
		{
			return INDEX_NONE;
		}

		// Collect names of all parameters
		TArray<FName> ParameterNames;
		ParameterCollection->GetParameterNames(ParameterNames, /*bVectorParameters=*/ false);
		int32 NumScalarParameters = ParameterNames.Num();
		ParameterCollection->GetParameterNames(ParameterNames, /*bVectorParameters=*/ true);

		// Find a parameter corresponding to ParameterIndex/ComponentIndex pair
		int32 Index;
		for (Index = 0; Index < ParameterNames.Num(); Index++)
		{
			FGuid ParameterId = ParameterCollection->GetParameterId(ParameterNames[Index]);
			int32 CheckParameterIndex, CheckComponentIndex;
			ParameterCollection->GetParameterIndex(ParameterId, CheckParameterIndex, CheckComponentIndex);
			if (CheckParameterIndex == ParameterIndex && CheckComponentIndex == ComponentIndex)
			{
				// Found
				break;
			}
		}
		if (Index >= ParameterNames.Num())
		{
			// Not found, should not happen
			return INDEX_NONE;
		}

		// Create code for parameter
		if (Index < NumScalarParameters)
		{
			const FCollectionScalarParameter* ScalarParameter = ParameterCollection->GetScalarParameterByName(ParameterNames[Index]);
			check(ScalarParameter);
			return Constant(ScalarParameter->DefaultValue);
		}
		else
		{
			const FCollectionVectorParameter* VectorParameter = ParameterCollection->GetVectorParameterByName(ParameterNames[Index]);
			check(VectorParameter);
			const FLinearColor& Color = VectorParameter->DefaultValue;
			return Constant4(Color.R, Color.G, Color.B, Color.A);
		}
	}

	virtual int32 PrecomputedAOMask() override
	{
		if (BlackTexture == nullptr)
		{
			BlackTexture = GetBlackTexture();
		}
		int32 Code = Compiler->TextureSample(
			Compiler->TextureParameter(SimplygonAOMaskTexture, BlackTexture),
			Compiler->LightmapUVs(),
			SAMPLERTYPE_Masks);
		if (Code != INDEX_NONE)
		{
			Code = Compiler->ForceCast(Code, MCT_Float1);
			// Reference: LightmapCommon.usf, GetAOMaterialMask()
			// "Undo sqrt which allocated more precision toward 0"
			Code = Compiler->Mul(Code, Code);
		}
		return Code;
	}

	virtual int32 SceneColor(int32 Offset, int32 UV, bool bUseOffset) override
	{
		return Compiler->Constant3(0.0f, 0.0f, 0.0f);
	}

private:
	UTexture* BlackTexture;
};

/*
 * Hook class used to alter FExportMaterialProxy behavior. Probably worth copy-pasting FExportMaterialProxy here
 * and doing modifications locally, or putting its declaration to MaterialUtilities.h with possibility to
 * inherit that class.
 */
class FSimplygonMaterialProxyHook : public ISimplygonMaterialProxyHook
{
public:
	static void SetStaticMeshComponent(const UStaticMeshComponent* InStaticMeshComponent)
	{
		StaticMeshComponent = InStaticMeshComponent;
	}

	virtual void AddReferencedTextures(TArray<UTexture*>& ReferencedTextures) override
	{
		ReferencedTextures.Add(GetBlackTexture());
	}

	virtual FMaterialCompiler* CreateCompilerProxy(FMaterialCompiler* InCompiler) override
	{
		return new FSimplygonMaterialCompiler(InCompiler);
	}

	/*
	 * Hooked FExportMaterialProxy methods.
	 */
	virtual bool ShouldCache(EShaderPlatform Platform, const FShaderType* ShaderType, const FVertexFactoryType* VertexFactoryType) const override
	{
		// Original FExportMaterialProxy code returns 'true' for every ShaderType/VertexFactoryType combination. But this
		// could crash ('assert') shader compiler when trying to compile some shader combinations which are really not
		// needed for FExportMaterialProxy (in particular, when compiling MP_Opacity property). So, disable unneeded
		// shaders. This will speed up shader compilation as well.
		// Reference: FLightmassMaterialProxy::ShouldCache()
		if (VertexFactoryType == nullptr)
		{
			return false;
		}
#if !USE_LOCALVERTEXFACTORY
		if (VertexFactoryType->GetFName() == FName(TEXT("FSimplygonVertexFactory")))
#else
		if (VertexFactoryType == FindVertexFactoryType(FName(TEXT("FLocalVertexFactory"), FNAME_Find)))
#endif
		{
			// we only need the non-light-mapped, base pass, local vertex factory shaders for drawing an opaque Material Tile
			if (FCString::Stristr(ShaderType->GetName(), TEXT("BasePassVSFNoLightMapPolicy")))
			{
				return true;
			}
			else if (FCString::Stristr(ShaderType->GetName(), TEXT("BasePassPSFNoLightMapPolicy")))
			{
				return true;
			}
		}

		return false;
	}
	virtual bool RequiresSynchronousCompilation() const override
	{
		return false;
	}
	virtual bool GetVectorValue(const FName ParameterName, FLinearColor* OutValue, const FMaterialRenderContext& Context) const override
	{
//		UE_LOG(LogSimplygonUtilities, Log, TEXT("VectorValue: %s"), *ParameterName.ToString());
		if (ParameterName == SimplygonActorWorldPositionName && StaticMeshComponent != nullptr)
		{
			AActor* Owner = StaticMeshComponent->GetOwner();
			if (Owner)
			{
				*OutValue = FLinearColor(Owner->GetActorLocation());
				return true;
			}
		}
		return false;
	}
	virtual bool GetScalarValue(const FName ParameterName, float* OutValue, const FMaterialRenderContext& Context) const override
	{
//		UE_LOG(LogSimplygonUtilities, Log, TEXT("ScalarValue: %s"), *ParameterName.ToString());
		return false;
	}
	virtual bool GetTextureValue(const FName ParameterName,const UTexture** OutValue, const FMaterialRenderContext& Context) const override
	{
//		UE_LOG(LogSimplygonUtilities, Log, TEXT("TextureValue: %s"), *ParameterName.ToString());
		if (ParameterName == SimplygonAOMaskTexture)
		{
			FLightMap2D* LightMap2D = nullptr;
			if (StaticMeshComponent != nullptr)
			{
				FLightMap* LightMap = StaticMeshComponent->LODData[0].LightMap;
				if (LightMap)
				{
					LightMap2D = LightMap->GetLightMap2D();
					*OutValue = LightMap2D->GetAOMaterialMaskTexture();
					return true;
				}
			}
		}
		return false;
	}

private:
	static const UStaticMeshComponent* StaticMeshComponent;
};

const UStaticMeshComponent* FSimplygonMaterialProxyHook::StaticMeshComponent = nullptr;

// Vertex data. Reference: TileRendering.cpp

/** 
* Vertex data for a screen quad.
*/
struct FMaterialMeshVertex
{
	FVector			Position;
	FPackedNormal	TangentX,
					TangentZ;
	uint32			Color;
	FVector2D		TextureCoordinate[MAX_STATIC_TEXCOORDS];

	void SetTangents(const FVector& InTangentX, const FVector& InTangentY, const FVector& InTangentZ)
	{
		TangentX = InTangentX;
		TangentZ = InTangentZ;
		// store determinant of basis in w component of normal vector
		TangentZ.Vector.W = GetBasisDeterminantSign(InTangentX,InTangentY,InTangentZ) < 0.0f ? 0 : 255;
	}
};

/**
 * A dummy vertex buffer used to give the FMeshVertexFactory something to reference as a stream source.
 */
class FMaterialMeshVertexBuffer : public FVertexBuffer
{
public:
	virtual void InitRHI() override
	{
		FRHIResourceCreateInfo CreateInfo;
		VertexBufferRHI = RHICreateVertexBuffer(sizeof(FMaterialMeshVertex),BUF_Static,CreateInfo);
	}
};
TGlobalResource<FMaterialMeshVertexBuffer> GDummyMeshRendererVertexBuffer;

#if !USE_LOCALVERTEXFACTORY

/**
 * Vertex factory for rendering meshes with materials.
 */
class FSimplygonVertexFactoryVertexShaderParameters : public FVertexFactoryShaderParameters
{
public:
	virtual void Bind(const FShaderParameterMap& ParameterMap) override
	{
		LightMapCoordinateScaleBiasParameter.Bind(ParameterMap,TEXT("LightMapCoordinateScaleBias"));
	}

	virtual void SetMesh(FRHICommandList& RHICmdList, FShader* VertexShader,const class FVertexFactory* VertexFactory,const class FSceneView& View,const struct FMeshBatchElement& BatchElement,uint32 DataFlags) const override
	{
		SetLightMapScale(RHICmdList, VertexShader, (FLightMap2D*)BatchElement.UserData);
	}

	void Serialize(FArchive& Ar) override
	{
		Ar << LightMapCoordinateScaleBiasParameter;
	}

	virtual uint32 GetSize() const override { return sizeof(*this); }

	void SetLightMapScale(FRHICommandList& RHICmdList, FShader* VertexShader, FLightMap2D* LightMap) const
	{
		FVector2D LightmapCoordinateScale(1.0f, 1.0f);
		FVector2D LightmapCoordinateBias(0.0f, 0.0f);
		if (LightMap)
		{
			LightmapCoordinateScale = LightMap->GetCoordinateScale();
			LightmapCoordinateBias = LightMap->GetCoordinateBias();
		}
		SetShaderValue(RHICmdList, VertexShader->GetVertexShader(),LightMapCoordinateScaleBiasParameter,FVector4(
			LightmapCoordinateScale.X,
			LightmapCoordinateScale.Y,
			LightmapCoordinateBias.X,
			LightmapCoordinateBias.Y
			));
	}

private:
	FShaderParameter LightMapCoordinateScaleBiasParameter;
};

/*
 * This code is disabled: it works, but shader code will reject passed AOMask samples at some point,
 * so it is not possible to use that without shader source modifications (details: BasePassPixelShader.usf,
 * Main() - it skips saving MaterialParameters.AOMaterialMask value when compiled with MATERIAL_SHADINGMODEL_UNLIT).
 * This is only the reason why FSimplygonMaterialCompiler and FSimplygonMaterialProxyHook has AOMask code.

class FSimplygonVertexFactoryPixelShaderParameters : public FVertexFactoryShaderParameters
{
public:
	virtual void Bind(const FShaderParameterMap& ParameterMap) override
	{
		AOMaterialMaskTexture.Bind(ParameterMap,TEXT("AOMaterialMaskTexture"));
		AOMaterialMaskSampler.Bind(ParameterMap,TEXT("AOMaterialMaskSampler"));
	}

	virtual void SetMesh(FRHICommandList& RHICmdList, FShader* PixelShader, const FVertexFactory* VertexFactory, const FSceneView& View, const FMeshBatchElement& BatchElement, uint32 DataFlags) const override
	{
		SetAOMaterialMaskTexture(RHICmdList, PixelShader, (FLightMap2D*)BatchElement.UserData);
	}

	void Serialize(FArchive& Ar) override
	{
		Ar << AOMaterialMaskTexture;
		Ar << AOMaterialMaskSampler;
	}

	virtual uint32 GetSize() const override { return sizeof(*this); }

	void SetAOMaterialMaskTexture(FRHICommandList& RHICmdList, FShader* PixelShader, FLightMap2D* LightMap) const
	{
		const UTexture2D* AOMaterialMaskTextureValue = LightMap ? LightMap->GetAOMaterialMaskTexture() : nullptr;

		FTexture* TextureResource = GBlackTexture;

		if (AOMaterialMaskTextureValue)
		{
			TextureResource = AOMaterialMaskTextureValue->Resource;
		}

		SetTextureParameter(
			RHICmdList, 
			PixelShader->GetPixelShader(),
			AOMaterialMaskTexture,
			AOMaterialMaskSampler,
			TextureResource
			);
	}

private:
	FShaderResourceParameter AOMaterialMaskTexture;
	FShaderResourceParameter AOMaterialMaskSampler;
};*/

class FSimplygonVertexFactory : public FLocalVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FSimplygonVertexFactory);
public:
	static bool ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType)
	{
		if (!Material->GetFriendlyName().StartsWith(TEXT("FExportMaterialRenderer")))
		{
			return false;
		}
		return FLocalVertexFactory::ShouldCache(Platform, Material, ShaderType);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		FLocalVertexFactory::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
		// Set some lightmap parameters to allow lightmap UV in shader
		// Reference: TLightMapPolicy::ModifyCompilationEnvironment()
		OutEnvironment.SetDefine(TEXT("LQ_TEXTURE_LIGHTMAP"),TEXT("1"));
		OutEnvironment.SetDefine(TEXT("NUM_LIGHTMAP_COEFFICIENTS"), NUM_LQ_LIGHTMAP_COEF);
	}

	static FVertexFactoryShaderParameters* ConstructShaderParameters(EShaderFrequency ShaderFrequency)
	{
		switch (ShaderFrequency)
		{
		case SF_Vertex:
			return new FSimplygonVertexFactoryVertexShaderParameters();
//		case SF_Pixel:
//			return new FSimplygonVertexFactoryPixelShaderParameters();
		default:
			return NULL;
		}
	}
};

IMPLEMENT_VERTEX_FACTORY_TYPE(FSimplygonVertexFactory,"LocalVertexFactory",true,true,true,true,true);

#endif // !USE_LOCALVERTEXFACTORY

static void InitVertexFactory(FLocalVertexFactory& VertexFactory, UStaticMesh* StaticMesh)
{
	// This method is always executed in context of rendering thread.

	FLocalVertexFactory::DataType Data;

	// position
	Data.PositionComponent = FVertexStreamComponent(
		&GDummyMeshRendererVertexBuffer,
		STRUCT_OFFSET(FMaterialMeshVertex,Position),
		sizeof(FMaterialMeshVertex),
		VET_Float3
		);
	// tangents
	Data.TangentBasisComponents[0] = FVertexStreamComponent(
		&GDummyMeshRendererVertexBuffer,
		STRUCT_OFFSET(FMaterialMeshVertex,TangentX),
		sizeof(FMaterialMeshVertex),
		VET_PackedNormal
		);
	Data.TangentBasisComponents[1] = FVertexStreamComponent(
		&GDummyMeshRendererVertexBuffer,
		STRUCT_OFFSET(FMaterialMeshVertex,TangentZ),
		sizeof(FMaterialMeshVertex),
		VET_PackedNormal
		);
	// color
	Data.ColorComponent = FVertexStreamComponent(
		&GDummyMeshRendererVertexBuffer,
		STRUCT_OFFSET(FMaterialMeshVertex,Color),
		sizeof(FMaterialMeshVertex),
		VET_Color
		);
	// UVs
	int32 UVIndex;
	for (UVIndex = 0; UVIndex < MAX_STATIC_TEXCOORDS - 1; UVIndex += 2)
	{
		Data.TextureCoordinates.Add(FVertexStreamComponent(
			&GDummyMeshRendererVertexBuffer,
			STRUCT_OFFSET(FMaterialMeshVertex,TextureCoordinate) + sizeof(FVector2D)* UVIndex,
			sizeof(FMaterialMeshVertex),
			VET_Float4
			));
	}
	// possible last UV channel if we have an odd number (by the way, MAX_STATIC_TEXCOORDS is even value, so most
	// likely the following code will never be executed)
	if (UVIndex < MAX_STATIC_TEXCOORDS)
	{
		Data.TextureCoordinates.Add(FVertexStreamComponent(
			&GDummyMeshRendererVertexBuffer,
			STRUCT_OFFSET(FMaterialMeshVertex,TextureCoordinate) + sizeof(FVector2D)* UVIndex,
			sizeof(FMaterialMeshVertex),
			VET_Float2
			));
	}
	// lightmap texture coordinate
	if (StaticMesh != nullptr && StaticMesh->LightMapCoordinateIndex >= 0)
	{
		Data.LightMapCoordinateComponent = FVertexStreamComponent(
			&GDummyMeshRendererVertexBuffer,
			STRUCT_OFFSET(FMaterialMeshVertex,TextureCoordinate) + sizeof(FVector2D) * StaticMesh->LightMapCoordinateIndex,
			sizeof(FMaterialMeshVertex),
			VET_Float2
			);
	}

	VertexFactory.SetData(Data);
}

/**
 * Structure holding all necessary data. Used here to avoid sending all of these parameters across deep call stack.
 */
struct FMaterialData
{
	/*
	 * Input data
	 */
	UMaterialInterface* Material;
	FExportMaterialProxyCache ProxyCache;
	const FRawMesh* Mesh;
	const FStaticLODModel* LODModel;
	const UStaticMeshComponent* StaticMeshComponent;
	int32 MaterialIndex;
	const FBox2D& TexcoordBounds;
	const TArray<FVector2D>& TexCoords;

	/*
	 * Output data
	 */
	float EmissiveScale;

	FMaterialData(
		UMaterialInterface* InMaterial,
		const FRawMesh* InMesh,
		const FStaticLODModel* InLODModel,
		int32 InMaterialIndex,
		const FBox2D& InTexcoordBounds,
		const TArray<FVector2D>& InTexCoords)
	:	Material(InMaterial)
	,	Mesh(InMesh)
	,	LODModel(InLODModel)
	,	StaticMeshComponent(nullptr)
	,	MaterialIndex(InMaterialIndex)
	,	TexcoordBounds(InTexcoordBounds)
	,	TexCoords(InTexCoords)
	,	EmissiveScale(0.0f)
	{}
};

/**
 * Canvas render item enqueued into renderer command list.
 */
class FSimplygonMaterialRenderItem : public FCanvasBaseRenderItem
{
public:
	FSimplygonMaterialRenderItem(
		FSceneViewFamily* InViewFamily,
		FMaterialData* InData,
		const FVector2D& InSize,
		const FMaterialRenderProxy* InMaterialRenderProxy,
		const FCanvas::FTransformEntry& InTransform=FCanvas::FTransformEntry(FMatrix::Identity) )
		:	Data(new FRenderData(
			InViewFamily,
			InData,
			InSize,
			InMaterialRenderProxy,
			InTransform))
	{}
	virtual ~FSimplygonMaterialRenderItem() override
	{}

private:
	class FRenderData
	{
	public:
		FRenderData(
			FSceneViewFamily* InViewFamily,
			FMaterialData* InData,
			const FVector2D& InSize,
			const FMaterialRenderProxy* InMaterialRenderProxy=NULL,
			const FCanvas::FTransformEntry& InTransform=FCanvas::FTransformEntry(FMatrix::Identity) )
			:	ViewFamily(InViewFamily)
			,	MeshData(InData)
			,	Size(InSize)
			,	MaterialRenderProxy(InMaterialRenderProxy)
			,	Transform(InTransform)
		{}
		FSceneViewFamily* ViewFamily;
		FMaterialData* MeshData;
		FVector2D Size;
		const FMaterialRenderProxy* MaterialRenderProxy;
		FCanvas::FTransformEntry Transform;
	};
	FRenderData* Data;	

public:
	// Reference: FCanvasTileItem::RenderMaterialTile
	static void EnqueueMaterialRender(
		class FCanvas* InCanvas,
		FSceneViewFamily* InViewFamily,
		FMaterialData* InData,
		const FVector2D& InSize,
		const FMaterialRenderProxy* InMaterialRenderProxy)
	{
		// get sort element based on the current sort key from top of sort key stack
		FCanvas::FCanvasSortElement& SortElement = InCanvas->GetSortElement(InCanvas->TopDepthSortKey());
		// get the current transform entry from top of transform stack
		const FCanvas::FTransformEntry& TopTransformEntry = InCanvas->GetTransformStack().Top();	
		// create a render batch
		FSimplygonMaterialRenderItem* RenderBatch = new FSimplygonMaterialRenderItem(
			InViewFamily,
			InData,
			InSize,
			InMaterialRenderProxy,
			TopTransformEntry);
		SortElement.RenderBatchArray.Add(RenderBatch);
	}

	static int32 FillStaticMeshData(
		bool bDuplicateTris,
		const FRawMesh& RawMesh,
		FRenderData& Data,
		TArray<FMaterialMeshVertex>& OutVerts,
		TArray<int32>& OutIndices)
	{
		// count triangles for selected material
		int32 NumTris = 0;
		int32 TotalNumFaces = RawMesh.FaceMaterialIndices.Num();
		for (int32 FaceIndex = 0; FaceIndex < TotalNumFaces; FaceIndex++)
		{
			if (RawMesh.FaceMaterialIndices[FaceIndex] == Data.MeshData->MaterialIndex)
			{
				NumTris++;
			}
		}
		if (NumTris == 0)
		{
			// there's nothing to do here
			return 0;
		}

		// vertices are not shared between triangles in FRawMesh, so NumVerts is NumTris * 3
		int32 NumVerts = NumTris * 3;

		// reserve renderer data
		OutVerts.Empty(NumVerts);
		OutIndices.Empty(bDuplicateTris ? NumVerts * 2: NumVerts);

		float U = 0;
		float V = 0;
		float SizeU = 1;
		float SizeV = 1;
		if (Data.MeshData->TexcoordBounds.bIsValid)
		{
			U = Data.MeshData->TexcoordBounds.Min.X;
			V = Data.MeshData->TexcoordBounds.Min.Y;
			SizeU = Data.MeshData->TexcoordBounds.Max.X - Data.MeshData->TexcoordBounds.Min.X;
			SizeV = Data.MeshData->TexcoordBounds.Max.Y - Data.MeshData->TexcoordBounds.Min.Y;
		}
		float ScaleX = Data.Size.X;
		float ScaleY = Data.Size.Y;
		uint32 DefaultColor = FColor::White.DWColor();

		// count number of texture coordinates for this mesh
		int32 NumTexcoords = 1;
		for (NumTexcoords = 1; NumTexcoords < MAX_STATIC_TEXCOORDS; NumTexcoords++)
		{
			if (RawMesh.WedgeTexCoords[NumTexcoords].Num() == 0)
				break;
		}

		bool bUseNewUVs = Data.MeshData->TexCoords.Num() > 0;
		if (bUseNewUVs)
		{
			check(Data.MeshData->TexCoords.Num() == RawMesh.WedgeTexCoords[0].Num());
		}

		// add vertices
		int32 VertIndex = 0;
		bool bHasVertexColor = (RawMesh.WedgeColors.Num() > 0);
		for (int32 FaceIndex = 0; FaceIndex < TotalNumFaces; FaceIndex++)
		{
			if (RawMesh.FaceMaterialIndices[FaceIndex] == Data.MeshData->MaterialIndex)
			{
				for (int32 Corner = 0; Corner < 3; Corner++)
				{
					int32 SrcVertIndex = FaceIndex * 3 + Corner;
					// add vertex
					FMaterialMeshVertex* Vert = new(OutVerts) FMaterialMeshVertex();
					if (!bUseNewUVs)
					{
						// compute vertex position from original UV
						const FVector2D& UV = RawMesh.WedgeTexCoords[0][SrcVertIndex];
						Vert->Position.Set((UV.X - U) * ScaleX, (UV.Y - V) * ScaleY, 0);
					}
					else
					{
						const FVector2D& UV = Data.MeshData->TexCoords[SrcVertIndex];
						Vert->Position.Set(UV.X * ScaleX, UV.Y * ScaleY, 0);
					}
					Vert->SetTangents(RawMesh.WedgeTangentX[SrcVertIndex], RawMesh.WedgeTangentY[SrcVertIndex], RawMesh.WedgeTangentZ[SrcVertIndex]);
					for (int32 TexcoordIndex = 0; TexcoordIndex < NumTexcoords; TexcoordIndex++)
						Vert->TextureCoordinate[TexcoordIndex] = RawMesh.WedgeTexCoords[TexcoordIndex][SrcVertIndex];
					Vert->Color = bHasVertexColor ? RawMesh.WedgeColors[SrcVertIndex].DWColor() : DefaultColor;
					// add index
					OutIndices.Add(VertIndex);
					VertIndex++;
				}
				if (bDuplicateTris)
				{
					// add the same triangle with opposite vertex order
					OutIndices.Add(VertIndex - 3);
					OutIndices.Add(VertIndex - 1);
					OutIndices.Add(VertIndex - 2);
				}
			}
		}

		return NumTris;
	}

	static int32 FillSkeletalMeshData(
		bool bDuplicateTris,
		const FStaticLODModel& LODModel,
		FRenderData& Data,
		TArray<FMaterialMeshVertex>& OutVerts,
		TArray<int32>& OutIndices)
	{
		TArray<FSoftSkinVertex> Vertices;
		FMultiSizeIndexContainerData IndexData;
		LODModel.GetVertices(Vertices);
		LODModel.MultiSizeIndexContainer.GetIndexBufferData(IndexData);

		int32 NumTris = 0;
		int32 NumVerts = 0;

#if WITH_APEX_CLOTHING
		const int32 SectionCount = LODModel.NumNonClothingSections();
#else
		const int32 SectionCount = LODModel.Sections.Num();
#endif // #if WITH_APEX_CLOTHING

		// count triangles and vertices for selected material
		for (int32 SectionIndex = 0; SectionIndex < SectionCount; SectionIndex++)
		{
			const FSkelMeshSection& Section = LODModel.Sections[SectionIndex];
			if (Section.MaterialIndex == Data.MeshData->MaterialIndex)
			{
				NumTris += Section.NumTriangles;
				NumVerts += LODModel.Chunks[Section.ChunkIndex].GetNumVertices();
			}
		}

		if (NumTris == 0)
		{
			// there's nothing to do here
			return 0;
		}

		bool bUseNewUVs = Data.MeshData->TexCoords.Num() > 0;

		if (bUseNewUVs)
		{
			// we should split all merged vertices because UVs are prepared per-corner, i.e. has
			// (NumTris * 3) vertices
			NumVerts = NumTris * 3;
		}

		// reserve renderer data
		OutVerts.Empty(NumVerts);
		OutIndices.Empty(bDuplicateTris ? NumVerts * 2: NumVerts);

		float U = 0;
		float V = 0;
		float SizeU = 1;
		float SizeV = 1;
		if (Data.MeshData->TexcoordBounds.bIsValid)
		{
			U = Data.MeshData->TexcoordBounds.Min.X;
			V = Data.MeshData->TexcoordBounds.Min.Y;
			SizeU = Data.MeshData->TexcoordBounds.Max.X - Data.MeshData->TexcoordBounds.Min.X;
			SizeV = Data.MeshData->TexcoordBounds.Max.Y - Data.MeshData->TexcoordBounds.Min.Y;
		}
		float ScaleX = Data.Size.X;
		float ScaleY = Data.Size.Y;
		uint32 DefaultColor = FColor::White.DWColor();

		int32 NumTexcoords = LODModel.NumTexCoords;


		// add vertices
		if (!bUseNewUVs)
		{
			// Use original UV from mesh, render indexed mesh as indexed mesh.

			uint32 FirstVertex = 0;
			uint32 OutVertexIndex = 0;

			for (int32 SectionIndex = 0; SectionIndex < SectionCount; SectionIndex++)
			{
				const FSkelMeshSection& Section = LODModel.Sections[SectionIndex];
				const FSkelMeshChunk& Chunk = LODModel.Chunks[Section.ChunkIndex];

				const int32 NumVertsInChunk = Chunk.GetNumVertices();

				if (Section.MaterialIndex == Data.MeshData->MaterialIndex)
				{
					// offset to remap source mesh vertex index to destination vertex index
					int32 IndexOffset = FirstVertex - OutVertexIndex;

					// copy vertices
					int32 SrcVertIndex = FirstVertex;
					for (int32 VertIndex = 0; VertIndex < NumVertsInChunk; VertIndex++)
					{
						const FSoftSkinVertex& SrcVert = Vertices[SrcVertIndex];
						FMaterialMeshVertex* DstVert = new(OutVerts) FMaterialMeshVertex();

						// compute vertex position from original UV
						const FVector2D& UV = SrcVert.UVs[0];
						DstVert->Position.Set((UV.X - U) * ScaleX, (UV.Y - V) * ScaleY, 0);

						DstVert->SetTangents(SrcVert.TangentX, SrcVert.TangentY, SrcVert.TangentZ);
						for (int32 TexcoordIndex = 0; TexcoordIndex < NumTexcoords; TexcoordIndex++)
							DstVert->TextureCoordinate[TexcoordIndex] = SrcVert.UVs[TexcoordIndex];
						DstVert->Color = SrcVert.Color.DWColor();

						SrcVertIndex++;
						OutVertexIndex++;
					}

					// copy indices
					int32 Index = Section.BaseIndex;
					for (uint32 TriIndex = 0; TriIndex < Section.NumTriangles; TriIndex++)
					{
						uint32 Index0 = IndexData.Indices[Index++] - IndexOffset;
						uint32 Index1 = IndexData.Indices[Index++] - IndexOffset;
						uint32 Index2 = IndexData.Indices[Index++] - IndexOffset;
						OutIndices.Add(Index0);
						OutIndices.Add(Index1);
						OutIndices.Add(Index2);
						if (bDuplicateTris)
						{
							// add the same triangle with opposite vertex order
							OutIndices.Add(Index0);
							OutIndices.Add(Index2);
							OutIndices.Add(Index1);
						}
					}
				}
				FirstVertex += NumVertsInChunk;
			}
		}
		else // bUseNewUVs
		{
			// Use external UVs. These UVs are prepared per-corner, so we should convert indexed mesh to non-indexed, without
			// sharing of vertices between triangles.

			uint32 OutVertexIndex = 0;

			for (int32 SectionIndex = 0; SectionIndex < SectionCount; SectionIndex++)
			{
				const FSkelMeshSection& Section = LODModel.Sections[SectionIndex];

				if (Section.MaterialIndex == Data.MeshData->MaterialIndex)
				{
					// copy vertices
					int32 LastIndex = Section.BaseIndex + Section.NumTriangles * 3;
					for (int32 Index = Section.BaseIndex; Index < LastIndex; Index += 3)
					{
						for (int32 Corner = 0; Corner < 3; Corner++)
						{
							int32 CornerIndex = Index + Corner;
							int32 SrcVertIndex = IndexData.Indices[CornerIndex];
							const FSoftSkinVertex& SrcVert = Vertices[SrcVertIndex];
							FMaterialMeshVertex* DstVert = new(OutVerts) FMaterialMeshVertex();

							const FVector2D& UV = Data.MeshData->TexCoords[CornerIndex];
							DstVert->Position.Set(UV.X * ScaleX, UV.Y * ScaleY, 0);

							DstVert->SetTangents(SrcVert.TangentX, SrcVert.TangentY, SrcVert.TangentZ);
							for (int32 TexcoordIndex = 0; TexcoordIndex < NumTexcoords; TexcoordIndex++)
								DstVert->TextureCoordinate[TexcoordIndex] = SrcVert.UVs[TexcoordIndex];
							DstVert->Color = SrcVert.Color.DWColor();

							OutIndices.Add(OutVertexIndex);
							OutVertexIndex++;
						}
						if (bDuplicateTris)
						{
							// add the same triangle with opposite vertex order
							OutIndices.Add(OutVertexIndex - 3);
							OutIndices.Add(OutVertexIndex - 1);
							OutIndices.Add(OutVertexIndex - 2);
						}
					}
				}
			}
		}

		return NumTris;
	}

	static int32 FillUVMeshData(FRenderData& Data, TArray<FMaterialMeshVertex>& OutVerts, TArray<int32>& OutIndices)
	{
		int32 NumWedges = Data.MeshData->TexCoords.Num();
		check((NumWedges > 0) && (NumWedges % 3 == 0));
		int32 NumTris = NumWedges / 3;

		OutVerts.Empty(NumWedges);
		OutIndices.Empty(NumWedges);

		uint32 DefaultColor = FColor::White.DWColor();

		// add vertices
		for (int32 WedgeIndex = 0; WedgeIndex < NumWedges; WedgeIndex++)
		{
			FMaterialMeshVertex* Vert = new(OutVerts) FMaterialMeshVertex();

			const FVector2D& UV = Data.MeshData->TexCoords[WedgeIndex];

			Vert->Position.Set(Data.Size.X * UV.X, Data.Size.Y * UV.Y, 0);
			Vert->SetTangents(FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1));
			FMemory::Memzero(&Vert->TextureCoordinate, sizeof(Vert->TextureCoordinate));
			Vert->TextureCoordinate[0].Set(0, 0);
			Vert->Color = DefaultColor;

			// add index
			OutIndices.Add(WedgeIndex);
		}

		return NumTris;
	}

	static int32 FillQuadData(FRenderData& Data, TArray<FMaterialMeshVertex>& OutVerts, TArray<int32>& OutIndices)
	{
		OutVerts.Empty(4);
		OutIndices.Empty(6);

		float U = 0;
		float V = 0;
		float SizeU = 1;
		float SizeV = 1;
		if (Data.MeshData->TexcoordBounds.bIsValid)
		{
			U = Data.MeshData->TexcoordBounds.Min.X;
			V = Data.MeshData->TexcoordBounds.Min.Y;
			SizeU = Data.MeshData->TexcoordBounds.Max.X - Data.MeshData->TexcoordBounds.Min.X;
			SizeV = Data.MeshData->TexcoordBounds.Max.Y - Data.MeshData->TexcoordBounds.Min.Y;
		}
		uint32 DefaultColor = FColor::White.DWColor();

		// add vertices
		for (int32 VertIndex = 0; VertIndex < 4; VertIndex++)
		{
			FMaterialMeshVertex* Vert = new(OutVerts) FMaterialMeshVertex();

			int X = VertIndex & 1;
			int Y = (VertIndex >> 1) & 1;

			Vert->Position.Set(Data.Size.X * X, Data.Size.Y * Y, 0);
			Vert->SetTangents(FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1));
			FMemory::Memzero(&Vert->TextureCoordinate, sizeof(Vert->TextureCoordinate));
			Vert->TextureCoordinate[0].Set(U + SizeU * X, V + SizeV * Y);
			Vert->Color = DefaultColor;
		}

		// add indices
		static const int32 Indices[6] = { 0, 2, 1, 2, 3, 1 };
		OutIndices.Append(Indices, 6);

		return 2;
	}

	// Reference: FTileRenderer::DrawTile
	// Note: this function is called from the rendering thread.
	static void RenderMaterial(
		FRHICommandListImmediate& RHICmdList,
		const class FSceneView& View,
		FRenderData& Data)
	{
#if !USE_LOCALVERTEXFACTORY
		FSimplygonVertexFactory VertexFactory;
#else
		FLocalVertexFactory VertexFactory;
#endif
		InitVertexFactory(VertexFactory, Data.MeshData->StaticMeshComponent ? Data.MeshData->StaticMeshComponent->StaticMesh : nullptr);
		VertexFactory.InitResource();

		FLightMap2D* LightMap2D = nullptr;
		if (Data.MeshData->StaticMeshComponent)
		{
			FLightMap* LightMap = Data.MeshData->StaticMeshComponent->LODData[0].LightMap;
			if (LightMap)
			{
				LightMap2D = LightMap->GetLightMap2D();
			}
		}

		FMeshBatch MeshElement;
		MeshElement.VertexFactory = &VertexFactory;
		MeshElement.DynamicVertexStride = sizeof(FMaterialMeshVertex);
		MeshElement.ReverseCulling = false;
		MeshElement.UseDynamicData = true;
		MeshElement.Type = PT_TriangleList;
		MeshElement.DepthPriorityGroup = SDPG_Foreground;
		FMeshBatchElement& BatchElement = MeshElement.Elements[0];
		BatchElement.PrimitiveUniformBufferResource = &GIdentityPrimitiveUniformBuffer;
		BatchElement.UserData = LightMap2D;
#if SHOW_WIREFRAME_MESH
		MeshElement.bWireframe = true;
#endif

		// Check if material is TwoSided - single-sided materials should be rendered with normal and reverse
		// triangle corner orders, to avoid problems with inside-out meshes or mesh parts. Note:
		// FExportMaterialProxy::GetMaterial() (which is really called here) ignores 'InFeatureLevel' parameter.
		const FMaterial* Material = Data.MaterialRenderProxy->GetMaterial(MATERIAL_FEATURE_LEVEL);
		bool bIsMaterialTwoSided = Material->IsTwoSided();

		TArray<FMaterialMeshVertex> Verts;
		TArray<int32> Indices;

		int32 NumTris = 0;
		if (Data.MeshData->Mesh != nullptr)
		{
			check(Data.MeshData->LODModel == nullptr)
			NumTris = FillStaticMeshData(!bIsMaterialTwoSided, *Data.MeshData->Mesh, Data, Verts, Indices);
		}
		else if (Data.MeshData->LODModel != nullptr)
		{
			NumTris = FillSkeletalMeshData(!bIsMaterialTwoSided, *Data.MeshData->LODModel, Data, Verts, Indices);
		}
		else if (Data.MeshData->TexCoords.Num())
		{
			NumTris = FillUVMeshData(Data, Verts, Indices);
		}
		else
		{
			// both are null, use simple rectangle
			NumTris = FillQuadData(Data, Verts, Indices);
		}

		if (NumTris)
		{
			MeshElement.UseDynamicData = true;
			MeshElement.DynamicVertexData = Verts.GetData();
			MeshElement.MaterialRenderProxy = Data.MaterialRenderProxy;

			// use index data
			BatchElement.FirstIndex = 0;
			BatchElement.NumPrimitives = bIsMaterialTwoSided ? NumTris : NumTris * 2;
			BatchElement.DynamicIndexData = Indices.GetData();
			BatchElement.DynamicIndexStride = sizeof(int32);
			BatchElement.MinVertexIndex = 0;
			BatchElement.MaxVertexIndex = Verts.Num() - 1;

			GetRendererModule().DrawTileMesh(RHICmdList, View, MeshElement, false /*bIsHitTesting*/, FHitProxyId());
		}

		VertexFactory.ReleaseResource();
	}

	// Reference: FCanvasTileRendererItem::Render_RenderThread
	virtual bool Render_RenderThread(FRHICommandListImmediate& RHICmdList, const FCanvas* Canvas) override
	{
		checkSlow(Data);
		// current render target set for the canvas
		const FRenderTarget* CanvasRenderTarget = Canvas->GetRenderTarget();
		FIntRect ViewRect(FIntPoint(0, 0), CanvasRenderTarget->GetSizeXY());

		// make a temporary view
		FSceneViewInitOptions ViewInitOptions;
		ViewInitOptions.ViewFamily = Data->ViewFamily;
		ViewInitOptions.SetViewRectangle(ViewRect);
		ViewInitOptions.ViewOrigin = FVector::ZeroVector;
		ViewInitOptions.ViewRotationMatrix = FMatrix::Identity;
		ViewInitOptions.ProjectionMatrix = Data->Transform.GetMatrix();
		ViewInitOptions.BackgroundColor = FLinearColor::Black;
		ViewInitOptions.OverlayColor = FLinearColor::White;

		bool bNeedsToSwitchVerticalAxis = RHINeedsToSwitchVerticalAxis(Canvas->GetShaderPlatform()) && !Canvas->GetAllowSwitchVerticalAxis();
		check(bNeedsToSwitchVerticalAxis == false);

		FSceneView* View = new FSceneView(ViewInitOptions);

		RenderMaterial(RHICmdList, *View, *Data);
	
		delete View;
		if( Canvas->GetAllowedModes() & FCanvas::Allow_DeleteOnRender )
		{
			delete Data;
		}
		if( Canvas->GetAllowedModes() & FCanvas::Allow_DeleteOnRender )
		{
			Data = NULL;
		}
		return true;
	}

	// Reference: FCanvasTileRendererItem::Render_GameThread
	virtual bool Render_GameThread(const FCanvas* Canvas) override
	{
		checkSlow(Data);
		// current render target set for the canvas
		const FRenderTarget* CanvasRenderTarget = Canvas->GetRenderTarget();
		FIntRect ViewRect(FIntPoint(0, 0), CanvasRenderTarget->GetSizeXY());

		// make a temporary view
		FSceneViewInitOptions ViewInitOptions;
		ViewInitOptions.ViewFamily = Data->ViewFamily;
		ViewInitOptions.SetViewRectangle(ViewRect);
		ViewInitOptions.ViewOrigin = FVector::ZeroVector;
		ViewInitOptions.ViewRotationMatrix = FMatrix::Identity;
		ViewInitOptions.ProjectionMatrix = Data->Transform.GetMatrix();
		ViewInitOptions.BackgroundColor = FLinearColor::Black;
		ViewInitOptions.OverlayColor = FLinearColor::White;

		FSceneView* View = new FSceneView(ViewInitOptions);

		bool bNeedsToSwitchVerticalAxis = RHINeedsToSwitchVerticalAxis(Canvas->GetShaderPlatform()) && !Canvas->GetAllowSwitchVerticalAxis();
		check(bNeedsToSwitchVerticalAxis == false);
		struct FDrawMaterialParameters
		{
			FSceneView* View;
			FRenderData* RenderData;
			uint32 AllowedCanvasModes;
		};
		FDrawMaterialParameters DrawMaterialParameters =
		{
			View,
			Data,
			Canvas->GetAllowedModes()
		};
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			DrawMaterialCommand,
			FDrawMaterialParameters, Parameters, DrawMaterialParameters,
		{
			RenderMaterial(RHICmdList, *Parameters.View, *Parameters.RenderData);

			delete Parameters.View;
			if (Parameters.AllowedCanvasModes & FCanvas::Allow_DeleteOnRender)
			{
				delete Parameters.RenderData;
			}
		});
		if (Canvas->GetAllowedModes() & FCanvas::Allow_DeleteOnRender)
		{
			Data = NULL;
		}
		return true;
	}
};

void PerformImageDilation(TArray<FColor>& InBMP, int32 InImageWidth, int32 InImageHeight, bool IsNormalMap)
{
	int32 PixelIndex = 0;
	int32 PixelIndices[16];

	for (int32 Y = 0; Y < InImageHeight; Y++)
	{
		for (int32 X = 0; X < InImageWidth; X++, PixelIndex++)
		{
			FColor& Color = InBMP[PixelIndex];
			if (Color.A == 0 || (IsNormalMap && Color.B == 0))
			{
				// process this pixel only if it is not filled with color; note: normal map has A==255, so detect
				// empty pixels by zero blue component, which is not possible for normal maps
				int32 NumPixelsToCheck = 0;
				// check line at Y-1
				if (Y > 0)
				{
					PixelIndices[NumPixelsToCheck++] = PixelIndex - InImageWidth; // X,Y-1
					if (X > 0)
					{
						PixelIndices[NumPixelsToCheck++] = PixelIndex - InImageWidth - 1; // X-1,Y-1
					}
					if (X < InImageWidth-1)
					{
						PixelIndices[NumPixelsToCheck++] = PixelIndex - InImageWidth + 1; // X+1,Y-1
					}
				}
				// check line at Y
				if (X > 0)
				{
					PixelIndices[NumPixelsToCheck++] = PixelIndex - 1; // X-1,Y
				}
				if (X < InImageWidth-1)
				{
					PixelIndices[NumPixelsToCheck++] = PixelIndex + 1; // X+1,Y
				}
				// check line at Y+1
				if (Y < InImageHeight-1)
				{
					PixelIndices[NumPixelsToCheck++] = PixelIndex + InImageWidth; // X,Y+1
					if (X > 0)
					{
						PixelIndices[NumPixelsToCheck++] = PixelIndex + InImageWidth - 1; // X-1,Y+1
					}
					if (X < InImageWidth-1)
					{
						PixelIndices[NumPixelsToCheck++] = PixelIndex + InImageWidth + 1; // X+1,Y+1
					}
				}
				// get color
				int32 BestColorValue = 0;
				FColor BestColor(0);
				for (int32 PixelToCheck = 0; PixelToCheck < NumPixelsToCheck; PixelToCheck++)
				{
					const FColor& ColorToCheck = InBMP[PixelIndices[PixelToCheck]];
					if (ColorToCheck.A != 0 && (!IsNormalMap || ColorToCheck.B != 0))
					{
						// consider only original pixels, not computed ones
						int32 ColorValue = ColorToCheck.R + ColorToCheck.G + ColorToCheck.B;
						if (ColorValue > BestColorValue)
						{
							BestColorValue = ColorValue;
							BestColor = ColorToCheck;
						}
					}
				}
				// put the computed pixel back
				if (BestColorValue != 0)
				{
					Color = BestColor;
					Color.A = 0;
#if VISUALIZE_DILATION
					Color.R = 255;
					Color.G = 0;
					Color.B = 0;
#endif
				}
			}
		}
	}
}


bool GRendererInitialized = false;

// Reference: FCanvasTileRendererItem::Render_GameThread
// Note: InMaterialProperty is used only when reading rendered bitmap (NormalMap and EmissiveColor has differences from other properties).
bool RenderMaterial(
	FMaterialData& InMaterialData,
	FMaterialRenderProxy* InMaterialProxy,
	EMaterialProperty InMaterialProperty,
	UTextureRenderTarget2D* InRenderTarget,
	TArray<FColor>& OutBMP)
{
	check(InRenderTarget);
	FTextureRenderTargetResource* RTResource = InRenderTarget->GameThread_GetRenderTargetResource();

	{
		// Create a canvas for the render target and clear it to black
		FCanvas Canvas(RTResource, NULL, FApp::GetCurrentTime() - GStartTime, FApp::GetDeltaTime(), FApp::GetCurrentTime() - GStartTime, GMaxRHIFeatureLevel);

		// create ViewFamily
		float CurrentRealTime = 0.f;
		float CurrentWorldTime = 0.f;
		float DeltaWorldTime = 0.f;

/*		if (!bFreezeTime)
		{
			CurrentRealTime = Canvas.GetCurrentRealTime();
			CurrentWorldTime = Canvas.GetCurrentWorldTime();
			DeltaWorldTime = Canvas.GetCurrentDeltaWorldTime();
		} */

		const FRenderTarget* CanvasRenderTarget = Canvas.GetRenderTarget();
		FSceneViewFamily ViewFamily( FSceneViewFamily::ConstructionValues(
			CanvasRenderTarget,
			NULL,
			FEngineShowFlags(ESFIM_Game))
			.SetWorldTimes( CurrentWorldTime, DeltaWorldTime, CurrentRealTime )
			.SetGammaCorrection( CanvasRenderTarget->GetDisplayGamma() ) );

		if (!GRendererInitialized)
		{
			// Force global shaders to be compiled and saved
			if (GShaderCompilingManager)
			{
				// Process any asynchronous shader compile results that are ready, limit execution time
				GShaderCompilingManager->ProcessAsyncResults(false, true);
			}

			// Initialize the renderer in a case if material LOD computed in UStaticMesh::PostLoad()
			// when loading a scene on UnrealEd startup. Use GetRendererModule().BeginRenderingViewFamily()
			// for that. Prepare a dummy scene because it is required by that function.
			FClassThumbnailScene DummyScene;
			DummyScene.SetClass(UStaticMesh::StaticClass());
			ViewFamily.Scene = DummyScene.GetScene();
			int32 X = 0, Y = 0, Width = 256, Height = 256;
			DummyScene.GetView(&ViewFamily, X, Y, Width, Height);
			GetRendererModule().BeginRenderingViewFamily(&Canvas, &ViewFamily);
			GRendererInitialized = true;
			ViewFamily.Scene = NULL;
		}

#if !SHOW_WIREFRAME_MESH
		Canvas.Clear(InRenderTarget->ClearColor);
#else
		Canvas.Clear(FLinearColor::Yellow);
#endif

		// add item for rendering
		FSimplygonMaterialRenderItem::EnqueueMaterialRender(
			&Canvas,
			&ViewFamily,
			&InMaterialData,
			FVector2D(InRenderTarget->SizeX, InRenderTarget->SizeY),
			InMaterialProxy
		);

		// rendering is performed here
		Canvas.Flush_GameThread();

		FlushRenderingCommands();
		Canvas.SetRenderTarget_GameThread(NULL);
		FlushRenderingCommands();
	}
			
	bool bNormalmap = (InMaterialProperty == MP_Normal);
	FReadSurfaceDataFlags ReadPixelFlags(bNormalmap ? RCM_SNorm : RCM_UNorm);
	ReadPixelFlags.SetLinearToGamma(false);

	bool result = false;
	
	if (InMaterialProperty != MP_EmissiveColor)
	{
		// Read normal color image
		result = RTResource->ReadPixels(OutBMP, ReadPixelFlags);
	}
	else
	{
		// Read HDR emissive image
		TArray<FFloat16Color> Color16;
		result = RTResource->ReadFloat16Pixels(Color16);
		// Find color scale value
		float MaxValue = 0;
		for (int32 PixelIndex = 0; PixelIndex < Color16.Num(); PixelIndex++)
		{
			FFloat16Color& Pixel16 = Color16[PixelIndex];
			float R = Pixel16.R.GetFloat();
			float G = Pixel16.G.GetFloat();
			float B = Pixel16.B.GetFloat();
			float Max = FMath::Max3(R, G, B);
			if (Max > MaxValue)
			{
				MaxValue = Max;
			}
		}
		if (MaxValue <= 0.01f)
		{
			// Black emissive, drop it
			return false;
		}
		// Now convert Float16 to Color
		OutBMP.SetNumUninitialized(Color16.Num());
		float Scale = 255.0f / MaxValue;
		for (int32 PixelIndex = 0; PixelIndex < Color16.Num(); PixelIndex++)
		{
			FFloat16Color& Pixel16 = Color16[PixelIndex];
			FColor& Pixel8 = OutBMP[PixelIndex];
			Pixel8.R = (uint8)FMath::RoundToInt(Pixel16.R.GetFloat() * Scale);
			Pixel8.G = (uint8)FMath::RoundToInt(Pixel16.G.GetFloat() * Scale);
			Pixel8.B = (uint8)FMath::RoundToInt(Pixel16.B.GetFloat() * Scale);
		}
		InMaterialData.EmissiveScale = MaxValue;
	}

	PerformImageDilation(OutBMP, InRenderTarget->GetSurfaceWidth(), InRenderTarget->GetSurfaceHeight(), bNormalmap);
	return result;
}

FMaterialRenderProxy* CreateExportMaterialProxy(FMaterialData& InMaterialData, EMaterialProperty InMaterialProperty)
{
	check(InMaterialProperty >= 0 && InMaterialProperty < ARRAY_COUNT(InMaterialData.ProxyCache.Proxies));
	if (InMaterialData.ProxyCache.Proxies[InMaterialProperty])
	{
		return InMaterialData.ProxyCache.Proxies[InMaterialProperty];
	}
	static FSimplygonMaterialProxyHook ProxyHook;
	FMaterialRenderProxy* Proxy = FMaterialUtilities::CreateExportMaterialProxy(InMaterialData.Material, InMaterialProperty, &ProxyHook);
	InMaterialData.ProxyCache.Proxies[InMaterialProperty] = Proxy;
	return Proxy;
}

FMaterialRenderProxy* CreateExportMaterialProxyForColor(const FColor& InColor)
{
	static FMaterialRenderProxy* Proxy = nullptr;
	if (!Proxy)
	{
		// Get Material'/Engine/EngineDebugMaterials/DebugMeshMaterial.DebugMeshMaterial' - this is translucent material
		// with 'Color' parameter attached to EmissiveColor property.
		UMaterial* ColorMaterialTemplate = GEngine->DebugMeshMaterial;
		static FSimplygonMaterialProxyHook ProxyHook; // use hook here to restrict compiled shader map (see ShouldCache)
		Proxy = FMaterialUtilities::CreateExportMaterialProxy(ColorMaterialTemplate, MP_EmissiveColor, &ProxyHook);
		GShaderCompilingManager->FinishAllCompilation();
	}
	return new FColoredMaterialRenderProxy(Proxy, InColor.ReinterpretAsLinear());
}

FColor GetDefaultPropertyColor(EMaterialProperty Property)
{
	// See GetDefaultExpressionForMaterialProperty() for property defaults.
	switch (Property)
	{
		case MP_Opacity:			return FColor(255, 255, 255);
		case MP_OpacityMask:		return FColor(255, 255, 255);
		case MP_Metallic:			return FColor(0, 0, 0);
		case MP_Specular:			return FColor(127, 127, 127);
		case MP_Roughness:			return FColor(127, 127, 127);
		case MP_AmbientOcclusion:	return FColor(255, 255, 255);
		case MP_EmissiveColor:		return FColor(0, 0, 0);
		case MP_BaseColor:			return FColor(0, 0, 0);
		case MP_Normal:				return FColor(128, 128, 255);
	}
	return FColor(0, 0, 0);
}

bool IsProxyCompiledSuccessfully(FMaterialRenderProxy* Proxy)
{
	bool bSuccess = false;

	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		FCheckCompileResult,
		FMaterialRenderProxy*, Proxy, Proxy,
		bool&, bSuccess, bSuccess,
	{
		// FExportMaterialProxy::GetMaterial() will return some default material if compilation failed, or
		// FExportMaterialProxy object itself.
		const FMaterial* Resource = Proxy->GetMaterial(MATERIAL_FEATURE_LEVEL);
		bSuccess = Resource->GetFriendlyName().StartsWith(TEXT("FExportMaterialRenderer"));
	});

	// Block until the command has finished executing.
	FlushRenderingCommands();
	return bSuccess;
}

// Reference: MaterialExportUtils::RenderMaterialPropertyToTexture, MaterialExportUtils::ExportMaterialProperty
bool FSimplygonUtilities::RenderMaterialPropertyToTexture(
	FMaterialData& InMaterialData,
	bool bInCompileOnly,
	EMaterialProperty InMaterialProperty,
	bool bInForceLinearGamma,
	EPixelFormat InPixelFormat,
	FIntPoint& InTargetSize,
	TArray<FColor>& OutSamples)
{
	// Check if this property is enabled by user for rendering.
	if (InTargetSize.X == 0 || InTargetSize.Y == 0)
	{
		return false;
	}

	UMaterial* RootMaterial = InMaterialData.Material->GetMaterial();
	check(RootMaterial);

	// Check if this property is used for this material setup (no opacity for BLEND_Opaque etc).
	if (!RootMaterial->IsPropertyActive(InMaterialProperty))
	{
		return false;
	}

	bool bReleaseProxy = false;
	FMaterialRenderProxy* MaterialProxy = nullptr;

	// Simple optimization: if material has this property disconnected, don't compile a shader for it, but use 1x1 texture
	// with predefined value instead. Note: for MaterialAttributes there's no easy way to check if property has expressions
	// connected. The only way is to call MaterialAttributes.IsConnected(InMaterialProperty), but this function will always
	// return 'false' if material was not compiled yet (see FMaterialAttributesInput::CompileWithDefault(), this function
	// calls FMaterialAttributesInput::SetConnectedProperty()).
	// Possible recover for MaterialAttributes: call to CompileWithDefault() for input - it should be fast (simple call to
	// MaterialCompiler without issuing expensive shader compilation).
	bool bHasPropertyConnected = !RootMaterial->bUseMaterialAttributes ?
									RootMaterial->GetExpressionInputForProperty(InMaterialProperty)->IsConnected() :
									true; //RootMaterial->MaterialAttributes.IsConnected(InMaterialProperty)
	if (!bHasPropertyConnected)
	{
		// Use simplified rendering
		if (!bInCompileOnly)
		{
			MaterialProxy = CreateExportMaterialProxyForColor(GetDefaultPropertyColor(InMaterialProperty));
			bReleaseProxy = true;
		}
		else
		{
			return true;
		}
	}

	if (MaterialProxy == nullptr)
	{
		// Create proxy if not using simplified rendering
		MaterialProxy = CreateExportMaterialProxy(InMaterialData, InMaterialProperty);
	}
	if (MaterialProxy == nullptr)
	{
		return false;
	}

	if (bInCompileOnly)
	{
		// Rendering will be executed on 2nd pass. Currently we only need to launch a shader compiler for this property.
		return true;
	}

	// Finalize shader compilation
#if 0
	// This block is disabled because
	// 1) GetMaterial() will return default material if this material is not yet compiled (see FExportMaterialProxy::GetMaterial())
	// 2) GetMaterial() requires access from rendering thread.
	FMaterial* Material = const_cast<FMaterial*>(MaterialProxy->GetMaterial(MATERIAL_FEATURE_LEVEL));
	Material->FinishCompilation();
#else
	GShaderCompilingManager->FinishAllCompilation();
#endif

	if (!IsProxyCompiledSuccessfully(MaterialProxy))
	{
		check(bReleaseProxy == false);
		UE_LOG(LogSimplygonUtilities, Warning, TEXT("Failed to compile property %s of material %s."), *GetNameOfMaterialProperty(InMaterialProperty), *RootMaterial->GetName());
		// Continue as if the material has this property disconnected.
		MaterialProxy = CreateExportMaterialProxyForColor(GetDefaultPropertyColor(InMaterialProperty));
		bReleaseProxy = true;
	}

	// Disallow garbage collection of RenderTarget.
	check(CurrentlyRendering == false);
	CurrentlyRendering = true;

	UTextureRenderTarget2D* RenderTarget = CreateRenderTarget(bInForceLinearGamma, InPixelFormat, InTargetSize);

	OutSamples.Empty(InTargetSize.X * InTargetSize.Y);
	bool bResult = RenderMaterial(InMaterialData, MaterialProxy, InMaterialProperty, RenderTarget, OutSamples);

#if 0
	// Check for uniform value
	// The code is disabled because if we will call MergeFlattenMaterials() to compress multiple "sparse" materials into a single one,
	// uniform non-black texture will affect all other materials, and this is bad. So we will render material anyway and analyze only
	// resulting materials (see OptimizeFlattenMaterialForUV()).
	bool bIsUniform = true;
	FColor MaxColor(0, 0, 0, 0);
	if (bResult)
	{
		// Find maximal color value
		int32 MaxColorValue = 0;
		for (int32 Index = 0; Index < OutSamples.Num(); Index++)
		{
			FColor Color = OutSamples[Index];
			int32 ColorValue = Color.R + Color.G + Color.B + Color.A;
			if (ColorValue > MaxColorValue)
			{
				MaxColorValue = ColorValue;
				MaxColor = Color;
			}
		}

		// Fill background with maximal color value and render again
		RenderTarget->ClearColor = FLinearColor(MaxColor);
		TArray<FColor> OutSamples2;
		RenderMaterial(InMaterialData, MaterialProxy, InMaterialProperty, RenderTarget, OutSamples2);
		for (int32 Index = 0; Index < OutSamples2.Num(); Index++)
		{
			FColor Color = OutSamples2[Index];
			if (Color != MaxColor)
			{
				bIsUniform = false;
				break;
			}
		}
	}

	// Replace uniform value with a single sample
	if (bIsUniform)
	{
		InTargetSize = FIntPoint(1, 1);
		OutSamples.Empty();
		OutSamples.Add(MaxColor);
	}
#endif

	if (bReleaseProxy)
	{
		delete MaterialProxy;
	}
	CurrentlyRendering = false;

	return bResult;
}

bool FSimplygonUtilities::ExportMaterial(
	FMaterialData& InMaterialData,
	FFlattenMaterial& OutFlattenMaterial,
	FExportMaterialProxyCache* CompiledMaterial)
{
	UMaterialInterface* Material = InMaterialData.Material;
	UE_LOG(LogSimplygonUtilities, Log, TEXT("Flattening material: %s"), *Material->GetName());

	if (CompiledMaterial)
	{
		// ExportMaterial was called with non-null CompiledMaterial. This means compiled shaders
		// should be stored outside, and could be re-used in next call to ExportMaterial.
		// FMaterialData already has "proxy cache" fiels, should swap it with CompiledMaterial,
		// and swap back before returning from this function.
		// Purpose of the following line: use compiled material cached from previous call.
		Exchange(*CompiledMaterial, InMaterialData.ProxyCache);
	}

	// Check if comandlet rendering is enabled
	if (!FApp::CanEverRender())
	{
		UE_LOG(LogSimplygonUtilities, Warning, TEXT("Commandlet rendering is disabled. This will mostlikely break the cooking process."));
	}

	FullyLoadMaterialStatic(Material);

	// Precache all used textures, otherwise could get everything rendered with low-res textures.
	TArray<UTexture*> MaterialTextures;
	Material->GetUsedTextures(MaterialTextures, EMaterialQualityLevel::Num, true, GMaxRHIFeatureLevel, true);

	for (int32 TexIndex = 0; TexIndex < MaterialTextures.Num(); TexIndex++)
	{
		UTexture* Texture = MaterialTextures[TexIndex];

		if (Texture == NULL)
		{
			continue;
		}

		UTexture2D* Texture2D = Cast<UTexture2D>(Texture);
		if (Texture2D)
		{
			Texture2D->SetForceMipLevelsToBeResident(30.0f, true);
			Texture2D->WaitForStreaming();
		}
	}

	// Set UStaticMeshComponent for rendering. Note for future changes: if we'll compile proxies for multiple
	// materials at the same time, StaticMeshComponent should be set only for rendering phase. So it could
	// be set to any value during compile phase.
	FSimplygonMaterialProxyHook::SetStaticMeshComponent(InMaterialData.StaticMeshComponent);

	// Render base color property
	bool bRenderEmissive = Material->IsPropertyActive(MP_EmissiveColor);
	bool bRenderOpacityMask = Material->IsPropertyActive(MP_OpacityMask) && Material->GetBlendMode() == BLEND_Masked;
	bool bRenderOpacity = Material->IsPropertyActive(MP_Opacity) && IsTranslucentBlendMode(Material->GetBlendMode());
	check(!bRenderOpacity || !bRenderOpacityMask);

	for (int32 Pass = 0; Pass < 2; Pass++)
	{
		// Perform this operation in 2 passes: the first will compile shaders, second will render.
		bool bCompileOnly = Pass == 0;

		// Compile shaders and render flatten material.
		RenderMaterialPropertyToTexture(InMaterialData, bCompileOnly, MP_BaseColor, false, PF_B8G8R8A8, OutFlattenMaterial.DiffuseSize, OutFlattenMaterial.DiffuseSamples);
		RenderMaterialPropertyToTexture(InMaterialData, bCompileOnly, MP_Metallic, true, PF_B8G8R8A8, OutFlattenMaterial.MetallicSize, OutFlattenMaterial.MetallicSamples);
		RenderMaterialPropertyToTexture(InMaterialData, bCompileOnly, MP_Specular, true, PF_B8G8R8A8, OutFlattenMaterial.SpecularSize, OutFlattenMaterial.SpecularSamples);
		RenderMaterialPropertyToTexture(InMaterialData, bCompileOnly, MP_Roughness, true, PF_B8G8R8A8, OutFlattenMaterial.RoughnessSize, OutFlattenMaterial.RoughnessSamples);
		RenderMaterialPropertyToTexture(InMaterialData, bCompileOnly, MP_Normal, true, PF_B8G8R8A8, OutFlattenMaterial.NormalSize, OutFlattenMaterial.NormalSamples);
		if (bRenderOpacityMask)
		{
			RenderMaterialPropertyToTexture(InMaterialData, bCompileOnly, MP_OpacityMask, true, PF_B8G8R8A8, OutFlattenMaterial.OpacitySize, OutFlattenMaterial.OpacitySamples);
		}
		else if (bRenderOpacity)
		{
			// Number of blend modes, let's UMaterial decide whether it wants this property
			RenderMaterialPropertyToTexture(InMaterialData, bCompileOnly, MP_Opacity, true, PF_B8G8R8A8, OutFlattenMaterial.OpacitySize, OutFlattenMaterial.OpacitySamples);
		}
		if (bRenderEmissive)
		{
			// PF_FloatRGBA is here to be able to render and read HDR image using ReadFloat16Pixels()
			RenderMaterialPropertyToTexture(InMaterialData, bCompileOnly, MP_EmissiveColor, false, PF_FloatRGBA, OutFlattenMaterial.EmissiveSize, OutFlattenMaterial.EmissiveSamples);
			OutFlattenMaterial.EmissiveScale = InMaterialData.EmissiveScale;
		}
		RenderMaterialPropertyToTexture(InMaterialData, bCompileOnly, MP_AmbientOcclusion, true, PF_B8G8R8A8, OutFlattenMaterial.AOSize, OutFlattenMaterial.AOSamples);
	}

	OutFlattenMaterial.MaterialId = Material->GetLightingGuid();

	if (CompiledMaterial)
	{
		// Store compiled material to external cache.
		Exchange(*CompiledMaterial, InMaterialData.ProxyCache);
	}

	// Just in case, remove UStaticMeshComponent
	FSimplygonMaterialProxyHook::SetStaticMeshComponent(nullptr);

#ifdef SAVE_INTERMEDIATE_TEXTURES
	FString FilenameString = FString::Printf(TEXT("%s-mat%d"), *Material->GetName(), InMaterialData.MaterialIndex);
	SaveFlattenMaterial(OutFlattenMaterial, *FilenameString);
#endif // SAVE_INTERMEDIATE_TEXTURES

	UE_LOG(LogSimplygonUtilities, Log, TEXT("Flattening done. (%s)"), *Material->GetName());
	return true;
}

void FSimplygonUtilities::RenderUVMask(const TArray<FVector2D>& InTexCoords, const FIntPoint& InTargetSize, TArray<FColor>& OutSamples)
{
	FMaterialRenderProxy* MaterialProxy = CreateExportMaterialProxyForColor(FColor::White);
	UTextureRenderTarget2D* RenderTarget = CreateRenderTarget(true, PF_B8G8R8A8, InTargetSize);

	FBox2D DummyBounds(FVector2D(0, 0), FVector2D(1, 1));
	// Prepare MaterialData without a mesh, but with TexCoords.
	FMaterialData MaterialData(nullptr, nullptr, nullptr, 0, DummyBounds, InTexCoords);

	RenderMaterial(MaterialData, MaterialProxy, MP_BaseColor, RenderTarget, OutSamples);

	delete MaterialProxy;

#if 0
	if (OutSamples.Num() > 0)
	{
		static int SaveIndex = 0;
		FString FilenameString = FString::Printf(SAVE_INTERMEDIATE_TEXTURES TEXT("/UVMask-%03d.bmp"), ++SaveIndex);
		FFileHelper::CreateBitmap(*FilenameString, InTargetSize.X, InTargetSize.Y, OutSamples.GetData());
	}
#endif
}

void FSimplygonUtilities::OptimizeFlattenMaterialForUV(FFlattenMaterial& Material, const TArray<FVector2D>& TexCoords)
{
#define PROCESS_FIELD(Index, SizeField, SamplesField) \
	case Index: Size = &Material.SizeField; Samples = &Material.SamplesField; break;

	FIntPoint LastSize(0, 0);
	TArray<FColor> MaskSamples;

	for (int32 ChannelIndex = 0; /* empty */; ChannelIndex++)
	{
		FIntPoint* Size = nullptr;
		TArray<FColor>* Samples = nullptr;
		switch (ChannelIndex)
		{
		PROCESS_FIELD(0, DiffuseSize, DiffuseSamples);
		PROCESS_FIELD(1, NormalSize, NormalSamples);
		PROCESS_FIELD(2, MetallicSize, MetallicSamples);
		PROCESS_FIELD(3, RoughnessSize, RoughnessSamples);
		PROCESS_FIELD(4, SpecularSize, SpecularSamples);
		PROCESS_FIELD(5, OpacitySize, OpacitySamples);
		PROCESS_FIELD(6, AOSize, AOSamples);
		PROCESS_FIELD(7, EmissiveSize, EmissiveSamples);
		}

		if (Size == nullptr)
		{
			break;
		}

		if (Samples->Num() <= 1)
		{
			// Skip empty bitmaps, or bitmaps already having 1 sample.
			continue;
		}

		// Render UV mask. Re-render only if some property was rendered with a different size.
		if (LastSize != *Size)
		{
			MaskSamples.Empty();
			RenderUVMask(TexCoords, *Size, MaskSamples);
			LastSize = *Size;
		}
		// Compare samples with mask.
		FColor SampleColor;
		bool bColorInitialized = false;
		bool bColorsAreSame = true;
		for (int32 SampleIndex = 0; SampleIndex < Samples->Num(); SampleIndex++)
		{
			if (MaskSamples[SampleIndex].R >= 128)
			{
				// This sample has color.
				if (bColorInitialized)
				{
					if ((*Samples)[SampleIndex] != SampleColor)
					{
						bColorsAreSame = false;
						break;
					}
				}
				else
				{
					SampleColor = (*Samples)[SampleIndex];
					bColorInitialized = true;
				}
			}
		}
		if (bColorsAreSame)
		{
			// Resize this property samples to 1 pixel.
			Samples->Init(SampleColor, 1);
			Size->X = Size->Y = 1;
		}
	}

#undef PROCESS_FIELD
}

bool FSimplygonUtilities::ExportMaterial(
	UMaterialInterface* InMaterial,
	FFlattenMaterial& OutFlattenMaterial,
	FExportMaterialProxyCache* CompiledMaterial)
{
	FBox2D DummyBounds(FVector2D(0, 0), FVector2D(1, 1));
	TArray<FVector2D> EmptyTexCoords;

	FMaterialData MaterialData(InMaterial, nullptr, nullptr, 0, DummyBounds, EmptyTexCoords);
	ExportMaterial(MaterialData, OutFlattenMaterial, CompiledMaterial);
	return true;
}

bool FSimplygonUtilities::ExportMaterial(
	UMaterialInterface* InMaterial,
	const FBox2D& InTexcoordBounds,
	FFlattenMaterial& OutFlattenMaterial,
	FExportMaterialProxyCache* CompiledMaterial)
{
	TArray<FVector2D> EmptyTexCoords;

	FMaterialData MaterialData(InMaterial, nullptr, nullptr, 0, InTexcoordBounds, EmptyTexCoords);
	ExportMaterial(MaterialData, OutFlattenMaterial, CompiledMaterial);
	return true;
}

bool FSimplygonUtilities::ExportMaterial(
	UMaterialInterface* InMaterial,
	const FMeshMaterialReductionData* InMeshData,
	int32 InMaterialIndex,
	FFlattenMaterial& OutFlattenMaterial,
	FExportMaterialProxyCache* CompiledMaterial)
{
	FMaterialData MaterialData(InMaterial, InMeshData->Mesh, InMeshData->LODModel, InMaterialIndex, InMeshData->TexcoordBounds, InMeshData->NewUVs);
	MaterialData.StaticMeshComponent = InMeshData->StaticMeshComponent;
	ExportMaterial(MaterialData, OutFlattenMaterial, CompiledMaterial);
	return true;
}

FFlattenMaterial* FSimplygonUtilities::CreateFlattenMaterial(
	const FSimplygonMaterialLODSettings& InMaterialLODSettings,
	int32 InTextureWidth,
	int32 InTextureHeight)
{
	// Create new material.
	FFlattenMaterial* Material = new FFlattenMaterial();
	// Override non-zero defaults with zeros.
	Material->DiffuseSize = FIntPoint::ZeroValue;
	Material->NormalSize = FIntPoint::ZeroValue;

	// Initialize size of enabled channels. Other channels
	FIntPoint ChannelSize(InTextureWidth, InTextureHeight);
	for (int32 ChannelIndex = 0; ChannelIndex < InMaterialLODSettings.ChannelsToCast.Num(); ChannelIndex++)
	{
		const FSimplygonChannelCastingSettings& Channel = InMaterialLODSettings.ChannelsToCast[ChannelIndex];
		if (Channel.bActive)
		{
			switch (Channel.MaterialChannel)
			{
			case ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_BASECOLOR:
				Material->DiffuseSize = ChannelSize;
				break;
			case ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_SPECULAR:
				Material->SpecularSize = ChannelSize;
				break;
			case ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_ROUGHNESS:
				Material->RoughnessSize = ChannelSize;
				break;
			case ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_METALLIC:
				Material->MetallicSize = ChannelSize;
				break;
			case ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_NORMALS:
				Material->NormalSize = ChannelSize;
				break;
			case ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_OPACITY:
				Material->OpacitySize = ChannelSize;
				break;
			case ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_EMISSIVE:
				Material->EmissiveSize = ChannelSize;
				break;
			case ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_AO:
				Material->AOSize = ChannelSize;
				break;
			default:
				UE_LOG(LogSimplygonUtilities, Error, TEXT("Unsupported material channel: %d"), (int32)Channel.MaterialChannel);
			}
		}
	}

	return Material;
}

void FSimplygonUtilities::MergeFlattenMaterials(FFlattenMaterial& DstMaterial, const FFlattenMaterial& SrcMaterial)
{
#define PROCESS_FIELD(Index, SizeField, SamplesField) \
	case Index: \
		SrcSize = &SrcMaterial.SizeField; \
		DstSize = &DstMaterial.SizeField; \
		SrcSamples = &SrcMaterial.SamplesField; \
		DstSamples = &DstMaterial.SamplesField; \
		break;

	for (int32 ChannelIndex = 0; /* empty */; ChannelIndex++)
	{
		const FIntPoint* SrcSize = nullptr;
		const FIntPoint* DstSize = nullptr;
		const TArray<FColor>* SrcSamples = nullptr;
		TArray<FColor>* DstSamples = nullptr;
		switch (ChannelIndex)
		{
		PROCESS_FIELD(0, DiffuseSize, DiffuseSamples);
		PROCESS_FIELD(1, NormalSize, NormalSamples);
		PROCESS_FIELD(2, MetallicSize, MetallicSamples);
		PROCESS_FIELD(3, RoughnessSize, RoughnessSamples);
		PROCESS_FIELD(4, SpecularSize, SpecularSamples);
		PROCESS_FIELD(5, OpacitySize, OpacitySamples);
		PROCESS_FIELD(6, AOSize, AOSamples);
//		PROCESS_FIELD(7, EmissiveSize, EmissiveSamples); - emissive channels is processed in a special way
		}
		if (SrcSize == nullptr)
		{
			break;
		}

		// Merge samples
		int32 NumSrcSamples = SrcSamples->Num();
		int32 NumDstSamples = DstSamples->Num();
		check(*SrcSize == *DstSize || NumSrcSamples == 1 || NumDstSamples == 1);
		if (NumSrcSamples && !NumDstSamples)
		{
			// Dst is empty, Src is not - simply copy data
			*DstSamples = *SrcSamples;
		}
		else if (NumSrcSamples && NumDstSamples)
		{
			// Src and Dst are both not empty, merge samples. Special case for 1 sample.
			check(NumSrcSamples == NumDstSamples || NumSrcSamples == 1 || NumDstSamples == 1);
			if (NumDstSamples < NumSrcSamples)
			{
				// Need to expand DstSamples to fit SrcSamples size.
				check(NumDstSamples == 1);
				DstSamples->Init((*DstSamples)[0], NumSrcSamples);
			}
			const FColor* SrcColor = SrcSamples->GetData();
			FColor* DstColor = DstSamples->GetData();
			int32 SrcStep = (NumSrcSamples == 1) ? 0 : 1;
			int32 NumSamples = FMath::Max(NumSrcSamples, NumDstSamples);
			for (int32 Sample = 0; Sample < NumSamples; Sample++)
			{
				DstColor->R = FMath::Max(SrcColor->R, DstColor->R);
				DstColor->G = FMath::Max(SrcColor->G, DstColor->G);
				DstColor->B = FMath::Max(SrcColor->B, DstColor->B);
				SrcColor += SrcStep;
				DstColor += 1;
			}
		}
	}

	// Process emissive channel separately because it has scaled color samples
	int32 NumSrcSamples = SrcMaterial.EmissiveSamples.Num();
	int32 NumDstSamples = DstMaterial.EmissiveSamples.Num();
	check(SrcMaterial.EmissiveSize == DstMaterial.EmissiveSize || NumSrcSamples == 1 || NumDstSamples == 1);
	if (NumSrcSamples && (SrcMaterial.EmissiveScale > 0 || DstMaterial.EmissiveScale > 0))
	{
		if (NumSrcSamples && !NumDstSamples)
		{
			// Dst is empty, Src is not - simply copy data
			DstMaterial.EmissiveSamples = SrcMaterial.EmissiveSamples;
			DstMaterial.EmissiveScale = SrcMaterial.EmissiveScale;
		}
		else if (NumSrcSamples && NumDstSamples)
		{
			check(NumSrcSamples == NumDstSamples || NumSrcSamples == 1 || NumDstSamples == 1);
			if (NumDstSamples < NumSrcSamples)
			{
				// Need to expand DstSamples to fit SrcSamples size.
				check(NumDstSamples == 1);
				DstMaterial.EmissiveSamples.Init(DstMaterial.EmissiveSamples[0], NumSrcSamples);
			}
			float DstEmissiveScale, SrcEmissiveScale;
			float ResultEmissiveScale;
			if (SrcMaterial.EmissiveScale > DstMaterial.EmissiveScale)
			{
				// Use SrcMaterial's EmissiveScale for result
				ResultEmissiveScale = SrcMaterial.EmissiveScale;
				SrcEmissiveScale = 1.0f;
				DstEmissiveScale = DstMaterial.EmissiveScale / SrcMaterial.EmissiveScale;
			}
			else
			{
				// Use DstMaterial's EmissiveScale for result
				ResultEmissiveScale = DstMaterial.EmissiveScale;
				DstEmissiveScale = 1.0f;
				SrcEmissiveScale = SrcMaterial.EmissiveScale / DstMaterial.EmissiveScale;
			}
			// Merge samples
			const FColor* SrcColor = SrcMaterial.EmissiveSamples.GetData();
			FColor* DstColor = DstMaterial.EmissiveSamples.GetData();
			int32 SrcStep = (NumSrcSamples == 1) ? 0 : 1;
			int32 NumSamples = FMath::Max(NumSrcSamples, NumDstSamples);
			for (int32 Sample = 0; Sample < NumSamples; Sample++)
			{
				DstColor->R = FMath::FloorToInt(FMath::Max<float>(SrcColor->R * SrcEmissiveScale, DstColor->R * DstEmissiveScale));
				DstColor->G = FMath::FloorToInt(FMath::Max<float>(SrcColor->G * SrcEmissiveScale, DstColor->G * DstEmissiveScale));
				DstColor->B = FMath::FloorToInt(FMath::Max<float>(SrcColor->B * SrcEmissiveScale, DstColor->B * DstEmissiveScale));
				SrcColor += SrcStep;
				DstColor += 1;
			}
		}
	}

#undef PROCESS_FIELD
}

/*
 * Debugging tool: save FFlattenMaterial BMP files file.
 */
void FSimplygonUtilities::SaveFlattenMaterial(const FFlattenMaterial& Material, const TCHAR* Filename)
{
#ifdef SAVE_INTERMEDIATE_TEXTURES

#define PROCESS_FIELD(Index, SizeField, SamplesField, Name) \
	case Index: \
		Size = &Material.SizeField; \
		Samples = &Material.SamplesField; \
		PropName = TEXT(Name); \
		break;

	for (int32 ChannelIndex = 0; /* empty */; ChannelIndex++)
	{
		const FIntPoint* Size = nullptr;
		const TArray<FColor>* Samples = nullptr;
		const TCHAR* PropName = nullptr;
		switch (ChannelIndex)
		{
		PROCESS_FIELD(0, DiffuseSize, DiffuseSamples, "Diffuse");
		PROCESS_FIELD(1, NormalSize, NormalSamples, "Normal");
		PROCESS_FIELD(2, MetallicSize, MetallicSamples, "Metallic");
		PROCESS_FIELD(3, RoughnessSize, RoughnessSamples, "Roughness");
		PROCESS_FIELD(4, SpecularSize, SpecularSamples, "Specular");
		PROCESS_FIELD(5, OpacitySize, OpacitySamples, "Opacity");
		PROCESS_FIELD(6, AOSize, AOSamples, "AO");
		PROCESS_FIELD(7, EmissiveSize, EmissiveSamples, "Emissive");
		}
		if (Size == nullptr)
		{
			break;
		}

		if (Samples->Num() > 0)
		{
			FString FilenameString = FString::Printf(
				SAVE_INTERMEDIATE_TEXTURES TEXT("/%s-%s.bmp"), Filename, PropName);
			FFileHelper::CreateBitmap(*FilenameString, Size->X, Size->Y, Samples->GetData());
		}
	}

#undef PROCESS_FIELD
#endif // SAVE_INTERMEDIATE_TEXTURES
}

void FSimplygonUtilities::AnalyzeMaterial(
	UMaterialInterface* InMaterial,
	const FSimplygonMaterialLODSettings& InMaterialLODSettings,
	int32& OutNumTexCoords,
	bool& OutUseVertexColor,
	bool& OutHasNonRepeatingPattern,
	bool& OutUsePerActorData)
{
	OutNumTexCoords = 0;
	OutUseVertexColor = false;
	OutHasNonRepeatingPattern = false;
	for (int32 ChannelIndex = 0; ChannelIndex < InMaterialLODSettings.ChannelsToCast.Num(); ChannelIndex++)
	{
		const FSimplygonChannelCastingSettings& Channel = InMaterialLODSettings.ChannelsToCast[ChannelIndex];
		if (!Channel.bActive)
		{
			continue;
		}
		EMaterialProperty Property;
		switch (Channel.MaterialChannel)
		{
		case ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_BASECOLOR:
			Property = MP_BaseColor;
			break;
		case ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_SPECULAR:
			Property = MP_Specular;
			break;
		case ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_ROUGHNESS:
			Property = MP_Roughness;
			break;
		case ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_METALLIC:
			Property = MP_Metallic;
			break;
		case ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_NORMALS:
			Property = MP_Normal;
			break;
		case ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_OPACITY:
			{
				EBlendMode BlendMode = InMaterial->GetBlendMode();
				if (BlendMode == BLEND_Masked)
				{
					Property = MP_OpacityMask;
				}
				else if (IsTranslucentBlendMode(BlendMode))
				{
					Property = MP_Opacity;
				}
				else
				{
					continue;
				}
			}
			break;
		case ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_EMISSIVE:
			Property = MP_EmissiveColor;
			break;
		default:
			UE_LOG(LogSimplygonUtilities, Error, TEXT("Unsupported material channel: %d"), (int32)Channel.MaterialChannel);
			continue;
		}
		// Analyze this material channel.
		int32 NumTextureCoordinates = 0;
		bool bUseVertexColor = false;
		bool bHasNonRepeatingPattern = false;
		bool bUsePerActorData = false;
		InMaterial->AnalyzeMaterialProperty(Property, NumTextureCoordinates, bUseVertexColor, bHasNonRepeatingPattern, bUsePerActorData);
		// Accumulate data.
		OutNumTexCoords = FMath::Max(NumTextureCoordinates, OutNumTexCoords);
		OutUseVertexColor |= bUseVertexColor;
		OutHasNonRepeatingPattern |= bHasNonRepeatingPattern;
		OutUsePerActorData |= bUsePerActorData;
	}
}

void FSimplygonUtilities::GetTextureCoordinateBoundsForRawMesh(const FRawMesh& InRawMesh, FBox2D& OutBounds)
{
	int32 NumWedges = InRawMesh.WedgeIndices.Num();
	int32 NumTris = NumWedges / 3;

	OutBounds.Init();
	int32 WedgeIndex = 0;
	for (int32 TriIndex = 0; TriIndex < NumTris; TriIndex++)
	{
		for (int32 CornerIndex = 0; CornerIndex < 3; CornerIndex++, WedgeIndex++)
		{
			OutBounds += InRawMesh.WedgeTexCoords[0][WedgeIndex];
		}
	}
	return;
}

void FSimplygonUtilities::GetTextureCoordinateBoundsForSkeletalMesh(const FStaticLODModel& LODModel, FBox2D& OutBounds)
{
	TArray<FSoftSkinVertex> Vertices;
	FMultiSizeIndexContainerData IndexData;
	LODModel.GetVertices(Vertices);
	LODModel.MultiSizeIndexContainer.GetIndexBufferData(IndexData);

#if WITH_APEX_CLOTHING
	const uint32 SectionCount = (uint32)LODModel.NumNonClothingSections();
#else
	const uint32 SectionCount = LODModel.Sections.Num();
#endif // #if WITH_APEX_CLOTHING
//	const uint32 TexCoordCount = LODModel.NumTexCoords;
//	check( TexCoordCount <= MAX_TEXCOORDS );

	OutBounds.Init();

	for (uint32 SectionIndex = 0; SectionIndex < SectionCount; ++SectionIndex)
	{
		const FSkelMeshSection& Section = LODModel.Sections[SectionIndex];
		const uint32 FirstIndex = Section.BaseIndex;
		const uint32 LastIndex = FirstIndex + Section.NumTriangles * 3;

		for (uint32 Index = FirstIndex; Index < LastIndex; ++Index)
		{
			uint32 VertexIndex = IndexData.Indices[Index];
			FSoftSkinVertex& Vertex = Vertices[VertexIndex];

//			for( uint32 TexCoordIndex = 0; TexCoordIndex < TexCoordCount; ++TexCoordIndex )
			{
//				FVector2D TexCoord = Vertex.UVs[TexCoordIndex];
				FVector2D TexCoord = Vertex.UVs[0];
				OutBounds += TexCoord;
			}
		}
	}
}

// Load material and all linked expressions/textures. This function will do nothing useful if material is already loaded.
// It's intended to work when we're baking a new material during engine startup, inside a PostLoad call - in this case we
// could have ExportMaterial() call for material which has not all components loaded yet.
void FSimplygonUtilities::FullyLoadMaterialStatic(UMaterialInterface* Material)
{
	FLinkerLoad* Linker = Material->GetLinker();
	if (Linker == nullptr)
	{
		return;
	}
	// Load material and all expressions.
	Linker->LoadAllObjects(true);
	Material->ConditionalPostLoad();
	// Now load all used textures.
	TArray<UTexture*> Textures;
	Material->GetUsedTextures(Textures, EMaterialQualityLevel::Num, true, MATERIAL_FEATURE_LEVEL, true);
	for (int32 i = 0; i < Textures.Num(); ++i)
	{
		UTexture* Texture = Textures[i];
		FLinkerLoad* Linker = Material->GetLinker();
		if (Linker)
		{
			Linker->Preload(Texture);
		}
	}
}

void FSimplygonUtilities::FullyLoadMaterial(UMaterialInterface* Material)
{
	FullyLoadMaterialStatic(Material);
}

void FSimplygonUtilities::PurgeMaterialTextures(UMaterialInterface* Material)
{
	TArray<UTexture*> Textures;
	Material->GetUsedTextures(Textures, EMaterialQualityLevel::Num, true, MATERIAL_FEATURE_LEVEL, true);
	for (int32 i = 0; i < Textures.Num(); ++i)
	{
		UTexture* Texture = Textures[i];
		Texture->MarkPlatformDataTransient();
	}
}

void FSimplygonUtilities::SaveMaterial(UMaterialInterface* Material)
{
	if (!GIsEditor || IsRunningCommandlet() || !Material)
	{
		return;
	}

	if (IsAsyncLoading() || IsLoading())
	{
		// We're most likely inside PostLoad() call, so don't do any saving right now.
		return;
	}

	UPackage* MaterialPackage = Material->GetOutermost();
	if (!MaterialPackage)
	{
		return;
	}

	UE_LOG(LogSimplygonUtilities, Log, TEXT("Saving material: %s"), *Material->GetName());
	TArray<UPackage*> PackagesToSave;
	TArray<UTexture*> Textures;

	Material->GetUsedTextures(Textures, EMaterialQualityLevel::Num, false, MATERIAL_FEATURE_LEVEL, true);
	for (int32 i = 0; i < Textures.Num(); ++i)
	{
		UTexture* Texture = Textures[i];
		if (Texture)
		{
			UPackage* Package = Texture->GetOutermost();
			FString Name = Package->GetPathName();
			// Avoid saving engine content we probably used in material
			if (!Name.StartsWith(TEXT("/Engine")))
			{
				PackagesToSave.Add(Package);
			}
		}
	}

	PackagesToSave.Add(MaterialPackage);
	FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, /*bCheckDirty=*/ false, /*bPromptToSave=*/ false);
	UE_LOG(LogSimplygonUtilities, Log, TEXT("Saved successfully (%s)"), *Material->GetName());
}

UTextureRenderTarget2D* FSimplygonUtilities::CreateRenderTarget(bool bInForceLinearGamma, EPixelFormat InPixelFormat, const FIntPoint& InTargetSize)
{
	// Find any pooled render tagret with suitable properties.
	for (int32 RTIndex = 0; RTIndex < RTPool.Num(); RTIndex++)
	{
		UTextureRenderTarget2D* RT = RTPool[RTIndex];
		if (RT->SizeX == InTargetSize.X &&
			RT->SizeY == InTargetSize.Y &&
			RT->OverrideFormat == InPixelFormat &&
			RT->bForceLinearGamma == bInForceLinearGamma)
		{
			return RT;
		}
	}

	// Not found - create a new one.
	UTextureRenderTarget2D* NewRT = NewObject<UTextureRenderTarget2D>();
	check(NewRT);
	NewRT->AddToRoot();
	NewRT->ClearColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
	NewRT->TargetGamma = 0.0f;
	NewRT->InitCustomFormat(InTargetSize.X, InTargetSize.Y, InPixelFormat, bInForceLinearGamma);

	RTPool.Add(NewRT);
	return NewRT;
}

void FSimplygonUtilities::ClearRTPool()
{
	if (CurrentlyRendering)
	{
		// Just in case - if garbage collection will happen during rendering, don't allow to GC used render target.
		return;
	}

	// Allow GC'ing of all render targets.
	for (int32 RTIndex = 0; RTIndex < RTPool.Num(); RTIndex++)
	{
		RTPool[RTIndex]->RemoveFromRoot();
	}
	RTPool.Empty();
}

//PRAGMA_ENABLE_OPTIMIZATION

#undef SHOW_WIREFRAME_MESH
#undef VISUALIZE_DILATION
#undef SAVE_INTERMEDIATE_TEXTURES
