// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#include "EnginePrivate.h"
#include "PhysicsPublic.h"
#include "Net/UnrealNetwork.h"
#include "MessageLog.h"
#include "Vehicles/T6Wheel.h"
#include "Vehicles/T6VehicleComponent.h"
#include "Vehicles/TireType.h"

#include "../PhysicsEngine/PhysXSupport.h"
#include "../Collision/PhysXCollision.h"
#include "PhysXVehicleManager.h"

#include "AI/Navigation/AvoidanceManager.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

#define LOCTEXT_NAMESPACE "UT6VehicleComponent"

UT6VehicleComponent::UT6VehicleComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer){
	ThrottleTorque = 100000000.0f;
	BrakeTorque = 50000000.0f;
	HandbrakeTorque = 50000000.0f;

	bUseRVOAvoidance = false;
	AvoidanceVelocity = FVector::ZeroVector;
	AvoidanceLockVelocity = FVector::ZeroVector;
	AvoidanceLockTimer = 0.0f;
	AvoidanceGroup.bGroup0 = true;
	GroupsToAvoid.Packed = 0xFFFFFFFF;
	GroupsToIgnore.Packed = 0;
	RVOAvoidanceRadius = 400.0f;
	RVOAvoidanceHeight = 200.0f;
	AvoidanceConsiderationRadius = 2000.0f;
	RVOSteeringStep = 0.5f;
	RVOThrottleStep = 0.25f;

	Gear = 1;
}

bool UT6VehicleComponent::CreateVehicle(){
	if (Super::CreateVehicle()) {
		if (bUseRVOAvoidance){
			UAvoidanceManager* AvoidanceManager = GetWorld()->GetAvoidanceManager();

			if (AvoidanceManager){
				AvoidanceManager->RegisterMovementComponent(this, AvoidanceWeight);
			}
		}

		return true;
	}
	else return false;
}

float UT6VehicleComponent::GetSteering(){
	const float Speed = FMath::Abs(GetForwardSpeed()) * 3600.f / 100000.f;

	float SteeringMult = SteeringCurve.GetRichCurve()->Eval(Speed);
	if (Gear == -1) {
		SteeringMult *= 0.5f;
	}

	return InputState.GetSteering() * SteeringMult;
}

float UT6VehicleComponent::GetAccel(){
	const float Speed = FMath::Abs(GetForwardSpeed()) * 3600.f / 100000.f;

	
	if (Gear > 0){
		return TorqueCurve.GetRichCurve()->Eval(Speed);
	}
	else{
		return TorqueCurve.GetRichCurve()->Eval(Speed * 15);
	}
}

const float StandstillTreshold = 5.0f;
const float StandstillTime = 0.1f;

