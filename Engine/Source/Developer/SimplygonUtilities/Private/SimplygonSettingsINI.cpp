#include "SimplygonUtilitiesPrivatePCH.h"
#include "SimplygonSettingsINI.h"

FSimplygonSettingsIni::FSimplygonSettingsIni()
{
	FSimplygonEnum::FillEnumOptions(FeatureImportanceOptions, FSimplygonEnum::GetFeatureImportanceEnum());
	FSimplygonEnum::FillEnumOptions(TextureStrechOptions, FSimplygonEnum::GetTextureStrechEnum());
	FSimplygonEnum::FillEnumOptions(CasterTypeOptions, FSimplygonEnum::GetCasterTypeEnum());
	FSimplygonEnum::FillEnumOptions(ColorChannelOptions, FSimplygonEnum::GetColorChannelEnum());
	FSimplygonEnum::FillEnumOptions(MaterialLODTypeOptions, FSimplygonEnum::GetMaterialLODTypeEnum());
	FSimplygonEnum::FillEnumOptions(TextureSmplingQualityOptions, FSimplygonEnum::GetSamplingQualityEnum());
	FSimplygonEnum::FillEnumOptions(TextureResolutionOptions, FSimplygonEnum::GetTextureResolutionEnum(), true);
}

void FSimplygonSettingsIni::Read( const FString& Filename, TArray<TSharedPtr<FSimplygonSettingsLODInfo>>& LODConfigInfo )
{
	if (GConfig == NULL || !GConfig->AreFileOperationsDisabled())
	{
		Empty();
		FString Text;

		if( FFileHelper::LoadFileToString( Text, *Filename ) )
		{
			// process the contents of the string
			ProcessInputFileContents(Filename, Text);
		}

		ParseIniIntoLODInfoArray(LODConfigInfo);

	}
}

bool FSimplygonSettingsIni::Write( const FString& Filename, const TArray<TSharedPtr<FSimplygonSettingsLODInfo>>& LODConfigInfo )
{

	FString Text;

	for( TIterator SectionIterator(*this); SectionIterator; ++SectionIterator )
	{
		const FString& SectionName = SectionIterator.Key();
		const FConfigSection& Section = SectionIterator.Value();

		// Flag to check whether a property was written on this section, 
		// if none we do not want to make any changes to the destination file on this round.
		bool bWroteASectionProperty = false;

		TSet< FName > PropertiesAddedLookup;

		for( FConfigSection::TConstIterator It2(Section); It2; ++It2 )
		{
			const FName PropertyName = It2.Key();
			const FString& PropertyValue = It2.Value();

			// Check if the we've already processed a property of this name. If it was part of an array we may have already written it out.
			if( !PropertiesAddedLookup.Contains( PropertyName ) )
			{
				// Check for an array of differing size. This will trigger a full writeout.
				// This also catches the case where the property doesn't exist in the source in non-array cases
				bool bDifferentNumberOfElements = false;

				const FConfigSection* SourceSection = NULL;
				if( SourceSection )
				{
					TArray< FString > SourceMatchingProperties;
					SourceSection->MultiFind( PropertyName, SourceMatchingProperties );

					TArray< FString > DestMatchingProperties;
					Section.MultiFind( PropertyName, DestMatchingProperties );

					bDifferentNumberOfElements = SourceMatchingProperties.Num() != DestMatchingProperties.Num();
				}



				// Check if the property matches the source configs. We do not wanna write it out if so.
				if( bDifferentNumberOfElements )
				{
					// If this is the first property we are writing of this section, then print the section name
					if( !bWroteASectionProperty )
					{
						Text += FString::Printf( TEXT("[%s]") LINE_TERMINATOR, *SectionName);
						bWroteASectionProperty = true;
					}

					//// Print the property to the file.
					TCHAR QuoteString[2] = {0,0};
					//if (ShouldExportQuotedString(PropertyValue))
					//{
					//	QuoteString[0] = TEXT('\"');
					//}

					// Write out our property, if it is an array we need to write out the entire array.
					TArray< FString > CompletePropertyToWrite;
					Section.MultiFind( PropertyName, CompletePropertyToWrite, true );

					// If we are writing to a default config file and this property is an array, we need to be careful to remove those from higher up the hierarchy
					bool bIsADefaultIniWrite = !Filename.Contains( FPaths::GameSavedDir() ) 
						&& !Filename.Contains( FPaths::EngineSavedDir() ) 
						&& FPaths::GetBaseFilename(Filename).StartsWith( TEXT( "Default" ) );

					for( int32 Idx = 0; Idx < CompletePropertyToWrite.Num(); Idx++ )
					{
						Text += FString::Printf( TEXT("%s=%s%s%s") LINE_TERMINATOR, 
							*PropertyName.ToString(), QuoteString, *CompletePropertyToWrite[ Idx ], QuoteString);	
					}


					PropertiesAddedLookup.Add( PropertyName );
				}
			}
		}

		// If we wrote any part of the section, then add some whitespace after the section.
		if( bWroteASectionProperty )
		{
			Text += LINE_TERMINATOR;
		}

	}

	// Ensure We have at least something to write
	Text += LINE_TERMINATOR;


	bool bResult = FFileHelper::SaveStringToFile( Text, *Filename );

	// Return if the write was successful
	return bResult;
}

