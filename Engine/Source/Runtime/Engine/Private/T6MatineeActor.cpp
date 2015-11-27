// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
Vehicle.cpp: AT6NavNode implementation
TODO: Put description here
=============================================================================*/

#include "EnginePrivate.h"
#include "Matinee/T6MatineeActor.h"
//#include "Vehicles/T6NavNodeComponent.h"





void AT6MatineeActor::NotifyEventTriggered(FName EventName, float EventTime, bool bUseCustomEventName) {
	Super::NotifyEventTriggered(EventName, EventTime, bUseCustomEventName);

	Delegate.ExecuteIfBound();	// Dit nog beter maken
}


//FOnMatineeEvent Delegate;
