#pragma once
#include "RemeshingSettingsLayout.h"
#include "AggregatorSettingsLayout.h"
#include "MaterialBakerSettingsLayout.h"
#include "PropertyEditing.h"

DECLARE_DELEGATE_OneParam( FOnLODTypeChanged, ESimplygonMassiveLOD );

class FMassiveLODSettingsLayout : public TSharedFromThis<FMassiveLODSettingsLayout>
{
public:
	FMassiveLODSettingsLayout();
	~FMassiveLODSettingsLayout();

	void CreateLayout( class IDetailLayoutBuilder& DetailBuilder );

	static bool GenerateMassiveLODMesh(
		const TArray<AActor*>& SourceActors,
		const struct FSimplygonRemeshingSettings& InMassiveLODSettings,
		FVector& OutProxyLocation,
		const FString& ProxyBasePackageName,
		TArray<UObject*>& OutAssetsToSync);

private:
	void AddSettings(class IDetailLayoutBuilder& DetailBuilder);
	void AddManagement(class IDetailLayoutBuilder& DetailBuilder);

	ECheckBoxState GetLODTypeState( ESimplygonMassiveLOD ButtonId ) const;
	void OnLODTypeChanged( ECheckBoxState NewRadioState, ESimplygonMassiveLOD RadioThatChanged );

	EVisibility GetRemeshingVisibility() const;
	EVisibility GetAggregateVisibility() const;

	void OnActorSelectionChanged(const TArray<UObject*>& NewSelection, bool bForceRefresh);
	void GenerateNewMassiveLODPackageName();

	FText GetMassiveLODPackageName() const;
	void OnPackageNameTextCommited(const FText& InText, ETextCommit::Type InCommitType);
	FReply OnSelectPackageNameClicked();

	bool IsRemeshingVisible();
	bool GetLevelEditorSelection(TArray<AActor*>& SelectedActors);

	FReply OnApply();

	FReply OnUnlinkSelection();

	FText GetParentName() const;
	FReply OnSetParent();
	FReply OnAddChildToParent();

private:
	///**Remeshing Widget*/
	TSharedPtr<FRemeshingSettingsLayout> RemeshingSettingsWidget;
	bool bRemeshingSettingsWidgetVisibility;
	TAttribute<EVisibility> RemeshingSettingsWidgetVisibility;

	///**Aggregate Widget*/
	TSharedPtr<FAggregatorSettingsLayout> AggregateSettingsWidget;
	bool bAggregateSettingsWidgetVisibility;
	TAttribute<EVisibility> AggregateSettingsWidgetVisibility;

	/**Material LOD Widget */
	TSharedPtr<FMaterialLODSettingsLayout> MaterialLODSettingsWidget;

	ESimplygonMassiveLOD CurrentLODType;
	FOnLODTypeChanged OnLODTypeSelectionChanged;

	///** MassiveLOD destination package name */
	FString MassiveLODPackageName;
	FString LODTypeString;

	TWeakObjectPtr<AActor> ParentActor;
	FText ParentName;

	FTextBlockStyle DeprecationTextStyle;
};