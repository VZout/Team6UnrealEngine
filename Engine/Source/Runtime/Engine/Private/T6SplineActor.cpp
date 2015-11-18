// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Vehicle.cpp: AT6NavNode implementation
	TODO: Put description here
=============================================================================*/

#include "EnginePrivate.h"
#include "GameFramework/T6SplineActor.h"
//#include "Vehicles/T6NavNodeComponent.h"

T6Spline::T6Spline(){
	//
}

T6Spline::T6Spline(const AT6SplineActor** CPs){
	SetCPs(CPs);
}

void T6Spline::SetCPs(const AT6SplineActor** CPs){
	const FVector CP0 = CPs[0]->GetTransform().GetTranslation();
	const FVector CP1 = CPs[1]->GetTransform().GetTranslation();
	const FVector CP2 = CPs[2]->GetTransform().GetTranslation();
	const FVector CP3 = CPs[3]->GetTransform().GetTranslation();

	p0 = CP1;
	m0 = (CP2 - CP0) / 2;

	p1 = CP2;
	m1 = (CP3 - CP1) / 2;
}

FVector T6Spline::GetValue(float t){
	float t2 = t * t;
	float t3 = t2 * t;

	float h00 = 2 * t3 - 3 * t2 + 1;
	float h10 = t3 - 2 * t2 + t;
	float h01 = -2 * t3 + 3 * t2;
	float h11 = t3 - t2;

	return p0 * h00 + m0 * h10 + p1 * h01 + m1 * h11;
}

FVector T6Spline::GetFirstDerivative(float t){
	float t2 = t * t;

	float h00 = 6 * t2 - 6 * t;
	float h10 = 3 * t2 - 4 * t + 1;
	float h01 = -6 * t2 + 6 * t;
	float h11 = 3 * t2 - 2 * t;

	return p0 * h00 + m0 * h10 + p1 * h01 + m1 * h11;
}

FVector T6Spline::GetSecondDerivative(float t){
	float h00 = 12 * t - 6;
	float h10 = 6 * t - 4;
	float h01 = -12 * t + 6;
	float h11 = 6 * t - 2;

	return p0 * h00 + m0 * h10 + p1 * h01 + m1 * h11;
}

AT6SplineActor::AT6SplineActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer){
	Spline = CreateDefaultSubobject<UT6SplineComponent>(TEXT("Spline"));

	RootComponent = Spline;
}