void UT6VehicleComponent::UpdateSimulation(float DeltaTime){
	UpdatedPrimitive->GetBodyInstance()->ExecuteOnPhysicsReadWrite([&]{
		const float Speed = GetForwardSpeed();

		float Throttle = InputState.GetThrottle();
		float Steering = GetSteering();
		float HandBrake = InputState.GetHandbrake();

		float Accel, Brake;

		if (FMath::Abs(Speed) < StandstillTreshold){
			SlowTimer += DeltaTime;

			if (SlowTimer > StandstillTime){
				if (Throttle < 0){
					Gear = -1;
				}
				else if (Throttle > 0){
					Gear = 1;
				}
				else Gear = 0;
			}
		}
		else{
			SlowTimer = 0;
		}
		
		if (Throttle * Gear > 0){
			Accel = GetAccel() * Throttle;
			Brake = 0;
		}
		else{
			Accel = 0;
			Brake = FMath::Abs(Throttle);
		}

		//

		float Handbrake = InputState.GetHandbrake();

		for (int i = 0; i < WheelSetups.Num(); i++){
			const FT6WheelSetup& WheelSetup = WheelSetups[i];

			float WheelBrake = Brake;
			if (i == 1){
				//WheelBrake += FMath::Clamp(1.0f - (FMath::Abs(Accel) + FMath::Abs(Steering)), 0.0f, 1.0f) * 0.05f;	// Automatisch wat remmen
			}

			float AccelTorque = Accel * this->ThrottleTorque * WheelSetup.ThrottleMult;
			float BrakeTorque = FMath::Max(WheelBrake * this->BrakeTorque * WheelSetup.BrakeMult, Handbrake * this->HandbrakeTorque * WheelSetup.HandbrakeMult);
			float SteeringAngle = Steering * FMath::DegreesToRadians(WheelSetup.SteerAngle);


			//UE_LOG(LogVehicles, Warning, TEXT("Applying input to wheel %d: %.2f, %.2f, %.2f, Gear:%.0f"), i, AccelTorque, BrakeTorque, SteeringAngle, Gear);

			PVehicle->setDriveTorque(i, AccelTorque);
			PVehicle->setBrakeTorque(i, BrakeTorque);
			PVehicle->setSteerAngle(i, SteeringAngle);

			//PVehicle->mWheelsSimData.setlo

			/*FBodyInstance* BodyInstance = UpdatedPrimitive->GetBodyInstance();
			BodyInstance->AddForce(UpdatedComponent->GetForwardVector() * (AccelTorque - BrakeTorque));
			BodyInstance->AddTorque(FVector::UpVector * SteeringAngle * 1000000);

			FVector LocalVelocity = (BodyInstance->GetUnrealWorldTransform().GetRotation().Inverse()) * BodyInstance->GetUnrealWorldVelocity();
			BodyInstance->AddForce(BodyInstance->GetUnrealWorldTransform().GetRotation().GetAxisY() * (LocalVelocity.Y * -10000));
				
			BodyInstance->WakeInstance();*/
		}
	});
}

void UT6VehicleComponent::UpdateAvoidance(float DeltaTime){
	if (AvoidanceLockTimer > 0.0f){
		AvoidanceLockTimer -= DeltaTime;
	}

	UpdateDefaultAvoidance();
}

void UT6VehicleComponent::UpdateDefaultAvoidance(){
	if (!bUseRVOAvoidance){
		return;
	}

	SCOPE_CYCLE_COUNTER(STAT_AI_ObstacleAvoidance);

	UAvoidanceManager* AvoidanceManager = GetWorld()->GetAvoidanceManager();
	if (AvoidanceManager && !bWasAvoidanceUpdated){
		AvoidanceManager->UpdateRVO(this);
		
		//Consider this a clean move because we didn't even try to avoid.
		SetAvoidanceVelocityLock(AvoidanceManager, AvoidanceManager->LockTimeAfterClean);
	}

	bWasAvoidanceUpdated = false;		//Reset for next frame
}

void UT6VehicleComponent::SetAvoidanceVelocityLock(class UAvoidanceManager* Avoidance, float Duration){
	Avoidance->OverrideToMaxWeight(AvoidanceUID, Duration);
	AvoidanceLockVelocity = AvoidanceVelocity;
	AvoidanceLockTimer = Duration;
}

void UT6VehicleComponent::UpdateState(float DeltaTime){
	// update input values
	APawn* MyOwner = UpdatedComponent ? Cast<APawn>(UpdatedComponent->GetOwner()) : NULL;

	if (MyOwner && MyOwner->IsLocallyControlled()){
		if (bUseRVOAvoidance){
			CalculateAvoidanceVelocity(DeltaTime);
			UpdateAvoidance(DeltaTime);
		}

		// and send to server
		ServerUpdateState(InputState);
	}
	else{
		InputState = ReplicatedInputState;
	}
}

bool UT6VehicleComponent::ServerUpdateState_Validate(const FT6VehicleInputState& InputState){
	return true;
}

void UT6VehicleComponent::ServerUpdateState_Implementation(const FT6VehicleInputState& InputState){
	this->ReplicatedInputState  = InputState;	// Hier misschien nog iets doen zodat de server niet de clientside settings overschrijft?
}

void UT6VehicleComponent::ClearInput(){
	InputState = FT6VehicleInputState();
}

void UT6VehicleComponent::SetThrottleInput(float Throttle){	
	InputState.SetThrottle(FMath::Clamp(Throttle, -1.0f, 1.0f));
}

