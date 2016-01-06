// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "PhysicsPublic.h"

#include "../PhysicsEngine/PhysXSupport.h"
#include "../Collision/PhysXCollision.h"
#include "PhysXVehicleManager.h"

#include "Vehicles/T6Tire.h"
#include "Vehicles/T6Wheel.h"
#include "Vehicles/T6VehicleComponentBase.h"

////////////////////////////////////////////////////////////////////////////
//Default tire force shader function.
//Taken from Michigan tire model.
//Computes tire long and lat forces plus the aligning moment arising from 
//the lat force and the torque to apply back to the wheel arising from the 
//long force (application of Newton's 3rd law).
////////////////////////////////////////////////////////////////////////////

#define ONE_TWENTYSEVENTH 0.037037f
#define ONE_THIRD 0.33333f
PX_FORCE_INLINE PxF32 XsmoothingFunction1(const PxF32 K) {
	//Equation 20 in CarSimEd manual Appendix F.
	//Looks a bit like a curve of sqrt(x) for 0<x<1 but reaching 1.0 on y-axis at K=3. 
	PX_ASSERT(K >= 0.0f);
	return PxMin(1.0f, K - ONE_THIRD*K*K + ONE_TWENTYSEVENTH*K*K*K);
}
PX_FORCE_INLINE PxF32 XsmoothingFunction2(const PxF32 K) {
	//Equation 21 in CarSimEd manual Appendix F.
	//Rises to a peak at K=0.75 and falls back to zero by K=3
	PX_ASSERT(K >= 0.0f);
	return (K - K*K + ONE_THIRD*K*K*K - ONE_TWENTYSEVENTH*K*K*K*K);
}

/**
\brief Prototype of shader function that is used to compute wheel torque and tire forces.
\param[in]  shaderData is the shader data for the tire being processed.  The shader data describes the tire data in the format required by the tire model that is implemented by the shader function.
\param[in]  tireFriction is the value of friction for the contact between the tire and the ground.
\param[in]  longSlip is the value of longitudinal slip experienced by the tire.
\param[in]  latSlip is the value of lateral slip experienced by the tire.
\param[in]  camber is the camber angle of the tire in radians.
\param[in]  wheelOmega is the rotational speed of the wheel.
\param[in]  wheelRadius is the distance from the tire surface to the center of the wheel.
\param[in]  recipWheelRadius is the reciprocal of wheelRadius.
\param[in]  restTireLoad is the load force experienced by the tire when the vehicle is at rest.
\param[in]  normalisedTireLoad is a pre-computed value equal to the load force on the tire divided by restTireLoad.
\param[in]  tireLoad is the load force currently experienced by the tire (= restTireLoad*normalisedTireLoad)
\param[in]  gravity is the magnitude of gravitational acceleration.
\param[in]  recipGravity is the reciprocal of the magnitude of gravitational acceleration.
\param[out] wheelTorque is the torque that is to be applied to the wheel around the wheel's axle.
\param[out] tireLongForceMag is the magnitude of the longitudinal tire force to be applied to the vehicle's rigid body.
\param[out] tireLatForceMag is the magnitude of the lateral tire force to be applied to the vehicle's rigid body.
\param[out] tireAlignMoment is the aligning moment of the tire that is to be applied to the vehicle's rigid body (not currently used).
@see PxVehicleWheelsDynData::setTireForceShaderFunction,  PxVehicleWheelsDynData::setTireForceShaderData
*/

/* Notes to self
 Totale tireload van alle wielen = Mass van alle bodies * Gravity
 
 */

