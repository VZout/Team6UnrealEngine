// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Vehicle.cpp: AT6Vehicle implementation
	TODO: Put description here
=============================================================================*/

#include "EnginePrivate.h"
#include "GameFramework/T6Vehicle.h"
#include "Vehicles/T6VehicleComponent.h"
#include "DisplayDebugHelpers.h"

FName AT6Vehicle::VehicleMovementComponentName(TEXT("MovementComp"));
FName AT6Vehicle::VehicleMeshComponentName(TEXT("VehicleMesh"));

AT6Vehicle::AT6Vehicle(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer){
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(VehicleMeshComponentName);
	Mesh->SetCollisionProfileName(UCollisionProfile::Vehicle_ProfileName);
	Mesh->BodyInstance.bSimulatePhysics = true;
	Mesh->BodyInstance.bNotifyRigidBodyCollision = true;
	Mesh->BodyInstance.bUseCCD = true;
	Mesh->bBlendPhysics = true;
	Mesh->bGenerateOverlapEvents = true;
	Mesh->bCanEverAffectNavigation = false;
	RootComponent = Mesh;

	VehicleMovement = CreateDefaultSubobject<UT6VehicleComponent, UT6VehicleComponent>(VehicleMovementComponentName);
	VehicleMovement->SetIsReplicated(true); // Enable replication by default
	VehicleMovement->UpdatedComponent = Mesh;
}

void AT6Vehicle::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos){
	static FName NAME_Vehicle = FName(TEXT("Vehicle"));

	Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);

#if WITH_VEHICLE
	if (DebugDisplay.IsDisplayOn(NAME_Vehicle)){
		((UT6VehicleComponent*)GetVehicleMovementComponent())->DrawDebug(Canvas, YL, YPos);
	}
#endif
}

class UT6VehicleComponentBase* AT6Vehicle::GetVehicleMovementComponent() const {
	return VehicleMovement;
}

/** Returns Mesh subobject **/
USkeletalMeshComponent* AT6Vehicle::GetMesh() const { return Mesh; }

/** Returns VehicleMovement subobject **/
UT6VehicleComponentBase* AT6Vehicle::GetVehicleMovement() const { return VehicleMovement; }
