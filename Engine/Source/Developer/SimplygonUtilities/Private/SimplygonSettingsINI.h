#pragma once

//static const string for section and value key lookup
static const TCHAR* UserMapSectionFormat = TEXT("UserMap%dSection");
static const TCHAR* PathCombineFormat = TEXT("%s/%s");
static const TCHAR* AutoLODSection = TEXT("Root/AutoLODSection");
static const TCHAR* LODCollectionSection = TEXT("Root/LODCollectionSection");
static const TCHAR* LODSectionFormatString = TEXT("Root/LODCollectionSection/LOD%dSection");
static const TCHAR* ReductionQualitySection = TEXT("MeshReductionQualitySection/FeaturePreservationSection");
static const TCHAR* ReductionPreProcesisngSection = TEXT("MeshReductionQualitySection/MeshPreProcessingSection");
static const TCHAR* ReductionPostProcesisngSection = TEXT("MeshReductionQualitySection/MeshPostProcessingSection");
static const TCHAR* NormalRecalSection = TEXT("MeshReductionQualitySection/MeshPostProcessingSection/NormalCalcSection");
static const TCHAR* ReductionMappingSection = TEXT("ReductionMappingSection");
static const TCHAR* AutomaticTextureSizeSection = TEXT("AutomaticTextureSizeSection");
static const TCHAR* TexCoordsGenerationSection = TEXT("TexCoordsGenerationSection");
static const TCHAR* OpacityMapSection = TEXT("CastChannelsSection/Opacity");
static const TCHAR* DiffuseMapSection = TEXT("CastChannelsSection/Diffuse");
static const TCHAR* SpecularMapSection = TEXT("CastChannelsSection/Specular");
static const TCHAR* NormalsMapSection = TEXT("CastChannelsSection/Normals");
static const TCHAR* UserMapSection = TEXT("CastChannelsSection/UserMapSection");
static const TCHAR* GroundPlaneSection = TEXT("GroundPlaneSection");
static const TCHAR* ProxyNormalRecalSection = TEXT("ProxySettingsSection/ProxyPostProcessingSection/ProxyNormalCalcSection");
static const TCHAR* ProxySettingsSection = TEXT("ProxySettingsSection");		
static const TCHAR* CascadedLODChain = TEXT("CascadedLODChain");
static const TCHAR* LODCount = TEXT("LODCount");
static const TCHAR* ProcessingType = TEXT("ProcessingType");
static const TCHAR* ReductionRatio = TEXT("ReductionRatio");
static const TCHAR* VertexColorImportance = TEXT("VertexColorImportance");
static const TCHAR* GeometricImportance = TEXT("GeometricImportance");
static const TCHAR* UVImportance = TEXT("UVImportance");
static const TCHAR* MaterialImportance = TEXT("MaterialImportance");
static const TCHAR* ShadingImportance = TEXT("ShadingImportance");
static const TCHAR* ObjectImportance = TEXT("ObjectImportance");
static const TCHAR* WeldingThreshold = TEXT("WeldingThreshold");
static const TCHAR* InvalidNormalRepair = TEXT("InvalidNormalRepair");
static const TCHAR* IsEnabled = TEXT("IsEnabled");
static const TCHAR* HardAngle = TEXT("HardAngle");
static const TCHAR* MaterialType = TEXT("MaterialType");
static const TCHAR* TexXSize = TEXT("TexXSize");
static const TCHAR* TexYSize = TEXT("TexYSize");
static const TCHAR* Supersampling = TEXT("Supersampling");
static const TCHAR* Power2 = TEXT("Power2");
static const TCHAR* GutterSpace = TEXT("GutterSpace");
static const TCHAR* MaxStretch = TEXT("MaxStretch");
static const TCHAR* sRGB = TEXT("sRGB");
static const TCHAR* BakeVertexColors = TEXT("BakeVertexColors");
static const TCHAR* ColorChannelsOverride = TEXT("ColorChannelsOverride");
static const TCHAR* FlipBackfacingNormals = TEXT("FlipBackfacingNormals");
static const TCHAR* TangentSpaceNormals = TEXT("TangentSpaceNormals");
static const TCHAR* NearestNeighborFillBetweenCharts = TEXT("NearestNeighborFillBetweenCharts");
static const TCHAR* FlipGreenAxis = TEXT("FlipGreenAxis");
static const TCHAR* MapCount = TEXT("MapCount");
static const TCHAR* ChannelName = TEXT("ChannelName");
static const TCHAR* ChannelType = TEXT("ChannelType");
static const TCHAR* SizeOnScreen = TEXT("SizeOnScreen");
static const TCHAR* GroundPlaneAxis = TEXT("GroundPlaneAxis");
static const TCHAR* GroundPlaneLevel = TEXT("GroundPlaneLevel");
static const TCHAR* MergeDistance = TEXT("MergeDistance");
static const TCHAR* LimitTriangleSize = TEXT("LimitTriangleSize");
static const TCHAR* MaxTriangleSize = TEXT("MaxTriangleSize");
static const TCHAR* ChannelTypeKey = TEXT("ChannelType");

class ISimplygonSettingsIniIO
{
public:
	virtual void Read( const FString& Filename, TArray<TSharedPtr<FSimplygonSettingsLODInfo>>& LODConfigInfo ) = 0;
	virtual bool Write( const FString& Filename, const  TArray<TSharedPtr<FSimplygonSettingsLODInfo>>& LODConfigInfo ) = 0;

