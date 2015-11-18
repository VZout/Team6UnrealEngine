// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "PhysicsPublic.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "MessageLog.h"
#include "Vehicles/T6Wheel.h"
#include "Vehicles/T6Tire.h"
#include "Vehicles/T6VehicleComponentBase.h"
#include "Vehicles/TireType.h"

#include "../PhysicsEngine/PhysXSupport.h"
#include "../Collision/PhysXCollision.h"
#include "PhysXVehicleManager.h"

#include "PhysicalMaterials/PhysicalMaterial.h"

#define LOCTEXT_NAMESPACE "UT6VehicleComponentBase"

UT6VehicleComponentBase::UT6VehicleComponentBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer){
	Mass = 1500.0f;

	DragCoefficient = 0.3f;
	SidewayDrag = 1000;

	ChassisWidth = 180.f;
	ChassisHeight = 140.f;

	InertiaTensorScale = FVector(1.0f, 1.0f, 1.0f);
	MinNormalizedTireLoad = 0.0f;
	MaxNormalizedTireLoad = 10.0f;
	   
    ThresholdLongitudinalSpeed = 5.f;
    LowForwardSpeedSubStepCount = 3;
    HighForwardSpeedSubStepCount = 1;

	// tire load filtering
	PxVehicleTireLoadFilterData PTireLoadFilterDef;
	MinNormalizedTireLoad = PTireLoadFilterDef.mMinNormalisedLoad;
	MinNormalizedTireLoadFiltered = PTireLoadFilterDef.mMinFilteredNormalisedLoad;
	MaxNormalizedTireLoad = PTireLoadFilterDef.mMaxNormalisedLoad;
	MaxNormalizedTireLoadFiltered = PTireLoadFilterDef.mMaxFilteredNormalisedLoad;

	FRichCurve* SteeringCurveData = SteeringCurve.GetRichCurve();
	for (int i = 0; i < 6; i++){
		SteeringCurveData->AddKey(i * 100, FMath::Pow(1.0f / 5.0f, i));
	}

	FRichCurve* TorqueCurveData = TorqueCurve.GetRichCurve();
	for (int i = 0; i < 12; i++) {
		TorqueCurveData->AddKey(i * 50, FMath::Pow(0.7f, i));
	}
}

void UT6VehicleComponentBase::FixupSkeletalMesh(){	// Functie nog renamen
	//in skeletal mesh case we must set the offset on the PrimitiveComponent's BodyInstance, which will later update the actual root body
	//this is needed for UI

	/*FRichCurve* TorqueCurveData = TorqueCurve.GetRichCurve();
	for (int i = 0; i < 12; i++) {
		float Value = TorqueCurveData->Eval(i * 50);

		UE_LOG(LogVehicles, Warning, TEXT("%d: %.2f"), i, Value)
	}*/

	USkeletalMeshComponent * Mesh = Cast<USkeletalMeshComponent>(GetMesh());
	check(Mesh);

	UPhysicsAsset* PhysicsAsset = Mesh->GetPhysicsAsset();
	check(PhysicsAsset);

	for (int32 i = 0; i < WheelSetups.Num(); i++){
		FT6WheelSetup& WheelSetup = WheelSetups[i];

		int32 BodySetupIdx = PhysicsAsset->FindBodyIndex(WheelSetup.BoneName);
		check(BodySetupIdx >= 0);

		UBodySetup* BodySetup = PhysicsAsset->BodySetup[BodySetupIdx];
		check(BodySetup);

		FBodyInstance* BodyInstance = Mesh->Bodies[BodySetupIdx];
		BodyInstance->SetResponseToAllChannels(ECR_Ignore);	//turn off collision for wheel automatically

		FBox Box = BodyInstance->GetBodyBounds();

		float Volume = BodySetup->GetVolume(FVector(1, 1, 1));
		float Radius = FMath::Pow(3 * Volume / (4 * 3.141592654), 1.0f / 3.0f);

		UE_LOG(LogVehicles, Warning, TEXT("Volume van wheel %d is %.2f, Radius is %.2f, (%.2f)"), i, Volume, Radius, Box.GetExtent().Z);

		WheelSetup.Radius = Radius;

		if (BodySetup->PhysicsType == PhysType_Default) { 	//if they set it to unfixed we don't fixup because they are explicitely saying Unfixed
			BodyInstance->SetInstanceSimulatePhysics(false);
		}

		//and get rid of constraints on the wheels. TODO: right now we remove all wheel constraints, we probably only want to remove parent constraints
		TArray<int32> WheelConstraints;
		PhysicsAsset->BodyFindConstraints(BodySetupIdx, WheelConstraints);
		for (int32 ConstraintIdx = 0; ConstraintIdx < WheelConstraints.Num(); ++ConstraintIdx){
			FConstraintInstance * ConstraintInstance = Mesh->Constraints[WheelConstraints[ConstraintIdx]];
			ConstraintInstance->TermConstraint();
		}

		Mesh->KinematicBonesUpdateType = EKinematicBonesUpdateToPhysics::SkipSimulatingBones;
	}
}

