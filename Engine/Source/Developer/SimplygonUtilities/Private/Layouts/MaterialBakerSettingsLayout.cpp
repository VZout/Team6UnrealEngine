#include "SimplygonUtilitiesPrivatePCH.h"
#include "MaterialBakerSettingsLayout.h"

#include "STextComboBox.h"

#define LOCTEXT_NAMESPACE "MaterialBakerSettingsLayout"

/************************************************************************/
/* Layout: Channel Casting Settings                                     */
/************************************************************************/
FCastingSettingsLayout::FCastingSettingsLayout(int32 InLODIndex, int32 InChannelIndex, const FSimplygonMaterialLODSettings& InMaterialLODSettings, bool bDefaultEnabled)
	: MaterialChannelEnum(FSimplygonEnum::GetMaterialChannelEnum()),
	  LODindex(InLODIndex), 
	  ChannelIndex(InChannelIndex), 
	  ParentMaterialLODSettings(InMaterialLODSettings)
{
	FSimplygonEnum::FillEnumOptions(CasterTypeOptions, FSimplygonEnum::GetCasterTypeEnum());
	FSimplygonEnum::FillEnumOptions(ColorChannelsOptions, FSimplygonEnum::GetColorChannelEnum());
}

FCastingSettingsLayout::~FCastingSettingsLayout()
{}

void FCastingSettingsLayout::GenerateHeaderRowContent( FDetailWidgetRow& NodeRow )
{
	FText MaterialChannelText = MaterialChannelEnum.GetDisplayNameText((int32)CasterSetting.MaterialChannel.GetValue());
	//MaterialChannelName = MakeShareable(new FString(MaterialChannelText.ToString()));

	NodeRow
		.NameContent()
		[
			SNew(STextBlock)
			.Text(MaterialChannelText)
			.Font( IDetailLayoutBuilder::GetDetailFont() )
			.IsEnabled(this, &FCastingSettingsLayout::IsMaterialLODEnabled)
		]
		.ValueContent()
		[
			SAssignNew(CastChannel, SCheckBox)
			.IsChecked(this, &FCastingSettingsLayout::IsCastChannelChecked)
			.OnCheckStateChanged(this, &FCastingSettingsLayout::OnCastChannelChanged)
			.IsEnabled(this, &FCastingSettingsLayout::IsMaterialLODEnabled)
		];
}

