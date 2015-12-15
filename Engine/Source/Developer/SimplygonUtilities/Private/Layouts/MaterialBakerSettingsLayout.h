#pragma once
#include "PropertyEditing.h"

#define SIMPLYGON_NUM_CHANNELS_TO_BAKE 8

/************************************************************************/
/* Layout: Channel Casting Settings                                     */
/************************************************************************/
class FCastingSettingsLayout : public IDetailCustomNodeBuilder, public TSharedFromThis<FCastingSettingsLayout>
{
public:
	FCastingSettingsLayout(int32 InLODIndex, int32 InChannelIndex, const FSimplygonMaterialLODSettings& InMaterialLODSettings, bool bDefaultEnabled = false);
	virtual ~FCastingSettingsLayout();

	const FSimplygonChannelCastingSettings& GetSettings() const;
	void UpdateSettings(const FSimplygonChannelCastingSettings& InSettings);

	void SetActive(bool bActive);

private:
	virtual void SetOnRebuildChildren( FSimpleDelegate InOnRegenerateChildren ) override {}
	virtual void GenerateHeaderRowContent( FDetailWidgetRow& NodeRow ) override;
	virtual void GenerateChildContent( IDetailChildrenBuilder& ChildrenBuilder ) override;
	virtual void Tick( float DeltaTime ) override{}
	virtual bool RequiresTick() const override { return false; }
	virtual FName GetName() const override { static FName SimplygonMaterialLODSettings("SimplygonChannelCasterSettings"); return SimplygonMaterialLODSettings; }
	virtual bool InitiallyCollapsed() const override { return true; }

	bool IsMaterialLODEnabled() const;

	EVisibility IsChannelCasterSetting() const;
	EVisibility IsColorCaster() const;
	EVisibility IsNormalsCaster() const;
	EVisibility IsMaterialLODSettingEnabled() const;

	//Cast Channel
	void OnCastChannelChanged(ECheckBoxState NewValue);
	ECheckBoxState IsCastChannelChecked() const;

	//Output Channels
	void OnColorChannelsTypeChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo);

	//Use sRGB
	void OnUseSRGBChanged(ECheckBoxState NewValue);
	ECheckBoxState IsUseSRGBChecked() const;
	bool IsUseSRGBEnabled() const;
	bool IsUseSRGBDisabled() const;

	//Bake Vertex Color
	void OnBakeVertexColorChanged(ECheckBoxState NewValue);
	ECheckBoxState IsBakeVertexColorChecked() const;
	bool IsBakeVertexColorEnabled() const;
	bool IsBakeVertexColorDisabled() const;

	//use tangent space normals
	void OnTangentSpaceNormalsChanged(ECheckBoxState NewValue);
	ECheckBoxState IsTangentSpaceNormalsChecked() const;
	bool IsTangentSpaceNormalsEnabled() const;
	bool IsTangentSpaceNormalsDisabled() const;

	//Flip backfacing normals
	void OnFlipBackfacingNormalsChanged(ECheckBoxState NewValue);
	ECheckBoxState IsFlipBackfacingNormalsChecked() const;
	bool IsFlipBackfacingNormalsEnabled() const;
	bool IsFlipBackfacingNormalsDisabled() const;

	//Flip Green Channel
	void OnFlipGreenChannelChanged(ECheckBoxState NewValue);
	ECheckBoxState IsFlipGreenChannelChecked() const;
	bool IsFlipGreenChannelEnabled() const;
	bool IsFlipGreenChannelDisabled() const;

private:
	TArray<TSharedPtr<FString> > CasterTypeOptions;
	TSharedPtr<class STextComboBox>  CasterTypeCombo;

	TArray<TSharedPtr<FString> > ColorChannelsOptions;
	TSharedPtr<class STextComboBox> ColorChannelsCombo;

	TSharedPtr<class SCheckBox> CastChannel;

	//TSharedPtr<FString> MaterialChannelName;

	UEnum& MaterialChannelEnum;
	FSimplygonChannelCastingSettings CasterSetting;

	int32 LODindex;
	int32 ChannelIndex;

	const FSimplygonMaterialLODSettings& ParentMaterialLODSettings;
};

/************************************************************************/
/* Layout: MaterialLOD Settings                                         */
/************************************************************************/
class FMaterialLODSettingsLayout : public IMaterialLODSettingsLayout
{
public:
	// Note: InShowVertexDataOption should be removed when all LOD types will support this option
	FMaterialLODSettingsLayout(int32 InLODIndex, bool InAllowReuseUVs, bool InAllowMultiMaterial = false, bool InShowVertexDataOption = false, bool InForceActive = false);

