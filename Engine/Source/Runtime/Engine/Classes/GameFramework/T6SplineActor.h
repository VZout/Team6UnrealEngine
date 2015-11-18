// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "GameFramework/Actor.h"
#include "Classes/Components/T6SplineComponent.h"
#include "T6SplineActor.generated.h"

class AT6SplineActor;

struct ENGINE_API T6Spline{
	FVector p0, m0, p1, m1;

	T6Spline();
	T6Spline(const AT6SplineActor** CPs);

	void SetCPs(const AT6SplineActor** CPs);

	FVector GetValue(float t);
	FVector GetFirstDerivative(float t);
	FVector GetSecondDerivative(float t);
};

UCLASS()
class ENGINE_API AT6SplineActor : public AActor{
	GENERATED_BODY()

public:
	UPROPERTY()
		UT6SplineComponent* Spline;

	UPROPERTY(Category = Connections, EditAnywhere, BlueprintReadWrite)
		TArray<AT6SplineActor*> Connections;

	AT6SplineActor(const FObjectInitializer& ObjectInitializer);


	/*AT6NavNode* GetRandomConnection();

	FRRSimpleSpline GenerateSpline();*/

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