void FCastingSettingsLayout::GenerateChildContent( IDetailChildrenBuilder& ChildrenBuilder )
{
	TAttribute<EVisibility> CastingSettingsVisibility( this, &FCastingSettingsLayout::IsChannelCasterSetting);
	TAttribute<EVisibility> ColorCasterSettingsVisibility( this, &FCastingSettingsLayout::IsColorCaster);
	TAttribute<EVisibility> NormalCasterSettingsVisibility( this, &FCastingSettingsLayout::IsNormalsCaster);
	{
		//Output Channels
		ChildrenBuilder.AddChildContent( LOCTEXT("CasterSettingColorChannels", "Output Channels") )
			.Visibility(CastingSettingsVisibility)
			.NameContent()
			[
				SNew(STextBlock)
				.Font( IDetailLayoutBuilder::GetDetailFont() )
				.Text(LOCTEXT("OutputChannels", "Output Channels"))
			]
		.ValueContent()
			[
				SAssignNew(ColorChannelsCombo, STextComboBox)
				.ContentPadding(0)
				.OptionsSource(&ColorChannelsOptions)
				.InitiallySelectedItem(ColorChannelsOptions[CasterSetting.ColorChannels])
				.OnSelectionChanged(this, &FCastingSettingsLayout::OnColorChannelsTypeChanged)

			];

		//Use sRGB
		ChildrenBuilder.AddChildContent( LOCTEXT("MaterialLODUseSRGB", "SRGB") )
			.Visibility(ColorCasterSettingsVisibility)
			.NameContent()
			[
				SNew(STextBlock)
				.Font( IDetailLayoutBuilder::GetDetailFont() )
				.Text(LOCTEXT("UseSRGB", "SRGB"))
			]
		.ValueContent()
			[
				SNew(SCheckBox)
				.IsChecked(this, &FCastingSettingsLayout::IsUseSRGBChecked)
				.OnCheckStateChanged(this, &FCastingSettingsLayout::OnUseSRGBChanged)
			];

		//Bake Vertex Color
		ChildrenBuilder.AddChildContent( LOCTEXT("MaterialLODBakeVertexColors", "Bake Vertex Colors") )
			.Visibility(ColorCasterSettingsVisibility)
			.NameContent()
			[
				SNew(STextBlock)
				.Font( IDetailLayoutBuilder::GetDetailFont() )
				.Text(LOCTEXT("BakeVertexColors", "Bake VertexColors"))
			]
		.ValueContent()
			[
				SNew(SCheckBox)
				.IsChecked(this, &FCastingSettingsLayout::IsBakeVertexColorChecked)
				.OnCheckStateChanged(this, &FCastingSettingsLayout::OnBakeVertexColorChanged)
			];

		//Tangentspace Normals
		ChildrenBuilder.AddChildContent( LOCTEXT("MaterialLODTangentSpace", "Tangentspace Normals") )
			.Visibility(NormalCasterSettingsVisibility)
			.NameContent()
			[
				SNew(STextBlock)
				.Font( IDetailLayoutBuilder::GetDetailFont() )
				.Text(LOCTEXT("TangentspaceNormals", "Tangentspace Normals"))
			]
		.ValueContent()
			[
				SNew(SCheckBox)
				.IsChecked(this, &FCastingSettingsLayout::IsTangentSpaceNormalsChecked)
				.OnCheckStateChanged(this, &FCastingSettingsLayout::OnTangentSpaceNormalsChanged)
			];

		//Flip Backfacing Normals
		ChildrenBuilder.AddChildContent( LOCTEXT("MaterialLODFlipBackfacing", "Flip Backfacing Normals") )
			.Visibility(NormalCasterSettingsVisibility)
			.NameContent()
			[
				SNew(STextBlock)
				.Font( IDetailLayoutBuilder::GetDetailFont() )
				.Text(LOCTEXT("FlipBackfacing", "Flip Backfacing Normals"))
			]
		.ValueContent()
			[
				SNew(SCheckBox)
				.IsChecked(this, &FCastingSettingsLayout::IsFlipBackfacingNormalsChecked)
				.OnCheckStateChanged(this, &FCastingSettingsLayout::OnFlipBackfacingNormalsChanged)
			];

		//Flip Green Channel
		ChildrenBuilder.AddChildContent( LOCTEXT("MaterialLODFlipGreenChannel", "Flip Green Channel") )
			.Visibility(NormalCasterSettingsVisibility)
			.NameContent()
			[
				SNew(STextBlock)
				.Font( IDetailLayoutBuilder::GetDetailFont() )
				.Text(LOCTEXT("FlipGreenChannel", "Flip Green Channel"))
			]
		.ValueContent()
			[
				SNew(SCheckBox)
				.IsChecked(this, &FCastingSettingsLayout::IsFlipGreenChannelChecked)
				.OnCheckStateChanged(this, &FCastingSettingsLayout::OnFlipGreenChannelChanged)
			];
	}
}

const FSimplygonChannelCastingSettings& FCastingSettingsLayout::GetSettings() const
{
	return CasterSetting;
}

void FCastingSettingsLayout::UpdateSettings( const FSimplygonChannelCastingSettings& InSettings )
{
	CasterSetting = InSettings;
	//FText MaterialChannelText = MaterialChannelEnum.GetDisplayNameText((int32)InSettings.MaterialChannel.GetValue());
	//MaterialChannelName = MakeShareable(new FString(MaterialChannelText.ToString()));
}

bool FCastingSettingsLayout::IsMaterialLODEnabled() const
{
	return ParentMaterialLODSettings.bActive;
}

