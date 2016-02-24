#include "EnginePrivate.h"
#include "Engine.h"
#include "PhysicsPublic.h"
#include "Vehicles/T6WheeledVehicleMovementComponent.h"

UT6WheeledVehicleMovementComponent::UT6WheeledVehicleMovementComponent(const FObjectInitializer& Initializer) : Super(Initializer){
	//
}

FPhysXVehicleManager* UT6WheeledVehicleMovementComponent::GetVehicleManager() const{
	return World->GetPhysicsScene()->GetVehicleManager();
}