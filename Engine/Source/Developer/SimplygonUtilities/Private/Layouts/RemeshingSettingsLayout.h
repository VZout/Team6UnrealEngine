#pragma once
#include "PropertyEditing.h"

class FRemeshingSettingsLayout : public IDetailCustomNodeBuilder 
	, public TSharedFromThis<FRemeshingSettingsLayout>
{
public:
	FRemeshingSettingsLayout(TAttribute<EVisibility>& InRemeshingSettingsVisisbility);
	virtual ~FRemeshingSettingsLayout();

	/** IDetailCustomNodeBuilder Interface*/
	virtual void SetOnRebuildChildren( FSimpleDelegate InOnRegenerateChildren ) override {}
	virtual void GenerateHeaderRowContent( FDetailWidgetRow& NodeRow ) override;
	virtual void GenerateChildContent( IDetailChildrenBuilder& ChildrenBuilder ) override;
	virtual void Tick( float DeltaTime ) override{}
	virtual bool RequiresTick() const override { return false; }
	virtual FName GetName() const override { static FName RemeshingSettingsName("RemeshingSettings"); return RemeshingSettingsName; }
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

	//Recalculate Normals
	float GetHardAngle() const;
	void OnHardAngleChanged(float NewValue);
	void OnHardAngleCommitted(float NewValue, ETextCommit::Type TextCommitType);

	//Merge Distance
	int32 GetMergeDistance() const;
	void OnMergeDistanceChanged(int32 NewValue);
	void OnMergeDistanceCommitted(int32 NewValue, ETextCommit::Type TextCommitType);

private:
	FSimplygonRemeshingSettings RemeshingSettings;

	TAttribute<EVisibility>& RemeshingSettingsVisisbility;
};