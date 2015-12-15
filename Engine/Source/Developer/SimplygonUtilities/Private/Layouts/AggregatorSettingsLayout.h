#pragma once
#include "PropertyEditing.h"

class FAggregatorSettingsLayout : public IDetailCustomNodeBuilder 
	, public TSharedFromThis<FAggregatorSettingsLayout>
{
public:
	FAggregatorSettingsLayout(TAttribute<EVisibility>& InAggregateSettingsVisisbility);
	virtual ~FAggregatorSettingsLayout();

	/** IDetailCustomNodeBuilder Interface*/
	virtual void SetOnRebuildChildren( FSimpleDelegate InOnRegenerateChildren ) override {}
	virtual void GenerateHeaderRowContent( FDetailWidgetRow& NodeRow ) override;
	virtual void GenerateChildContent( IDetailChildrenBuilder& ChildrenBuilder ) override;
	virtual void Tick( float DeltaTime ) override{}
	virtual bool RequiresTick() const override { return false; }
	virtual FName GetName() const override { static FName AggregateSettingsName("AggregateSettings"); return AggregateSettingsName; }
	virtual bool InitiallyCollapsed() const override { return true; }

	const FSimplygonRemeshingSettings& GetSettings() const;

private:
	//Use MassiveLOD
	ECheckBoxState GetUseMassiveLODState() const;
	void OnUseMassiveLODChanged(ECheckBoxState NewValue);

	//Size On screen
	int32 GetSizeOnScreen() const;
	void OnSizeOnScreenChanged(int32 NewValue);
	void OnSizeOnScreenCommitted(int32 NewValue, ETextCommit::Type TextCommitType);

private:
	FSimplygonRemeshingSettings AggregateSettings;

	TAttribute<EVisibility>& AggregateSettingsVisisbility;

};