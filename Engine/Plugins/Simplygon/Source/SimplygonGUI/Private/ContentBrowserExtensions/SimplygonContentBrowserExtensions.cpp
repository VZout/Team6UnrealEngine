//Reference: \Engine\Plugins\2D\Paper2D\Source\Paper2DEditor\Private\ContentBrowserExtensions
#include "SimplygonGUIPrivatePCH.h"
#include "SimplygonContentBrowserExtensions.h"
#include "SimplygonLODGroupUtils.h"
#include "ContentBrowserModule.h"


#define LOCTEXT_NAMESPACE "SimplygonGUI"

DECLARE_LOG_CATEGORY_EXTERN(LogSimplygonGUICBExtensions, Log, All);
DEFINE_LOG_CATEGORY(LogSimplygonGUICBExtensions);

//////////////////////////////////////////////////////////////////////////

FContentBrowserMenuExtender_SelectedAssets ContentBrowserExtenderDelegate;
FDelegateHandle ContentBrowserExtenderDelegateHandle;

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// FContentBrowserSelectedAssetExtensionBase

struct FContentBrowserSelectedAssetExtensionBase
{
public:
	TArray<class FAssetData> SelectedAssets;

public:
	virtual void Execute() {}
	virtual ~FContentBrowserSelectedAssetExtensionBase() {}
};

//////////////////////////////////////////////////////////////////////////
// FAssignLODGroupExtension

struct FAssignLODGroupExtension : public FContentBrowserSelectedAssetExtensionBase
{
	int32 LODGroupIndex;
	FAssignLODGroupExtension()
	{
		LODGroupIndex = 0;
	}

	void ApplyLODGroupToStaticMeshes(TArray<UStaticMesh*>& StaticMeshes)
	{
		FScopedSlowTask SlowTask(StaticMeshes.Num(), LOCTEXT("ApplyingLODGroups", "Applying LODGroups"));
		SlowTask.MakeDialog();

		for (UStaticMesh* StaticMesh : StaticMeshes)
		{
			FSimplygonLODGroupUtils::ApplyLODGroupToStaticMesh(StaticMesh, LODGroupIndex);

			SlowTask.EnterProgressFrame(1);
		}
	}

	virtual void Execute() override
	{
		TArray<UStaticMesh*> StaticMeshes;
		for (auto AssetIt = SelectedAssets.CreateConstIterator(); AssetIt; ++AssetIt)
		{
			const FAssetData& AssetData = *AssetIt;
			if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(AssetData.GetAsset()))
			{
				StaticMeshes.Add(StaticMesh);
			}
		}

		ApplyLODGroupToStaticMeshes(StaticMeshes);
	}
};

//////////////////////////////////////////////////////////////////////////
// FSimplygonGUIContentBrowserExtensions_Impl

class FSimplygonGUIContentBrowserExtensions_Impl
{
public:
	static void ExecuteSelectedContentFunctor(TSharedPtr<FContentBrowserSelectedAssetExtensionBase> SelectedAssetFunctor)
	{
		SelectedAssetFunctor->Execute();
	}

	static void PopulateLODGroupMenu(class FMenuBuilder& MenuBuilder, TArray<FAssetData> SelectedAssets)
	{
		TArray<FName> LODGroupNames;
		LODGroupNames.Reset();
		UStaticMesh::GetLODGroups(LODGroupNames);

		for (int32 LODGroupIndex = 0; LODGroupIndex < LODGroupNames.Num(); ++LODGroupIndex)
		{
			TSharedPtr<FAssignLODGroupExtension> AssignLODGroupFunctor = MakeShareable(new FAssignLODGroupExtension());
			AssignLODGroupFunctor->SelectedAssets = SelectedAssets;
			AssignLODGroupFunctor->LODGroupIndex = LODGroupIndex;
			
			FUIAction Action_AssignLODGroupToAsset(
				FExecuteAction::CreateStatic(&FSimplygonGUIContentBrowserExtensions_Impl::ExecuteSelectedContentFunctor, StaticCastSharedPtr<FContentBrowserSelectedAssetExtensionBase>(AssignLODGroupFunctor)));

			MenuBuilder.AddMenuEntry(
				FText::FromName(LODGroupNames[LODGroupIndex]),
				LOCTEXT("LODGroupTypeTooltip", "Click to assign current LOD Group to selected actors"),
				FSlateIcon(),
				Action_AssignLODGroupToAsset
				);
		}
	}

	static void CreateLODGroupActionsSubMenu(FMenuBuilder& MenuBuilder, TArray<FAssetData> SelectedAssets)
	{
		MenuBuilder.BeginSection("Simplygon", LOCTEXT("SimplygonHeading", "Simplygon"));
		{
			MenuBuilder.AddSubMenu(
				LOCTEXT("LODGroupSubMenu", "Assign LOD Group"),
				LOCTEXT("LODGroupSubMenu_ToolTip", "Assign LODGroup to selection"),
				FNewMenuDelegate::CreateStatic(&PopulateLODGroupMenu, SelectedAssets),
				false,
				FSlateIcon(FEditorStyle::GetStyleSetName(), "SimplygonIcon.TabIcon"));
		}

		MenuBuilder.EndSection();
	}

	static TSharedRef<FExtender> OnExtendContentBrowserAssetSelectionMenu(const TArray<FAssetData>& SelectedAssets)
	{
		TSharedRef<FExtender> Extender(new FExtender());

		// Run through the assets to determine if any meet our criteria
		bool bAnyStaticMesh = false;
		for (auto AssetIt = SelectedAssets.CreateConstIterator(); AssetIt; ++AssetIt)
		{
			const FAssetData& Asset = *AssetIt;
			bAnyStaticMesh = bAnyStaticMesh || (Asset.AssetClass == UStaticMesh::StaticClass()->GetFName());
		}

		if (bAnyStaticMesh)
		{
			// Add the sprite actions sub-menu extender
			Extender->AddMenuExtension(
				"GetAssetActions",
				EExtensionHook::After,
				nullptr,
				FMenuExtensionDelegate::CreateStatic(&FSimplygonGUIContentBrowserExtensions_Impl::CreateLODGroupActionsSubMenu, SelectedAssets));
		}

		return Extender;
	}

	static TArray<FContentBrowserMenuExtender_SelectedAssets>& GetExtenderDelegates()
	{
		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
		return ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
	}

};

//////////////////////////////////////////////////////////////////////////
// FPaperContentBrowserExtensions

void FSimplygonGUIContentBrowserExtensions::InstallHooks()
{
	ContentBrowserExtenderDelegate = FContentBrowserMenuExtender_SelectedAssets::CreateStatic(&FSimplygonGUIContentBrowserExtensions_Impl::OnExtendContentBrowserAssetSelectionMenu);

	TArray<FContentBrowserMenuExtender_SelectedAssets>& CBMenuExtenderDelegates = FSimplygonGUIContentBrowserExtensions_Impl::GetExtenderDelegates();
	CBMenuExtenderDelegates.Add(ContentBrowserExtenderDelegate);
	ContentBrowserExtenderDelegateHandle = CBMenuExtenderDelegates.Last().GetHandle();
}

void FSimplygonGUIContentBrowserExtensions::RemoveHooks()
{
	TArray<FContentBrowserMenuExtender_SelectedAssets>& CBMenuExtenderDelegates = FSimplygonGUIContentBrowserExtensions_Impl::GetExtenderDelegates();
	CBMenuExtenderDelegates.RemoveAll([](const FContentBrowserMenuExtender_SelectedAssets& Delegate){ return Delegate.GetHandle() == ContentBrowserExtenderDelegateHandle; });
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE