#pragma once
#include "T6WheelSetup.generated.h"

class UT6Wheel;

#if WITH_VEHICLE
namespace physx{
	class PxVehicleDrive;
	class PxVehicleWheels;
	class PxVehicleWheelsSimData;
}
#endif

USTRUCT()
struct ENGINE_API FT6WheelSetup{
	GENERATED_USTRUCT_BODY()

	// The wheel class to use
	UPROPERTY(EditAnywhere, Category=WheelSetup)
	TSubclassOf<UT6Wheel> WheelClass;

	// Bone name on mesh to create wheel at
	UPROPERTY(EditAnywhere, Category=WheelSetup)
	FName BoneName;

	// Additional offset to give the wheels for this axle.
	UPROPERTY(EditAnywhere, Category=WheelSetup)
	FVector AdditionalOffset;

	// Additional offset to give the wheels for this axle.
	UPROPERTY(EditAnywhere, Category = Control)
	float ThrottleMult;

	UPROPERTY(EditAnywhere, Category = Control)
	float BrakeMult;
	
	UPROPERTY(EditAnywhere, Category = Control)
	float HandbrakeMult;

	/** Tire type for the wheel. Determines friction */
	UPROPERTY(EditAnywhere, Category = Tire)
	float Friction;

	/** Max normalized tire load at which the tire can deliver no more lateral stiffness no matter how much extra load is applied to the tire. */
	UPROPERTY(EditAnywhere, Category = Tire, meta = (ClampMin = "0.01", UIMin = "0.01"))
		float											LatStiffMaxLoad;

	/** How much lateral stiffness to have given lateral slip */
	UPROPERTY(EditAnywhere, Category = Tire, meta = (ClampMin = "0.01", UIMin = "0.01"))
		float											LatStiffValue;

	/** How much longitudinal stiffness to have given longitudinal slip */
	UPROPERTY(EditAnywhere, Category = Tire)
		float											LongStiffValue;

	/** Vertical offset from vehicle center of mass where suspension forces are applied */
	UPROPERTY(EditAnywhere, Category = Suspension)
		FVector											SuspensionAxis;

	/** Vertical offset from vehicle center of mass where suspension forces are applied */
	UPROPERTY(EditAnywhere, Category = Suspension)
		float											SuspensionForceOffset;

	/** How far the wheel can go above the resting position */
	UPROPERTY(EditAnywhere, Category = Suspension)
		float											SuspensionMaxRaise;

	/** How far the wheel can drop below the resting position */
	UPROPERTY(EditAnywhere, Category = Suspension)
		float											SuspensionMaxDrop;

	/** (PSuspensionData.mSpringStrength) (pos) Oscillation frequency of suspension. Standard cars have values between 5 and 10 */
	UPROPERTY(EditAnywhere, Category = Suspension)
		float											SuspensionNaturalFrequency;

	/**
	*	(PSuspensionData.mSpringDamperRate) (vel) The rate at which energy is dissipated from the spring. Standard cars have values between 0.8 and 1.2.
	*	values < 1 are more sluggish, values > 1 or more twitchy
	*/
	UPROPERTY(EditAnywhere, Category = Suspension)
		float											SuspensionDampingRatio;

	/** max brake torque for this wheel (Nm) */
	UPROPERTY(EditAnywhere, Category = Brakes)
		float											MaxBrakeTorque;

	/**
	*	Max handbrake brake torque for this wheel (Nm). A handbrake should have a stronger brake torque
	*	than the brake. This will be ignored for wheels that are not affected by the handbrake.
	*/
	UPROPERTY(EditAnywhere, Category = Brakes)
		float											MaxHandBrakeTorque;

	/** Whether handbrake should affect this wheel */
	UPROPERTY(EditAnywhere, Category = Wheel)
		bool											bAffectedByHandbrake;

	/** Mass of this wheel */
	UPROPERTY(EditAnywhere, Category = Wheel, meta = (ClampMin = "0.01", UIMin = "0.01"))
		float											Mass;

	/** Damping rate for this wheel (Kgm^2/s) */
	UPROPERTY(EditAnywhere, Category = Wheel, meta = (ClampMin = "0.01", UIMin = "0.01"))
		float											DampingRate;

	// steer angle in degrees for this wheel
	UPROPERTY(EditAnywhere, Category = WheelsSetup, meta = (ClampMin = "0", UIMin = "0"))
		float SteerAngle;

	// Worldspace location of this wheel last frame
	UPROPERTY(transient)
		float											Radius;

	FT6WheelSetup();
};
