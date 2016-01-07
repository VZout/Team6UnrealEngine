// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

//  AActor  used to controll matinee's and to replicate activation, playback, and other relevant flags to net clients

#pragma once

#include "MatineeActor.h"
#include "T6MatineeActor.generated.h"

typedef TFunction<void(float EventTime)> FT6MatineeEvent;

UCLASS(MinimalAPI, Blueprintable)
class AT6MatineeActor : public AMatineeActor{
	GENERATED_BODY()

public:
	TMap<FName, FT6MatineeEvent> Delegates;

	AT6MatineeActor();

	void AddDelegate(FName Name, const FT6MatineeEvent& Delegate){
		Delegates.Add(Name, Delegate);
	}

	void RemoveDelegate(FName Name){
		Delegates.Remove(Name);
	}

	virtual void NotifyEventTriggered(FName EventName, float EventTime, bool bUseCustomEventName = false) override;
};



