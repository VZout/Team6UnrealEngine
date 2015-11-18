// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "GameFramework/Pawn.h"
#include "T6Vehicle.generated.h"

class FDebugDisplayInfo;

/**
 * T6Vehicle is the base T6 vehicle pawn actor.
 * By default it uses UT6VehicleMovementComponent4W for its simulation, but this can be overridden by inheriting from the class and modifying its constructor like so:
 * Super(ObjectInitializer.SetDefautSubobjectClass<UMyMovement>(VehicleMovementComponentName))
 * Where UMyMovement is the new movement type that inherits from UT6VehicleMovementComponent
 * 
 * @see https://docs.unrealengine.com/latest/INT/Engine/Physics/Vehicles/VehicleUserGuide/
 * @see UT6VehicleMovementComponent4W
 */

UCLASS(abstract, config=Game, BlueprintType)
class ENGINE_API AT6Vehicle : public APawn{
	GENERATED_UCLASS_BODY()

private_subobject:
	/**  The main skeletal mesh associated with this Vehicle */
	DEPRECATED_FORGAME(4.6, "Mesh should not be accessed directly, please use GetMesh() function instead. Mesh will soon be private and your code will not compile.")
	UPROPERTY(Category = Vehicle, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* Mesh;

	/** vehicle simulation component */
	DEPRECATED_FORGAME(4.6, "VehicleMovement should not be accessed directly, please use GetVehicleMovement() function instead. VehicleMovement will soon be private and your code will not compile.")
	UPROPERTY(Category = Vehicle, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UT6VehicleComponentBase* VehicleMovement;
public:

	/** Name of the MeshComponent. Use this name if you want to prevent creation of the component (with ObjectInitializer.DoNotCreateDefaultSubobject). */
	static FName VehicleMeshComponentName;

	/** Name of the VehicleMovement. Use this name if you want to use a different class (with ObjectInitializer.SetDefaultSubobjectClass). */
	static FName VehicleMovementComponentName;

	/** Util to get the T6 vehicle movement component */
	class UT6VehicleComponentBase* GetVehicleMovementComponent() const;

	// Begin AActor interface
	virtual void DisplayDebug(class UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos) override;
	// End Actor interface

	/** Returns Mesh subobject **/
	class USkeletalMeshComponent* GetMesh() const;
	/** Returns VehicleMovement subobject **/
	class UT6VehicleComponentBase* GetVehicleMovement() const;
};
