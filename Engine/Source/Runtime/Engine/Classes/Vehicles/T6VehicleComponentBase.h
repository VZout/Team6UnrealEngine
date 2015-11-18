// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "GameFramework/PawnMovementComponent.h"
#include "AI/Navigation/NavigationAvoidanceTypes.h"
#include "AI/RVOAvoidanceInterface.h"
#include "Vehicles/T6WheelSetup.h"
#include "Curves/CurveFloat.h"
#include "T6VehicleComponentBase.generated.h"

class UT6Wheel;

#if WITH_VEHICLE
namespace physx{
	class PxVehicleDrive;
	class PxVehicleWheels;
	class PxVehicleNoDrive;
	class PxVehicleWheelsSimData;
}
#endif

class UTireType;


/**
 * Component to handle the vehicle simulation for an actor.
 */
UCLASS(hidecategories = (PlanarMovement, "Components|Movement|Planar", Activation, "Components|Activation"), meta = (BlueprintSpawnableComponent))
class ENGINE_API UT6VehicleComponentBase : public UPawnMovementComponent{
	GENERATED_UCLASS_BODY()

	// The instanced PhysX vehicle
	physx::PxVehicleNoDrive* PVehicle;

	/** Wheels to create */
	UPROPERTY(EditAnywhere, Category=VehicleSetup)
	TArray<FT6WheelSetup> WheelSetups;

	/** DragCoefficient of the vehicle chassis. */
	UPROPERTY(EditAnywhere, Category = Drag)
	float DragCoefficient;

	/** Chassis width used for drag force computation (cm)*/
	UPROPERTY(EditAnywhere, Category = Drag, meta = (ClampMin = "0.01", UIMin = "0.01"))
	float ChassisWidth;

	/** Chassis height used for drag force computation (cm)*/
	UPROPERTY(EditAnywhere, Category = Drag, meta = (ClampMin = "0.01", UIMin = "0.01"))
	float ChassisHeight;

	// Drag area in cm^2
	UPROPERTY(transient)
	float DragArea;

	UPROPERTY(EditAnywhere, Category = Drag)
	float SidewayDrag;

	/** Maximum steering versus forward speed (km/h) */
	UPROPERTY(EditAnywhere, Category = SteeringSetup)
	FRuntimeFloatCurve SteeringCurve;

	/** Maximum torque versus forward speed (km/h) */
	UPROPERTY(EditAnywhere, Category = Engine)
	FRuntimeFloatCurve TorqueCurve;

	// Debug drag magnitude last applied
	UPROPERTY(transient)
	float DebugDragMagnitude;

	/** Mass to set the vehicle chassis to. It's much easier to tweak vehicle settings when the mass doesn't change due to tweaks with the physics asset. [kg] */
	UPROPERTY(EditAnywhere, Category = VehicleSetup, meta = (ClampMin = "0.01", UIMin = "0.01"))
	float Mass;

	/** Center of Mass to set the vehicle chassis to.*/
	UPROPERTY(EditAnywhere, Category = VehicleSetup)
	FVector COMOffset;

	/** Scales the vehicle's inertia in each direction (forward, right, up) */
	UPROPERTY(EditAnywhere, Category = VehicleSetup, AdvancedDisplay)
	FVector InertiaTensorScale;

	/** Clamp normalized tire load to this value */
	UPROPERTY(EditAnywhere, Category = TireLoad, AdvancedDisplay)
	float MinNormalizedTireLoad;

	/** Clamp normalized tire load to this value */
	UPROPERTY(EditAnywhere, Category = TireLoad, AdvancedDisplay)
	float MinNormalizedTireLoadFiltered;

	/** Clamp normalized tire load to this value */
	UPROPERTY(EditAnywhere, Category = TireLoad, AdvancedDisplay)
	float MaxNormalizedTireLoad;

	/** Clamp normalized tire load to this value */
	UPROPERTY(EditAnywhere, Category = TireLoad, AdvancedDisplay)
	float MaxNormalizedTireLoadFiltered;

    /** PhysX sub-steps More sub-steps provides better stability but with greater computational cost. Typically, vehicles require more sub-steps at very low forward speeds. The threshold longitudinal speed has a default value of 5 metres per second. */
    UPROPERTY(EditAnywhere, Category=VehicleSetup, AdvancedDisplay, meta = (ClampMin = "0.1", UIMin = "1.0", ClampMax = "1000.0", UIMax = "10.0"))
    float ThresholdLongitudinalSpeed;
    
    /** The sub-step count below the threshold longitudinal speed has a default of 3. */
    UPROPERTY(EditAnywhere, Category=VehicleSetup, AdvancedDisplay, meta = (ClampMin = "1", UIMin = "1", ClampMax = "20", UIMax = "20"))
    int32 LowForwardSpeedSubStepCount;
    
    /** The sub-step count above the threshold longitudinal speed has a default of 1. */
    UPROPERTY(EditAnywhere, Category=VehicleSetup, AdvancedDisplay, meta = (ClampMin = "1", UIMin = "1", ClampMax = "20", UIMax = "20"))
    int32 HighForwardSpeedSubStepCount;
    
	// Our instanced wheels
	UPROPERTY(transient, duplicatetransient, BlueprintReadOnly, Category = Wheels)
	TArray<UT6Wheel*> Wheels;

	// The value of PhysXVehicleManager::VehicleSetupTag when this vehicle created its physics state.
	// Used to recreate the physics if the blueprint changes.
	uint32 VehicleSetupTag;

	UPROPERTY(transient)
		TArray<UTireType*> TireTypes;

#if WITH_VEHICLE
	/** Compute the forces generates from a spinning tire */
	//virtual void GenerateTireForces(class UT6Wheel* Wheel, const FT6TireShaderInput& Input, FT6TireShaderOutput& Output);

	// Physics state
	virtual bool ShouldCreatePhysicsState() const override;
	virtual bool HasValidPhysicsState() const override;

	virtual void CreatePhysicsState() override;
	virtual void DestroyPhysicsState() override;
	
	// Vehicle setup
	virtual void FixupSkeletalMesh();

	virtual bool CanCreateVehicle() const;
	virtual bool CreateVehicle();
	virtual void SetupVehicleMass();

	virtual void SetupWheelShapes();
	virtual void SetupWheelPhysics(physx::PxVehicleWheelsSimData* PWheelsSimData);

	// Util functions
	virtual FVector GetWheelRestingPosition(const FT6WheelSetup& WheelSetup);
	virtual FVector GetLocalCOM() const;

	// Updating
	virtual void PreTick(float DeltaTime);
	virtual void TickVehicle(float DeltaTime);

	virtual void UpdateDrag(float DeltaTime);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif //WITH_EDITOR

#endif // WITH_VEHICLE

	/** How fast the vehicle is moving forward */
	UFUNCTION(BlueprintCallable, Category="Game|Components|WheeledVehicleMovement")
	float GetForwardSpeed() const;

	/** Get current engine's rotation speed */
	UFUNCTION(BlueprintCallable, Category="Game|Components|WheeledVehicleMovement")
	float GetWheelRotationSpeed() const;

	UFUNCTION(BlueprintCallable, Category = "Game|Components|WheeledVehicleMovement")
	bool IsInAir() const;

protected:

	virtual void UpdateState(float DeltaTime);

#if WITH_VEHICLE
	/** Pass input values to vehicle simulation */
	virtual void UpdateSimulation( float DeltaTime );

	/** Get the mesh this vehicle is tied to */
	class USkinnedMeshComponent* GetMesh();

#endif // WITH_VEHICLE
};