bool UT6VehicleComponentBase::CanCreateVehicle() const{
	if (UpdatedComponent == nullptr) {
		UE_LOG(LogVehicles, Warning, TEXT("Cannot create vehicle for %s. UpdatedComponent is not set."), *GetPathName());
		return false;
	}

	if (UpdatedPrimitive == nullptr) {
		UE_LOG(LogVehicles, Warning, TEXT("Cannot create vehicle for %s. UpdatedComponent is not a PrimitiveComponent."), *GetPathName());
		return false;
	}

	if (UpdatedPrimitive->GetBodyInstance() == nullptr) {
		UE_LOG(LogVehicles, Warning, TEXT("Cannot create vehicle for %s. UpdatedComponent has not initialized its rigid body actor."), *GetPathName());
		return false;
	}

	if (!UpdatedPrimitive->GetBodyInstance()->IsDynamic()){
		UE_LOG(LogVehicles, Warning, TEXT("Cannot create vehicle for %s. GetBodyInstance is not dynamic"), *GetPathName());
		return false;
	}

	if (WheelSetups.Num() == 0){
		UE_LOG(LogVehicles, Warning, TEXT("Cannot create vehicle for %s. We have no wheels"), *GetPathName());
		return false;
	}

	USkeletalMeshComponent * Mesh = Cast<USkeletalMeshComponent>(((UT6VehicleComponentBase*)this)->GetMesh());
	
	if (Mesh == nullptr){
		UE_LOG(LogVehicles, Warning, TEXT("Cannot create vehicle for %s. We have no mesh assigned"), *GetPathName());

		return false;
	}

	UPhysicsAsset* PhysicsAsset = Mesh->GetPhysicsAsset();

	if (PhysicsAsset == nullptr){
		UE_LOG(LogVehicles, Warning, TEXT("Cannot create vehicle for %s. We have no physics asset assigned"), *GetPathName());

		return false;
	}

	for (int32 i = 0; i < WheelSetups.Num(); i++){
		const FT6WheelSetup& WheelSetup = WheelSetups[i];

		if (WheelSetup.WheelClass == nullptr) {
			UE_LOG(LogVehicles, Warning, TEXT("Cannot create vehicle for %s. Wheel %d is not set."), *GetPathName(), i);
			return false;
		}
		else if (WheelSetup.BoneName == NAME_None){
			UE_LOG(LogVehicles, Warning, TEXT("Cannot create vehicle for %s. Wheel %d has no bone assigned."), *GetPathName(), i);
			return false;
		}

		int32 BodySetupIdx = PhysicsAsset->FindBodyIndex(WheelSetup.BoneName);

		if (BodySetupIdx < 0){
			UE_LOG(LogVehicles, Warning, TEXT("Cannot create vehicle for %s. Wheel %d bone cannot be found in physics asset."), *GetPathName(), i);
			return false;
		}

		UBodySetup* BodySetup = PhysicsAsset->BodySetup[BodySetupIdx];

		if (BodySetup == nullptr){
			UE_LOG(LogVehicles, Warning, TEXT("Cannot create vehicle for %s. Wheel %d bone has no bodysetup."), *GetPathName(), i);
			return false;
		}
	}

	return true;
}

bool UT6VehicleComponentBase::CreateVehicle(){
	// Init wheel objects
	Wheels.Reset();

	// Instantiate the wheels
	for (int32 i = 0; i < WheelSetups.Num(); i++) {
		UT6Wheel* Wheel = NewObject<UT6Wheel>(this, WheelSetups[i].WheelClass);
		check(Wheel);

		Wheels.Add(Wheel);
	}

	// Compute constants
	DragArea = ChassisWidth * ChassisHeight;

	// Setup vehicle
	SetupVehicleMass();

	// Setup wheels
	SetupWheelShapes();

	PxVehicleWheelsSimData* PWheelsSimData = PxVehicleWheelsSimData::allocate(WheelSetups.Num());
	SetupWheelPhysics(PWheelsSimData);

	// Create the vehicle
	PVehicle = PxVehicleNoDrive::allocate(WheelSetups.Num());
	check(PVehicle);

	ExecuteOnPxRigidDynamicReadWrite(UpdatedPrimitive->GetBodyInstance(), [&](PxRigidDynamic* PRigidDynamic){
		PVehicle->setup(GPhysXSDK, PRigidDynamic, *PWheelsSimData);
		PVehicle->setToRestState();

		// cleanup
		PWheelsSimData->free();
	});

	PWheelsSimData = NULL;

	// Init Wheels
	PVehicle->mWheelsDynData.setTireForceShaderFunction(T6TireShader);

	for (int32 i = 0; i < Wheels.Num(); i++) {
		PVehicle->mWheelsDynData.setTireForceShaderData(i, Wheels[i]);

		Wheels[i]->Init(this, i);
	}

	return true;
}

