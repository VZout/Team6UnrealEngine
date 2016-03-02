// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	T6Spline.cpp
=============================================================================*/

#include "EnginePrivate.h"
#include "Components/T6SplineNodeComponent.h"
#include "ComponentInstanceDataCache.h"

UT6SplineComponent::UT6SplineComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer){
	//
}

UT6SplineNodeComponent::UT6SplineNodeComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer){
	//
}

void UT6SplineNodeComponent::PostLoad(){
	Super::PostLoad();
	//UpdateT6Spline();
}


void UT6SplineNodeComponent::PostEditImport(){
	Super::PostEditImport();
	//UpdateT6Spline();
}

//
class FT6SplineInstanceData : public FSceneComponentInstanceData{
public:
	explicit FT6SplineInstanceData(const UT6SplineNodeComponent* SourceComponent) : FSceneComponentInstanceData(SourceComponent){
		//
	}

	virtual void ApplyToComponent(UActorComponent* Component, const ECacheApplyPhase CacheApplyPhase) override{
		FSceneComponentInstanceData::ApplyToComponent(Component, CacheApplyPhase);
	}

	FInterpCurveVector T6SplineInfo;
};

/*FName UT6SplineNodeComponent::GetComponentInstanceDataType() const{
	static const FName T6SplineInstanceDataTypeName(TEXT("T6SplineInstanceData"));
	return T6SplineInstanceDataTypeName;
}*/

FActorComponentInstanceData* UT6SplineNodeComponent::GetComponentInstanceData() const{
	FT6SplineInstanceData* T6SplineInstanceData = new FT6SplineInstanceData(this);

	return T6SplineInstanceData;
}

#if WITH_EDITOR
void UT6SplineNodeComponent::PreEditChange(UProperty* PropertyAboutToChange){
	Super::PreEditChange(PropertyAboutToChange);
}

void UT6SplineNodeComponent::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedChainEvent){
	Super::PostEditChangeChainProperty(PropertyChangedChainEvent);
}

void UT6SplineNodeComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent){
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
