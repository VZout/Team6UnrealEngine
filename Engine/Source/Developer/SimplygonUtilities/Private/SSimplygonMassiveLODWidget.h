#pragma once
#include "MassiveLODSettingsLayout.h"
#include "PropertyEditing.h"

class SSimplygonMassiveLODWidget : public SCompoundWidget, public FNotifyHook
{
public:
	SLATE_BEGIN_ARGS(SSimplygonMassiveLODWidget)
	{
	}
	SLATE_EVENT( FOnLODTypeChanged, OnLODTypeSelectionChanged )
	SLATE_END_ARGS()

	/** Destructor **/
	virtual ~SSimplygonMassiveLODWidget();

	/** SWidget functions */
	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime);

	/** From FNotifyHook */
	virtual void NotifyPostChange( const FPropertyChangedEvent& PropertyChangedEvent, UProperty* PropertyThatChanged ) override;

private:
	TSharedRef<IDetailCustomization> MakeLevelEditorSettings();

private:
	/** Simplygon Level Editor settings */
	TSharedPtr<class IDetailsView> LevelEditorSettingsView;

};

class FSimplygonMassiveLODSettings : public IDetailCustomization
{
public:
	FSimplygonMassiveLODSettings();
	~FSimplygonMassiveLODSettings();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( class IDetailLayoutBuilder& DetailBuilder ) override;

private:

private:
	/** Level of detail settings for the details panel */
	TSharedPtr<FMassiveLODSettingsLayout> MassiveLODSettingsLayout;


};