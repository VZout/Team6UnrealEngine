// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ComponentVisualizersPrivatePCH.h"
#include "T6SplineComponentVisualizer.h"
#include "ScopedTransaction.h"
#include "GameFramework/T6SplineActor.h"

#define LOCTEXT_NAMESPACE "T6SplineComponentVisualizer"

IMPLEMENT_HIT_PROXY(T6SplineNodeProxy, HComponentVisProxy);
IMPLEMENT_HIT_PROXY(T6SplineLineProxy, HComponentVisProxy);

const float PointSize = 32;
const float LineSize = 32;

class FT6SplineComponentVisualizerCommands : public TCommands < FT6SplineComponentVisualizerCommands > {
public:
	FT6SplineComponentVisualizerCommands() : TCommands <FT6SplineComponentVisualizerCommands>("T6SplineComponentVisualizer",	// Context name for fast lookup
		LOCTEXT("T6SplineComponentVisualizer", "T6Spline Component Visualizer"),	// Localized context name for displaying
		NAME_None,	// Parent
		FEditorStyle::GetStyleSetName()){
		//
	}

	virtual void RegisterCommands() override{
		//
	}
};

FT6SplineComponentVisualizer::FT6SplineComponentVisualizer(){
	LastSelectedNode = nullptr;
	bAllowDuplication = true;

	bSnappingLinePresent = false;
	SnappingNode = nullptr;

	//FT6SplineComponentVisualizer::Register();

	SplineComponentVisualizerActions = MakeShareable(new FUICommandList);
}

FT6SplineComponentVisualizer::~FT6SplineComponentVisualizer(){
	//
}

void FT6SplineComponentVisualizer::OnRegister(){
	//const auto& Commands = FSplineComponentVisualizerCommands::Get();
}

void FT6SplineComponentVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI){
	const UT6SplineComponent* SplineComponent = Cast<const UT6SplineComponent>(Component);

	if (SplineComponent == nullptr){
		return;
	}

	AT6SplineActor* Actor = (AT6SplineActor*)Component->GetOwner();

	for (TActorIterator<AT6SplineActor> Iterator(Actor->GetWorld()); Iterator; ++Iterator){
		AT6SplineActor* Node = *Iterator;

		// Render the node itself
		FLinearColor Color;
		if (!bSnappingLinePresent){
			Color = SelectedNodes.Contains(Node) ? FLinearColor::Red : FLinearColor::White;
		}
		else{
			if (!SelectedNodes.Contains(Node)){
				Color = Node == SnappingNode ? FLinearColor::Green : FLinearColor::Red;
			}
			else Color = FLinearColor::White;
		}
		PDI->SetHitProxy(new T6SplineNodeProxy(Node->GetRootComponent()));
		PDI->DrawPoint(Node->GetTransform().GetTranslation(), Color, PointSize, SDPG_Foreground);
		PDI->SetHitProxy(nullptr);

		TArray<const AT6SplineActor*> Spline;
		Spline.Add(Node);

		DrawVisualizationInternal(Node, View, PDI, Spline);
	}

	if (bSnappingLinePresent){
		FLinearColor Color = SnappingNode != nullptr ? FLinearColor::Green : FLinearColor::Red;

		for (int i = 0; i < SelectedNodes.Num(); i++){
			PDI->DrawLine(SelectedNodes[i]->GetTransform().GetTranslation(), SnappingPos, Color, SDPG_Foreground, LineSize);
		}
	}
}

void FT6SplineComponentVisualizer::DrawVisualizationInternal(const AT6SplineActor* Node, const FSceneView* View, FPrimitiveDrawInterface* PDI, TArray<const AT6SplineActor*>& ControlPoints){
	if (ControlPoints.Num() == 4){
		// Gather the applicable control points
		const AT6SplineActor** CPs = ControlPoints.GetData() + (ControlPoints.Num() - 4);

		// Create the spline to be rendered
		T6Spline Spline(CPs);

		bool Selected = SelectedLines.Contains(T6SplineLine(CPs[1], CPs[2]));

		PDI->SetHitProxy(new T6SplineLineProxy(CPs));

		FVector OldPos = Spline.GetValue(0);
		
		for (int i = 1; i <= 8; i++){
			float t = i / 8.0f;

			FVector Pos = Spline.GetValue(t);

			FLinearColor Color = Selected ? FLinearColor::White : FMath::Lerp(FLinearColor::Red, FLinearColor::Green, t);

			PDI->DrawLine(OldPos, Pos, Color, SDPG_Foreground, LineSize);

			OldPos = Pos;
		}

		PDI->SetHitProxy(nullptr);
	}
	else if (Node->IsLeafNode() && ControlPoints.Num() >= 2){
		FVector OldPos = ControlPoints[0]->GetTransform().GetTranslation();

		for (int i = 1; i < ControlPoints.Num(); i++){
			//PDI->SetHitProxy(new T6SplineLineProxy(ControlPoints[i - 1], ControlPoints[i]));

			bool Selected = SelectedLines.Contains(T6SplineLine(ControlPoints[i - 1], ControlPoints[i]));
			FLinearColor Color = Selected ? FLinearColor::White : FLinearColor::Red;

			FVector Pos = ControlPoints[i]->GetTransform().GetTranslation();

			PDI->DrawLine(OldPos, Pos, Color, SDPG_Foreground, LineSize);

			OldPos = Pos;
		}

		//PDI->SetHitProxy(nullptr);
	}
	else{
		for (int i = 0; i < Node->Connections.Num(); i++){
			AT6SplineActor* Child = Cast<AT6SplineActor>(Node->Connections[i]);

			if (Child != nullptr){
				ControlPoints.Push(Child);
				DrawVisualizationInternal(Child, View, PDI, ControlPoints);
				ControlPoints.Pop();
			}
		}
	}
}

void FT6SplineComponentVisualizer::EndEditing(){
	SelectedNodes.Empty();
	LastSelectedNode = nullptr;

	bAllowDuplication = true;
}

bool FT6SplineComponentVisualizer::HandleInputDelta(FEditorViewportClient* ViewportClient, FViewport* Viewport, FVector& DeltaTranslate, FRotator& DeltaRotate, FVector& DeltaScale){
	if (ViewportClient->IsAltPressed()){
		if (bAllowDuplication){
			OnDuplicateKey();

			// Don't duplicate again until we release LMB
			bAllowDuplication = false;
		}
	}

	if (bSnappingLinePresent){
		SnappingPos +=/* LastSelectedNode->GetTransform().GetTranslation() +*/ DeltaTranslate;

		SnappingNode = nullptr;

		for (TActorIterator<AT6SplineActor> Iterator(LastSelectedNode->GetWorld()); Iterator; ++Iterator){
			AT6SplineActor* Node = *Iterator;

			if (SelectedNodes.Contains(Node)){
				continue;
			}

			if ((Node->GetTransform().GetTranslation() - SnappingPos).SizeSquared() < 100000){
				SnappingNode = Node;
			}
		}

		bSnappingLinePresent = true;

		GEditor->RedrawLevelEditingViewports(true);

		return true;
	}

	for (int i = 0; i < SelectedNodes.Num(); i++){
		AT6SplineActor* Actor = (AT6SplineActor*)SelectedNodes[i];

		if (!DeltaTranslate.IsZero()){
			FTransform Transform = Actor->GetTransform();
			Transform.SetTranslation(Transform.GetTranslation() + DeltaTranslate);

			Actor->SetActorTransform(Transform);
		}
	}

	return true;
}

bool FT6SplineComponentVisualizer::HandleInputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event){
	if ((Key == EKeys::LeftAlt || Key == EKeys::RightAlt)){
		if (Event == IE_Pressed){
			OnDuplicateKey();
		}

		return true;
	}
	else if ((Key == EKeys::LeftShift || Key == EKeys::RightShift)){
		if (Event == IE_Pressed){
			bSnappingLinePresent = true;
			SnappingNode = nullptr;

			SnappingPos = LastSelectedNode->GetTransform().GetTranslation();

			NotifyComponentModified();
		}
		/*else if (Event == IE_Released && bSnappingLinePresent){
			if (SnappingNode != nullptr){
				const FScopedTransaction Transaction(LOCTEXT("AddConnection", "Add spline connection"));

				for (int i = 0; i < SelectedNodes.Num(); i++){
					SelectedNodes[i]->Connections.Add(SnappingNode);
				}

				SnappingNode = nullptr;
			}

			bSnappingLinePresent = false;

			NotifyComponentModified();
		}*/

		return true;
	}
	else if (Key == EKeys::Delete && Event == IE_Pressed){
		if (SelectedLines.Num() != 0){
			OnDeleteLines();

			return true;
		}
		else{
			OnDeleteNodes();
		}
	}

	//if (Key == EKeys::SpaceBar && Event == IE_Pressed && !bSnappingLinePresent){
	if (Key == EKeys::LeftMouseButton && Event == IE_Released && bSnappingLinePresent){
		if (SnappingNode != nullptr){
			const FScopedTransaction Transaction(LOCTEXT("AddConnection", "Add spline connection"));

			for (int i = 0; i < SelectedNodes.Num(); i++){
				SelectedNodes[i]->Modify();
				SelectedNodes[i]->Connections.AddUnique(SnappingNode);
			}

			SnappingNode = nullptr;
		}

		bSnappingLinePresent = false;

		NotifyComponentModified();

		return false;
	}

	/*if (ViewportClient->IsShiftPressed() && Event == IE_Pressed){
		return true;
	}*/

	/*if (ViewportClient->IsAltPressed() && Event == IE_Pressed){
		return true;
	}*/

	if (Event == IE_Pressed){
		//bHandled = false;// SplineComponentVisualizerActions->ProcessCommandBindings(Key, FSlateApplication::Get().GetModifierKeys(), false);
	}

	return false;
}

bool FT6SplineComponentVisualizer::VisProxyHandleClick(FLevelEditorViewportClient* InViewportClient, HComponentVisProxy* VisProxy, const FViewportClick& Click){
	/*bool bEditing = false;

	if (VisProxy && VisProxy->Component.IsValid()){
		const USplineComponent* SplineComp = CastChecked<const USplineComponent>(VisProxy->Component.Get());

		SplineCompPropName = GetComponentPropertyName(SplineComp);
		if (SplineCompPropName != NAME_None){
			SplineOwningActor = SplineComp->GetOwner();
		}
	}*/

	if (VisProxy && VisProxy->Component.IsValid()){
		const UT6SplineComponent* Component = CastChecked<const UT6SplineComponent>(VisProxy->Component.Get());

		AT6SplineActor* Actor = (AT6SplineActor*)Component->GetOwner();

		if (VisProxy->IsA(T6SplineNodeProxy::StaticGetType())){
			if (Click.GetKey() != EKeys::RightMouseButton || !SelectedNodes.Contains(Actor)){
				ChangeNodeSelection(Actor, InViewportClient->IsCtrlPressed());
			}

			return true;
		}
		else if (VisProxy->IsA(T6SplineLineProxy::StaticGetType())){
			if (Click.GetKey() != EKeys::RightMouseButton || !SelectedNodes.Contains(Actor)){
				T6SplineLineProxy* LineProxy = (T6SplineLineProxy*)VisProxy;

				if (Click.GetKey() == EKeys::RightMouseButton){
					T6Spline Spline((const AT6SplineActor**)LineProxy->CPs);

					FVector SubsegmentStart = Spline.GetValue(0);

					float ClosestDistance = TNumericLimits<float>::Max();
					FVector BestLocation = SubsegmentStart;

					for (int i = 1; i <= 16; i++){
						FVector SubsegmentEnd = Spline.GetValue(i / 16.0f);

						FVector SplineClosest;
						FVector RayClosest;
						FMath::SegmentDistToSegmentSafe(SubsegmentStart, SubsegmentEnd, Click.GetOrigin(), Click.GetOrigin() + Click.GetDirection() * 50000.0f, SplineClosest, RayClosest);

						const float Distance = FVector::DistSquared(SplineClosest, RayClosest);
						if (Distance < ClosestDistance){
							ClosestDistance = Distance;
							BestLocation = SplineClosest;
						}

						SubsegmentStart = SubsegmentEnd;
					}

					OnInsertKey(LineProxy->CPs[1], LineProxy->CPs[2], BestLocation);
				}
				else ChangeLineSelection(T6SplineLine(LineProxy->CPs[1], LineProxy->CPs[2]), InViewportClient->IsCtrlPressed());
			}

			return true;
		}
		else{
			return false;	// Error
		}
	}
	else return false;
}

void FT6SplineComponentVisualizer::ChangeNodeSelection(AT6SplineActor* Actor, bool bIsCtrlHeld){
	if (!bIsCtrlHeld){
		GEditor->GetSelectedActors()->DeselectAll();
		GEditor->SelectActor(Actor, true, true);

		if (Actor != nullptr){
			SelectedNodes.Empty();
			SelectedNodes.Add(Actor);

			LastSelectedNode = Actor;
		}
	}
	else{
		if (SelectedNodes.Contains(Actor)){
			GEditor->SelectActor(Actor, false, false);

			SelectedNodes.Remove(Actor);

			if (SelectedNodes.Num() != 0){
				LastSelectedNode = SelectedNodes[rand() % SelectedNodes.Num()];
			}
			else{
				LastSelectedNode = nullptr;
			}
		}
		else{
			GEditor->SelectActor(Actor, true, false);

			SelectedNodes.Add(Actor);

			LastSelectedNode = Actor;
		}
	}

	SelectedLines.Empty();
}