static void T6VehicleComputeTireForceDefault(const void* tireShaderData, PxF32 tireFriction, PxF32 longSlip, PxF32 latSlip, PxF32 camber, PxF32 wheelOmega, PxF32 wheelRadius,  PxF32 recipWheelRadius,
	PxF32 restTireLoad, PxF32 normalisedTireLoad, PxF32 tireLoad, PxF32 gravity, PxF32 recipGravity, PxF32& wheelTorque, PxF32& tireLongForceMag, PxF32& tireLatForceMag, PxF32& tireAlignMoment) {
	PX_UNUSED(wheelOmega);
	PX_UNUSED(recipWheelRadius);

	// Load heeft geen invloed op de friction
	//tireLoad = restTireLoad;
	//normalisedTireLoad = 1;

	//longSlip = 0;

	const PxVehicleTireData& tireData = *((PxVehicleTireData*)tireShaderData);

	/*UE_LOG(LogVehicles, Warning, TEXT("Friction = %.2f LongSlip = %.2f LatSlip = %.2f"), tireFriction, longSlip, latSlip);
	UE_LOG(LogVehicles, Warning, TEXT("Camber = %.2f Omega = %.2f Radius = %.2f Gravity = %.2f"), camber, wheelOmega, wheelRadius, gravity);
	UE_LOG(LogVehicles, Warning, TEXT("RestLoad = %.2f NormLoad = %.2f Load = %.2f"), restTireLoad, normalisedTireLoad, tireLoad);
	UE_LOG(LogVehicles, Warning, TEXT("RestLoad = %.2f NormLoad = %.2f Load = %.2f"), restTireLoad, normalisedTireLoad, tireLoad);*/


	/*tireLongForceMag = wheelOmega;// longSlip;
	tireLatForceMag = 0;// latSlip;
	wheelTorque = -tireLongForceMag * wheelRadius;
	tireAlignMoment = 0;

	return;*/

	



	PX_ASSERT(tireFriction>0);
	PX_ASSERT(tireLoad>0);

	wheelTorque = 0.0f;
	tireLongForceMag = 0.0f;
	tireLatForceMag = 0.0f;
	tireAlignMoment = 0.0f;

	//If long slip/lat slip/camber are all zero than there will be zero tire force.
	if (FMath::IsNearlyZero(latSlip) && FMath::IsNearlyZero(longSlip) && FMath::IsNearlyZero(camber)) {
		return;
	}

	//Compute the lateral stiffness
	const PxF32 latStiff = restTireLoad*tireData.mLatStiffY*XsmoothingFunction1(normalisedTireLoad*3.0f / tireData.mLatStiffX);

	//Get the longitudinal stiffness
	const PxF32 longStiff = tireData.mLongitudinalStiffnessPerUnitGravity*gravity;
	const PxF32 recipLongStiff = tireData.getRecipLongitudinalStiffnessPerUnitGravity()*recipGravity;

	//Get the camber stiffness.
	const PxF32 camberStiff = tireData.mCamberStiffnessPerUnitGravity*gravity;

	//Carry on and compute the forces.
	const PxF32 TEff = PxTan(latSlip - camber*camberStiff / latStiff);
	const PxF32 K = PxSqrt(latStiff*TEff*latStiff*TEff + longStiff*longSlip*longStiff*longSlip) / (tireFriction*tireLoad);
	//const PxF32 KAbs=PxAbs(K);
	PxF32 FBar = XsmoothingFunction1(K);//K - ONE_THIRD*PxAbs(K)*K + ONE_TWENTYSEVENTH*K*K*K;
	PxF32 MBar = XsmoothingFunction2(K); //K - KAbs*K + ONE_THIRD*K*K*K - ONE_TWENTYSEVENTH*KAbs*K*K*K;
	//Mbar = PxMin(Mbar, 1.0f);
	PxF32 nu = 1;
	if (K <= 2.0f*PxPi) {
		const PxF32 latOverlLong = latStiff*recipLongStiff;
		nu = 0.5f*(1.0f + latOverlLong - (1.0f - latOverlLong)*PxCos(K*0.5f));
	}
	const PxF32 FZero = tireFriction*tireLoad / (PxSqrt(longSlip*longSlip + nu*TEff*nu*TEff));
	const PxF32 fz = longSlip*FBar*FZero;
	const PxF32 fx = -nu*TEff*FBar*FZero;
	//TODO: pneumatic trail.
	const PxF32 pneumaticTrail = 1.0f;
	const PxF32	fMy = nu * pneumaticTrail * TEff * MBar * FZero;

	//We can add the torque to the wheel.
	wheelTorque = -fz*wheelRadius;
	tireLongForceMag = fz;
	tireLatForceMag = fx;
	tireAlignMoment = fMy;

	//UE_LOG(LogVehicles, Warning, TEXT("Torque = %.2f, LongForceMag = %.2f, ForceMag = %.2f, AlignMoment = %.2f"), wheelTorque, tireLongForceMag, tireLatForceMag, tireAlignMoment);
}


struct FT6TireShaderInput{
	// Friction value of the tire contact.
	float TireFriction;

	// Longitudinal slip of the tire
	float LongSlip;

	// Lateral slip of the tire.
	float LatSlip;

	// Rotational speed of the wheel, in radians.
	float WheelOmega;

	// The distance from the tire surface to the center of the wheel.
	float WheelRadius;

	// 1 / WheelRadius
	float RecipWheelRadius;

	// How much force (weight) is pushing on the tire when the vehicle is at rest.
	float RestTireLoad;

	// How much force (weight) is pushing on the tire right now.
	float TireLoad;

	// RestTireLoad / TireLoad
	float NormalizedTireLoad;

	// Acceleration due to gravity
	float Gravity;

	// 1 / Gravity
	float RecipGravity;
};

struct FT6TireShaderOutput{
	// The torque to be applied to the wheel around the wheel axle. Opposes the engine torque on the wheel
	float WheelTorque;

	// The magnitude of the longitudinal tire force to be applied to the vehicle's rigid body.
	float LongForce;

	// The magnitude of the lateral tire force to be applied to the vehicle's rigid body.
	float LatForce;

	FT6TireShaderOutput() {}

