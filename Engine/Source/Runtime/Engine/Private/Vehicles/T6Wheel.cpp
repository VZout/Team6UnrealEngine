// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "GameFramework/PawnMovementComponent.h"
#include "PhysicsPublic.h"
#include "Vehicles/T6Wheel.h"
#include "Vehicles/TireType.h"

#include "../PhysicsEngine/PhysXSupport.h"
#include "../Vehicles/PhysXVehicleManager.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Vehicles/T6VehicleComponentBase.h"

UT6Wheel::UT6Wheel(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CollisionMeshObj(TEXT("/Engine/EngineMeshes/Cylinder"));
	CollisionMesh = CollisionMeshObj.Object;

	//ShapeRadius = 0;// Deze wordt automatisch ingevuld 30.0f;
	ShapeWidth = 10.0f;
}

void UT6Wheel::Init(UT6VehicleComponentBase* VehicleSim, int32 WheelIndex) {
	check(VehicleSim);
	check(VehicleSim->Wheels.IsValidIndex(WheelIndex));

	this->VehicleSim = VehicleSim;
	this->WheelIndex = WheelIndex;
	WheelShape = NULL;

	FPhysXVehicleManager* VehicleManager = VehicleSim->GetWorld()->GetPhysicsScene()->GetVehicleManager();
	SCOPED_SCENE_READ_LOCK(VehicleManager->GetScene());

	const int32 WheelShapeIdx = VehicleSim->PVehicle->mWheelsSimData.getWheelShapeMapping(WheelIndex);
	check(WheelShapeIdx >= 0);

	VehicleSim->PVehicle->getRigidDynamicActor()->getShapes(&WheelShape, 1, WheelShapeIdx);
	check(WheelShape);

	OldLocation = Location = GetPhysicsLocation();
}

void UT6Wheel::Shutdown(){
	WheelShape = NULL;
}

FT6WheelSetup& UT6Wheel::GetWheelSetup(){
	return VehicleSim->WheelSetups[WheelIndex];
}

void UT6Wheel::Tick(float DeltaTime) {
	OldLocation = Location;
	Location = GetPhysicsLocation();
	Velocity = (Location - OldLocation) / DeltaTime;
}

FVector UT6Wheel::GetPhysicsLocation() {
	if (WheelShape){
		FPhysXVehicleManager* VehicleManager = VehicleSim->GetWorld()->GetPhysicsScene()->GetVehicleManager();
		SCOPED_SCENE_READ_LOCK(VehicleManager->GetScene());

		PxVec3 PLocation = VehicleSim->PVehicle->getRigidDynamicActor()->getGlobalPose().transform(WheelShape->getLocalPose()).p;
		return P2UVector(PLocation);
	}
	else return FVector(0.0f);
}

#if WITH_EDITOR
void UT6Wheel::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) {
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Trigger a runtime rebuild of the PhysX vehicle
	FPhysXVehicleManager::VehicleSetupTag++;
}

#endif //WITH_EDITOR

UPhysicalMaterial* UT6Wheel::GetContactSurfaceMaterial() {
	FPhysXVehicleManager* VehicleManager = VehicleSim->GetWorld()->GetPhysicsScene()->GetVehicleManager();
	SCOPED_SCENE_READ_LOCK(VehicleManager->GetScene());

	const PxMaterial* ContactSurface = VehicleManager->GetWheelsStates_AssumesLocked(VehicleSim)[WheelIndex].tireSurfaceMaterial;

	if (ContactSurface){
		return FPhysxUserData::Get<UPhysicalMaterial>(ContactSurface->userData);
	}
	else return nullptr;
}

UPrimitiveComponent* UT6Wheel::GetContactSurfaceComponent(){
	FPhysXVehicleManager* VehicleManager = VehicleSim->GetWorld()->GetPhysicsScene()->GetVehicleManager();
	SCOPED_SCENE_READ_LOCK(VehicleManager->GetScene());

	const PxWheelQueryResult& QueryResult = VehicleManager->GetWheelsStates_AssumesLocked(VehicleSim)[WheelIndex];

	//PxShape* ContactShape = VehicleManager->GetWheelsStates_AssumesLocked(VehicleSim)[WheelIndex].tireContactShape;

	if (QueryResult.tireContactActor){
		FPhysxUserData* UserData = (FPhysxUserData*)QueryResult.tireContactActor->userData;

		UPrimitiveComponent* Component =  FPhysxUserData::Get<UPrimitiveComponent>(QueryResult.tireContactActor->userData);

		return Component;
	}
	else return nullptr;
}

