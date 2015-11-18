// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "GameFramework/PawnMovementComponent.h"
#include "AI/Navigation/NavigationAvoidanceTypes.h"
#include "AI/RVOAvoidanceInterface.h"
#include "Vehicles/T6VehicleComponentBase.h"
#include "T6VehicleComponent.generated.h"

USTRUCT()
struct ENGINE_API FT6VehicleInputState{
	GENERATED_USTRUCT_BODY()

	// input replication: throttle
	UPROPERTY()
		int8 Throttle;

	void SetThrottle(float Throttle) {
		this->Throttle = Throttle * 127.0f;
	}

	float GetThrottle() {
		return Throttle / 127.0f;
	}

	UPROPERTY()
		int8 Steering;

	void SetSteering(float Steering) {
		this->Steering = Steering * 127.0f;
	}

	float GetSteering() {
		return Steering / 127.0f;
	}

	// input replication: handbrake
	UPROPERTY()
		uint8 Handbrake;

	float SetHandbrake(float Handbrake){
		return Handbrake * 255.0f;
	}

	float GetHandbrake(){
		return Handbrake / 255.0f;
	}
};

/**
 * Component to handle the vehicle simulation for an actor.
 */
UCLASS()
class ENGINE_API UT6VehicleComponent : public UT6VehicleComponentBase, public IRVOAvoidanceInterface {
	GENERATED_UCLASS_BODY()

	DECLARE_DELEGATE_TwoParams(FPostProcessAvoidanceSignature, UT6VehicleComponent* /*comp*/, FVector& /*velocity*/);
	
	/** delegate for modifying avoidance velocity */
	FPostProcessAvoidanceSignature PostProcessAvoidance;

	float Gear;
	float SlowTimer;

	float GetSteering();
	float GetAccel();

	/** Set the user input for the vehicle throttle */
	UFUNCTION(BlueprintCallable, Category = "Game|Components|WheeledVehicleMovement")
		void ClearInput();

	/** Set the user input for the vehicle throttle */
	UFUNCTION(BlueprintCallable, Category="Game|Components|WheeledVehicleMovement")
		void SetThrottleInput(float Throttle);
	
	/** Set the user input for the vehicle steering */
	UFUNCTION(BlueprintCallable, Category="Game|Components|WheeledVehicleMovement")
		void SetSteeringInput(float Steering);

	/** Set the user input for handbrake */
	UFUNCTION(BlueprintCallable, Category="Game|Components|WheeledVehicleMovement")
	void SetHandbrakeInput(bool bNewHandbrake);

	UPROPERTY(Category = "Engine", EditAnywhere, BlueprintReadWrite)
	float ThrottleTorque;

	UPROPERTY(Category = "Engine", EditAnywhere, BlueprintReadWrite)
	float BrakeTorque;

	UPROPERTY(Category = "Engine", EditAnywhere, BlueprintReadWrite)
	float HandbrakeTorque;

	// RVO Avoidance

	/** If set, component will use RVO avoidance */
	UPROPERTY(Category = "Avoidance", EditAnywhere, BlueprintReadWrite)
	uint32 bUseRVOAvoidance : 1;
	
	/** Vehicle Radius to use for RVO avoidance (usually half of vehicle width) */
	UPROPERTY(Category = "Avoidance", EditAnywhere, BlueprintReadWrite)
	float RVOAvoidanceRadius;
	
	/** Vehicle Height to use for RVO avoidance (usually vehicle height) */
	UPROPERTY(Category = "Avoidance", EditAnywhere, BlueprintReadWrite)
	float RVOAvoidanceHeight;
	
	/** Area Radius to consider for RVO avoidance */
	UPROPERTY(Category = "Avoidance", EditAnywhere, BlueprintReadWrite)
	float AvoidanceConsiderationRadius;

	/** Value by which to alter steering per frame based on calculated avoidance */
	UPROPERTY(Category = "Avoidance", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0"))
	float RVOSteeringStep;

	/** Value by which to alter throttle per frame based on calculated avoidance */
	UPROPERTY(Category = "Avoidance", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0"))
	float RVOThrottleStep;
	
	/** calculate RVO avoidance and apply it to current velocity */
	virtual void CalculateAvoidanceVelocity(float DeltaTime);

	/** No default value, for now it's assumed to be valid if GetAvoidanceManager() returns non-NULL. */
	UPROPERTY(Category = "Avoidance", VisibleAnywhere, BlueprintReadOnly, AdvancedDisplay)
	int32 AvoidanceUID;

	/** Moving actor's group mask */
	UPROPERTY(Category = "Avoidance", EditAnywhere, BlueprintReadOnly, AdvancedDisplay)
	FNavAvoidanceMask AvoidanceGroup;
	
	UFUNCTION(BlueprintCallable, Category = "Pawn|Components|WheeledVehicleMovement")
	void SetAvoidanceGroup(int32 GroupFlags);
	
	/** Will avoid other agents if they are in one of specified groups */
	UPROPERTY(Category = "Avoidance", EditAnywhere, BlueprintReadOnly, AdvancedDisplay)
	FNavAvoidanceMask GroupsToAvoid;

	UFUNCTION(BlueprintCallable, Category = "Pawn|Components|WheeledVehicleMovement")
	void SetGroupsToAvoid(int32 GroupFlags);
	
	/** Will NOT avoid other agents if they are in one of specified groups, higher priority than GroupsToAvoid */
	UPROPERTY(Category = "Avoidance", EditAnywhere, BlueprintReadOnly, AdvancedDisplay)
	FNavAvoidanceMask GroupsToIgnore;

	UFUNCTION(BlueprintCallable, Category = "Pawn|Components|WheeledVehicleMovement")
	void SetGroupsToIgnore(int32 GroupFlags);

	/** De facto default value 0.5 (due to that being the default in the avoidance registration function), indicates RVO behavior. */
	UPROPERTY(Category = "Avoidance", EditAnywhere, BlueprintReadOnly)
	float AvoidanceWeight;
	
	/** Change avoidance state and register with RVO manager if necessary */
	UFUNCTION(BlueprintCallable, Category = "Pawn|Components|WheeledVehicleMovement")
	void SetAvoidanceEnabled(bool bEnable);

public:
	// Setup
	virtual bool CreateVehicle();

	// Update
	virtual void UpdateSimulation(float DeltaTime);
	void UpdateState(float DeltaTime);

	// Debug stuff
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos);
	virtual void DrawDebugLines();


	UPROPERTY(transient)
	FT6VehicleInputState InputState;

	// replicated state of vehicle 
	UPROPERTY(Transient, Replicated)
	FT6VehicleInputState ReplicatedInputState;

	/** Pass current state to server */
	UFUNCTION(reliable, server, WithValidation)
	void ServerUpdateState(const FT6VehicleInputState& InputState2);

	/** Update RVO Avoidance for simulation */
	void UpdateAvoidance(float DeltaTime);
		
	/** called in Tick to update data in RVO avoidance manager */
	void UpdateDefaultAvoidance();
	
	/** lock avoidance velocity */
	void SetAvoidanceVelocityLock(class UAvoidanceManager* Avoidance, float Duration);
	
	/** Was avoidance updated in this frame? */
	UPROPERTY(Transient)
	uint32 bWasAvoidanceUpdated : 1;
	
	/** Calculated avoidance velocity used to adjust steering and throttle */
	FVector AvoidanceVelocity;
	
	/** forced avoidance velocity, used when AvoidanceLockTimer is > 0 */
	FVector AvoidanceLockVelocity;
	
	/** remaining time of avoidance velocity lock */
	float AvoidanceLockTimer;
	
	/** BEGIN IRVOAvoidanceInterface */
	virtual void SetRVOAvoidanceUID(int32 UID) override;
	virtual int32 GetRVOAvoidanceUID() override;
	virtual void SetRVOAvoidanceWeight(float Weight) override;
	virtual float GetRVOAvoidanceWeight() override;
	virtual FVector GetRVOAvoidanceOrigin() override;
	virtual float GetRVOAvoidanceRadius() override;
	virtual float GetRVOAvoidanceHeight() override;
	virtual float GetRVOAvoidanceConsiderationRadius() override;
	virtual FVector GetVelocityForRVOConsideration() override;
	virtual int32 GetAvoidanceGroupMask() override;
	virtual int32 GetGroupsToAvoidMask() override;
	virtual int32 GetGroupsToIgnoreMask() override;
	/** END IRVOAvoidanceInterface */
};