	virtual bool GetString( const TCHAR* Section, const TCHAR* Key, FString& Value ) const = 0;
	virtual bool GetText( const TCHAR* Section, const TCHAR* Key, FText& Value ) const = 0;
	virtual bool GetInt64( const TCHAR* Section, const TCHAR* Key, int64& Value ) const = 0 ;
	virtual bool GetInt32( const TCHAR* Section, const TCHAR* Key, int32& Value ) const = 0 ;
	virtual bool GetFloat( const TCHAR* Section, const TCHAR* Key, float& Value ) const = 0 ;
	virtual bool GetBoolean( const TCHAR* Section, const TCHAR* Key, bool& Value ) const = 0 ;
	virtual bool GetAxisIndex(const TCHAR* Section, const TCHAR* Key, int32& Value) = 0;
	virtual bool GetFeatureImportance(const TCHAR* Section, const TCHAR* Key, EMeshFeatureImportance::Type& Value) = 0;
	virtual bool GetTextureStrech(const TCHAR* Section, const TCHAR* Key, ESimplygonTextureStrech::Type& Value) = 0;
	virtual bool GetTextureSamplingQuality(const TCHAR* Section, const TCHAR* Key, ESimplygonTextureSamplingQuality::Type& Value) = 0;
	virtual bool GetTextureResolution(const TCHAR* Section, const TCHAR* Key, ESimplygonTextureResolution::Type& Value) = 0;

	virtual void SetString( const TCHAR* Section, const TCHAR* Key, const TCHAR* Value ) = 0 ;
	virtual void SetText( const TCHAR* Section, const TCHAR* Key, const FText& Value ) = 0 ;
	virtual void SetInt64( const TCHAR* Section, const TCHAR* Key, const int64 Value ) = 0 ;
	virtual void SetInt32( const TCHAR* Section, const TCHAR* Key, const int32 Value ) = 0 ;
	virtual void SetFloat( const TCHAR* Section, const TCHAR* Key, const float Value ) = 0 ;
	virtual void SetBoolean( const TCHAR* Section, const TCHAR* Key, const bool Value ) = 0 ;

};

class FSimplygonSettingsIni : public TMap<FString, FConfigSection>, ISimplygonSettingsIniIO
{
public:
	FSimplygonSettingsIni();

	/** ISimplygonSettingsIniIO interface */
	virtual void Read( const FString& Filename, TArray<TSharedPtr<FSimplygonSettingsLODInfo>>& LODConfigInfo ) override;
	virtual bool Write( const FString& Filename, const TArray<TSharedPtr<FSimplygonSettingsLODInfo>>& LODConfigInfo ) override;
	virtual bool GetString( const TCHAR* Section, const TCHAR* Key, FString& Value ) const override;
	virtual bool GetText( const TCHAR* Section, const TCHAR* Key, FText& Value ) const override;
	virtual bool GetInt64( const TCHAR* Section, const TCHAR* Key, int64& Value ) const override;
	virtual bool GetInt32( const TCHAR* Section, const TCHAR* Key, int32& Value ) const override;
	virtual bool GetFloat( const TCHAR* Section, const TCHAR* Key, float& Value ) const override;
	virtual bool GetBoolean( const TCHAR* Section, const TCHAR* Key, bool& Value ) const override;
	virtual bool GetAxisIndex(const TCHAR* Section, const TCHAR* Key, int32& Value) override;
	virtual bool GetFeatureImportance(const TCHAR* Section, const TCHAR* Key, EMeshFeatureImportance::Type& Value) override;
	virtual bool GetTextureStrech(const TCHAR* Section, const TCHAR* Key, ESimplygonTextureStrech::Type& Value) override;
	virtual bool GetTextureSamplingQuality(const TCHAR* Section, const TCHAR* Key, ESimplygonTextureSamplingQuality::Type& Value) override;
	virtual bool GetTextureResolution(const TCHAR* Section, const TCHAR* Key, ESimplygonTextureResolution::Type& Value) override;
	virtual void SetString( const TCHAR* Section, const TCHAR* Key, const TCHAR* Value ) override;
	virtual void SetText( const TCHAR* Section, const TCHAR* Key, const FText& Value ) override;
	virtual void SetInt64( const TCHAR* Section, const TCHAR* Key, const int64 Value ) override;
	virtual void SetInt32(const TCHAR* Section, const TCHAR* Key, const int32 Value) override;
	virtual void SetFloat( const TCHAR* Section, const TCHAR* Key, const float Value ) override;
	virtual void SetBoolean(const TCHAR* Section, const TCHAR* Key, const bool Value) override;


	static FString AsString( const FText& Text );
	void ProcessInputFileContents(const FString& Filename, FString& Contents);
	void ParseIniIntoLODInfoArray(TArray<TSharedPtr<FSimplygonSettingsLODInfo>>& LODConfigInfo);

private:
	TArray<FString> FeatureImportanceOptions;
	TArray<FString> TextureStrechOptions;
	TArray<FString> CasterTypeOptions;
	TArray<FString> ColorChannelOptions;
	TArray<FString> MaterialLODTypeOptions;
	TArray<FString> TextureSmplingQualityOptions;
	TArray<FString> TextureResolutionOptions;
};