void UT6VehicleComponent::SetSteeringInput(float Steering){
	InputState.SetSteering(FMath::Clamp(Steering, -1.0f, 1.0f));
}

void UT6VehicleComponent::SetHandbrakeInput(bool bNewHandbrake){
	InputState.Handbrake = bNewHandbrake ? 255 : 0;
}

void UT6VehicleComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UT6VehicleComponent, ReplicatedInputState);
}

static bool XYEqual(const FVector& v1, const FVector& v2, float Tolerance = KINDA_SMALL_NUMBER){
	return FMath::Abs(v1.X - v2.X) <= Tolerance && FMath::Abs(v1.Y - v2.Y) <= Tolerance;;
}

void UT6VehicleComponent::CalculateAvoidanceVelocity(float DeltaTime){
	if (!bUseRVOAvoidance){
		return;
	}
	
	SCOPE_CYCLE_COUNTER(STAT_AI_ObstacleAvoidance);
	
	UAvoidanceManager* AvoidanceManager = GetWorld()->GetAvoidanceManager();
	APawn* MyOwner = UpdatedComponent ? Cast<APawn>(UpdatedComponent->GetOwner()) : NULL;
	
	if (AvoidanceWeight >= 1.0f || AvoidanceManager == NULL || MyOwner == NULL){
		return;
	}
	
	if (MyOwner->Role != ROLE_Authority){	
		return;
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	const bool bShowDebug = AvoidanceManager->IsDebugEnabled(AvoidanceUID);
#endif

	// since we don't assign the avoidance velocity but instead use it to adjust steering and throttle,
	// always reset the avoidance velocity to the current velocity
	AvoidanceVelocity = GetVelocityForRVOConsideration();

	if (!AvoidanceVelocity.IsZero()){
		//See if we're doing a locked avoidance move already, and if so, skip the testing and just do the move.
		if (AvoidanceLockTimer > 0.0f){
			AvoidanceVelocity = AvoidanceLockVelocity;

			PostProcessAvoidance.ExecuteIfBound(this, AvoidanceVelocity);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			if (bShowDebug){
				DrawDebugLine(GetWorld(), GetRVOAvoidanceOrigin(), GetRVOAvoidanceOrigin() + AvoidanceVelocity, FColor::Blue, true, 0.5f, SDPG_MAX);
			}
#endif
		}
		else{
			FVector NewVelocity = AvoidanceManager->GetAvoidanceVelocityForComponent(this);

			if (!XYEqual(NewVelocity, AvoidanceVelocity)){//NewVelocity.Equals(AvoidanceVelocity)){		//Really want to branch hint that this will probably not pass
				//Had to divert course, lock this avoidance move in for a short time. This will make us a VO, so unlocked others will know to avoid us.
				AvoidanceVelocity = NewVelocity;
				SetAvoidanceVelocityLock(AvoidanceManager, AvoidanceManager->LockTimeAfterAvoid);

				PostProcessAvoidance.ExecuteIfBound(this, AvoidanceVelocity);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
				if (bShowDebug) {
					DrawDebugLine(GetWorld(), GetRVOAvoidanceOrigin(), GetRVOAvoidanceOrigin() + AvoidanceVelocity, FColor::Red, true, 20.0f, SDPG_MAX, 10.0f);
				}
#endif
			}
			else{
				//Although we didn't divert course, our velocity for this frame is decided. We will not reciprocate anything further, so treat as a VO for the remainder of this frame.
				SetAvoidanceVelocityLock(AvoidanceManager, AvoidanceManager->LockTimeAfterClean);	//10 ms of lock time should be adequate.
			}
		}

		AvoidanceManager->UpdateRVO(this);
		bWasAvoidanceUpdated = true;
	}
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	else if (bShowDebug){
		DrawDebugLine(GetWorld(), GetRVOAvoidanceOrigin(), GetRVOAvoidanceOrigin() + GetVelocityForRVOConsideration(), FColor::Yellow, true, 0.05f, SDPG_MAX);
	}

	if (bShowDebug){
		FVector UpLine(0, 0, 500);
		DrawDebugLine(GetWorld(), GetRVOAvoidanceOrigin(), GetRVOAvoidanceOrigin() + UpLine, (AvoidanceLockTimer > 0.01f) ? FColor::Red : FColor::Blue, true, 0.05f, SDPG_MAX, 5.0f);
	}
#endif
}

void UT6VehicleComponent::SetAvoidanceGroup(int32 GroupFlags){
	AvoidanceGroup.SetFlagsDirectly(GroupFlags);
}

void UT6VehicleComponent::SetGroupsToAvoid(int32 GroupFlags){
	GroupsToAvoid.SetFlagsDirectly(GroupFlags);
}

void UT6VehicleComponent::SetGroupsToIgnore(int32 GroupFlags){
	GroupsToIgnore.SetFlagsDirectly(GroupFlags);
}

void UT6VehicleComponent::SetAvoidanceEnabled(bool bEnable){
	if (bUseRVOAvoidance != bEnable){
		bUseRVOAvoidance = bEnable;
		
		UAvoidanceManager* AvoidanceManager = GetWorld()->GetAvoidanceManager();
		if (AvoidanceManager && bEnable && AvoidanceUID == 0){
			AvoidanceManager->RegisterMovementComponent(this, AvoidanceWeight);
		}
	}
}

void UT6VehicleComponent::SetRVOAvoidanceUID(int32 UID){
	AvoidanceUID = UID;
}

int32 UT6VehicleComponent::GetRVOAvoidanceUID(){
	return AvoidanceUID;
}

void UT6VehicleComponent::SetRVOAvoidanceWeight(float Weight){
	AvoidanceWeight = Weight;
}

float UT6VehicleComponent::GetRVOAvoidanceWeight(){
	return AvoidanceWeight;
}

FVector UT6VehicleComponent::GetRVOAvoidanceOrigin(){
	return UpdatedComponent->GetComponentLocation();
}

float UT6VehicleComponent::GetRVOAvoidanceRadius(){
	return RVOAvoidanceRadius;
}

float UT6VehicleComponent::GetRVOAvoidanceHeight(){
	return RVOAvoidanceHeight;
}

float UT6VehicleComponent::GetRVOAvoidanceConsiderationRadius(){
	return AvoidanceConsiderationRadius;
}

FVector UT6VehicleComponent::GetVelocityForRVOConsideration(){
	return UpdatedComponent->GetComponentVelocity();
}

int32 UT6VehicleComponent::GetAvoidanceGroupMask(){
	return AvoidanceGroup.Packed;
}

int32 UT6VehicleComponent::GetGroupsToAvoidMask(){
	return GroupsToAvoid.Packed;
}

int32 UT6VehicleComponent::GetGroupsToIgnoreMask(){
	return GroupsToIgnore.Packed;
}


static void T6DrawTelemetryGraph( uint32 Channel, const PxVehicleGraph& PGraph, UCanvas* Canvas, float GraphX, float GraphY, float GraphWidth, float GraphHeight, float& OutX ){
	PxF32 PGraphXY[2*PxVehicleGraph::eMAX_NB_SAMPLES];
	PxVec3 PGraphColor[PxVehicleGraph::eMAX_NB_SAMPLES];
	char PGraphTitle[PxVehicleGraph::eMAX_NB_TITLE_CHARS];

	PGraph.computeGraphChannel( Channel, PGraphXY, PGraphColor, PGraphTitle );

	FString Label = ANSI_TO_TCHAR(PGraphTitle);
	Canvas->SetDrawColor( FColor( 255, 255, 0 ) );
	UFont* Font = GEngine->GetSmallFont();
	Canvas->DrawText( Font, Label, GraphX, GraphY );

	float XL, YL;
	Canvas->TextSize( Font, Label, XL, YL );

	float LineGraphHeight = GraphHeight - YL - 4.0f;
	float LineGraphY = GraphY + YL + 4.0f;

	FCanvasTileItem TileItem( FVector2D(GraphX, LineGraphY), GWhiteTexture, FVector2D( GraphWidth, GraphWidth ), FLinearColor( 0.0f, 0.125f, 0.0f, 0.25f ) );
	TileItem.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem( TileItem );

	Canvas->SetDrawColor( FColor( 0, 32, 0, 128 ) );
	for ( uint32 i = 2; i < 2 * PxVehicleGraph::eMAX_NB_SAMPLES; i += 2 ){
		float x1 = PGraphXY[i-2];
		float y1 = PGraphXY[i-1];
		float x2 = PGraphXY[i];
		float y2 = PGraphXY[i+1];

		x1 = FMath::Clamp( x1 + 0.50f, 0.0f, 1.0f );
		x2 = FMath::Clamp( x2 + 0.50f, 0.0f, 1.0f );
		y1 = 1.0f - FMath::Clamp( y1 + 0.50f, 0.0f, 1.0f );
		y2 = 1.0f - FMath::Clamp( y2 + 0.50f, 0.0f, 1.0f );

		FCanvasLineItem LineItem( FVector2D( GraphX + x1 * GraphWidth, LineGraphY + y1 * LineGraphHeight ), FVector2D( GraphX + x2 * GraphWidth, LineGraphY + y2 * LineGraphHeight ) );
		LineItem.SetColor( FLinearColor( 1.0f, 0.5f, 0.0f, 1.0f ) );
		LineItem.Draw( Canvas->Canvas );
	}

	OutX = FMath::Max(XL,GraphWidth);
}

void UT6VehicleComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos) {
	if (PVehicle == NULL) {
		return;
	}

	FPhysXVehicleManager* MyVehicleManager = World->GetPhysicsScene()->GetVehicleManager();

	MyVehicleManager->SetRecordTelemetry(this, true);

	UFont* RenderFont = GEngine->GetSmallFont();
	// draw drive data
	{
		Canvas->SetDrawColor(FColor::White);
		float forwardSpeedKmH = GetForwardSpeed() * 3600.f / 100000.f;	//convert from cm/s to km/h
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Speed (km/h): %d"), (int32)forwardSpeedKmH), 4, YPos);
		YPos += Canvas->DrawText( RenderFont, FString::Printf( TEXT("Steering: %f"), GetSteering() ), 4, YPos  );
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Throttle: %f"), InputState.GetThrottle() * GetAccel()), 4, YPos);
		//YPos += Canvas->DrawText( RenderFont, FString::Printf( TEXT("Brake: %f"), BrakeInput ), 4, YPos  );
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("RPM: %f"), GetWheelRotationSpeed()), 4, YPos);
		//YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Gear: %d"), GetCurrentGear()), 4, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Drag: %.3f"), DebugDragMagnitude), 4, YPos);
	}

	SCOPED_SCENE_READ_LOCK(MyVehicleManager->GetScene());
	PxWheelQueryResult* WheelsStates = MyVehicleManager->GetWheelsStates_AssumesLocked(this);
	check(WheelsStates);

	// draw wheel data
	for (uint32 w = 0; w < PVehicle->mWheelsSimData.getNbWheels(); ++w) {

		const PxMaterial* ContactSurface = WheelsStates[w].tireSurfaceMaterial;
		const PxReal TireFriction = WheelsStates[w].tireFriction;
		const PxReal LatSlip = WheelsStates[w].lateralSlip;
		const PxReal LongSlip = WheelsStates[w].longitudinalSlip;

		const PxReal RotSpeed = PVehicle->mWheelsDynData.getWheelRotationSpeed(w);
		const PxReal Radius = PVehicle->mWheelsSimData.getWheelData(w).mRadius;

		const PxReal WheelSpeed = RotSpeed * Radius;

		UPhysicalMaterial* ContactSurfaceMaterial = ContactSurface ? FPhysxUserData::Get<UPhysicalMaterial>(ContactSurface->userData) : NULL;
		const FString ContactSurfaceString = ContactSurfaceMaterial ? ContactSurfaceMaterial->GetName() : FString(TEXT("NONE"));

		Canvas->SetDrawColor(FColor::White);

		Canvas->DrawText(RenderFont, FString::Printf(TEXT("[%d]"), w), 4, YPos);

		Canvas->DrawText(RenderFont, FString::Printf(TEXT("LatSlip: %.3f"), LatSlip), YL * 4, YPos);
		Canvas->DrawText(RenderFont, FString::Printf(TEXT("LongSlip: %.3f"), LongSlip), YL * 12, YPos);
		Canvas->DrawText(RenderFont, FString::Printf(TEXT("Speed: %d"), (int32)WheelSpeed), YL * 22, YPos);
		Canvas->DrawText(RenderFont, FString::Printf(TEXT("Contact Surface: %s"), *ContactSurfaceString), YL * 74, YPos);
		if ((int32)w < Wheels.Num()) {
			UT6Wheel* Wheel = Wheels[w];
			Canvas->DrawText(RenderFont, FString::Printf(TEXT("Load: %.3f"), Wheel->DebugNormalizedTireLoad), YL * 30, YPos);
			Canvas->DrawText(RenderFont, FString::Printf(TEXT("Torque: %d"), (int32)Wheel->DebugWheelTorque), YL * 40, YPos);
			Canvas->DrawText(RenderFont, FString::Printf(TEXT("Long Force: %d"), (int32)Wheel->DebugLongForce), YL * 50, YPos);
			Canvas->DrawText(RenderFont, FString::Printf(TEXT("Lat Force: %d"), (int32)Wheel->DebugLatForce), YL * 62, YPos);
		}
		else {
			Canvas->DrawText(RenderFont, TEXT("Wheels array insufficiently sized!"), YL * 50, YPos);
		}

		YPos += YL;
	}

	// draw wheel graphs
	PxVehicleTelemetryData* TelemetryData = MyVehicleManager->GetTelemetryData_AssumesLocked();

	if (TelemetryData) {
		const float GraphWidth(100.0f), GraphHeight(100.0f);

		int GraphChannels[] = {
			PxVehicleWheelGraphChannel::eWHEEL_OMEGA,
			PxVehicleWheelGraphChannel::eSUSPFORCE,
			PxVehicleWheelGraphChannel::eTIRE_LONG_SLIP,
			PxVehicleWheelGraphChannel::eNORM_TIRE_LONG_FORCE,
			PxVehicleWheelGraphChannel::eTIRE_LAT_SLIP,
			PxVehicleWheelGraphChannel::eNORM_TIRE_LAT_FORCE,
			PxVehicleWheelGraphChannel::eNORMALIZED_TIRELOAD,
			PxVehicleWheelGraphChannel::eTIRE_FRICTION
		};

		for (uint32 w = 0; w < PVehicle->mWheelsSimData.getNbWheels(); ++w) {
			float CurX = 4;

			for (uint32 i = 0; i < ARRAY_COUNT(GraphChannels); i++) {
				float OutX = GraphWidth;
				T6DrawTelemetryGraph( GraphChannels[i], TelemetryData->getWheelGraph(w), Canvas, CurX, YPos, GraphWidth, GraphHeight, OutX );
				CurX += OutX + 10.f;
			}

			YPos += GraphHeight + 10.f;
			YPos += YL;
		}
	}

	DrawDebugLines();
}

