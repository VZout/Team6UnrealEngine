// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	T6Spline.cpp
=============================================================================*/

#include "EnginePrivate.h"
#include "Components/T6SplineComponent.h"
#include "ComponentInstanceDataCache.h"


UT6SplineComponent::UT6SplineComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer){
	//
}

void UT6SplineComponent::PostLoad(){
	Super::PostLoad();
	//UpdateT6Spline();
}


void UT6SplineComponent::PostEditImport(){
	Super::PostEditImport();
	//UpdateT6Spline();
}

//
class FT6SplineInstanceData : public FSceneComponentInstanceData{
public:
	explicit FT6SplineInstanceData(const UT6SplineComponent* SourceComponent) : FSceneComponentInstanceData(SourceComponent){
		//
	}

	virtual void ApplyToComponent(UActorComponent* Component, const ECacheApplyPhase CacheApplyPhase) override{
		FSceneComponentInstanceData::ApplyToComponent(Component, CacheApplyPhase);
	}

	FInterpCurveVector T6SplineInfo;
};

/*FName UT6SplineComponent::GetComponentInstanceDataType() const{
	static const FName T6SplineInstanceDataTypeName(TEXT("T6SplineInstanceData"));
	return T6SplineInstanceDataTypeName;
}*/

FActorComponentInstanceData* UT6SplineComponent::GetComponentInstanceData() const{
	FT6SplineInstanceData* T6SplineInstanceData = new FT6SplineInstanceData(this);

	return T6SplineInstanceData;
}

#if WITH_EDITOR
void UT6SplineComponent::PreEditChange(UProperty* PropertyAboutToChange){
	Super::PreEditChange(PropertyAboutToChange);
}

void UT6SplineComponent::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedChainEvent){
	Super::PostEditChangeChainProperty(PropertyChangedChainEvent);
}

void UT6SplineComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent){
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
