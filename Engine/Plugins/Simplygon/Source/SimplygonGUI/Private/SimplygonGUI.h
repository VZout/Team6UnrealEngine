#pragma once

class FSimplygonGUI : public ISimplygonGUI
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};