void UT6VehicleComponentBase::SetupVehicleMass(){
	ExecuteOnPxRigidDynamicReadWrite(UpdatedPrimitive->GetBodyInstance(), [&](PxRigidDynamic* PVehicleActor){
		// Override mass
		const float MassRatio = Mass / PVehicleActor->getMass();

		PxVec3 PInertiaTensor = PVehicleActor->getMassSpaceInertiaTensor();

		PInertiaTensor.x *= InertiaTensorScale.X * MassRatio;
		PInertiaTensor.y *= InertiaTensorScale.Y * MassRatio;
		PInertiaTensor.z *= InertiaTensorScale.Z * MassRatio;

		PVehicleActor->setMassSpaceInertiaTensor(PInertiaTensor);
		PVehicleActor->setMass(Mass);

		const PxVec3 PCOMOffset = U2PVector(GetLocalCOM());
		PVehicleActor->setCMassLocalPose(PxTransform(PCOMOffset, PxQuat::createIdentity()));
	});
}

void UT6VehicleComponentBase::SetupWheelShapes(){
	static PxMaterial* WheelMaterial = GPhysXSDK->createMaterial(0.0f, 0.0f, 0.0f);

	ExecuteOnPxRigidDynamicReadWrite(UpdatedPrimitive->GetBodyInstance(), [&](PxRigidDynamic* PVehicleActor){
		// Add wheel shapes to actor
		for (int32 i = 0; i < WheelSetups.Num(); i++){
			FT6WheelSetup& WheelSetup = WheelSetups[i];
			UT6Wheel* Wheel = WheelSetup.WheelClass.GetDefaultObject();
			check(Wheel);

			const FVector WheelOffset = GetWheelRestingPosition(WheelSetup);
			const PxTransform PLocalPose = PxTransform(U2PVector(WheelOffset));
			PxShape* PWheelShape = nullptr;

			// Prepare shape
			if (Wheel->CollisionMesh && Wheel->CollisionMesh->BodySetup){
				FBoxSphereBounds MeshBounds = Wheel->CollisionMesh->GetBounds();
				FVector MeshScaleV;

				MeshScaleV.X = WheelSetup.Radius / MeshBounds.BoxExtent.X;
				MeshScaleV.Y = Wheel->ShapeWidth / MeshBounds.BoxExtent.Y;
				MeshScaleV.Z = WheelSetup.Radius / MeshBounds.BoxExtent.Z;

				PxMeshScale MeshScale(U2PVector(UpdatedComponent->RelativeScale3D * MeshScaleV), PxQuat::createIdentity());
				/*if (Wheel->CollisionMesh->BodySetup->TriMesh){
					PxTriangleMesh* TriMesh = Wheel->CollisionMesh->BodySetup->TriMesh;

					// No eSIMULATION_SHAPE flag for wheels
					PWheelShape = PVehicleActor->createShape(PxTriangleMeshGeometry(TriMesh, MeshScale), *WheelMaterial, PxShapeFlag::eSCENE_QUERY_SHAPE | PxShapeFlag::eVISUALIZATION);
					PWheelShape->setLocalPose(PLocalPose);
				}*/
				if (Wheel->CollisionMesh->BodySetup->TriMeshes.Num()){
					PxTriangleMesh* TriMesh = Wheel->CollisionMesh->BodySetup->TriMeshes[0];

					// No eSIMULATION_SHAPE flag for wheels
					PWheelShape = PVehicleActor->createShape(PxTriangleMeshGeometry(TriMesh, MeshScale), *WheelMaterial, PxShapeFlag::eSCENE_QUERY_SHAPE | PxShapeFlag::eVISUALIZATION);
					PWheelShape->setLocalPose(PLocalPose);
				}
				else if (Wheel->CollisionMesh->BodySetup->AggGeom.ConvexElems.Num() == 1){
					PxConvexMesh* ConvexMesh = Wheel->CollisionMesh->BodySetup->AggGeom.ConvexElems[0].ConvexMesh;
					PWheelShape = PVehicleActor->createShape(PxConvexMeshGeometry(ConvexMesh, MeshScale), *WheelMaterial, PLocalPose);
				}

				check(PWheelShape);
			}
			else{
				PWheelShape = PVehicleActor->createShape(PxSphereGeometry(WheelSetup.Radius), *WheelMaterial, PLocalPose);
			}

			// Init filter data
			FCollisionResponseContainer CollisionResponse;
			CollisionResponse.SetAllChannels(ECR_Ignore);

			PxFilterData PWheelQueryFilterData, PDummySimData;
			CreateShapeFilterData(ECC_Vehicle, UpdatedComponent->GetUniqueID(), CollisionResponse, 0, 0, PWheelQueryFilterData, PDummySimData, false, false, false);

			//// Give suspension raycasts the same group ID as the chassis so that they don't hit each other
			PWheelShape->setQueryFilterData(PWheelQueryFilterData);
		}
	});
}

void UT6VehicleComponentBase::SetupWheelPhysics(PxVehicleWheelsSimData* PWheelsSimData) {
	ExecuteOnPxRigidDynamicReadWrite(UpdatedPrimitive->GetBodyInstance(), [&](PxRigidDynamic* PVehicleActor){
		const PxReal LengthScale = 100.f; // Convert default from m to cm

		// Control substepping
		PWheelsSimData->setSubStepCount(ThresholdLongitudinalSpeed * LengthScale, LowForwardSpeedSubStepCount, HighForwardSpeedSubStepCount);
		PWheelsSimData->setMinLongSlipDenominator(4.f * LengthScale);

		// Prealloc data for the sprung masses
		PxVec3 WheelOffsets[32];
		float SprungMasses[32];
		check(WheelSetups.Num() <= 32);

		// Calculate wheel offsets first, necessary for sprung masses
		for (int32 i = 0; i < WheelSetups.Num(); i++){
			WheelOffsets[i] = U2PVector(GetWheelRestingPosition(WheelSetups[i]));
		}

		// Now that we have all the wheel offsets, calculate the sprung masses
		PxVec3 PLocalCOM = U2PVector(GetLocalCOM());
		PxVehicleComputeSprungMasses(WheelSetups.Num(), WheelOffsets, PLocalCOM, PVehicleActor->getMass(), 2, SprungMasses);

		for (int32 i = 0; i < WheelSetups.Num(); i++){
			FT6WheelSetup& WheelSetup = WheelSetups[i];

			UT6Wheel* Wheel = WheelSetup.WheelClass.GetDefaultObject();

			// init wheel data
			PxVehicleWheelData PWheelData;
			PWheelData.mRadius = WheelSetup.Radius;
			PWheelData.mWidth =  Wheel->ShapeWidth;
			PWheelData.mMass = WheelSetup.Mass;
			PWheelData.mMOI = 0.5f * PWheelData.mMass * FMath::Square(PWheelData.mRadius);	// Klopt dit wel? Moet in meters en niet in cm
			PWheelData.mDampingRate = WheelSetup.DampingRate;

			// Worden deze 3 gebruikt?
			PWheelData.mMaxBrakeTorque = 0;// M2ToCm2(WheelSetup.MaxBrakeTorque);
			PWheelData.mMaxHandBrakeTorque = 0;// WheelSetup.bAffectedByHandbrake ? M2ToCm2(WheelSetup.MaxHandBrakeTorque) : 0.0f;
			PWheelData.mMaxSteer = 0;// FMath::DegreesToRadians(WheelSetup.SteerAngle);

			UTireType* TireType = NewObject<UTireType>(this, UTireType::StaticClass());
			TireType->SetFrictionScale(WheelSetup.Friction);
			TireTypes.Add(TireType);

			// init tire data
			PxVehicleTireData PTireData;
			PTireData.mType = TireType->GetTireTypeID();// WheelSetup.TireType ? WheelSetup.TireType->GetTireTypeID() : GEngine->DefaultTireType->GetTireTypeID();
			//PTireData.mCamberStiffnessPerUnitGravity = 0.0f;	// Uitzoeken
			//PTireData.mFrictionVsSlipGraph	// Uitzoeken
			PTireData.mLatStiffX = WheelSetup.LatStiffMaxLoad;
			PTireData.mLatStiffY = WheelSetup.LatStiffValue;
			PTireData.mLongitudinalStiffnessPerUnitGravity = WheelSetup.LongStiffValue;

			// init suspension data
			PxVehicleSuspensionData PSuspensionData;
			PSuspensionData.mSprungMass = SprungMasses[i];
			PSuspensionData.mSpringStrength = WheelSetup.SuspensionNaturalFrequency * PSuspensionData.mSprungMass;// FMath::Square(WheelSetup.SuspensionNaturalFrequency) * PSuspensionData.mSprungMass;
			PSuspensionData.mSpringDamperRate = WheelSetup.SuspensionDampingRatio * PSuspensionData.mSprungMass;// *2.0f * FMath::Sqrt(PSuspensionData.mSpringStrength * PSuspensionData.mSprungMass);
			PSuspensionData.mMaxCompression = WheelSetup.SuspensionMaxRaise;
			PSuspensionData.mMaxDroop = WheelSetup.SuspensionMaxDrop;
			//PSuspensionData.mCamberAtRest;	// Uitzoeken
			//PSuspensionData.mCamberAtMaxCompression
			//PSuspensionData.mCamberAtMaxDroop

			// init offsets
			const PxVec3 PWheelOffset = WheelOffsets[i];

			PxVec3 PSuspTravelDirection = U2PVector(WheelSetup.SuspensionAxis);// PxVec3(0.0f, 0.0f, -1.0f);
			PxVec3 PWheelCentreCMOffset = PWheelOffset - PLocalCOM;
			PxVec3 PSuspForceAppCMOffset = PxVec3(PWheelCentreCMOffset.x, PWheelCentreCMOffset.y, WheelSetup.SuspensionForceOffset);
			PxVec3 PTireForceAppCMOffset = PSuspForceAppCMOffset;

			// finalize sim data
			PWheelsSimData->setWheelData(i, PWheelData);
			PWheelsSimData->setTireData(i, PTireData);
			PWheelsSimData->setSuspensionData(i, PSuspensionData);
			PWheelsSimData->setSuspTravelDirection(i, PSuspTravelDirection);
			PWheelsSimData->setWheelCentreOffset(i, PWheelCentreCMOffset);
			PWheelsSimData->setSuspForceAppPointOffset(i, PSuspForceAppCMOffset);
			PWheelsSimData->setTireForceAppPointOffset(i, PTireForceAppCMOffset);
		}

		const int32 NumShapes = PVehicleActor->getNbShapes();
		const int32 NumChassisShapes = NumShapes - WheelSetups.Num();
		check(NumChassisShapes >= 1);

		TArray<PxShape*> Shapes;
		Shapes.AddZeroed(NumShapes);

		PVehicleActor->getShapes(Shapes.GetData(), NumShapes);

		for (int32 i = 0; i < WheelSetups.Num(); i++){
			const int32 WheelShapeIndex = NumChassisShapes + i;

			PWheelsSimData->setWheelShapeMapping(i, WheelShapeIndex);
			PWheelsSimData->setSceneQueryFilterData(i, Shapes[WheelShapeIndex]->getQueryFilterData());
		}

		// tire load filtering
		PxVehicleTireLoadFilterData PTireLoadFilter;
		PTireLoadFilter.mMinNormalisedLoad = MinNormalizedTireLoad;
		PTireLoadFilter.mMinFilteredNormalisedLoad = MinNormalizedTireLoadFiltered;
		PTireLoadFilter.mMaxNormalisedLoad = MaxNormalizedTireLoad;
		PTireLoadFilter.mMaxFilteredNormalisedLoad = MaxNormalizedTireLoadFiltered;
		PWheelsSimData->setTireLoadFilterData(PTireLoadFilter);
	});
}