bool FSimplygonSettingsIni::GetString( const TCHAR* Section, const TCHAR* Key, FString& Value ) const
{
	const FConfigSection* Sec = Find( Section );
	if( Sec == NULL )
	{
		return false;
	}
	const FString* PairString = Sec->Find( Key );
	if( PairString == NULL )
	{
		return false;
	}

	//this prevents our source code text gatherer from trying to gather the following messages
#define LOC_DEFINE_REGION
	if( FCString::Strstr( **PairString, TEXT("LOCTEXT") ) )
	{
		UE_LOG( LogConfig, Warning, TEXT( "FConfigFile::GetString( %s, %s ) contains LOCTEXT"), Section, Key );
		return false;
	}
	else
	{
		Value = **PairString;
		return true;
	}
#undef LOC_DEFINE_REGION
}

bool FSimplygonSettingsIni::GetText( const TCHAR* Section, const TCHAR* Key, FText& Value ) const
{
	const FConfigSection* Sec = Find( Section );
	if( Sec == NULL )
	{
		return false;
	}
	const FString* PairString = Sec->Find( Key );
	if( PairString == NULL )
	{
		return false;
	}
	return FParse::Text( **PairString, Value, Section );
}

bool FSimplygonSettingsIni::GetInt64( const TCHAR* Section, const TCHAR* Key, int64& Value ) const
{
	FString Text; 
	if( GetString( Section, Key, Text ) )
	{
		Value = FCString::Atoi64(*Text);
		return true;
	}
	return false;
}

bool FSimplygonSettingsIni::GetInt32( const TCHAR* Section, const TCHAR* Key, int32& Value ) const
{
	FString Text; 
	if( GetString( Section, Key, Text ) )
	{
		Value = FCString::Atoi(*Text);
		return true;
	}
	return false;
}

bool FSimplygonSettingsIni::GetFloat( const TCHAR* Section, const TCHAR* Key, float& Value ) const
{
	FString Text; 
	if( GetString( Section, Key, Text ) )
	{
		Value = FCString::Atof(*Text);
		return true;
	}
	return false;
}

bool FSimplygonSettingsIni::GetBoolean( const TCHAR* Section, const TCHAR* Key, bool& Value ) const
{
	FString Text; 
	if( GetString( Section, Key, Text ) )
	{
		if(Text.ToLower().Compare(TEXT("true")) == 0 )
		{
			Value = true;
		}
		else
		{
			Value = false;
		}
		return true;
	}
	return false;
}

bool FSimplygonSettingsIni::GetAxisIndex(const TCHAR* Section, const TCHAR* Key, int32& Value)
{

	FString Text; 
	if( GetString( Section, Key, Text ) )
	{
		if(Text.Compare(TEXT("-Y"))== 0)
		{
			Value = 0;
		}
		return true;
	}
	return false;
}

bool FSimplygonSettingsIni::GetFeatureImportance(const TCHAR* Section, const TCHAR* Key, EMeshFeatureImportance::Type& Value)
{
	FString Text; 
	if( GetString( Section, Key, Text ) )
	{
		Value = (EMeshFeatureImportance::Type)FeatureImportanceOptions.Find(Text);
		return true;
	}
	return false;
}

bool FSimplygonSettingsIni::GetTextureStrech(const TCHAR* Section, const TCHAR* Key, ESimplygonTextureStrech::Type& Value)
{
	FString Text; 
	if( GetString( Section, Key, Text ) )
	{

		Value = (ESimplygonTextureStrech::Type)TextureStrechOptions.Find(Text);
		if(Value == -1)
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Text));
		}
		return true;
	}
	return false;
}

bool FSimplygonSettingsIni::GetTextureSamplingQuality(const TCHAR* Section, const TCHAR* Key, ESimplygonTextureSamplingQuality::Type& Value)
{
	FString Text; 
	if( GetString( Section, Key, Text ) )
	{
		Value = (ESimplygonTextureSamplingQuality::Type)TextureSmplingQualityOptions.Find(Text);
		if(Value == -1)
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Text));
		}
		return true;
	}
	return false;
}

