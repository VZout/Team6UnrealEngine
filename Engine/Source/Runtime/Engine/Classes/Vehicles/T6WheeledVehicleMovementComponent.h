// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "GameFramework/PawnMovementComponent.h"

#include "T6WheeledVehicleMovementComponent.generated.h"

//class UT6Wheel;

//#if WITH_VEHICLE
namespace physx{
	class PxVehicleDrive;
	class PxVehicleWheels;
	class PxVehicleNoDrive;
	class PxVehicleWheelsSimData;
}
//#endif
/*
UCLASS(hidecategories = (PlanarMovement, "Components|Movement|Planar", Activation, "Components|Activation"), meta = (BlueprintSpawnableComponent))
class ENGINE_API IVehicleInterface : public UPawnMovementComponent{
	GENERATED_UCLASS_BODY()

	physx::PxVehicleNoDrive* PVehicle;

	//UPROPERTY(transient, duplicatetransient, BlueprintReadOnly, Category = Wheels)
	//TArray<UT6Wheel*> Wheels;

#if WITH_VEHICLE
	virtual void PreTick(float DeltaTime){}
	virtual void TickVehicle(float DeltaTime){}
#endif // WITH_VEHICLE

	class FPhysXVehicleManager* GetVehicleManager() const;
};*/

UCLASS(hidecategories = (PlanarMovement, "Components|Movement|Planar", Activation, "Components|Activation"), meta = (BlueprintSpawnableComponent))
class ENGINE_API UT6WheeledVehicleMovementComponent : public UPawnMovementComponent{
	GENERATED_UCLASS_BODY()
public:
	physx::PxVehicleNoDrive* PVehicle;

#if WITH_VEHICLE
	virtual void PreTick(float DeltaTime){}
	virtual void TickVehicle(float DeltaTime){}
#endif // WITH_VEHICLE

	class FPhysXVehicleManager* GetVehicleManager() const;
};