FVector UT6VehicleComponentBase::GetWheelRestingPosition(const FT6WheelSetup& WheelSetup){
	check(WheelSetup.BoneName != NAME_None);

	USkinnedMeshComponent* Mesh = GetMesh();

	check(Mesh && Mesh->SkeletalMesh);
	
	FVector Offset = WheelSetup.AdditionalOffset;

	
	const FVector BonePosition = Mesh->SkeletalMesh->GetComposedRefPoseMatrix(WheelSetup.BoneName).GetOrigin() * Mesh->RelativeScale3D;

	//BonePosition is local for the root BONE of the skeletal mesh - however, we are using the Root BODY which may have its own transform, so we need to return the position local to the root BODY
	const FMatrix RootBodyMTX = Mesh->SkeletalMesh->GetComposedRefPoseMatrix(Mesh->GetBodyInstance()->BodySetup->BoneName);
	const FVector LocalBonePosition = RootBodyMTX.InverseTransformPosition(BonePosition);
	Offset += LocalBonePosition;

	UE_LOG(LogVehicles, Warning, TEXT("Wheel %s resting point = %s"), *WheelSetup.BoneName.ToString(), *Offset.ToString());

	if (WheelSetups.Num() == 2){
		Offset.Y = 0;	// Lelijke hack om sloppyness van artists te fixen
	}

	return Offset;
}