void UT6VehicleComponent::DrawDebugLines() {
	if (PVehicle == NULL) {
		return;
	}

	FPhysXVehicleManager* MyVehicleManager = World->GetPhysicsScene()->GetVehicleManager();

	MyVehicleManager->SetRecordTelemetry(this, true);

	PxRigidDynamic* PActor = PVehicle->getRigidDynamicActor();

	// gather wheel shapes
	PxShape* PShapeBuffer[32];
	PActor->getShapes(PShapeBuffer, 32);
	const uint32 PNumWheels = PVehicle->mWheelsSimData.getNbWheels();

	UWorld* World = GetWorld();

	// draw chassis orientation
	const PxTransform GlobalT = PActor->getGlobalPose();
	const PxTransform T = GlobalT.transform(PActor->getCMassLocalPose());
	const PxVec3 ChassisExtent = PActor->getWorldBounds().getExtents();
	const float ChassisSize = ChassisExtent.magnitude();
	DrawDebugLine(World, P2UVector(T.p), P2UVector(T.p + T.rotate(PxVec3(ChassisSize, 0, 0))), FColor::Red);
	DrawDebugLine(World, P2UVector(T.p), P2UVector(T.p + T.rotate(PxVec3(0, ChassisSize, 0))), FColor::Green);
	DrawDebugLine(World, P2UVector(T.p), P2UVector(T.p + T.rotate(PxVec3(0, 0, ChassisSize))), FColor::Blue);

	SCOPED_SCENE_READ_LOCK(MyVehicleManager->GetScene());
	PxVehicleTelemetryData* TelemetryData = MyVehicleManager->GetTelemetryData_AssumesLocked();

	PxWheelQueryResult* WheelsStates = MyVehicleManager->GetWheelsStates_AssumesLocked(this);
	check(WheelsStates);

	for (uint32 w = 0; w < PNumWheels; ++w) {
		// render suspension raycast

		const FVector SuspensionStart = P2UVector(WheelsStates[w].suspLineStart);
		const FVector SuspensionEnd = P2UVector(WheelsStates[w].suspLineStart + WheelsStates[w].suspLineDir * WheelsStates[w].suspLineLength);
		const FColor SuspensionColor = WheelsStates[w].tireSurfaceMaterial == NULL ? FColor(255, 64, 64) : FColor(64, 255, 64);
		DrawDebugLine(World, SuspensionStart, SuspensionEnd, SuspensionColor);

		// render wheel radii
		const int32 ShapeIndex = PVehicle->mWheelsSimData.getWheelShapeMapping(w);
		const PxF32 WheelRadius = PVehicle->mWheelsSimData.getWheelData(w).mRadius;
		const PxF32 WheelWidth = PVehicle->mWheelsSimData.getWheelData(w).mWidth;
		const FTransform WheelTransform = P2UTransform(PActor->getGlobalPose().transform(PShapeBuffer[ShapeIndex]->getLocalPose()));
		const FVector WheelLocation = WheelTransform.GetLocation();
		const FVector WheelLatDir = WheelTransform.TransformVector(FVector(0.0f, 1.0f, 0.0f));
		const FVector WheelLatOffset = WheelLatDir * WheelWidth * 0.50f;
		//const FVector WheelRotDir = FQuat( WheelLatDir, PVehicle->mWheelsDynData.getWheelRotationAngle(w) ) * FVector( 1.0f, 0.0f, 0.0f );
		const FVector WheelRotDir = WheelTransform.TransformVector(FVector(1.0f, 0.0f, 0.0f));
		const FVector WheelRotOffset = WheelRotDir * WheelRadius;

		const FVector CylinderStart = WheelLocation + WheelLatOffset;
		const FVector CylinderEnd = WheelLocation - WheelLatOffset;

		DrawDebugCylinder(World, CylinderStart, CylinderEnd, WheelRadius, 16, SuspensionColor);
		DrawDebugLine(World, WheelLocation, WheelLocation + WheelRotOffset, SuspensionColor);

		const FVector ContactPoint = P2UVector(WheelsStates[w].tireContactPoint);
		DrawDebugBox(World, ContactPoint, FVector(4.0f), FQuat::Identity, SuspensionColor);

		if (TelemetryData) {
			// Draw all tire force app points.
			const PxVec3& PAppPoint = TelemetryData->getTireforceAppPoints()[w];
			const FVector AppPoint = P2UVector(PAppPoint);
			DrawDebugBox(World, AppPoint, FVector(5.0f), FQuat::Identity, FColor(255, 0, 255));

			// Draw all susp force app points.
			const PxVec3& PAppPoint2 = TelemetryData->getSuspforceAppPoints()[w];
			const FVector AppPoint2 = P2UVector(PAppPoint2);
			DrawDebugBox(World, AppPoint2, FVector(5.0f), FQuat::Identity, FColor(0, 255, 255));
		}
	}
}

#undef LOCTEXT_NAMESPACE