bool FSimplygonSettingsIni::GetTextureResolution(const TCHAR* Section, const TCHAR* Key, ESimplygonTextureResolution::Type& Value)
{
	FString Text;
	if (GetString(Section, Key, Text))
	{
		Value = (ESimplygonTextureResolution::Type)TextureResolutionOptions.Find(Text);
		if (Value == -1)
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Text));
		}
		return true;
	}
	return false;
}

void FSimplygonSettingsIni::SetString( const TCHAR* Section, const TCHAR* Key, const TCHAR* Value )
{
	FConfigSection* Sec  = Find( Section );
	if( Sec == NULL )
	{
		Sec = &Add( Section, FConfigSection() );
	}

	FString* Str = Sec->Find( Key );
	if( Str == NULL )
	{
		Sec->Add( Key, Value );
	}
	else if( FCString::Strcmp(**Str,Value)!=0 )
	{
		*Str = Value;
	}
}

void FSimplygonSettingsIni::SetText( const TCHAR* Section, const TCHAR* Key, const FText& Value )
{
	FConfigSection* Sec  = Find( Section );
	if( Sec == NULL )
	{
		Sec = &Add( Section, FConfigSection() );
	}

	FString* Str = Sec->Find( Key );
	const FString StrValue = FSimplygonSettingsIni::AsString( Value );

	if( Str == NULL )
	{
		Sec->Add( Key, StrValue );
	}
	else if( FCString::Strcmp(**Str, *StrValue)!=0 )
	{
		*Str = StrValue;
	}
}

void FSimplygonSettingsIni::SetInt64( const TCHAR* Section, const TCHAR* Key, const int64 Value )
{
	TCHAR Text[MAX_SPRINTF]=TEXT("");
	FCString::Sprintf( Text, TEXT("%lld"), Value );
	SetString( Section, Key, Text );
}

void FSimplygonSettingsIni::SetInt32(const TCHAR* Section, const TCHAR* Key, const int32 Value)
{
	TCHAR Text[MAX_SPRINTF]=TEXT("");
	FCString::Sprintf( Text, TEXT("%d"), Value );
	SetString( Section, Key, Text );
}

void FSimplygonSettingsIni::SetFloat( const TCHAR* Section, const TCHAR* Key, const float Value )
{
	TCHAR Text[MAX_SPRINTF]=TEXT("");
	FCString::Sprintf( Text, TEXT("%f"), Value );
	SetString( Section, Key, Text );
}

void FSimplygonSettingsIni::SetBoolean(const TCHAR* Section, const TCHAR* Key, const bool Value)
{

	TCHAR Text[MAX_SPRINTF]=TEXT("");
	if(Value == true)
		SetString( Section, Key, TEXT("true") );
	else
		SetString( Section, Key, TEXT("false") );


}

FString FSimplygonSettingsIni::AsString( const FText& Text )
{
	if( Text.IsTransient() )
	{
		UE_LOG( LogConfig, Warning, TEXT( "FTextFriendHelper::AsString() Transient FText") );
		return FString(TEXT("Error: Transient FText"));
	}

	FString Str;
	if( FTextInspector::GetSourceString(Text) )
	{
		FString SourceString( *FTextInspector::GetSourceString(Text) );
		for( auto Iter( SourceString.CreateConstIterator()); Iter; ++Iter )
		{
			const TCHAR Ch = *Iter;
			if( Ch == '\"' )
			{
				Str += TEXT("\\\"");
			}
			else if( Ch == '\t' )
			{
				Str += TEXT("\\t");
			}
			else if( Ch == '\r' )
			{
				Str += TEXT("\\r");
			}
			else if( Ch == '\n' )
			{
				Str += TEXT("\\n");
			}
			else if( Ch == '\\' )
			{
				Str += TEXT("\\\\");
			}
			else
			{
				Str += Ch;
			}
		}
	}

	//this prevents our source code text gatherer from trying to gather the following messages
#define LOC_DEFINE_REGION
	if( Text.IsCultureInvariant() )
	{
		return FString::Printf( TEXT( "NSLOCTEXT(\"\",\"\",\"%s\")" ), *Str );
	}
	else
	{
		/*return FString::Printf( TEXT( "NSLOCTEXT(\"%s\",\"%s\",\"%s\")" ),
			FTextInspector::GetNamespace(Text) ? **FTextInspector::GetNamespace(Text) : TEXT(""), FTextInspector::GetKey(Text) ? **FTextInspector::GetKey(Text) : TEXT(""), *Str );*/
		return FString::Printf(TEXT("NSLOCTEXT(\"%s\",\"%s\",\"%s\")"),
			*FTextInspector::GetNamespace(Text).Get(TEXT("")), *FTextInspector::GetKey(Text).Get(TEXT("")), *Str);
	}
#undef LOC_DEFINE_REGION
}

void FSimplygonSettingsIni::ProcessInputFileContents(const FString& Filename, FString& Contents)
{	
	FString Text = Contents;
	const TCHAR* Ptr = Text.Len() > 0 ? *Text : NULL;
	FConfigSection* CurrentSection = NULL;
	bool Done = false;
	while( !Done && Ptr != NULL )
	{
		// Advance past new line characters
		while( *Ptr=='\r' || *Ptr=='\n' )
		{
			Ptr++;
		}			
		// read the next line
		FString TheLine;
		int32 LinesConsumed = 0;
		FParse::LineExtended(&Ptr, TheLine, LinesConsumed, false);
		if (Ptr == NULL || *Ptr == 0)
		{
			Done = true;
		}
		TCHAR* Start = const_cast<TCHAR*>(*TheLine);

		// Strip trailing spaces from the current line
		while( *Start && FChar::IsWhitespace(Start[FCString::Strlen(Start)-1]) )
		{
			Start[FCString::Strlen(Start)-1] = 0;
		}

		// If the first character in the line is [ and last char is ], this line indicates a section name
		if( *Start=='[' && Start[FCString::Strlen(Start)-1]==']' )
		{
			// Remove the brackets
			Start++;
			Start[FCString::Strlen(Start)-1] = 0;

			// If we don't have an existing section by this name, add one
			CurrentSection = Find( Start );
			if( !CurrentSection )
			{
				CurrentSection = &Add( Start, FConfigSection() );
			}
		}

		// Otherwise, if we're currently inside a section, and we haven't reached the end of the stream
		else if( CurrentSection && *Start )
		{
			TCHAR* Value = 0;

			// ignore [comment] lines that start with ;
			if(*Start != (TCHAR)';')
			{
				Value = FCString::Strstr(Start,TEXT("="));
			}

			// Ignore any lines that don't contain a key-value pair
			if( Value )
			{
				// Terminate the propertyname, advancing past the =
				*Value++ = 0;

				// strip leading whitespace from the property name
				while ( *Start && FChar::IsWhitespace(*Start) )
					Start++;

				// Strip trailing spaces from the property name.
				while( *Start && FChar::IsWhitespace(Start[FCString::Strlen(Start)-1]) )
					Start[FCString::Strlen(Start)-1] = 0;

				// Strip leading whitespace from the property value
				while ( *Value && FChar::IsWhitespace(*Value) )
					Value++;

				// strip trailing whitespace from the property value
				while( *Value && FChar::IsWhitespace(Value[FCString::Strlen(Value)-1]) )
					Value[FCString::Strlen(Value)-1] = 0;

				// If this line is delimited by quotes
				if( *Value=='\"' )
				{
					FString PreprocessedValue = FString(Value).TrimQuotes().ReplaceQuotesWithEscapedQuotes();
					const TCHAR* NewValue = *PreprocessedValue;

					FString ProcessedValue;
					//epic moelfke: fixed handling of escaped characters in quoted string
					while (*NewValue && *NewValue != '\"')
					{
						if (*NewValue != '\\') // unescaped character
						{
							ProcessedValue += *NewValue++;
						}
						else if( *++NewValue == '\0')// escape character encountered at end
						{
							break;
						}
						else if (*NewValue == '\\') // escaped backslash "\\"
						{
							ProcessedValue += '\\';
							NewValue++;
						}
						else if (*NewValue == '\"') // escaped double quote "\""
						{
							ProcessedValue += '\"';
							NewValue++;
						}
						else if ( *NewValue == TEXT('n') )
						{
							ProcessedValue += TEXT('\n');
							NewValue++;
						}
						else if( *NewValue == TEXT('u') && NewValue[1] && NewValue[2] && NewValue[3] && NewValue[4] )	// \uXXXX - UNICODE code point
						{
							ProcessedValue += FParse::HexDigit(NewValue[1])*(1<<12) + FParse::HexDigit(NewValue[2])*(1<<8) + FParse::HexDigit(NewValue[3])*(1<<4) + FParse::HexDigit(NewValue[4]);
							NewValue += 5;
						}
						else if( NewValue[1] ) // some other escape sequence, assume it's a hex character value
						{
							ProcessedValue += FParse::HexDigit(NewValue[0])*16 + FParse::HexDigit(NewValue[1]);
							NewValue += 2;
						}
					}

					// Add this pair to the current FConfigSection
					CurrentSection->Add(Start, *ProcessedValue);
				}
				else
				{
					// Add this pair to the current FConfigSection
					CurrentSection->Add(Start, Value);
				}
			}
		}
	}

	// Avoid memory wasted in array slack.
	Shrink();
	for( TMap<FString,FConfigSection>::TIterator It(*this); It; ++It )
	{
		It.Value().Shrink();
	}
}

