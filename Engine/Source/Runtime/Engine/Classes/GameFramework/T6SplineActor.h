// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "GameFramework/Actor.h"
#include "Classes/Components/T6SplineNodeComponent.h"
#include "T6SplineActor.generated.h"

class AT6SplineNodeActor;

struct ENGINE_API T6Spline{
	FVector p0, m0, p1, m1;

	T6Spline();
	T6Spline(const AT6SplineNodeActor** CPs);

	void SetCPs(const AT6SplineNodeActor** CPs);

	FVector GetValue(float t);
	FVector GetFirstDerivative(float t);
	FVector GetSecondDerivative(float t);
};

UCLASS()
class ENGINE_API AT6SplineNodeActor : public AActor{
	GENERATED_BODY()

public:
	UPROPERTY()
	UT6SplineNodeComponent* Spline;

	UPROPERTY(Category = Connections, EditAnywhere, BlueprintReadWrite)
	TArray<AT6SplineNodeActor*> Connections;

	AT6SplineNodeActor(const FObjectInitializer& ObjectInitializer);

	bool IsLeafNode() const{
		for (int i = 0; i < Connections.Num(); i++){
			if (Connections[i] != nullptr){
				return false;
			}
		}

		return true;
	}

private:
	//void GenerateSplineInternal(FVector v[4], int Index);
};
