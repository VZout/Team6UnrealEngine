// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*
 * Component to handle the vehicle simulation for an actor
 */

#pragma once
#include "T6Wheel.generated.h"

class UPhysicalMaterial;

#if WITH_PHYSX
namespace physx
{
	class PxShape;
}
#endif // WITH_PHYSX

UCLASS(BlueprintType, Blueprintable)
class ENGINE_API UT6Wheel : public UObject{
	GENERATED_UCLASS_BODY()

	/** 
	 * Static mesh with collision setup for wheel, will be used to create wheel shape
	 * (if empty, sphere will be added as wheel shape, check bDontCreateShape flag)
	 */
	UPROPERTY(EditDefaultsOnly, Category=Shape)
	class UStaticMesh*								CollisionMesh;

	/** Width of the wheel */
	UPROPERTY(EditAnywhere, Category = Wheel, meta = (ClampMin = "0.01", UIMin = "0.01"))
	float											ShapeWidth;

	/** The vehicle that owns us */
	UPROPERTY(transient)
	class UT6VehicleComponentBase*						VehicleSim;

	// Our index in the vehicle's (and setup's) wheels array
	UPROPERTY(transient)
	int32											WheelIndex;

	// Longitudinal slip experienced by the wheel
	UPROPERTY(transient)
	float											DebugLongSlip;

	// Lateral slip experienced by the wheel
	UPROPERTY(transient)
	float											DebugLatSlip;

	// How much force the tire experiences at rest devided by how much force it is experiencing now
	UPROPERTY(transient)
	float											DebugNormalizedTireLoad;

	// Wheel torque
	UPROPERTY(transient)
	float											DebugWheelTorque;

	// Longitudinal force the wheel is applying to the chassis
	UPROPERTY(transient)
	float											DebugLongForce;

	// Lateral force the wheel is applying to the chassis
	UPROPERTY(transient)
	float											DebugLatForce;

	// Worldspace location of this wheel
	UPROPERTY(transient)
	FVector											Location;

	// Worldspace location of this wheel last frame
	UPROPERTY(transient)
	FVector											OldLocation;

	// Current velocity of the wheel center (change in location over time)
	UPROPERTY(transient)
	FVector											Velocity;

	UFUNCTION(BlueprintCallable, Category="Game|Components|WheeledVehicleMovement")
	float GetSteerAngle();

	UFUNCTION(BlueprintCallable, Category="Game|Components|WheeledVehicleMovement")
	float GetRotationAngle();

	UFUNCTION(BlueprintCallable, Category="Game|Components|WheeledVehicleMovement")
	FVector GetSuspensionOffset();


#if WITH_PHYSX

	// Our wheelshape
	physx::PxShape*									WheelShape;

	/**
	 * Initialize this wheel instance
	 */
	virtual void Init(class UT6VehicleComponentBase* InVehicleSim, int32 InWheelIndex);

	/**
	 * Notify this wheel it will be removed from the scene
	 */
	virtual void Shutdown();

	/**
	 * Get the wheel setup we were created from
	 */
	struct FT6WheelSetup& GetWheelSetup();

	/**
	 * Tick this wheel when the vehicle ticks
	 */
	virtual void Tick( float DeltaTime );

#if WITH_EDITOR

	/**
	 * Respond to a property change in editor
	 */
	virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent ) override;

#endif //WITH_EDITOR

protected:

	/**
	 * Get the wheel's location in physics land
	 */
	FVector GetPhysicsLocation();

#endif // WITH_PHYSX

public:

	/** Get contact surface material */
	UPhysicalMaterial* GetContactSurfaceMaterial();
};