void FSimplygonSettingsIni::ParseIniIntoLODInfoArray(TArray<TSharedPtr<FSimplygonSettingsLODInfo>>& LODConfigInfo)
{
	//Fetch LOD Count

	//Section Keys


	int32 TotalNumberOfLODs = 0;
	bool IsCascaded = false;
	float LastReductionRatio = 1.0f;
	GetInt32(LODCollectionSection,LODCount, TotalNumberOfLODs);
	GetBoolean(AutoLODSection, CascadedLODChain, IsCascaded);
	static const int32 MAX_SECTION_PATH = 1024;
	if(TotalNumberOfLODs > 0 && TotalNumberOfLODs <= MAX_STATIC_MESH_LODS)
	{
		if(LODConfigInfo.Num() < TotalNumberOfLODs)
		{
			for(int32 Index = 0; Index < TotalNumberOfLODs; ++Index )
			{
				LODConfigInfo.Add(MakeShareable(new FSimplygonSettingsLODInfo()));
			}
		}



		for(int32 Index = 0; Index < TotalNumberOfLODs; ++Index )
		{
			FSimplygonSettingsLODInfo* CurrentLODInfo = LODConfigInfo[Index].Get();
			TCHAR CurrentLODSection[MAX_SECTION_PATH + 1] = { 0 };
			FCString::Snprintf(CurrentLODSection,MAX_PATH,LODSectionFormatString,Index);
			FString CurrentLODSectionString(CurrentLODSection);
			//lod type
			FString ProcessingTypeValue;
			GetString(CurrentLODSection,ProcessingType,ProcessingTypeValue);

			CurrentLODInfo->LODIndex = Index;

			if(ProcessingTypeValue.Compare(TEXT("ProxyLOD"))==0)
			{
				CurrentLODInfo->bIsRemeshing = true;


				if(!CurrentLODInfo->RemeshingSettings.IsValid())
				{
					CurrentLODInfo->RemeshingSettings = MakeShareable(new FSimplygonRemeshingSettings());
				}

				CurrentLODInfo->RemeshingSettings->bActive = true;

				//screen size parameter
				GetInt32(CurrentLODSection,SizeOnScreen,CurrentLODInfo->RemeshingSettings->ScreenSize);

				//ground plane
				FString GroundPlaneSectionFullPath = CurrentLODSection + FString(TEXT("/")) + GroundPlaneSection;
				GetBoolean(*GroundPlaneSectionFullPath, IsEnabled, CurrentLODInfo->RemeshingSettings->bUseClippingPlane );
				GetAxisIndex(*GroundPlaneSectionFullPath,GroundPlaneAxis,CurrentLODInfo->RemeshingSettings->AxisIndex);
				//TODO : Add Setting for negative halfspace

				//normal recal
				FString NormalRecalFullPath = CurrentLODSection + FString(TEXT("/")) + ProxyNormalRecalSection;
				GetBoolean(*NormalRecalFullPath, IsEnabled, CurrentLODInfo->RemeshingSettings->bRecalculateNormals );
				GetFloat(*NormalRecalFullPath, HardAngle, CurrentLODInfo->RemeshingSettings->HardAngleThreshold );

				//proxy settings
				FString ProxySettingsSectionFullPath = CurrentLODSection + FString(TEXT("/")) + ProxySettingsSection;
				GetInt32(*ProxySettingsSectionFullPath,MergeDistance,CurrentLODInfo->RemeshingSettings->MergeDistance);

				FSimplygonMaterialLODSettings& CurrentMaterialLODSettings = CurrentLODInfo->RemeshingSettings->MaterialLODSettings;

				//mapping section
				FString MappingSectionFullPath = CurrentLODSection + FString(TEXT("/")) + ReductionMappingSection;
				GetBoolean(*MappingSectionFullPath, IsEnabled, CurrentMaterialLODSettings.bActive );

				if(CurrentMaterialLODSettings.bActive)
				{
					ESimplygonTextureResolution::Type TextureWidthValue, TextureHeightValue;
					GetTextureResolution(*MappingSectionFullPath, TexXSize, TextureWidthValue);
					GetTextureResolution(*MappingSectionFullPath, TexYSize, TextureHeightValue);
					CurrentMaterialLODSettings.TextureWidth = TextureWidthValue;
					CurrentMaterialLODSettings.TextureHeight = TextureHeightValue;
					ESimplygonTextureSamplingQuality::Type SamplingQuality;
					GetTextureSamplingQuality(*MappingSectionFullPath, Supersampling,SamplingQuality);
					CurrentMaterialLODSettings.SamplingQuality = SamplingQuality;

					TMap<FString, FSimplygonChannelCastingSettings> ChannelsMapping;
					TMap<FString, int32> ChannelsIndexMapping;
					TArray<FSimplygonChannelCastingSettings>& CurrentChannelSettings = CurrentMaterialLODSettings.ChannelsToCast;

					for (int32 ChannelIndex = 0; ChannelIndex < CurrentChannelSettings.Num(); ChannelIndex++)
					{
						if (CurrentChannelSettings[ChannelIndex].MaterialChannel == ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_DIFFUSE || CurrentChannelSettings[ChannelIndex].MaterialChannel == ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_BASECOLOR)
						{
							ChannelsMapping.Add("Basecolor", CurrentChannelSettings[ChannelIndex]);
							ChannelsIndexMapping.Add("Basecolor", ChannelIndex);
						}
						else if (CurrentChannelSettings[ChannelIndex].MaterialChannel == ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_SPECULAR)
						{
							ChannelsMapping.Add("Specular", CurrentChannelSettings[ChannelIndex]);
							ChannelsIndexMapping.Add("Specular", ChannelIndex);
						}
						else if (CurrentChannelSettings[ChannelIndex].MaterialChannel == ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_NORMALS)
						{
							ChannelsMapping.Add("Normals", CurrentChannelSettings[ChannelIndex]);
							ChannelsIndexMapping.Add("Normals", ChannelIndex);
						}
					}

					//automatic size section
					FString AutoTexSizeSectionFullPath = CurrentLODSection + FString(TEXT("/")) + ReductionMappingSection + FString(TEXT("/")) +AutomaticTextureSizeSection;
					GetBoolean(*AutoTexSizeSectionFullPath, IsEnabled, CurrentMaterialLODSettings.bUseAutomaticSizes );

					//texture coordinate generation
					FString TexCoordGenFullPath = CurrentLODSection + FString(TEXT("/")) + ReductionMappingSection + FString(TEXT("/")) +TexCoordsGenerationSection;
					GetInt32(*TexCoordGenFullPath,GutterSpace,CurrentMaterialLODSettings.GutterSpace);

					ESimplygonTextureStrech::Type TextureStrechValue;
					GetTextureStrech(*TexCoordGenFullPath,MaxStretch,TextureStrechValue);
					CurrentMaterialLODSettings.TextureStrech = TextureStrechValue;

					//diffuse or basecolor
					FString DiffuseMapSectionFullPath = CurrentLODSection +FString(TEXT("/")) + ReductionMappingSection+ FString(TEXT("/")) +DiffuseMapSection;
					GetBoolean(*DiffuseMapSectionFullPath,IsEnabled,ChannelsMapping[TEXT("Basecolor")].bActive);
					GetBoolean(*DiffuseMapSectionFullPath,sRGB,ChannelsMapping[TEXT("Basecolor")].bUseSRGB);
					GetBoolean(*DiffuseMapSectionFullPath,BakeVertexColors,ChannelsMapping[TEXT("Basecolor")].bBakeVertexColors);
					//(MappingSectionFullPath,sRGB,ChannelsMapping[TEXT("Basecolor")].bUseSRGB);

					//specular
					FString SpecularMapSectionFullPath = CurrentLODSection + FString(TEXT("/")) + ReductionMappingSection+ FString(TEXT("/")) +SpecularMapSection;
					GetBoolean(*SpecularMapSectionFullPath,IsEnabled,ChannelsMapping[TEXT("Specular")].bActive);
					GetBoolean(*SpecularMapSectionFullPath,sRGB,ChannelsMapping[TEXT("Specular")].bUseSRGB);
					GetBoolean(*SpecularMapSectionFullPath,BakeVertexColors,ChannelsMapping[TEXT("Specular")].bBakeVertexColors);

					//normals
					FString NormalsMapSectionFullPath = CurrentLODSection + FString(TEXT("/")) + ReductionMappingSection+ FString(TEXT("/")) +NormalsMapSection;
					GetBoolean(*NormalsMapSectionFullPath,IsEnabled,ChannelsMapping[TEXT("Normals")].bActive);
					GetBoolean(*NormalsMapSectionFullPath, sRGB, ChannelsMapping[TEXT("Normals")].bUseSRGB);
					GetBoolean(*NormalsMapSectionFullPath, FlipBackfacingNormals, ChannelsMapping[TEXT("Normals")].bFlipBackfacingNormals);
					GetBoolean(*NormalsMapSectionFullPath, TangentSpaceNormals, ChannelsMapping[TEXT("Normals")].bUseTangentSpaceNormals);
					GetBoolean(*NormalsMapSectionFullPath, FlipGreenAxis, ChannelsMapping[TEXT("Normals")].bFlipGreenChannel);


					CurrentChannelSettings[ChannelsIndexMapping[TEXT("Basecolor")]] = ChannelsMapping[TEXT("Basecolor")];
					CurrentChannelSettings[ChannelsIndexMapping[TEXT("Specular")]] = ChannelsMapping[TEXT("Specular")];
					CurrentChannelSettings[ChannelsIndexMapping[TEXT("Normals")]] = ChannelsMapping[TEXT("Normals")];
				}

			}
			else
			{
				CurrentLODInfo->bIsReduction = true;
				if(!CurrentLODInfo->ReductionSettings.IsValid())
				{
					CurrentLODInfo->ReductionSettings = MakeShareable(new FMeshReductionSettings());
				}
				//screen size parameter
				float reductionRatio = 0.0f;
				GetFloat(CurrentLODSection, ReductionRatio, reductionRatio);

				if (!IsCascaded)
				{
					CurrentLODInfo->ReductionSettings->PercentTriangles = reductionRatio * 0.01f;
				}
				else
				{
					CurrentLODInfo->ReductionSettings->PercentTriangles = ((reductionRatio)* 0.01f) * LastReductionRatio;
					LastReductionRatio = CurrentLODInfo->ReductionSettings->PercentTriangles;
				}

				//get feature importacne settings
				FString FeatureImportanceSectionFullPath = CurrentLODSection + FString(TEXT("/")) + ReductionQualitySection;
				EMeshFeatureImportance::Type ShadingImportanceValue,TextureImportanceValue, SilhoutteImportanceValue;
				GetFeatureImportance(*FeatureImportanceSectionFullPath,ShadingImportance, ShadingImportanceValue);
				CurrentLODInfo->ReductionSettings->ShadingImportance = ShadingImportanceValue;
				GetFeatureImportance(*FeatureImportanceSectionFullPath, UVImportance, TextureImportanceValue);
				CurrentLODInfo->ReductionSettings->TextureImportance = TextureImportanceValue;
				GetFeatureImportance(*FeatureImportanceSectionFullPath,GeometricImportance, SilhoutteImportanceValue);
				CurrentLODInfo->ReductionSettings->SilhouetteImportance = SilhoutteImportanceValue;

				//preProcessing
				FString PreProcessingFullPath = CurrentLODSection + FString(TEXT("/")) + ReductionPreProcesisngSection;
				GetFloat(*PreProcessingFullPath,WeldingThreshold,CurrentLODInfo->ReductionSettings->WeldingThreshold);

				//postprocessing normal recal section
				FString PostProcessingFullPath = CurrentLODSection + FString(TEXT("/")) + NormalRecalSection;
				GetFloat(*PostProcessingFullPath,HardAngle,CurrentLODInfo->ReductionSettings->HardAngleThreshold);
				GetBoolean(*PostProcessingFullPath,IsEnabled,CurrentLODInfo->ReductionSettings->bRecalculateNormals);

				//mapping sections
				FSimplygonMaterialLODSettings& CurrentMaterialLODSettings = CurrentLODInfo->ReductionSettings->MaterialLODSettings;

				//mapping section

				FString MappingSectionFullPath = CurrentLODSection + FString(TEXT("/")) + ReductionMappingSection;
				GetBoolean(*MappingSectionFullPath, IsEnabled, CurrentMaterialLODSettings.bActive );

				if(CurrentMaterialLODSettings.bActive)
				{
					ESimplygonTextureResolution::Type TextureWidthValue, TextureHeightValue;
					GetTextureResolution(*MappingSectionFullPath, TexXSize, TextureWidthValue);
					GetTextureResolution(*MappingSectionFullPath, TexYSize, TextureHeightValue);
					CurrentMaterialLODSettings.TextureWidth = TextureWidthValue;
					CurrentMaterialLODSettings.TextureHeight = TextureHeightValue;

					ESimplygonTextureSamplingQuality::Type SamplingQualityValue;
					GetTextureSamplingQuality(*MappingSectionFullPath, Supersampling,SamplingQualityValue);
					CurrentMaterialLODSettings.SamplingQuality = SamplingQualityValue;

					TMap<FString,FSimplygonChannelCastingSettings> ChannelsMapping;
					TMap<FString, int32> ChannelsIndexMapping;
					TArray<FSimplygonChannelCastingSettings>& CurrentChannelSettings = CurrentMaterialLODSettings.ChannelsToCast;

					for(int32 ChannelIndex=0; ChannelIndex < CurrentChannelSettings.Num() ; ChannelIndex++)
					{
						if (CurrentChannelSettings[ChannelIndex].MaterialChannel == ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_DIFFUSE || CurrentChannelSettings[ChannelIndex].MaterialChannel == ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_BASECOLOR)
						{
							ChannelsMapping.Add("Basecolor", CurrentChannelSettings[ChannelIndex]);
							ChannelsIndexMapping.Add("Basecolor", ChannelIndex);
						}
						else if (CurrentChannelSettings[ChannelIndex].MaterialChannel == ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_SPECULAR)
						{
							ChannelsMapping.Add("Specular", CurrentChannelSettings[ChannelIndex]);
							ChannelsIndexMapping.Add("Specular", ChannelIndex);
						}
						else if (CurrentChannelSettings[ChannelIndex].MaterialChannel == ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_NORMALS)
						{
							ChannelsMapping.Add("Normals", CurrentChannelSettings[ChannelIndex]);
							ChannelsIndexMapping.Add("Normals", ChannelIndex);
						}
					}

					//automatic size section
					FString AutoTexSizeSectionFullPath = CurrentLODSection + FString(TEXT("/")) + ReductionMappingSection +FString(TEXT("/")) +AutomaticTextureSizeSection;
					GetBoolean(*AutoTexSizeSectionFullPath, IsEnabled, CurrentMaterialLODSettings.bUseAutomaticSizes );

					//texture coordinate generation
					FString TexCoordGenFullPath = CurrentLODSection + FString(TEXT("/")) + ReductionMappingSection + FString(TEXT("/")) +TexCoordsGenerationSection;
					GetInt32(*TexCoordGenFullPath,GutterSpace,CurrentMaterialLODSettings.GutterSpace);
					ESimplygonTextureStrech::Type StrechValue;
					GetTextureStrech(*TexCoordGenFullPath,MaxStretch,StrechValue);
					CurrentMaterialLODSettings.TextureStrech = StrechValue;

					//diffuse or basecolor
					FString DiffuseMapSectionFullPath = CurrentLODSection + FString(TEXT("/")) + ReductionMappingSection+ FString(TEXT("/")) +DiffuseMapSection;
					GetBoolean(*DiffuseMapSectionFullPath,IsEnabled,ChannelsMapping[TEXT("Basecolor")].bActive);
					GetBoolean(*DiffuseMapSectionFullPath,sRGB,ChannelsMapping[TEXT("Basecolor")].bUseSRGB);
					GetBoolean(*DiffuseMapSectionFullPath,BakeVertexColors,ChannelsMapping[TEXT("Basecolor")].bBakeVertexColors);
					//(MappingSectionFullPath,sRGB,ChannelsMapping[TEXT("Basecolor")].bUseSRGB);

					//specular
					FString SpecularMapSectionFullPath = CurrentLODSection + FString(TEXT("/")) + ReductionMappingSection+ FString(TEXT("/")) +SpecularMapSection;
					GetBoolean(*SpecularMapSectionFullPath,IsEnabled,ChannelsMapping[TEXT("Specular")].bActive);
					GetBoolean(*SpecularMapSectionFullPath,sRGB,ChannelsMapping[TEXT("Specular")].bUseSRGB);
					GetBoolean(*SpecularMapSectionFullPath,BakeVertexColors,ChannelsMapping[TEXT("Specular")].bBakeVertexColors);

					//normals
					FString NormalsMapSectionFullPath = CurrentLODSection + FString(TEXT("/")) + ReductionMappingSection+ FString(TEXT("/")) +NormalsMapSection;
					GetBoolean(*NormalsMapSectionFullPath,IsEnabled,ChannelsMapping[TEXT("Normals")].bActive);
					GetBoolean(*NormalsMapSectionFullPath,sRGB,ChannelsMapping[TEXT("Normals")].bUseSRGB);
					GetBoolean(*NormalsMapSectionFullPath, FlipBackfacingNormals, ChannelsMapping[TEXT("Normals")].bFlipBackfacingNormals);
					GetBoolean(*NormalsMapSectionFullPath,TangentSpaceNormals,ChannelsMapping[TEXT("Normals")].bUseTangentSpaceNormals);
					GetBoolean(*NormalsMapSectionFullPath, FlipGreenAxis, ChannelsMapping[TEXT("Normals")].bFlipGreenChannel);



					CurrentChannelSettings[ChannelsIndexMapping[TEXT("Basecolor")]] = ChannelsMapping[TEXT("Basecolor")];
					CurrentChannelSettings[ChannelsIndexMapping[TEXT("Specular")]] = ChannelsMapping[TEXT("Specular")];
					CurrentChannelSettings[ChannelsIndexMapping[TEXT("Normals")]] = ChannelsMapping[TEXT("Normals")];

				}


			}


		}
	}
}