	virtual const FSimplygonMaterialLODSettings& GetSettings() override;
	virtual void UpdateSettings(const FSimplygonMaterialLODSettings& InSettings) override;

	const FSimplygonChannelCastingSettings& GetSettingsFromChannelWidget(uint8 ChannelIndex ) const override;

	virtual void SetAllowReuseUVs(bool InAllowReuseUVs) override;
	virtual void SetAllowMultiMaterial(bool InAllowMultiMaterial) override;
	virtual void SetForceActive(bool InForceActive) override;

private:
	/** IDetailCustomNodeBuilder Interface*/
	virtual void SetOnRebuildChildren( FSimpleDelegate InOnRegenerateChildren ) override {}
	virtual void GenerateHeaderRowContent( FDetailWidgetRow& NodeRow ) override;
	virtual void GenerateChildContent( IDetailChildrenBuilder& ChildrenBuilder ) override;
	virtual void Tick( float DeltaTime ) override{}
	virtual bool RequiresTick() const override { return false; }
	virtual FName GetName() const override { static FName SimplygonMaterialLODSettings("SimplygonMaterialLODSettings"); return SimplygonMaterialLODSettings; }
	virtual bool InitiallyCollapsed() const override { return true; }

	EVisibility IsMaterialLODSettingEnabled() const;
	EVisibility IsReuseUvVisible() const;
	EVisibility IsMultiMaterialVisible() const;

	void SetEnableMaterialLODState(ECheckBoxState NewValue);
	ECheckBoxState GetEnableMaterialLODState() const;
	bool CouldBeInactive() const;

	// Automatic Texture sizing
	void OnUseAutomaticTextureSizesChanged(ECheckBoxState NewValue);
	ECheckBoxState IsUseAutomaticTextureSizesChecked() const;
	bool IsUseAutomaticTextureSizeEnabled() const;
	bool IsUseAutomaticTextureSizeDisabled() const;

	//Use existing UVs
	void OnUseExistingChartsChanged(ECheckBoxState NewValue);
	ECheckBoxState IsUseExistingChartsChecked() const;

	//Allow multi-material
	void OnMultiMaterialChanged(ECheckBoxState NewValue);
	ECheckBoxState IsMultiMaterialChecked() const;

	//Prefer two-sided
	void OnPreferTwoSidedChanged(ECheckBoxState NewValue);
	ECheckBoxState IsPreferTwoSidedChecked() const;

	//Bake vertex data
	void OnBakeVertexDataChanged(ECheckBoxState NewValue);
	ECheckBoxState IsBakeVertexDataChecked() const;

	//Bake actor data
	void OnBakeActorDataChanged(ECheckBoxState NewValue);
	ECheckBoxState IsBakeActorDataChecked() const;
	bool IsBakeActorDataEnabled() const;

	//TextureWidth and Height
	void OnTextureWidthChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo);
	void OnTextureHeightChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo);

	//Sampling Quality
	void OnSamplingQualityChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo);

	//Texture Stretch
	void OnTextureStrechChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo);

	//Gutter space
	int32 GetGutterSpace() const;
	void OnGutterSpaceChanged(int32 NewValue);
	void OnGutterSpaceCommitted(int32 NewValue, ETextCommit::Type TextCommitType);

private:
	//Texture resolution
	TArray<TSharedPtr<FString> > TextureResolutionOptions;
	TSharedPtr<STextComboBox> TextureHeightCombo;
	TSharedPtr<STextComboBox> TextureWidthCombo;

	//Texture Stretch
	TArray<TSharedPtr<FString> > TextureStretchOptions;
	TSharedPtr<class STextComboBox> TextureStretchTypesCombo;

	//Sampling Quality
	TArray<TSharedPtr<FString> > SamplingQualityOptions;
	TSharedPtr<class STextComboBox> SamplingQualityTypeCombo;

	//Caster stuff
	TArray<TSharedPtr<FString> > CasterOptions;
	TSharedPtr<class STextComboBox> CasterTypeCombo;
	TSharedPtr<FCastingSettingsLayout> ChannelsWidgets[SIMPLYGON_NUM_CHANNELS_TO_BAKE];

	FSimplygonMaterialLODSettings MaterialLODSettings;

	TSharedPtr<class SCheckBox> EnableMaterialLOD;

	int32 LODIndex;
	bool bAllowReuseUVs;
	bool bAllowMultiMaterial;
	bool bForceActive;
	bool bShowVertexDataOption;
};