FVector UT6VehicleComponentBase::GetLocalCOM() const{
	check(UpdatedPrimitive);

	FVector LocalCOM = FVector::ZeroVector;

	const FBodyInstance* BodyInst = UpdatedPrimitive->GetBodyInstance();

	if (BodyInst != nullptr){
		ExecuteOnPxRigidDynamicReadOnly(BodyInst, [&](const PxRigidDynamic* PVehicleActor){
			PxTransform PCOMTransform = PVehicleActor->getCMassLocalPose();
			LocalCOM = P2UVector(PCOMTransform.p);
		});
	}

	UE_LOG(LogVehicles, Warning, TEXT("COM = %s"), *LocalCOM.ToString());

	LocalCOM += COMOffset;
	LocalCOM.Y = 0;	// Lelijke hack om sloppyness van artists te fixen

	return LocalCOM;
}

USkinnedMeshComponent* UT6VehicleComponentBase::GetMesh(){
	return Cast<USkinnedMeshComponent>(UpdatedComponent);
}

/*
void T6LogVehicleSettings( PxVehicleWheels* Vehicle ){
	const float VehicleMass = Vehicle->getRigidDynamicActor()->getMass();
	const FVector VehicleMOI = P2UVector( Vehicle->getRigidDynamicActor()->getMassSpaceInertiaTensor() );

	UE_LOG( LogPhysics, Warning, TEXT("Vehicle Mass: %f"), VehicleMass );
	UE_LOG( LogPhysics, Warning, TEXT("Vehicle MOI: %s"), *VehicleMOI.ToString() );

	const PxVehicleWheelsSimData& SimData = Vehicle->mWheelsSimData;
	for ( int32 WheelIdx = 0; WheelIdx < 4; ++WheelIdx ){
		const  PxVec3& suspTravelDir = SimData.getSuspTravelDirection(WheelIdx);
		const PxVec3& suspAppPointOffset = SimData.getSuspForceAppPointOffset(WheelIdx);
		const PxVec3& tireForceAppPointOffset = SimData.getTireForceAppPointOffset(WheelIdx);
		const PxVec3& wheelCenterOffset = SimData.getWheelCentreOffset(WheelIdx);			
		const PxVehicleSuspensionData& SuspensionData = SimData.getSuspensionData( WheelIdx );
		const PxVehicleWheelData& WheelData = SimData.getWheelData( WheelIdx );
		const PxVehicleTireData& TireData = SimData.getTireData( WheelIdx );

		UE_LOG( LogPhysics, Warning, TEXT("Wheel %d suspension: travelDir ={%f, %f, %f} "), WheelIdx, suspTravelDir.x, suspTravelDir.y, suspTravelDir.z );
		UE_LOG( LogPhysics, Warning, TEXT("Wheel %d suspension: suspAppPointOffset ={%f, %f, %f} "), WheelIdx, suspAppPointOffset.x, suspAppPointOffset.y, suspAppPointOffset.z );
		UE_LOG( LogPhysics, Warning, TEXT("Wheel %d suspension: tireForceAppPointOffset ={%f, %f, %f} "), WheelIdx, tireForceAppPointOffset.x, tireForceAppPointOffset.y, tireForceAppPointOffset.z );
		UE_LOG( LogPhysics, Warning, TEXT("Wheel %d suspension: wheelCenterOffset ={%f, %f, %f} "), WheelIdx, wheelCenterOffset.x, wheelCenterOffset.y, wheelCenterOffset.z );
		UE_LOG( LogPhysics, Warning, TEXT("Wheel %d suspension: MaxCompress=%f, MaxDroop=%f, Damper=%f, Strength=%f, SprungMass=%f"),
			WheelIdx, SuspensionData.mMaxCompression, SuspensionData.mMaxDroop, SuspensionData.mSpringDamperRate, SuspensionData.mSpringStrength, SuspensionData.mSprungMass );

		UE_LOG( LogPhysics, Warning, TEXT("Wheel %d wheel: Damping=%f, Mass=%f, MOI=%f, Radius=%f"),
			WheelIdx, WheelData.mDampingRate, WheelData.mMass, WheelData.mMOI, WheelData.mRadius );

		UE_LOG( LogPhysics, Warning, TEXT("Wheel %d tire: LatStiffX=%f, LatStiffY=%f, LongStiff=%f"),
			WheelIdx, TireData.mLatStiffX, TireData.mLatStiffY, TireData.mLongitudinalStiffnessPerUnitGravity );
	}
}*/

