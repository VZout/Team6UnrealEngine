// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

//  AActor  used to controll matinee's and to replicate activation, playback, and other relevant flags to net clients

#pragma once

#include "MatineeActor.h"
#include "T6MatineeActor.generated.h"

UCLASS(MinimalAPI, Blueprintable)
class AT6MatineeActor : public AMatineeActor{
	GENERATED_BODY()

public:
	FOnMatineeEvent Delegate;

	AT6MatineeActor();

	virtual void NotifyEventTriggered(FName EventName, float EventTime, bool bUseCustomEventName = false) override;
};



