// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "PrimitiveComponent.h"
#include "T6SplineNodeComponent.generated.h"

/** 
 *	A T6Spline component is a T6Spline shape which can be used for other purposes (e.g. animating objects). It does not contain rendering capabilities itself (outside the editor) 
 *	@see https://docs.unrealengine.com/latest/INT/Resources/ContentExamples/Blueprint_T6Splines
 */
UCLASS(ClassGroup=Utility, meta=(BlueprintSpawnableComponent))
class ENGINE_API UT6SplineComponent : public UPrimitiveComponent{
	GENERATED_UCLASS_BODY()
};

UCLASS()
class ENGINE_API UT6SplineNodeComponent : public UT6SplineComponent{
	GENERATED_UCLASS_BODY()

	// Begin UActorComponent interface.
	virtual FActorComponentInstanceData* GetComponentInstanceData() const override;
	//virtual FName GetComponentInstanceDataType() const override;
	// End UActorComponent interface.

	// UObject interface
	virtual void PostLoad() override;
	virtual void PostEditImport() override;
#if WITH_EDITOR
	virtual void PreEditChange(UProperty* PropertyAboutToChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedChainEvent) override;
	// End of UObject interface
#endif
};