void FT6SplineComponentVisualizer::ChangeLineSelection(const T6SplineLine& Line, bool bIsCtrlHeld){
	if (!bIsCtrlHeld){
		SelectedLines.Empty();
		SelectedLines.Add(Line);
	}
	else{
		if (SelectedLines.Contains(Line)){
			SelectedLines.Remove(Line);
		}
		else{
			SelectedLines.Add(Line);
		}
	}
}

bool FT6SplineComponentVisualizer::GetWidgetLocation(const FEditorViewportClient* ViewportClient, FVector& OutLocation) const{
	if (LastSelectedNode != nullptr){
		// Otherwise use the last key index set
		check(SelectedNodes.Contains(LastSelectedNode));

		OutLocation = LastSelectedNode->GetTransform().GetTranslation();
		return true;
	}

	return false;
}

void FT6SplineComponentVisualizer::NotifyComponentModified(){
	GEditor->RedrawLevelEditingViewports(true);
}

void FT6SplineComponentVisualizer::OnDeleteLines(){
	const FScopedTransaction Transaction(LOCTEXT("RemoveSplineConnection", "Remove spline connection"));

	for (int i = 0; i < SelectedLines.Num(); i++){
		SelectedLines[i].Node->Modify();

		SelectedLines[i].Node->Connections.Remove(SelectedLines[i].Target);
	}

	SelectedLines.Empty();

	NotifyComponentModified();
}

void FT6SplineComponentVisualizer::OnDeleteNodes(){
	const FScopedTransaction Transaction(LOCTEXT("RemoveSpline", "Remove spline"));
	// Todo: hier alle objecten door itereren, en de connections van deze node toevoegen in hun lijsten

	for (TActorIterator<AT6SplineActor> Iterator(LastSelectedNode->GetWorld()); Iterator; ++Iterator){
		AT6SplineActor* Node = *Iterator;

		for (int i = 0; i < SelectedNodes.Num(); i++){
			AT6SplineActor* SelectedNode = SelectedNodes[i];

			if (Node->Connections.Find(SelectedNode) != INDEX_NONE){
				Node->Modify();

				Node->Connections.Remove(SelectedNode);

				for (int j = 0; j < SelectedNode->Connections.Num(); j++){
					if (SelectedNode->Connections[j] != nullptr){
						Node->Connections.AddUnique(SelectedNode->Connections[j]);
					}
				}
			}
		}
	}

	//GEditor->

	for (int i = 0; i < SelectedNodes.Num(); i++){
		SelectedNodes[i]->Modify();
		SelectedNodes[i]->Destroy();
	}

	SelectedNodes.Empty();
	LastSelectedNode = nullptr;
}

void FT6SplineComponentVisualizer::OnDuplicateKey(){
	const FScopedTransaction Transaction(LOCTEXT("DuplicateSplinePoint", "Duplicate Spline Point"));

	TArray<AT6SplineActor*> SelectedNodes = this->SelectedNodes;

	ChangeNodeSelection(nullptr, false);

	for (int i = 0; i < SelectedNodes.Num(); i++){
		AT6SplineActor* SelectedNode = SelectedNodes[i];

		SelectedNode->Modify();

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Template = SelectedNode;

		AT6SplineActor* Actor = SelectedNode->GetWorld()->SpawnActor<AT6SplineActor>(SelectedNode->GetClass(), SpawnParameters);
		Actor->Connections.Empty();
	
		SelectedNode->Connections.AddUnique(Actor);
		
		GEditor->SelectActor(Actor, true, true);
	}

	NotifyComponentModified();
}

void FT6SplineComponentVisualizer::OnInsertKey(AT6SplineActor* Node1, AT6SplineActor* Node2, const FVector& Pos){
	const FScopedTransaction Transaction(LOCTEXT("InsertSplinePoint", "Insert Spline Point"));

	Node1->Modify();

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Template = Node2;

	AT6SplineActor* Actor = Node1->GetWorld()->SpawnActor<AT6SplineActor>(Node1->GetClass(), Pos, FRotator(), SpawnParameters);
	Actor->Connections.Empty();

	Node1->Connections.Remove(Node2);
	Node1->Connections.AddUnique(Actor);

	Actor->Connections.AddUnique(Node2);

	//ChangeNodeSelection(nullptr, false);
	//GEditor->SelectActor(Actor, true, true);

	NotifyComponentModified();
}

#undef LOCTEXT_NAMESPACE
