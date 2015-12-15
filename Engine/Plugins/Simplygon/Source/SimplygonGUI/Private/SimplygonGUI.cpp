// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SimplygonGUIPrivatePCH.h"
#include "SimplygonGUI.h"
#include "ContentBrowserExtensions/SimplygonContentBrowserExtensions.h"
#include "LevelEditorExtensions/SimplygonLevelEditorExtensions.h"

IMPLEMENT_MODULE( FSimplygonGUI, SimplygonGUI )

#define LOCTEXT_NAMESPACE "SimplygonPlugin"

void FSimplygonGUI::StartupModule()
{
	if (GIsEditor && !IsRunningCommandlet())
	{
		FSimplygonLevelEditorMenuExtensions::InstallHooks();
		FSimplygonGUIContentBrowserExtensions::InstallHooks();
	}
}

void FSimplygonGUI::ShutdownModule()
{
	if (GIsEditor && !IsRunningCommandlet())
	{
		FSimplygonLevelEditorMenuExtensions::RemoveHooks();
		FSimplygonGUIContentBrowserExtensions::RemoveHooks();
	}
}

#define LOCTEXT_NAMESPACE


