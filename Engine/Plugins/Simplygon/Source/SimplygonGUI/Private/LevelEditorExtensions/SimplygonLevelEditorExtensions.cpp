#include "SimplygonGUIPrivatePCH.h"
#include "SimplygonLevelEditorExtensions.h"
#include "SimplygonLODGroupUtils.h"
#include "LevelEditor.h"

#define LOCTEXT_NAMESPACE "SimplygonPlugin"

//////////////////////////////////////////////////////////////////////////

FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors LevelEditorMenuExtenderDelegate;
FDelegateHandle LevelEditorExtenderDelegateHandle;

//////////////////////////////////////////////////////////////////////////
// FSimplygonLevelEditorMenuExtensions_Impl

class FSimplygonLevelEditorMenuExtensions_Impl
{
public:

	static void ApplyLODGroup(int32 LODGroupIndex)
	{
		bool bHasActors = false;
		bool bHasValidActors = true;
		TArray<AActor*> SelectedActors;

		//Gather Actors
		USelection* ActorSelection = GEditor->GetSelectedActors();
		for (FSelectionIterator Iter(*ActorSelection); Iter; ++Iter)
		{
			AActor* Actor = Cast<AActor>(*Iter);

			if (Actor)
			{
				SelectedActors.Add(Actor);
			}
		}

		bHasActors = SelectedActors.Num() > 0;
		if (!bHasActors)
		{
			return;
		}

		{
			FScopedSlowTask SlowTask(SelectedActors.Num(), LOCTEXT("ApplyingLODGroups", "Applying LODGroups"));
			SlowTask.MakeDialog();
			for (AActor* Actor : SelectedActors)
			{
				if (!Actor)
				{
					continue;
				}

				TArray<UStaticMeshComponent*> Components;
				Actor->GetComponents<UStaticMeshComponent>(Components);

				for (UStaticMeshComponent* StaticMeshComponent : Components)
				{
					FSimplygonLODGroupUtils::ApplyLODGroupToStaticMesh(StaticMeshComponent->StaticMesh, LODGroupIndex);
				}

				SlowTask.EnterProgressFrame(1);
			}
		}
		
	}

	static void PopulateLODGroupMenu(class FMenuBuilder& MenuBuilder)
	{
		TArray<FName> LODGroupNames;
		LODGroupNames.Reset();
		UStaticMesh::GetLODGroups(LODGroupNames);

		for (int32 LODGroupIndex = 0; LODGroupIndex < LODGroupNames.Num(); ++LODGroupIndex)
		{
			FUIAction CreateLODGroupAction(FExecuteAction::CreateStatic(&ApplyLODGroup, LODGroupIndex));

			MenuBuilder.AddMenuEntry(
				FText::FromName(LODGroupNames[LODGroupIndex]),
				LOCTEXT("LODGroupTypeTooltip", "Click to assign current LOD Group to selected actors"),
				FSlateIcon(),
				CreateLODGroupAction
				);
		}
	}

	static void CreateLODGroupActionsMenuEntries(FMenuBuilder& MenuBuilder)
	{
		MenuBuilder.BeginSection("Simplygon", LOCTEXT("SimplygonHeading", "Simplygon"));
		{
			MenuBuilder.AddSubMenu(
				LOCTEXT("LODGroupSubMenu", "Assign LOD Group"),
				LOCTEXT("LODGroupSubMenu_ToolTip", "Assign LODGroup to selection"),
				FNewMenuDelegate::CreateStatic(&PopulateLODGroupMenu),
				false,
				FSlateIcon(FEditorStyle::GetStyleSetName(), "SimplygonIcon.TabIcon"));
		}

		MenuBuilder.EndSection();
	}


	static TSharedRef<FExtender> OnExtendLevelEditorMenu(const TSharedRef<FUICommandList> CommandList, TArray<AActor*> SelectedActors)
	{
		TSharedRef<FExtender> Extender(new FExtender());

		// Run through the actors to determine if any meet our criteria
		bool bCanAssignLODGroup = false;

		for (AActor* Actor : SelectedActors)
		{
			TInlineComponentArray<UActorComponent*> ActorComponents;
			Actor->GetComponents(ActorComponents);

			for (UActorComponent* Component : ActorComponents)
			{
				if (Component->IsA(UStaticMeshComponent::StaticClass()))
				{
					bCanAssignLODGroup = true;
				}
			}
		}

		if (bCanAssignLODGroup)
		{
			// Add the sprite actions sub-menu extender
			Extender->AddMenuExtension(
				"ActorType",
				EExtensionHook::Before,
				nullptr,
				FMenuExtensionDelegate::CreateStatic(&FSimplygonLevelEditorMenuExtensions_Impl::CreateLODGroupActionsMenuEntries));
		}

		return Extender;
	}

};

//////////////////////////////////////////////////////////////////////////
// FSimplygonLevelEditorMenuExtensions

void FSimplygonLevelEditorMenuExtensions::InstallHooks()
{
	LevelEditorMenuExtenderDelegate = FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors::CreateStatic(&FSimplygonLevelEditorMenuExtensions_Impl::OnExtendLevelEditorMenu);

	FLevelEditorModule& LevelEditorModule = FModuleManager::Get().LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	auto& MenuExtenders = LevelEditorModule.GetAllLevelViewportContextMenuExtenders();
	MenuExtenders.Add(LevelEditorMenuExtenderDelegate);
	LevelEditorExtenderDelegateHandle = MenuExtenders.Last().GetHandle();
}

void FSimplygonLevelEditorMenuExtensions::RemoveHooks()
{
	// Remove level viewport context menu extenders
	if (FModuleManager::Get().IsModuleLoaded("LevelEditor"))
	{
		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		LevelEditorModule.GetAllLevelViewportContextMenuExtenders().RemoveAll([&](const FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors& Delegate) {
			return Delegate.GetHandle() == LevelEditorExtenderDelegateHandle;
		});
	}
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE