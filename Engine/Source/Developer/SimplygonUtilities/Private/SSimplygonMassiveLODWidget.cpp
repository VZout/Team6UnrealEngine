#include "SimplygonUtilitiesPrivatePCH.h"
#include "SSimplygonMassiveLODWidget.h"

#include "MeshUtilities.h"

#define LOCTEXT_NAMESPACE "MassiveLODWidget"

SSimplygonMassiveLODWidget::~SSimplygonMassiveLODWidget()
{}

void SSimplygonMassiveLODWidget::Construct(const FArguments& InArgs)
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.bHideSelectionTip = true;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.NotifyHook = this;

	LevelEditorSettingsView = PropertyEditorModule.CreateDetailView( DetailsViewArgs );


	FOnGetDetailCustomizationInstance LayoutCustomStaticMeshProperties = FOnGetDetailCustomizationInstance::CreateSP( this, &SSimplygonMassiveLODWidget::MakeLevelEditorSettings );
	LevelEditorSettingsView->RegisterInstancedCustomPropertyLayout( FSimplygonDummyClass::StaticClass(), LayoutCustomStaticMeshProperties );

	LevelEditorSettingsView->SetObject(FSimplygonDummyClass::StaticClass());

	//Check if Simplygon is initialized
	ISimplygonModule& SimplygonModule = FModuleManager::LoadModuleChecked<ISimplygonModule>("SimplygonMeshReduction");
	bool bSimplygonInitialized = SimplygonModule.GetMeshMaterialReductionInterface() &&
								 SimplygonModule.GetMeshMergingInterface() &&
								 SimplygonModule.GetMeshReductionInterface();

	TSharedPtr<SWidget> MainLayout = NULL;
	if (bSimplygonInitialized)
	{
		MainLayout = LevelEditorSettingsView;
	}
	else
	{
		MainLayout = SNew(STextBlock)
		.Text(LOCTEXT("SimplygonFailedToInitialize", " Simplygon is not initialized, check log for errors"));

	}

	ChildSlot
		[
			SNew( SScrollBox )
			+SScrollBox::Slot()
			[
				//Settings
				SNew( SBox )
				[
					MainLayout.ToSharedRef()
				]
			]
		];

}

void SSimplygonMassiveLODWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

void SSimplygonMassiveLODWidget::NotifyPostChange( const FPropertyChangedEvent& PropertyChangedEvent, UProperty* PropertyThatChanged )
{

}

TSharedRef<IDetailCustomization> SSimplygonMassiveLODWidget::MakeLevelEditorSettings()
{
	TSharedRef<FSimplygonMassiveLODSettings> NewSettings = MakeShareable( new FSimplygonMassiveLODSettings() );
	return NewSettings;
}

/*************************************************
************ FSimplygonLevelEditorSettings *******
**************************************************/
FSimplygonMassiveLODSettings::FSimplygonMassiveLODSettings()
{}

FSimplygonMassiveLODSettings::~FSimplygonMassiveLODSettings()
{}

void FSimplygonMassiveLODSettings::CustomizeDetails( class IDetailLayoutBuilder& DetailBuilder )
{
	MassiveLODSettingsLayout = MakeShareable( new FMassiveLODSettingsLayout() );
	MassiveLODSettingsLayout->CreateLayout( DetailBuilder );
}

#undef LOCTEXT_NAMESPACE