	FT6TireShaderOutput(float f) : WheelTorque(f), LongForce(f), LatForce(f) {
		//
	}
};

void GenerateTireForces(UT6Wheel* Wheel, const FT6TireShaderInput& Input, FT6TireShaderOutput& Output) {
	const void* realShaderData = &Wheel->VehicleSim->PVehicle->mWheelsSimData.getTireData(Wheel->WheelIndex);

	float Dummy;

	T6VehicleComputeTireForceDefault(realShaderData, Input.TireFriction, Input.LongSlip, Input.LatSlip, 0.0f, Input.WheelOmega, Input.WheelRadius, Input.RecipWheelRadius, Input.RestTireLoad, Input.NormalizedTireLoad, Input.TireLoad, Input.Gravity,
		Input.RecipGravity, Output.WheelTorque, Output.LongForce, Output.LatForce, Dummy);

	ensureMsgf(Output.WheelTorque == Output.WheelTorque, TEXT("Output.WheelTorque is bad: %f"), Output.WheelTorque);
	ensureMsgf(Output.LongForce == Output.LongForce, TEXT("Output.LongForce is bad: %f"), Output.LongForce);
	ensureMsgf(Output.LatForce == Output.LatForce, TEXT("Output.LatForce is bad: %f"), Output.LatForce);

	//UE_LOG( LogVehicles, Warning, TEXT("Friction = %f	LongSlip = %f	LatSlip = %f"), Input.TireFriction, Input.LongSlip, Input.LatSlip );
	//UE_LOG( LogVehicles, Warning, TEXT("WheelTorque= %f	LongForce = %f	LatForce = %f"), Output.WheelTorque, Output.LongForce, Output.LatForce );
	//UE_LOG( LogVehicles, Warning, TEXT("RestLoad= %f	NormLoad = %f	TireLoad = %f"),Input.RestTireLoad, Input.NormalizedTireLoad, Input.TireLoad );
}

/**
* PhysX shader for tire friction forces
* tireFriction - friction value of the tire contact.
* longSlip - longitudinal slip of the tire.
* latSlip - lateral slip of the tire.
* camber - camber angle of the tire
* wheelOmega - rotational speed of the wheel.
* wheelRadius - the distance from the tire surface and the center of the wheel.
* recipWheelRadius - the reciprocal of wheelRadius.
* restTireLoad - the load force experienced by the tire when the vehicle is at rest.
* normalisedTireLoad - a value equal to the load force on the tire divided by the restTireLoad.
* tireLoad - the load force currently experienced by the tire.
* gravity - magnitude of gravitational acceleration.
* recipGravity - the reciprocal of the magnitude of gravitational acceleration.
* wheelTorque - the torque to be applied to the wheel around the wheel axle.
* tireLongForceMag - the magnitude of the longitudinal tire force to be applied to the vehicle's rigid body.
* tireLatForceMag - the magnitude of the lateral tire force to be applied to the vehicle's rigid body.
* tireAlignMoment - the aligning moment of the tire that is to be applied to the vehicle's rigid body (not currently used).
*/
void T6TireShader(const void* shaderData, const PxF32 tireFriction, const PxF32 longSlip, const PxF32 latSlip, const PxF32 camber, const PxF32 wheelOmega, const PxF32 wheelRadius, const PxF32 recipWheelRadius, const PxF32 restTireLoad,
				  const PxF32 normalisedTireLoad, const PxF32 tireLoad, const PxF32 gravity, const PxF32 recipGravity, PxF32& wheelTorque, PxF32& tireLongForceMag, PxF32& tireLatForceMag, PxF32& tireAlignMoment){
	UT6Wheel* Wheel = (UT6Wheel*)shaderData;

	FT6TireShaderInput Input;

	Input.TireFriction = tireFriction;
	Input.LongSlip = longSlip;
	Input.LatSlip = latSlip;
	Input.WheelOmega = wheelOmega;
	Input.WheelRadius = wheelRadius;
	Input.RecipWheelRadius = recipWheelRadius;
	Input.NormalizedTireLoad = normalisedTireLoad;
	Input.RestTireLoad = restTireLoad;
	Input.TireLoad = tireLoad;
	Input.Gravity = gravity;
	Input.RecipGravity = recipGravity;

	FT6TireShaderOutput Output(0.0f);

	GenerateTireForces(Wheel, Input, Output);

	wheelTorque = Output.WheelTorque;
	tireLongForceMag = Output.LongForce;
	tireLatForceMag = Output.LatForce;

	if ( /*Wheel->bDebugWheels*/true) {
		Wheel->DebugLongSlip = longSlip;
		Wheel->DebugLatSlip = latSlip;
		Wheel->DebugNormalizedTireLoad = normalisedTireLoad;
		Wheel->DebugWheelTorque = wheelTorque;
		Wheel->DebugLongForce = tireLongForceMag;
		Wheel->DebugLatForce = tireLatForceMag;
	}
}