EVisibility FCastingSettingsLayout::IsMaterialLODSettingEnabled() const
{
	return IsMaterialLODEnabled() ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility FCastingSettingsLayout::IsChannelCasterSetting() const
{
	return CasterSetting.bActive && IsMaterialLODEnabled() ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility FCastingSettingsLayout::IsColorCaster() const
{
	return (CasterSetting.bActive && CasterSetting.Caster == ESimplygonCasterType::Color && IsMaterialLODEnabled()) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility FCastingSettingsLayout::IsNormalsCaster() const
{
	return (CasterSetting.bActive && CasterSetting.Caster == ESimplygonCasterType::Normals && IsMaterialLODEnabled()) ? EVisibility::Visible : EVisibility::Collapsed;
}

ECheckBoxState FCastingSettingsLayout::IsCastChannelChecked() const
{
	return CasterSetting.bActive ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void FCastingSettingsLayout::OnCastChannelChanged(ECheckBoxState NewValue)
{
	CasterSetting.bActive = (NewValue == ECheckBoxState::Checked);
}

void FCastingSettingsLayout::SetActive(bool bActive)
{
	CasterSetting.bActive = bActive;
}

//Output Channels
void FCastingSettingsLayout::OnColorChannelsTypeChanged( TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo )
{
	const ESimplygonColorChannels::Type ColorChannels = (ESimplygonColorChannels::Type)ColorChannelsOptions.Find(NewValue);
	if (CasterSetting.ColorChannels != ColorChannels)
	{

		CasterSetting.ColorChannels = ColorChannels;
	}
}

//Use sRGB
void FCastingSettingsLayout::OnUseSRGBChanged( ECheckBoxState NewValue )
{
	CasterSetting.bUseSRGB = (NewValue == ECheckBoxState::Checked);

}

ECheckBoxState FCastingSettingsLayout::IsUseSRGBChecked() const
{
	return CasterSetting.bUseSRGB ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; 
}

bool FCastingSettingsLayout::IsUseSRGBEnabled() const
{
	if(CasterSetting.bUseSRGB)
	{
		return true;
	}
	return false;
}

bool FCastingSettingsLayout::IsUseSRGBDisabled() const
{
	if(!CasterSetting.bUseSRGB)
	{
		return true;
	}
	return false;
}

//Bake Vertex Color
void FCastingSettingsLayout::OnBakeVertexColorChanged( ECheckBoxState NewValue )
{
	CasterSetting.bBakeVertexColors = (NewValue == ECheckBoxState::Checked);
}

ECheckBoxState FCastingSettingsLayout::IsBakeVertexColorChecked() const
{
	return CasterSetting.bBakeVertexColors ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; 
}

bool FCastingSettingsLayout::IsBakeVertexColorEnabled() const
{
	if(CasterSetting.bBakeVertexColors)
	{
		return true;
	}
	return false;
}

bool FCastingSettingsLayout::IsBakeVertexColorDisabled() const
{
	if(!CasterSetting.bBakeVertexColors)
	{
		return true;
	}
	return false;
}

//Tangentspace Normals
void FCastingSettingsLayout::OnTangentSpaceNormalsChanged( ECheckBoxState NewValue )
{
	CasterSetting.bUseTangentSpaceNormals = (NewValue == ECheckBoxState::Checked);
}

ECheckBoxState FCastingSettingsLayout::IsTangentSpaceNormalsChecked() const
{
	return CasterSetting.bUseTangentSpaceNormals ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; 
}

bool FCastingSettingsLayout::IsTangentSpaceNormalsEnabled() const
{
	if(CasterSetting.bUseTangentSpaceNormals)
	{
		return true;
	}
	return false;
}

bool FCastingSettingsLayout::IsTangentSpaceNormalsDisabled() const
{
	if(CasterSetting.bUseTangentSpaceNormals)
	{
		return true;
	}
	return false;
}

//Flip backfacing Normals
void FCastingSettingsLayout::OnFlipBackfacingNormalsChanged( ECheckBoxState NewValue )
{
	CasterSetting.bFlipBackfacingNormals = (NewValue == ECheckBoxState::Checked);
}

ECheckBoxState FCastingSettingsLayout::IsFlipBackfacingNormalsChecked() const
{
	return CasterSetting.bFlipBackfacingNormals ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; 
}


bool FCastingSettingsLayout::IsFlipBackfacingNormalsEnabled() const
{
	if(CasterSetting.bFlipBackfacingNormals)
	{
		return true;
	}
	return false;
}

bool FCastingSettingsLayout::IsFlipBackfacingNormalsDisabled() const
{
	if(CasterSetting.bFlipBackfacingNormals)
	{
		return true;
	}
	return false;
}

//Flip Green Channel
void FCastingSettingsLayout::OnFlipGreenChannelChanged(ECheckBoxState NewValue)
{
	CasterSetting.bFlipGreenChannel = (NewValue == ECheckBoxState::Checked);
}

ECheckBoxState FCastingSettingsLayout::IsFlipGreenChannelChecked() const
{
	return CasterSetting.bFlipGreenChannel ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

bool FCastingSettingsLayout::IsFlipGreenChannelEnabled() const
{
	if (CasterSetting.bFlipGreenChannel)
	{
		return true;
	}
	return false;
}

bool FCastingSettingsLayout::IsFlipGreenChannelDisabled() const
{
	if (CasterSetting.bFlipGreenChannel)
	{
		return true;
	}
	return false;
}

/************************************************************************/
/* Layout: MaterialLOD Settings                                         */
/************************************************************************/
FMaterialLODSettingsLayout::FMaterialLODSettingsLayout(int32 InLODIndex, bool InAllowReuseUVs, bool InAllowMultiMaterial, bool InShowVertexDataOption, bool InForceActive)
	: LODIndex(InLODIndex)
	, bAllowReuseUVs(InAllowReuseUVs)
	, bAllowMultiMaterial(InAllowMultiMaterial)
	, bShowVertexDataOption(InShowVertexDataOption)
{
	FSimplygonEnum::FillEnumOptions( TextureResolutionOptions , FSimplygonEnum::GetTextureResolutionEnum(), true);
	FSimplygonEnum::FillEnumOptions( TextureStretchOptions , FSimplygonEnum::GetTextureStrechEnum());
	FSimplygonEnum::FillEnumOptions( SamplingQualityOptions , FSimplygonEnum::GetSamplingQualityEnum());

	FSimplygonEnum::FillEnumOptions( CasterOptions , FSimplygonEnum::GetCasterTypeEnum());
	for(int32 channelIndex = 0; channelIndex < MaterialLODSettings.ChannelsToCast.Num(); channelIndex++)
	{

		ChannelsWidgets[channelIndex] = MakeShareable(new FCastingSettingsLayout(LODIndex, channelIndex, MaterialLODSettings));
	}

	//Default values
	SetForceActive(InForceActive);
	MaterialLODSettings.bActive = true;
}

void FMaterialLODSettingsLayout::GenerateHeaderRowContent( FDetailWidgetRow& NodeRow ) 
{
	NodeRow.NameContent()
		[
			SNew( STextBlock )
			.Text( LOCTEXT("MaterialBaking", "Material Baking") )
			.Font( IDetailLayoutBuilder::GetDetailFont() )
		];

	NodeRow.ValueContent()
		[
			SAssignNew(EnableMaterialLOD, SCheckBox)
			.IsChecked(this, &FMaterialLODSettingsLayout::GetEnableMaterialLODState)
			.IsEnabled(this, &FMaterialLODSettingsLayout::CouldBeInactive)
			.OnCheckStateChanged(this, &FMaterialLODSettingsLayout::SetEnableMaterialLODState)
		];
}
void FMaterialLODSettingsLayout::GenerateChildContent( IDetailChildrenBuilder& ChildrenBuilder )
{
	//Reuse UVs should only be available for Reduction
	TAttribute<EVisibility> ReuseUvVisibility(this, &FMaterialLODSettingsLayout::IsReuseUvVisible);
	TAttribute<EVisibility> MultiMaterialVisibility(this, &FMaterialLODSettingsLayout::IsMultiMaterialVisible);

	//Material baking settings should only be visible when it's enabled
	TAttribute<EVisibility> MaterialLODSettingsVisibility( this, &FMaterialLODSettingsLayout::IsMaterialLODSettingEnabled);
	
	{
		ChildrenBuilder.AddChildContent(LOCTEXT("MaterialLODUseUseExistingCharts", "Reuse UVs"))
			.Visibility(ReuseUvVisibility)
			.NameContent()
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text(LOCTEXT("ReuseUVs", "Reuse UVs"))
			]
		.ValueContent()
			[
				SNew(SCheckBox)
				.IsChecked(this, &FMaterialLODSettingsLayout::IsUseExistingChartsChecked)
				.OnCheckStateChanged(this, &FMaterialLODSettingsLayout::OnUseExistingChartsChanged)
			];

		if (bShowVertexDataOption)
		{
			//Bake vertex data
			ChildrenBuilder.AddChildContent(LOCTEXT("MaterialLODBakeVertexData", "Bake Vertex Data"))
				.Visibility(MaterialLODSettingsVisibility)
				.NameContent()
				[
					SNew(STextBlock)
					.Font(IDetailLayoutBuilder::GetDetailFont())
					.ToolTipText(LOCTEXT("BakeVertexDataToolTip", "Enable this option when source mesh uses vertex colors or multiple UVs in materials. For aggregate LOD this will disable sharing materials between meshes."))
					.Text(LOCTEXT("BakeVertexData", "Bake Vertex Data"))
				]
			.ValueContent()
				[
					SNew(SCheckBox)
					.IsChecked(this, &FMaterialLODSettingsLayout::IsBakeVertexDataChecked)
					.OnCheckStateChanged(this, &FMaterialLODSettingsLayout::OnBakeVertexDataChanged)
				];

			//Bake actor data
			ChildrenBuilder.AddChildContent(LOCTEXT("MaterialLODBakeActorData", "Bake Actor Data"))
				.Visibility(MaterialLODSettingsVisibility)
				.NameContent()
				[
					SNew(STextBlock)
					.Font(IDetailLayoutBuilder::GetDetailFont())
					.ToolTipText(LOCTEXT("BakeActorDataToolTip", "Enable this option when mesh materials uses features like ActorPosition or AO Mask. For aggregate LOD this will disable sharing materials between meshes."))
					.Text(LOCTEXT("BakeActorData", "Bake Actor Data"))
				]
			.ValueContent()
				[
					SNew(SCheckBox)
					.IsChecked(this, &FMaterialLODSettingsLayout::IsBakeActorDataChecked)
					.OnCheckStateChanged(this, &FMaterialLODSettingsLayout::OnBakeActorDataChanged)
					.IsEnabled(this, &FMaterialLODSettingsLayout::IsBakeActorDataEnabled)
				];
		}

		//Automatic texture size
		ChildrenBuilder.AddChildContent( LOCTEXT("MaterialLODUseAutomaticSize", "Use Automatic Sizes") )
			.Visibility(MaterialLODSettingsVisibility)
			.NameContent()
			[
				SNew(STextBlock)
				.Font( IDetailLayoutBuilder::GetDetailFont() )
				.Text(LOCTEXT("UseAutomaticSize", "Use Automatic Sizes"))
			]
		.ValueContent()
			[
				SNew(SCheckBox)
				.IsChecked(this, &FMaterialLODSettingsLayout::IsUseAutomaticTextureSizesChecked)
				.OnCheckStateChanged(this, &FMaterialLODSettingsLayout::OnUseAutomaticTextureSizesChanged)
			];

		//Texture Width
		ChildrenBuilder.AddChildContent( LOCTEXT("MaterialLODTextureWidth", "Texture Width") )
			.Visibility(MaterialLODSettingsVisibility)
			.NameContent()
			[
				SNew(STextBlock)
				.Font( IDetailLayoutBuilder::GetDetailFont() )
				.Text(LOCTEXT("TextureWidth", "Texture Width"))
			]
		.ValueContent()
			[
				SAssignNew(TextureWidthCombo, STextComboBox)
				//.Font( IDetailLayoutBuilder::GetDetailFont() )
				.ContentPadding(0)
				.OptionsSource(&TextureResolutionOptions)
				.InitiallySelectedItem(TextureResolutionOptions[MaterialLODSettings.TextureWidth])
				.OnSelectionChanged(this, &FMaterialLODSettingsLayout::OnTextureWidthChanged)
				.IsEnabled(this,&FMaterialLODSettingsLayout::IsUseAutomaticTextureSizeDisabled)
			];

		//Texture Height
		ChildrenBuilder.AddChildContent( LOCTEXT("MaterialLODTextureHeight", "Texture Height") )
			.Visibility(MaterialLODSettingsVisibility)
			.NameContent()
			[
				SNew(STextBlock)
				.Font( IDetailLayoutBuilder::GetDetailFont() )
				.Text(LOCTEXT("TextureHeight", "Texture Height"))
			]
		.ValueContent()
			[
				SAssignNew(TextureHeightCombo, STextComboBox)
				//.Font( IDetailLayoutBuilder::GetDetailFont() )
				.ContentPadding(0)
				.OptionsSource(&TextureResolutionOptions)
				.InitiallySelectedItem(TextureResolutionOptions[MaterialLODSettings.TextureHeight])
				.OnSelectionChanged(this, &FMaterialLODSettingsLayout::OnTextureHeightChanged)
				.IsEnabled(this,&FMaterialLODSettingsLayout::IsUseAutomaticTextureSizeDisabled)

			];

		//sampling quality
		ChildrenBuilder.AddChildContent( LOCTEXT("MaterialLODSamplingQuality", "Sampling Quality") )
			.Visibility(MaterialLODSettingsVisibility)
			.NameContent()
			[
				SNew(STextBlock)
				.Font( IDetailLayoutBuilder::GetDetailFont() )
				.Text(LOCTEXT("SamplingQuality", "Sampling Quality"))
			]
		.ValueContent()
			[
				SAssignNew(SamplingQualityTypeCombo, STextComboBox)
				//.Font( IDetailLayoutBuilder::GetDetailFont() )
				.ContentPadding(0)
				.OptionsSource(&SamplingQualityOptions)
				.InitiallySelectedItem(SamplingQualityOptions[MaterialLODSettings.SamplingQuality])
				.OnSelectionChanged(this, &FMaterialLODSettingsLayout::OnSamplingQualityChanged)

			];

		//texture strech
		ChildrenBuilder.AddChildContent( LOCTEXT("MaterialLODTextureStrech", "Texture Strech") )
			.Visibility(MaterialLODSettingsVisibility)
			.NameContent()
			[
				SNew(STextBlock)
				.Font( IDetailLayoutBuilder::GetDetailFont() )
				.Text(LOCTEXT("TextureStretch", "Texture Stretch"))
			]
		.ValueContent()
			[
				SAssignNew(TextureStretchTypesCombo, STextComboBox)
				//.Font( IDetailLayoutBuilder::GetDetailFont() )
				.ContentPadding(0)
				.OptionsSource(&TextureStretchOptions)
				.InitiallySelectedItem( MaterialLODSettings.TextureStrech >= 0 && MaterialLODSettings.TextureStrech < 6 ? /*Just to be sure..*/
				TextureStretchOptions[MaterialLODSettings.TextureStrech] : TextureStretchOptions[ESimplygonTextureStrech::Medium])
				.OnSelectionChanged(this, &FMaterialLODSettingsLayout::OnTextureStrechChanged)

			];

		//Gutter space
		ChildrenBuilder.AddChildContent( LOCTEXT("MaterialLODGutterSpace", "Gutter Space") )
			.Visibility(MaterialLODSettingsVisibility)
			.NameContent()
			[
				SNew(STextBlock)
				.Font( IDetailLayoutBuilder::GetDetailFont() )
				.Text(LOCTEXT("GutterSpace", "Gutter Space"))
			]
		.ValueContent()
			[
				SNew(SSpinBox<int32>)
				.Font( IDetailLayoutBuilder::GetDetailFont() )
				.MinValue(4)
				.MaxValue(10)
				.Value(this, &FMaterialLODSettingsLayout::GetGutterSpace)
				.OnValueChanged(this, &FMaterialLODSettingsLayout::OnGutterSpaceChanged)
				.OnValueCommitted(this, &FMaterialLODSettingsLayout::OnGutterSpaceCommitted)
			];

		//Allow multi-material
		ChildrenBuilder.AddChildContent( LOCTEXT("MaterialLODAllowMultiMaterial", "Allow MultiMaterial") )
			.Visibility(MultiMaterialVisibility)
			.NameContent()
			[
				SNew(STextBlock)
				.Font( IDetailLayoutBuilder::GetDetailFont() )
				.ToolTipText(LOCTEXT("AllowMultiMaterialToolTip", "Enabling this option will force materials with incompatible blend modes to be baked into separate output materials."))
				.Text(LOCTEXT("AllowMultiMaterial", "Allow MultiMaterial"))
			]
		.ValueContent()
			[
				SNew(SCheckBox)
				.IsChecked(this, &FMaterialLODSettingsLayout::IsMultiMaterialChecked)
				.OnCheckStateChanged(this, &FMaterialLODSettingsLayout::OnMultiMaterialChanged)
			];

		//Prefer TwoSided materials
		ChildrenBuilder.AddChildContent( LOCTEXT("MaterialLODPreferTwoSided", "Prefer TwoSided") )
			.NameContent()
			[
				SNew(STextBlock)
				.Font( IDetailLayoutBuilder::GetDetailFont() )
				.ToolTipText(LOCTEXT("PreferTwoSidedMaterialToolTip", "When this option is enabled and combined source materials has both single- and two-sided settings, use two-sided for LOD material. When unchecked, single-sided material will be created."))
				.Text(LOCTEXT("PreferTwoSided", "Prefer TwoSided"))
			]
		.ValueContent()
			[
				SNew(SCheckBox)
				.IsChecked(this, &FMaterialLODSettingsLayout::IsPreferTwoSidedChecked)
				.OnCheckStateChanged(this, &FMaterialLODSettingsLayout::OnPreferTwoSidedChanged)
			];

		for(int32 channelIndex = 0; channelIndex < MaterialLODSettings.ChannelsToCast.Num(); channelIndex++)
		{
			if (channelIndex < SIMPLYGON_NUM_CHANNELS_TO_BAKE && ChannelsWidgets[channelIndex].IsValid())
			{
				ChannelsWidgets[channelIndex]->UpdateSettings(MaterialLODSettings.ChannelsToCast[channelIndex]);
				ChildrenBuilder.AddChildCustomBuilder(ChannelsWidgets[channelIndex].ToSharedRef());

			}
		}

	}
}

const FSimplygonMaterialLODSettings& FMaterialLODSettingsLayout::GetSettings()
{
	//Update channels before returning the settings
	FSimplygonMaterialLODSettings& OutMaterialLODSetting = MaterialLODSettings;
	for(int ChannelIndex=0; ChannelIndex < OutMaterialLODSetting.ChannelsToCast.Num(); ChannelIndex++)
	{
		if (ChannelIndex < SIMPLYGON_NUM_CHANNELS_TO_BAKE)
		{
			OutMaterialLODSetting.ChannelsToCast[ChannelIndex] = GetSettingsFromChannelWidget(ChannelIndex);
		}
	}

	return MaterialLODSettings;
}

void FMaterialLODSettingsLayout::UpdateSettings(const FSimplygonMaterialLODSettings& InSettings)
{
	MaterialLODSettings = InSettings;
	// add new casting channels if material LOD settings were created with older version of Simplygon integration
	FSimplygonMaterialLODSettings DefaultSettings;
	while (MaterialLODSettings.ChannelsToCast.Num() < DefaultSettings.ChannelsToCast.Num())
	{
		MaterialLODSettings.ChannelsToCast.Add(DefaultSettings.ChannelsToCast[MaterialLODSettings.ChannelsToCast.Num()]);
	}
}

const FSimplygonChannelCastingSettings& FMaterialLODSettingsLayout::GetSettingsFromChannelWidget(uint8 ChannelIndex ) const
{

	return ChannelsWidgets[ChannelIndex]->GetSettings();
}

void FMaterialLODSettingsLayout::SetAllowReuseUVs(bool InAllowReuseUVs)
{
	bAllowReuseUVs = InAllowReuseUVs;
}

void FMaterialLODSettingsLayout::SetAllowMultiMaterial(bool InAllowMultiMaterial)
{
	bAllowMultiMaterial = InAllowMultiMaterial;
}

void FMaterialLODSettingsLayout::SetForceActive(bool InForceActive)
{
	bForceActive = InForceActive;
	if (bForceActive)
	{
		MaterialLODSettings.bActive = true;
		// Enable BaseColor channel if nothing else is enabled
		bool bHasActiveChannels = false;
		int BaseColorChannelIndex = INDEX_NONE;
		for (int32 ChannelIndex = 0; ChannelIndex < MaterialLODSettings.ChannelsToCast.Num(); ChannelIndex++)
		{
			FSimplygonChannelCastingSettings& Channel = MaterialLODSettings.ChannelsToCast[ChannelIndex];
			if (Channel.bActive)
			{
				bHasActiveChannels = true;
				break;
			}
			if (Channel.MaterialChannel == ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_BASECOLOR)
			{
				BaseColorChannelIndex = ChannelIndex;
			}
		}
		if (!bHasActiveChannels && BaseColorChannelIndex >= 0)
		{
			MaterialLODSettings.ChannelsToCast[BaseColorChannelIndex].bActive = true;
			ChannelsWidgets[BaseColorChannelIndex]->SetActive(true);
		}
	}
}

EVisibility FMaterialLODSettingsLayout::IsMaterialLODSettingEnabled() const
{
	return MaterialLODSettings.bActive ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility FMaterialLODSettingsLayout::IsReuseUvVisible() const
{
	return (MaterialLODSettings.bActive && bAllowReuseUVs ) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility FMaterialLODSettingsLayout::IsMultiMaterialVisible() const
{
	return (MaterialLODSettings.bActive && bAllowMultiMaterial ) ? EVisibility::Visible : EVisibility::Collapsed;
}

ECheckBoxState FMaterialLODSettingsLayout::GetEnableMaterialLODState() const 
{ 
	return MaterialLODSettings.bActive ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; 
} 

bool FMaterialLODSettingsLayout::CouldBeInactive() const
{
	return !bForceActive;
}

void FMaterialLODSettingsLayout::SetEnableMaterialLODState(ECheckBoxState NewValue)
{
	MaterialLODSettings.bActive = (NewValue == ECheckBoxState::Checked);
}

// Automatic Texture sizing
void FMaterialLODSettingsLayout::OnUseAutomaticTextureSizesChanged(ECheckBoxState NewValue)
{
	MaterialLODSettings.bUseAutomaticSizes = (NewValue == ECheckBoxState::Checked);
}

ECheckBoxState FMaterialLODSettingsLayout::IsUseAutomaticTextureSizesChecked() const
{
	return MaterialLODSettings.bUseAutomaticSizes ? ECheckBoxState::Checked: ECheckBoxState::Unchecked;
}

bool FMaterialLODSettingsLayout::IsUseAutomaticTextureSizeEnabled() const
{
	if(MaterialLODSettings.bUseAutomaticSizes)
	{
		return true;
	}
	return false;
}

bool FMaterialLODSettingsLayout::IsUseAutomaticTextureSizeDisabled() const
{
	if(!MaterialLODSettings.bUseAutomaticSizes)
	{
		return true;
	}
	return false;
}

//Use existing UVs
ECheckBoxState FMaterialLODSettingsLayout::IsUseExistingChartsChecked() const
{
	return MaterialLODSettings.bReuseExistingCharts ? ECheckBoxState::Checked: ECheckBoxState::Unchecked;
}

void FMaterialLODSettingsLayout::OnUseExistingChartsChanged(ECheckBoxState NewValue)
{
	MaterialLODSettings.bReuseExistingCharts = (NewValue == ECheckBoxState::Checked);
}

//Allow multi-material
ECheckBoxState FMaterialLODSettingsLayout::IsMultiMaterialChecked() const
{
	return MaterialLODSettings.bAllowMultiMaterial ? ECheckBoxState::Checked: ECheckBoxState::Unchecked;
}

void FMaterialLODSettingsLayout::OnMultiMaterialChanged(ECheckBoxState NewValue)
{
	MaterialLODSettings.bAllowMultiMaterial = (NewValue == ECheckBoxState::Checked);
}

//Prefer two-sided
void FMaterialLODSettingsLayout::OnPreferTwoSidedChanged(ECheckBoxState NewValue)
{
	MaterialLODSettings.bPreferTwoSideMaterials = (NewValue == ECheckBoxState::Checked);
}

ECheckBoxState FMaterialLODSettingsLayout::IsPreferTwoSidedChecked() const
{
	return MaterialLODSettings.bPreferTwoSideMaterials ? ECheckBoxState::Checked: ECheckBoxState::Unchecked;
}

//Bake vertex data
ECheckBoxState FMaterialLODSettingsLayout::IsBakeVertexDataChecked() const
{
	return MaterialLODSettings.bBakeVertexData ? ECheckBoxState::Checked: ECheckBoxState::Unchecked;
}

void FMaterialLODSettingsLayout::OnBakeVertexDataChanged(ECheckBoxState NewValue)
{
	MaterialLODSettings.bBakeVertexData = (NewValue == ECheckBoxState::Checked);
}

//Bake actor data
ECheckBoxState FMaterialLODSettingsLayout::IsBakeActorDataChecked() const
{
	return MaterialLODSettings.bBakeActorData ? ECheckBoxState::Checked: ECheckBoxState::Unchecked;
}

void FMaterialLODSettingsLayout::OnBakeActorDataChanged(ECheckBoxState NewValue)
{
	MaterialLODSettings.bBakeActorData = (NewValue == ECheckBoxState::Checked);
}

bool FMaterialLODSettingsLayout::IsBakeActorDataEnabled() const
{
	//Yes it should be bBakeVertexData here - BakeVertexData needs to be enabled in order for BakeActorData to work.
	return MaterialLODSettings.bBakeVertexData == true;
}

//Texture Width and Height
void FMaterialLODSettingsLayout::OnTextureWidthChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo)
{
	const ESimplygonTextureResolution::Type TextureWidth = (ESimplygonTextureResolution::Type)TextureResolutionOptions.Find(NewValue);
	if (MaterialLODSettings.TextureWidth != TextureWidth)
	{
		/*	if (FEngineAnalytics::IsAvailable())
		{
		FEngineAnalytics::GetProvider().RecordEvent(TEXT("Editor.Usage.StaticMesh.Material"), TEXT("TextureImportance"), *NewValue.Get());
		}*/
		MaterialLODSettings.TextureWidth = TextureWidth;
	}
}

void FMaterialLODSettingsLayout::OnTextureHeightChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo)
{
	const ESimplygonTextureResolution::Type TextureHeight = (ESimplygonTextureResolution::Type)TextureResolutionOptions.Find(NewValue);
	if (MaterialLODSettings.TextureHeight != TextureHeight)
	{
		/*if (FEngineAnalytics::IsAvailable())
		{
		FEngineAnalytics::GetProvider().RecordEvent(TEXT("Editor.Usage.StaticMesh.ReductionSettings"), TEXT("TextureImportance"), *NewValue.Get());
		}*/
		MaterialLODSettings.TextureHeight = TextureHeight;
	}
}

//Sampling Quality
void FMaterialLODSettingsLayout::OnSamplingQualityChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo)
{
	const ESimplygonTextureSamplingQuality::Type SamplingQuality = (ESimplygonTextureSamplingQuality::Type)SamplingQualityOptions.Find(NewValue);
	if (MaterialLODSettings.SamplingQuality != SamplingQuality)
	{

		MaterialLODSettings.SamplingQuality = SamplingQuality;
	}
}

//Texture Stretch
void FMaterialLODSettingsLayout::OnTextureStrechChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo)
{
	const ESimplygonTextureStrech::Type TextureStretch = (ESimplygonTextureStrech::Type)TextureStretchOptions.Find(NewValue);
	if (MaterialLODSettings.TextureStrech != TextureStretch)
	{

		MaterialLODSettings.TextureStrech = TextureStretch;
	}
}

//Gutter Space
int32 FMaterialLODSettingsLayout::GetGutterSpace() const
{
	return MaterialLODSettings.GutterSpace;
}

void FMaterialLODSettingsLayout::OnGutterSpaceChanged(int32 NewValue)
{
	MaterialLODSettings.GutterSpace = NewValue;
}

void FMaterialLODSettingsLayout::OnGutterSpaceCommitted(int32 NewValue, ETextCommit::Type TextCommitType)
{
	/*if (FEngineAnalytics::IsAvailable())
	{
	FEngineAnalytics::GetProvider().RecordEvent(TEXT("Editor.Usage.StaticMesh.ReductionSettings"), TEXT("PercentTriangles"), FString::Printf(TEXT("%.1f"), NewValue));
	}*/
	OnGutterSpaceChanged(NewValue);
}

#undef LOCTEXT_NAMESPACE