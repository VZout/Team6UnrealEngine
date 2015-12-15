#include "SimplygonUtilitiesPrivatePCH.h"
#include "AggregatorSettingsLayout.h"


#define LOCTEXT_NAMESPACE "AggregatorSettingsLayout"

FAggregatorSettingsLayout::FAggregatorSettingsLayout(TAttribute<EVisibility>& InAggregateSettingsVisisbility)
	: AggregateSettingsVisisbility(InAggregateSettingsVisisbility)
{
	AggregateSettings.bUseAggregateLOD = true; //Important
}

FAggregatorSettingsLayout::~FAggregatorSettingsLayout()
{}

void FAggregatorSettingsLayout::GenerateHeaderRowContent( FDetailWidgetRow& NodeRow )
{
	NodeRow
		.Visibility(AggregateSettingsVisisbility)
		.NameContent()
		[
			SNew(STextBlock)
			.Visibility(AggregateSettingsVisisbility)
			.Text(LOCTEXT("AggregateSettings", "Aggregate Settings"))
			.Font(IDetailLayoutBuilder::GetDetailFontBold())
		];
}
void FAggregatorSettingsLayout::GenerateChildContent( IDetailChildrenBuilder& ChildrenBuilder )
{
	//Use MassiveLOD
	ChildrenBuilder.AddChildContent(LOCTEXT("AsParent", "As Parent"))
		.Visibility(AggregateSettingsVisisbility)
		.NameContent()
		[
			SNew( STextBlock )
			.Font( IDetailLayoutBuilder::GetDetailFont() )
			.ToolTipText( LOCTEXT("MassiveLODToolTip", "Enabling this will link selected actors as childern to the new mesh. Switch distance is based on 'Size On Screen'.") )
			.Text( LOCTEXT("AsParent", "As Parent") )
		]
	.ValueContent()
		[
			SNew(SCheckBox)
			.IsChecked(this, &FAggregatorSettingsLayout::GetUseMassiveLODState)
			.OnCheckStateChanged(this, &FAggregatorSettingsLayout::OnUseMassiveLODChanged)
		];

	//Size On Screen
	ChildrenBuilder.AddChildContent( LOCTEXT("SizeOnScreen", "Size On Screen") )
		.Visibility(AggregateSettingsVisisbility)
		.NameContent()
		[
			SNew( STextBlock )
			.Font( IDetailLayoutBuilder::GetDetailFont() )
			.ToolTipText( LOCTEXT("SizeOnScreenToolTip", "Specify at what pixel-size on screen the MassiveLOD should switch.") )
			.Text( LOCTEXT("SizeOnScreen", "Size On Screen") )
		]
	.ValueContent()
		[
			SNew(SSpinBox<int32>)
			.Font( IDetailLayoutBuilder::GetDetailFont() )
			.MinValue(1)
			.MaxValue(5000)
			.Value(this, &FAggregatorSettingsLayout::GetSizeOnScreen)
			.OnValueChanged(this, &FAggregatorSettingsLayout::OnSizeOnScreenChanged)
			.OnValueCommitted(this, &FAggregatorSettingsLayout::OnSizeOnScreenCommitted)
		];
}

const FSimplygonRemeshingSettings& FAggregatorSettingsLayout::GetSettings() const
{
	return AggregateSettings;
}

//Use MassiveLOD
ECheckBoxState FAggregatorSettingsLayout::GetUseMassiveLODState() const
{
	return AggregateSettings.bUseMassiveLOD ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void FAggregatorSettingsLayout::OnUseMassiveLODChanged(ECheckBoxState NewValue)
{
	AggregateSettings.bUseMassiveLOD = (NewValue == ECheckBoxState::Checked);
}

//Size on screen
int32 FAggregatorSettingsLayout::GetSizeOnScreen() const
{
	return AggregateSettings.ScreenSize;
}

void FAggregatorSettingsLayout::OnSizeOnScreenChanged(int32 NewValue)
{
	AggregateSettings.ScreenSize = NewValue;
}

void FAggregatorSettingsLayout::OnSizeOnScreenCommitted(int32 NewValue, ETextCommit::Type TextCommitType)
{
	OnSizeOnScreenChanged(NewValue);
}

#undef LOCTEXT_NAMESPACE