// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "PhysicalMaterials/PhysicalMaterial.h"


void FMeshProxySettings::PostLoadDeprecated()
{
	FMeshProxySettings DefaultObject;
	
	if (TextureWidth_DEPRECATED != DefaultObject.TextureHeight_DEPRECATED)
	{
		Material.BaseColorMapSize.X = TextureWidth_DEPRECATED;
		Material.NormalMapSize.X = TextureWidth_DEPRECATED;
		Material.MetallicMapSize.X = TextureWidth_DEPRECATED;
		Material.RoughnessMapSize.X = TextureWidth_DEPRECATED;
		Material.SpecularMapSize.X = TextureWidth_DEPRECATED;
	}
	if (TextureHeight_DEPRECATED != DefaultObject.TextureHeight_DEPRECATED)
	{
		Material.BaseColorMapSize.Y = TextureHeight_DEPRECATED;
		Material.NormalMapSize.Y = TextureHeight_DEPRECATED;
		Material.MetallicMapSize.Y = TextureHeight_DEPRECATED;
		Material.RoughnessMapSize.Y = TextureHeight_DEPRECATED;
		Material.SpecularMapSize.Y = TextureHeight_DEPRECATED;
	}
	if (bExportNormalMap_DEPRECATED != DefaultObject.bExportNormalMap_DEPRECATED)
	{
		Material.bNormalMap = bExportNormalMap_DEPRECATED;
	}
	if (bExportMetallicMap_DEPRECATED != DefaultObject.bExportMetallicMap_DEPRECATED)
	{
		Material.bMetallicMap = bExportMetallicMap_DEPRECATED;
	}
	if (bExportRoughnessMap_DEPRECATED != DefaultObject.bExportRoughnessMap_DEPRECATED)
	{
		Material.bRoughnessMap = bExportRoughnessMap_DEPRECATED;
	}
	if (bExportSpecularMap_DEPRECATED != DefaultObject.bExportSpecularMap_DEPRECATED)
	{
		Material.bSpecularMap = bExportSpecularMap_DEPRECATED;
	}
}

UEngineBaseTypes::UEngineBaseTypes(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UEngineTypes::UEngineTypes(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

ECollisionChannel UEngineTypes::ConvertToCollisionChannel(ETraceTypeQuery TraceType)
{
	return UCollisionProfile::Get()->ConvertToCollisionChannel(true, (int32)TraceType);
}

ECollisionChannel UEngineTypes::ConvertToCollisionChannel(EObjectTypeQuery ObjectType)
{
	return UCollisionProfile::Get()->ConvertToCollisionChannel(false, (int32)ObjectType);
}

EObjectTypeQuery UEngineTypes::ConvertToObjectType(ECollisionChannel CollisionChannel)
{
	return UCollisionProfile::Get()->ConvertToObjectType(CollisionChannel);
}

ETraceTypeQuery UEngineTypes::ConvertToTraceType(ECollisionChannel CollisionChannel)
{
	return UCollisionProfile::Get()->ConvertToTraceType(CollisionChannel);
}

void FDamageEvent::GetBestHitInfo(AActor const* HitActor, AActor const* HitInstigator, FHitResult& OutHitInfo, FVector& OutImpulseDir) const
{
	ensure(HitActor);
	if (HitActor)
	{
		// fill out the hitinfo as best we can
		OutHitInfo.Actor = const_cast<AActor*>(HitActor);
		OutHitInfo.bBlockingHit = true;
		OutHitInfo.BoneName = NAME_None;
		OutHitInfo.Component = Cast<UPrimitiveComponent>(HitActor->GetRootComponent());
		
		// assume the actor got hit in the center of his root component
		OutHitInfo.ImpactPoint = HitActor->GetActorLocation();
		OutHitInfo.Location = OutHitInfo.ImpactPoint;
		
		// assume hit came from instigator's location
		OutImpulseDir = HitInstigator ? 
			( OutHitInfo.ImpactPoint - HitInstigator->GetActorLocation() ).GetSafeNormal()
			: FVector::ZeroVector;

		// assume normal points back toward instigator
		OutHitInfo.ImpactNormal = -OutImpulseDir;
		OutHitInfo.Normal = OutHitInfo.ImpactNormal;
	}
}

void FPointDamageEvent::GetBestHitInfo(AActor const* HitActor, AActor const* HitInstigator, FHitResult& OutHitInfo, FVector& OutImpulseDir) const
{
	// assume the actor got hit in the center of his root component
	OutHitInfo = HitInfo;
	OutImpulseDir = ShotDirection;
}


void FRadialDamageEvent::GetBestHitInfo(AActor const* HitActor, AActor const* HitInstigator, FHitResult& OutHitInfo, FVector& OutImpulseDir) const
{
	ensure(ComponentHits.Num() > 0);

	// for now, just return the first one
	OutHitInfo = ComponentHits[0];
	OutImpulseDir = (OutHitInfo.ImpactPoint - Origin).GetSafeNormal();
}


float FRadialDamageParams::GetDamageScale(float DistanceFromEpicenter) const
{
	float const ValidatedInnerRadius = FMath::Max(0.f, InnerRadius);
	float const ValidatedOuterRadius = FMath::Max(OuterRadius, ValidatedInnerRadius);
	float const ValidatedDist = FMath::Max(0.f, DistanceFromEpicenter);

	if (ValidatedDist >= ValidatedOuterRadius)
	{
		// outside the radius, no effect
		return 0.f;
	}

	if ( (DamageFalloff == 0.f)	|| (ValidatedDist <= ValidatedInnerRadius) )
	{
		// no falloff or inside inner radius means full effect
		return 1.f;
	}

	// calculate the interpolated scale
	float DamageScale = 1.f - ( (ValidatedDist - ValidatedInnerRadius) / (ValidatedOuterRadius - ValidatedInnerRadius) );
	DamageScale = FMath::Pow(DamageScale, DamageFalloff);

	return DamageScale;
}

FLightmassDebugOptions::FLightmassDebugOptions()
	: bDebugMode(false)
	, bStatsEnabled(false)
	, bGatherBSPSurfacesAcrossComponents(true)
	, CoplanarTolerance(0.001f)
	, bUseImmediateImport(true)
	, bImmediateProcessMappings(true)
	, bSortMappings(true)
	, bDumpBinaryFiles(false)
	, bDebugMaterials(false)
	, bPadMappings(true)
	, bDebugPaddings(false)
	, bOnlyCalcDebugTexelMappings(false)
	, bUseRandomColors(false)
	, bColorBordersGreen(false)
	, bColorByExecutionTime(false)
	, ExecutionTimeDivisor(15.0f)
{}

USceneComponent* FComponentReference::GetComponent(AActor* OwningActor) const
{
	USceneComponent* Result = NULL;
	// Component is specified directly, use that
	if(OverrideComponent.IsValid())
	{
		Result = OverrideComponent.Get();
	}
	else
	{
		// Look in Actor if specified, OwningActor if not
		AActor* SearchActor = (OtherActor != NULL) ? OtherActor : OwningActor;
		if(SearchActor)
		{
			if(ComponentProperty != NAME_None)
			{
				UObjectPropertyBase* ObjProp = FindField<UObjectPropertyBase>(SearchActor->GetClass(), ComponentProperty);
				if(ObjProp != NULL)
				{
					// .. and return the component that is there
					Result = Cast<USceneComponent>(ObjProp->GetObjectPropertyValue_InContainer(SearchActor));
				}
			}
			else
			{
				Result = Cast<USceneComponent>(SearchActor->GetRootComponent());
			}
		}
	}

	return Result;
}

FString FHitResult::ToString() const
{
	return FString::Printf(TEXT("bBlockingHit:%s bStartPenetrating:%s Time:%f Location:%s ImpactPoint:%s Normal:%s ImpactNormal:%s TraceStart:%s TraceEnd:%s PenetrationDepth:%f Item:%d PhysMaterial:%s Actor:%s Component:%s BoneName:%s FaceIndex:%d"),
		bBlockingHit == true ? TEXT("True") : TEXT("False"),
		bStartPenetrating == true ? TEXT("True") : TEXT("False"),
		Time,
		*Location.ToString(),
		*ImpactPoint.ToString(),
		*Normal.ToString(),
		*ImpactNormal.ToString(),
		*TraceStart.ToString(),
		*TraceEnd.ToString(),
		PenetrationDepth,
		Item,
		PhysMaterial.IsValid() ? *PhysMaterial->GetName() : TEXT("None"),
		Actor.IsValid() ? *Actor->GetName() : TEXT("None"),
		Component.IsValid() ? *Component->GetName() : TEXT("None"),
		BoneName.IsValid() ? *BoneName.ToString() : TEXT("None"),
		FaceIndex);
}

FRepMovement::FRepMovement()
	: LinearVelocity(ForceInit)
	, AngularVelocity(ForceInit)
	, Location(ForceInit)
	, Rotation(ForceInit)
	, bSimulatedPhysicSleep(false)
	, bRepPhysics(false)
	, LocationQuantizationLevel(EVectorQuantization::RoundWholeNumber)
	, VelocityQuantizationLevel(EVectorQuantization::RoundWholeNumber)
	, RotationQuantizationLevel(ERotatorQuantization::ByteComponents)
{
}

//@third party code - BEGIN SIMPLYGON
FSimplygonMaterialLODSettings::FSimplygonMaterialLODSettings()
	: bActive(false)
	, MaterialLODType(EMaterialLODType::BakeTexture)
	, bUseAutomaticSizes(false)
	, TextureWidth(ESimplygonTextureResolution::Type::TextureResolution_512)
	, TextureHeight(ESimplygonTextureResolution::Type::TextureResolution_512)
	, SamplingQuality(ESimplygonTextureSamplingQuality::Low)
	, GutterSpace(4)
	, TextureStrech(ESimplygonTextureStrech::Medium)
	, bReuseExistingCharts(false)
	, bBakeVertexData(true)
	, bBakeActorData(false)
	, bAllowMultiMaterial(false)
	, bPreferTwoSideMaterials(false)
{
	/*
	 * Note: when adding new channels here, should update
	 * - FSimplygonUtilities::CreateFlattenMaterial()
	 * - MaterialBakerSettingsLayout.h, SIMPLYGON_NUM_CHANNELS_TO_BAKE constant
	 * - MaterialExportUtils::FFlattenMaterial struct
	 */
	ChannelsToCast.Add(FSimplygonChannelCastingSettings(ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_BASECOLOR,ESimplygonCasterType::Color,ESimplygonColorChannels::RGB));
	ChannelsToCast.Add(FSimplygonChannelCastingSettings(ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_SPECULAR,ESimplygonCasterType::Color,ESimplygonColorChannels::RGB));
	ChannelsToCast.Add(FSimplygonChannelCastingSettings(ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_ROUGHNESS,ESimplygonCasterType::Color,ESimplygonColorChannels::RGB));
	ChannelsToCast.Add(FSimplygonChannelCastingSettings(ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_METALLIC,ESimplygonCasterType::Color,ESimplygonColorChannels::RGB));
	ChannelsToCast.Add(FSimplygonChannelCastingSettings(ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_NORMALS,ESimplygonCasterType::Normals,ESimplygonColorChannels::RGB));
	ChannelsToCast.Add(FSimplygonChannelCastingSettings(ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_OPACITY,ESimplygonCasterType::Opacity,ESimplygonColorChannels::RGB));
	ChannelsToCast.Add(FSimplygonChannelCastingSettings(ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_EMISSIVE,ESimplygonCasterType::Color,ESimplygonColorChannels::RGB));
	ChannelsToCast.Add(FSimplygonChannelCastingSettings(ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_AO,ESimplygonCasterType::Color,ESimplygonColorChannels::RGB));
}

FSimplygonMaterialLODSettings::FSimplygonMaterialLODSettings(const FSimplygonMaterialLODSettings& Other)
	: bActive(Other.bActive)
	, MaterialLODType(Other.MaterialLODType)
	, bUseAutomaticSizes(Other.bUseAutomaticSizes)
	, TextureWidth(Other.TextureWidth)
	, TextureHeight(Other.TextureHeight)
	, SamplingQuality(Other.SamplingQuality)
	, GutterSpace(Other.GutterSpace)
	, TextureStrech(Other.TextureStrech)
	, bReuseExistingCharts(Other.bReuseExistingCharts)
	, bBakeVertexData(Other.bBakeVertexData)
	, bBakeActorData(Other.bBakeActorData)
	, bAllowMultiMaterial(Other.bAllowMultiMaterial)
	, bPreferTwoSideMaterials(Other.bPreferTwoSideMaterials)
{
	ChannelsToCast.Empty();
	for(int ItemIndex=0; ItemIndex < Other.ChannelsToCast.Num(); ItemIndex++)
	{
		ChannelsToCast.Add(Other.ChannelsToCast[ItemIndex]);
		
	}
}

bool FSimplygonMaterialLODSettings::operator==(const FSimplygonMaterialLODSettings& Other) const
{
	if (bActive == false && Other.bActive == false)
	{
		// Ignore other fields when both objects are inactive
		return true;
	}

	if (bActive != Other.bActive)
	{
		return false;
	}

	// Compare channels. Do that carefully to avoid false 'differs' when comparing objects
	// prepared with different integration versions, i.e. when number of casting channels differs.
	int32 NumCastChannels = FMath::Max(ChannelsToCast.Num(), Other.ChannelsToCast.Num());
	for (int32 ItemIndex = 0; ItemIndex < NumCastChannels; ItemIndex++)
	{
		if (!ChannelsToCast.IsValidIndex(ItemIndex))
		{
			// 'this' has less ChannelsToCast items - check if corresponding Other's channel enabled
			if (Other.ChannelsToCast[ItemIndex].bActive)
			{
				// enabled, so settings differs
				return false;
			}
			continue;
		}
		// Check opposite condition
		if (!Other.ChannelsToCast.IsValidIndex(ItemIndex))
		{
			if (ChannelsToCast[ItemIndex].bActive)
			{
				return false;
			}
			continue;
		}
		// This channel exists in both structures, compare settings
		if (ChannelsToCast[ItemIndex] != Other.ChannelsToCast[ItemIndex])
		{
			return false;
		}
	}
	
	// Compare other fields
	return MaterialLODType == Other.MaterialLODType
		&& bUseAutomaticSizes == Other.bUseAutomaticSizes
		&& TextureWidth == Other.TextureWidth
		&& TextureHeight == Other.TextureHeight
		&& SamplingQuality == Other.SamplingQuality
		&& GutterSpace == Other.GutterSpace
		&& TextureStrech == Other.TextureStrech
		&& bReuseExistingCharts == Other.bReuseExistingCharts
		&& bBakeVertexData == Other.bBakeVertexData
		&& bBakeActorData == Other.bBakeActorData
		&& bAllowMultiMaterial == Other.bAllowMultiMaterial
		&& bPreferTwoSideMaterials == Other.bPreferTwoSideMaterials;
}
//@third party code - END SIMPLYGON