FBodyInstance* UT6Wheel::GetContactSurfaceBody(){
	FPhysXVehicleManager* VehicleManager = VehicleSim->GetWorld()->GetPhysicsScene()->GetVehicleManager();
	SCOPED_SCENE_READ_LOCK(VehicleManager->GetScene());

	const PxWheelQueryResult& QueryResult = VehicleManager->GetWheelsStates_AssumesLocked(VehicleSim)[WheelIndex];

	if (QueryResult.tireContactActor){
		return FPhysxUserData::Get<FBodyInstance>(QueryResult.tireContactActor->userData);
	}
	else return nullptr;
}

float UT6Wheel::GetSteerAngle() {
	FPhysXVehicleManager* VehicleManager = VehicleSim->GetWorld()->GetPhysicsScene()->GetVehicleManager();
	SCOPED_SCENE_READ_LOCK(VehicleManager->GetScene());
	return FMath::RadiansToDegrees(VehicleManager->GetWheelsStates_AssumesLocked(VehicleSim)[WheelIndex].steerAngle);
}

float UT6Wheel::GetRotationAngle() {
	FPhysXVehicleManager* VehicleManager = VehicleSim->GetWorld()->GetPhysicsScene()->GetVehicleManager();
	SCOPED_SCENE_READ_LOCK(VehicleManager->GetScene());

	float RotationAngle = -1.0f * FMath::RadiansToDegrees(VehicleSim->PVehicle->mWheelsDynData.getWheelRotationAngle(WheelIndex));
	check(!FMath::IsNaN(RotationAngle));
	return RotationAngle;
}

FVector UT6Wheel::GetSuspensionOffset() {
	FPhysXVehicleManager* VehicleManager = VehicleSim->GetWorld()->GetPhysicsScene()->GetVehicleManager();
	SCOPED_SCENE_READ_LOCK(VehicleManager->GetScene());

	physx::PxWheelQueryResult& Wheel = VehicleManager->GetWheelsStates_AssumesLocked(VehicleSim)[WheelIndex];

	//FVector SuspensionDir = P2UVector(/*Wheel.suspLineStart + */Wheel.suspLineDir * -Wheel.suspJounce);// this->GetWheelSetup().SuspensionAxis;// P2UVector(Wheel.suspLineDir);

	//UE_LOG(LogVehicles, Warning, TEXT("Wheel %d axis: %s"), WheelIndex, *SuspensionDir.ToString());

	//return SuspensionDir;// *Wheel.suspJounce;

	return this->GetWheelSetup().SuspensionAxis * -Wheel.suspJounce;
}

void UT6Wheel::GetRaycastData(FVector& Start, FVector& End, FVector& Hit){
	FPhysXVehicleManager* VehicleManager = VehicleSim->GetWorld()->GetPhysicsScene()->GetVehicleManager();
	SCOPED_SCENE_READ_LOCK(VehicleManager->GetScene());

	physx::PxWheelQueryResult& Wheel = VehicleManager->GetWheelsStates_AssumesLocked(VehicleSim)[WheelIndex];

	FVector Dir = P2UVector(Wheel.suspLineDir);

	Start = P2UVector(Wheel.suspLineStart);
	End = Start + Dir * Wheel.suspLineLength;
	Hit = Start + Dir * -Wheel.suspJounce;
}

FTransform UT6Wheel::GetTransform(){
	FPhysXVehicleManager* VehicleManager = VehicleSim->GetWorld()->GetPhysicsScene()->GetVehicleManager();
	SCOPED_SCENE_READ_LOCK(VehicleManager->GetScene());

	physx::PxWheelQueryResult& Wheel = VehicleManager->GetWheelsStates_AssumesLocked(VehicleSim)[WheelIndex];

	return P2UTransform(Wheel.localPose);
}