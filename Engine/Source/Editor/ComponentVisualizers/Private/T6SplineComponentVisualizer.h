// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ComponentVisualizer.h"
#include "Components/T6SplineComponent.h"
#include "GameFramework/T6SplineActor.h"

struct T6SplineNodeProxy : public HComponentVisProxy{
	DECLARE_HIT_PROXY();

	T6SplineNodeProxy(const UActorComponent* InComponent) : HComponentVisProxy(InComponent, HPP_Wireframe){
		//
	}
};

struct T6SplineLineProxy : public HComponentVisProxy{
	DECLARE_HIT_PROXY();

	AT6SplineActor* CPs[4];

	T6SplineLineProxy(const AT6SplineActor** CPs) : HComponentVisProxy(CPs[1]->GetRootComponent(), HPP_Wireframe){
		for (int i = 0; i < 4; i++){
			this->CPs[i] = (AT6SplineActor*)CPs[i];
		}
	}
};

struct T6SplineLine{
	AT6SplineActor* Node;
	AT6SplineActor* Target;

	T6SplineLine(){
		//
	}

	T6SplineLine(const AT6SplineActor* Node, const AT6SplineActor* Target){
		this->Node = (AT6SplineActor*)Node;
		this->Target = (AT6SplineActor*)Target;
	}

	bool operator==(const T6SplineLine& Other) const{
		return Node == Other.Node && Target == Other.Target;
	}
};

/** T6SplineComponent visualizer/edit functionality */
class FT6SplineComponentVisualizer : public FComponentVisualizer{
	bool bAllowDuplication;

	UPROPERTY(transient)
	TArray<AT6SplineActor*> SelectedNodes;

	UPROPERTY(transient)
	TArray<T6SplineLine> SelectedLines;

	UPROPERTY(transient)
	AT6SplineActor* LastSelectedNode;

	UPROPERTY(transient)
	TSharedPtr<FUICommandList> SplineComponentVisualizerActions;

	UPROPERTY(transient)
	FVector SnappingPos;

	UPROPERTY(transient)
	bool bSnappingLinePresent;

	UPROPERTY(transient)
	AT6SplineActor* SnappingNode;

	UPROPERTY(transient)
	FVector SelectedSplinePosition;

public:
	FT6SplineComponentVisualizer();
	virtual ~FT6SplineComponentVisualizer();


	// Begin FComponentVisualizer interface
	virtual void OnRegister() override;
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	virtual bool VisProxyHandleClick(FLevelEditorViewportClient* InViewportClient, HComponentVisProxy* VisProxy, const FViewportClick& Click);
	virtual void EndEditing() override;

	virtual bool GetWidgetLocation(const FEditorViewportClient* ViewportClient, FVector& OutLocation) const;

	virtual bool HandleInputDelta(FEditorViewportClient* ViewportClient, FViewport* Viewport, FVector& DeltaTranslate, FRotator& DeltaRotate, FVector& DeltaScale) override;
	virtual bool HandleInputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event);
	// End FComponentVisualizer interface

	void OnDeleteLines();
	void OnDeleteNodes();

	void OnDuplicateKey();
	void OnInsertKey(AT6SplineActor* Node1, AT6SplineActor* Node2, const FVector& Pos);

	void ChangeNodeSelection(AT6SplineActor* Actor, bool bIsCtrlHeld);
	void ChangeLineSelection(const T6SplineLine& Line, bool bIsCtrlHeld);

	void DrawVisualizationInternal(const AT6SplineActor* Actor, const FSceneView* View, FPrimitiveDrawInterface* PDI, TArray<const AT6SplineActor*>& ControlPoints);

	void NotifyComponentModified();
};
