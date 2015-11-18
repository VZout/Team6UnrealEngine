// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "PhysicsPublic.h"
#include "Vehicles/T6WheelSetup.h"
#include "Vehicles/TireType.h"

//TEMPLATE Load Obj From Path
template <typename ObjClass>
static FORCEINLINE ObjClass* LoadObjFromPath(const FName& Path) {
	if (Path == NAME_None) return NULL;

	return Cast<ObjClass>(StaticLoadObject(ObjClass::StaticClass(), NULL, *Path.ToString()));
}

FT6WheelSetup::FT6WheelSetup() : WheelClass(UT6Wheel::StaticClass()), BoneName(NAME_None), AdditionalOffset(0.0f){
	ThrottleMult = 0;	// RW: 1
	BrakeMult = 1;
	HandbrakeMult = 0;
	Friction = 2000;
	LatStiffValue = 4.0f;
	LongStiffValue = 4000.0f;	// RW: 16000.0
	SteerAngle = 45.0f;

	Mass = 20.0f;
	bAffectedByHandbrake = true;
	
	MaxBrakeTorque = 1500.f;
	MaxHandBrakeTorque = 3000.f;
	DampingRate = 0.25f;
	LatStiffMaxLoad = 2.0f;
	
	SuspensionAxis = FVector(0, 0, -1);
	SuspensionForceOffset = 0.0f;
	SuspensionMaxRaise = 10.0f;
	SuspensionMaxDrop = 10.0f;
	SuspensionNaturalFrequency = 7.0f;
	SuspensionDampingRatio = 1.0f;

	//static ConstructorHelpers::FObjectFinder<UTireType> TireTypeObj(TEXT("/Engine/EngineTireTypes/DefaultTireType"));
	//TireType = TireTypeObj.Object;

	Friction = 1.0f;// TireType = LoadObjFromPath<UTireType>("/Engine/EngineTireTypes/DefaultTireType");
}