bool UT6VehicleComponentBase::ShouldCreatePhysicsState() const {
	// only create physx vehicle in game
	if (GetWorld()->IsGameWorld()){
		FPhysScene* PhysScene = World->GetPhysicsScene();

		if (PhysScene && PhysScene->GetVehicleManager()){
			if (CanCreateVehicle()){
				return true;
			}
		}
	}

	return false;
}

void UT6VehicleComponentBase::CreatePhysicsState(){
	Super::CreatePhysicsState();

	VehicleSetupTag = FPhysXVehicleManager::VehicleSetupTag;

	FixupSkeletalMesh();

	if (CreateVehicle()){
		FPhysScene* PhysScene = World->GetPhysicsScene();

		FPhysXVehicleManager* VehicleManager = PhysScene->GetVehicleManager();
		VehicleManager->AddVehicle(this);

		//T6LogVehicleSettings( PVehicle );
		SCOPED_SCENE_WRITE_LOCK(VehicleManager->GetScene());
		PVehicle->getRigidDynamicActor()->wakeUp();
	}
}

void UT6VehicleComponentBase::DestroyPhysicsState(){
	Super::DestroyPhysicsState();

	if (PVehicle){
		for (int32 i = 0; i < Wheels.Num(); i++) {
			Wheels[i]->Shutdown();
		}

		Wheels.Reset();

		World->GetPhysicsScene()->GetVehicleManager()->RemoveVehicle(this);
		PVehicle = nullptr;

		if (UpdatedComponent){
			UpdatedComponent->RecreatePhysicsState();
		}
	}
}

bool UT6VehicleComponentBase::HasValidPhysicsState() const{
	return PVehicle != nullptr;
}

void UT6VehicleComponentBase::TickVehicle(float DeltaTime){
	// movement updates and replication
	if (PVehicle && UpdatedComponent){
		FBodyInstance* BodyInstance = UpdatedPrimitive->GetBodyInstance();

		//UpdatedPrimitive->ComponentAngularVelocity = UpdatedComponent->ComponentAngularVelocity = BodyInstance->GetAngularVelocity();	// Onduidelijk welke van de 2 ik moet hebben

		APawn* MyOwner = Cast<APawn>(UpdatedComponent->GetOwner());

		if (MyOwner){
			UpdateSimulation(DeltaTime);
		}
	}

	// update wheels
	for (int32 i = 0; i < Wheels.Num(); i++){
		Wheels[i]->Tick(DeltaTime);
	}

	UpdateDrag(DeltaTime);
}

bool UT6VehicleComponentBase::IsInAir() const {
	FPhysXVehicleManager* MyVehicleManager = World->GetPhysicsScene()->GetVehicleManager();

	if (MyVehicleManager == nullptr){
		return false;
	}

	const PxVehicleWheelQueryResult* Result = MyVehicleManager->GetVehicleState_AssumesLocked(this);

	if (Result == nullptr){
		return false;
	}

	if (!Result->wheelQueryResults) {
		return true;
	}

	for (PxU32 i = 0; i < Result->nbWheelQueryResults; i++){
		if (Result->wheelQueryResults[i].isInAir){
			return true;
		}
	}
	return false;
}

void UT6VehicleComponentBase::UpdateDrag(float DeltaTime){
	if (PVehicle && UpdatedPrimitive){
		float ForwardSpeed = GetForwardSpeed();

		if (FMath::Abs(ForwardSpeed) > 1.f){
			FVector GlobalForwardVector = UpdatedComponent->GetForwardVector();
			
			float SpeedSquared = ForwardSpeed * ForwardSpeed;
			float ChassisDragArea = ChassisHeight * ChassisWidth;
			float AirDensity = 1.25 / (100 * 100 * 100); //kg/cm^3
			float DragMag = 0.5f * AirDensity * SpeedSquared * DragCoefficient * ChassisDragArea;

			FVector DragVector = GlobalForwardVector * -DragMag;

			FBodyInstance* BodyInstance = UpdatedPrimitive->GetBodyInstance();
			BodyInstance->AddForce(DragVector, false);

			if (true){//!IsInAir()){
				FVector LocalVelocity = (BodyInstance->GetUnrealWorldTransform().GetRotation().Inverse()) * BodyInstance->GetUnrealWorldVelocity();

				float SideDragMag = 0.5f * AirDensity * SpeedSquared * SidewayDrag;
				BodyInstance->AddForce(BodyInstance->GetUnrealWorldTransform().GetRotation().GetAxisY() * (LocalVelocity.Y * -SideDragMag), false);
			}

			DebugDragMagnitude = DragMag;
		}
	}
}

