#include "SimplygonUtilitiesPrivatePCH.h"
#include "RemeshingSettingsLayout.h"


#define LOCTEXT_NAMESPACE "RemeshingSettingsLayout"

FRemeshingSettingsLayout::FRemeshingSettingsLayout(TAttribute<EVisibility>& InRemeshingSettingsVisisbility)
	: RemeshingSettingsVisisbility(InRemeshingSettingsVisisbility)
{
	RemeshingSettings.bUseAggregateLOD = false; //Important
}

FRemeshingSettingsLayout::~FRemeshingSettingsLayout()
{}

void FRemeshingSettingsLayout::GenerateHeaderRowContent( FDetailWidgetRow& NodeRow )
{
	NodeRow
		.Visibility(RemeshingSettingsVisisbility)
		.NameContent()
		[
			SNew(STextBlock)
			.Visibility(RemeshingSettingsVisisbility)
			.Text(LOCTEXT("RemeshingSettings", "Remeshing Settings"))
			.Font(IDetailLayoutBuilder::GetDetailFontBold())
		];
}

void FRemeshingSettingsLayout::GenerateChildContent( IDetailChildrenBuilder& ChildrenBuilder )
{
	//Use MassiveLOD
	ChildrenBuilder.AddChildContent(LOCTEXT("UseMassiveLOD", "Use MassiveLOD") )
		.Visibility(RemeshingSettingsVisisbility)
		.NameContent()
		[
			SNew( STextBlock )
			.Font( IDetailLayoutBuilder::GetDetailFont() )
			.Text( LOCTEXT("AsParent", "As Parent") )
		]
	.ValueContent()
		[
			SNew(SCheckBox)
			.IsChecked(this, &FRemeshingSettingsLayout::GetUseMassiveLODState)
			.OnCheckStateChanged(this, &FRemeshingSettingsLayout::OnUseMassiveLODChanged)
		];

	//Size On Screen
	ChildrenBuilder.AddChildContent( LOCTEXT("SizeOnScreen", "Size On Screen") )
		.Visibility(RemeshingSettingsVisisbility)
		.NameContent()
		[
			SNew( STextBlock )
			.Font( IDetailLayoutBuilder::GetDetailFont() )
			.Text( LOCTEXT("SizeOnScreen", "Size On Screen") )
		]
	.ValueContent()
		[
			SNew(SSpinBox<int32>)
			.Font( IDetailLayoutBuilder::GetDetailFont() )
			.MinValue(1)
			.MaxValue(5000)
			.Value(this, &FRemeshingSettingsLayout::GetSizeOnScreen)
			.OnValueChanged(this, &FRemeshingSettingsLayout::OnSizeOnScreenChanged)
			.OnValueCommitted(this, &FRemeshingSettingsLayout::OnSizeOnScreenCommitted)
		];

	//Merge Distance
	ChildrenBuilder.AddChildContent( LOCTEXT("MergeDistance", "Merge Distance") )
		.Visibility(RemeshingSettingsVisisbility)
		.NameContent()
		[
			SNew( STextBlock )
			.Font( IDetailLayoutBuilder::GetDetailFont() )
			.Text( LOCTEXT("MergeDistance", "Merge Distance") )
		]
	.ValueContent()
		[
			SNew(SSpinBox<int32>)
			.Font( IDetailLayoutBuilder::GetDetailFont() )
			.MinValue(0)
			.MaxValue(5000)
			.Value(this, &FRemeshingSettingsLayout::GetMergeDistance)
			.OnValueChanged(this, &FRemeshingSettingsLayout::OnMergeDistanceChanged)
			.OnValueCommitted(this, &FRemeshingSettingsLayout::OnMergeDistanceCommitted)
		];

	//Recalculate Normals
	/*ChildrenBuilder.AddChildContent( LOCTEXT("RecalculateNormals", "Recalculate Normals") )
		.Visibility(RemeshingSettingsVisisbility)
		.NameContent()
		[
		SNew( STextBlock )
		.Font( IDetailLayoutBuilder::GetDetailFontBold() )
		.Text( LOCTEXT("RecalculateNormals", "Recalculate Normals").ToString() )
		];*/

		ChildrenBuilder.AddChildContent( LOCTEXT("HardAngle", "Hard Angle") )
		.Visibility(RemeshingSettingsVisisbility)

		.NameContent()
		[
			SNew(SBox)
			.Content()
			[
				SNew( STextBlock )
				.Font( IDetailLayoutBuilder::GetDetailFont() )
				.Text( LOCTEXT("HardAngle", "Hard Angle") )
			]
		]
	.ValueContent()
		[
			SNew(SSpinBox<float>)
			.Font( IDetailLayoutBuilder::GetDetailFont() )
			.MinValue(0.0f)
			.MaxValue(180.0f)
			.Value(this, &FRemeshingSettingsLayout::GetHardAngle)
			.OnValueChanged(this, &FRemeshingSettingsLayout::OnHardAngleChanged)
			.OnValueCommitted(this, &FRemeshingSettingsLayout::OnHardAngleCommitted)
		];
}


const FSimplygonRemeshingSettings& FRemeshingSettingsLayout::GetSettings() const
{
	return RemeshingSettings;
}

//Use MassiveLOD
ECheckBoxState FRemeshingSettingsLayout::GetUseMassiveLODState() const
{
	return RemeshingSettings.bUseMassiveLOD ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void FRemeshingSettingsLayout::OnUseMassiveLODChanged(ECheckBoxState NewValue)
{
	RemeshingSettings.bUseMassiveLOD = (NewValue == ECheckBoxState::Checked);
}

//Size on screen
int32 FRemeshingSettingsLayout::GetSizeOnScreen() const
{
	return RemeshingSettings.ScreenSize;
}

void FRemeshingSettingsLayout::OnSizeOnScreenChanged(int32 NewValue)
{
	RemeshingSettings.ScreenSize = NewValue;
}

void FRemeshingSettingsLayout::OnSizeOnScreenCommitted(int32 NewValue, ETextCommit::Type TextCommitType)
{
	OnSizeOnScreenChanged(NewValue);
}

//Recalculate Normals
float FRemeshingSettingsLayout::GetHardAngle() const
{
	return RemeshingSettings.HardAngleThreshold;
}
void FRemeshingSettingsLayout::OnHardAngleChanged(float NewValue)
{
	RemeshingSettings.HardAngleThreshold = NewValue;
}
void FRemeshingSettingsLayout::OnHardAngleCommitted(float NewValue, ETextCommit::Type TextCommitType)
{
	OnHardAngleChanged(NewValue);
}

//Merge Distance
int32 FRemeshingSettingsLayout::GetMergeDistance() const
{
	return RemeshingSettings.MergeDistance;
}

void FRemeshingSettingsLayout::OnMergeDistanceChanged(int32 NewValue)
{
	RemeshingSettings.MergeDistance = NewValue;
}

void FRemeshingSettingsLayout::OnMergeDistanceCommitted(int32 NewValue, ETextCommit::Type TextCommitType)
{
	OnMergeDistanceChanged(NewValue);
}

#undef LOCTEXT_NAMESPACE