void UT6VehicleComponentBase::PreTick(float DeltaTime){
	// movement updates and replication
	if (PVehicle && UpdatedComponent){
		APawn* MyOwner = Cast<APawn>(UpdatedComponent->GetOwner());

		if (MyOwner){
			UpdateState(DeltaTime);
		}
	}

	if (VehicleSetupTag != FPhysXVehicleManager::VehicleSetupTag){
		RecreatePhysicsState();
	}
}

void UT6VehicleComponentBase::UpdateSimulation(float DeltaTime){	// Deze implementatie vervangemn
	//
}

void UT6VehicleComponentBase::UpdateState(float DeltaTime) {
	//
}

float UT6VehicleComponentBase::GetForwardSpeed() const{
	float ForwardSpeed = 0.f;

	if (PVehicle){
		UpdatedPrimitive->GetBodyInstance()->ExecuteOnPhysicsReadOnly([&]{
			ForwardSpeed = PVehicle->computeForwardSpeed();
		});
	}

	return ForwardSpeed;
}

float UT6VehicleComponentBase::GetWheelRotationSpeed() const{
	if (PVehicle && WheelSetups.Num()){
		float TotalWheelSpeed = 0.0f;

		for (int32 i = 0; i < WheelSetups.Num(); i++){
			const PxReal WheelSpeed = PVehicle->mWheelsDynData.getWheelRotationSpeed(i);
			TotalWheelSpeed += WheelSpeed;
		}

		const float CurrentRPM = TotalWheelSpeed / WheelSetups.Num();
		return CurrentRPM;
	}

	return 0.0f;
}

/*
bool UT6VehicleComponentBase::CheckSlipThreshold(float AbsLongSlipThreshold, float AbsLatSlipThreshold) const{
	if (PVehicle == NULL){
		return false;
	}

	FPhysXVehicleManager* MyVehicleManager = World->GetPhysicsScene()->GetVehicleManager();
	SCOPED_SCENE_READ_LOCK(MyVehicleManager->GetScene());

	PxWheelQueryResult * WheelsStates = MyVehicleManager->GetWheelsStates_AssumesLocked(this);
	check(WheelsStates);

	PxReal MaxLongSlip = 0.f;
	PxReal MaxLatSlip = 0.f;

	// draw wheel data
	for (uint32 w = 0; w < PVehicle->mWheelsSimData.getNbWheels(); ++w){
		const PxReal AbsLongSlip = FMath::Abs(WheelsStates[w].longitudinalSlip);
		const PxReal AbsLatSlip = FMath::Abs(WheelsStates[w].lateralSlip);

		if (AbsLongSlip > AbsLongSlipThreshold){
			return true;
		}

		if (AbsLatSlip > AbsLatSlipThreshold){
			return true;
		}
	}

	return false;
}*/
/*
float UT6VehicleComponentBase::GetMaxSpringForce() const{
	if (PVehicle == NULL){
		return false;
	}

	FPhysXVehicleManager* MyVehicleManager = World->GetPhysicsScene()->GetVehicleManager();
	SCOPED_SCENE_READ_LOCK(MyVehicleManager->GetScene());

	PxWheelQueryResult * WheelsStates = MyVehicleManager->GetWheelsStates_AssumesLocked(this);
	check(WheelsStates);

	PxReal MaxSpringCompression = 0.f;

	// draw wheel data
	for (uint32 w = 0; w < PVehicle->mWheelsSimData.getNbWheels(); ++w){
		MaxSpringCompression = WheelsStates[w].suspSpringForce > MaxSpringCompression ? WheelsStates[w].suspSpringForce : MaxSpringCompression;
	}

	return MaxSpringCompression;
}*/

#if WITH_EDITOR

void UT6VehicleComponentBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent ){
	Super::PostEditChangeProperty( PropertyChangedEvent );

	// Trigger a runtime rebuild of the PhysX vehicle
	FPhysXVehicleManager::VehicleSetupTag++;

	const FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == TEXT("SteeringCurve")) {
		//make sure values are capped between 0 and 1
		TArray<FRichCurveKey> SteerKeys = SteeringCurve.GetRichCurve()->GetCopyOfKeys();
		for (int32 KeyIdx = 0; KeyIdx < SteerKeys.Num(); ++KeyIdx) {
			float NewValue = FMath::Clamp(SteerKeys[KeyIdx].Value, 0.f, 1.f);
			SteeringCurve.GetRichCurve()->UpdateOrAddKey(SteerKeys[KeyIdx].Time, NewValue);
		}
	}
}

#endif // WITH_EDITOR
/*
void UT6VehicleComponentBase::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME( UT6VehicleComponentBase, ReplicatedState );
}*/

#undef LOCTEXT_NAMESPACE