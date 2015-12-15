#include "SimplygonUtilitiesPrivatePCH.h"
#include "MassiveLODSettingsLayout.h"

#include "LevelEditor.h"
#include "AssetRegistryModule.h"
#include "ContentBrowserModule.h"
#include "Dialogs/DlgPickAssetPath.h"
#include "ScopedTransaction.h"
#include "MeshUtilities.h"
#include "LandscapeRender.h"
#include "MaterialUtilities.h"
#include "MaterialExportUtils.h"
#include "RawMesh.h"
#include "StaticMeshResources.h"

#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionTextureSample.h"

#include "ImageUtils.h"

#include "Engine/LODActor.h"
#include "SimplygonUtilitiesModule.h"

#define LOCTEXT_NAMESPACE "MassiveLODSettingsLayout"

#define ENABLE_UNLINKPARENT_BUTTON 0
/************************************************************************/
/* Layout: Massive LOD Settings                                        */
/************************************************************************/
#define TTF_FONT(RelativePath, ...) FSlateFontInfo(ContentFromEngine(TEXT(RelativePath), TEXT(".ttf")), __VA_ARGS__)
namespace
{
	FString ContentFromEngine(const FString& RelativePath, const TCHAR* Extension)
	{
		static const FString ContentDir = FPaths::EngineDir() / TEXT("Content/Slate");
		return ContentDir / RelativePath + Extension;
	}
}

FMassiveLODSettingsLayout::FMassiveLODSettingsLayout()
{
	CurrentLODType = ESimplygonMassiveLOD::Remeshing;
	bRemeshingSettingsWidgetVisibility = true;
	LODTypeString = TEXT("ProxyLOD");
	ParentName = FText::FromString(TEXT("No Active Parent"));

	GenerateNewMassiveLODPackageName();

	DeprecationTextStyle = FTextBlockStyle()
		.SetFont(TTF_FONT("Fonts/Roboto-Bold", 12))
		.SetColorAndOpacity(FLinearColor(1.0f, 0.0f, 0.0f, 1.0f))
		.SetShadowOffset(FVector2D::ZeroVector)
		.SetShadowColorAndOpacity(FLinearColor::Black);
}

FMassiveLODSettingsLayout::~FMassiveLODSettingsLayout()
{
	FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>( "LevelEditor" );
	LevelEditor.OnActorSelectionChanged().RemoveAll(this);
}

void FMassiveLODSettingsLayout::CreateLayout( class IDetailLayoutBuilder& DetailBuilder )
{
	FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>( "LevelEditor" );
	LevelEditor.OnActorSelectionChanged().AddSP(this, &FMassiveLODSettingsLayout::OnActorSelectionChanged);

	AddSettings(DetailBuilder);

	AddManagement(DetailBuilder);

}

void FMassiveLODSettingsLayout::AddSettings(class IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& MainSettingsCategory =
		DetailBuilder.EditCategory( "OptimizationSettings", LOCTEXT("OptimizationSettings", "Settings") );

	MainSettingsCategory.AddCustomRow(LOCTEXT("Deprecation", "Deprecation"))
		.WholeRowContent()
		.HAlign(HAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			[
				SNew(STextBlock)
				.TextStyle(&DeprecationTextStyle)
				.Text(LOCTEXT("MassiveLODDeprecation1", "Deprecated: This tool will be deprecated as from 4.11. Use HLOD instead."))
			]
			+ SVerticalBox::Slot()
				[
					SNew(STextBlock)
					.TextStyle(&DeprecationTextStyle)
					.Text(LOCTEXT("MassiveLODDeprecation2", "More info will be available on Simplygon's website."))
				]

			
		];

	//LOD Type
	MainSettingsCategory.AddCustomRow( LOCTEXT("LODType", "Type") )
		.NameContent()
		[
			SNew(SBox)
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFontBold())
				.Text(LOCTEXT("LODType", "Type"))
			]
			
		]
	.ValueContent()
		.MinDesiredWidth(200.0f)
		.MaxDesiredWidth(200.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.Padding(0,4,0,0)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				[
					SNew(SCheckBox)
					.IsChecked(this, &FMassiveLODSettingsLayout::GetLODTypeState, ESimplygonMassiveLOD::Remeshing)
					.Style(FEditorStyle::Get(), "Property.ToggleButton.Start")
					.OnCheckStateChanged(this, &FMassiveLODSettingsLayout::OnLODTypeChanged, ESimplygonMassiveLOD::Remeshing)
					//.ToolTipText(LOCTEXT(""))
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(3, 2)
						[
							SNew(SImage)
							.Image(FEditorStyle::GetBrush("Mobility.Static"))
						]

						+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.VAlign(VAlign_Center)
							.HAlign(HAlign_Center)
							.Padding(6, 2)
							[
								SNew(STextBlock)
								.Text(LOCTEXT("Remeshing", "Remeshing"))
								.Font(IDetailLayoutBuilder::GetDetailFont())
								//.ColorAndOpacity(this, &FSceneComponentDetails::GetMobilityTextColor, MobilityPropertyWeak, EComponentMobility::Stationary)
							]
					]
				]
				+ SHorizontalBox::Slot()
					[
						SNew(SCheckBox)
						.IsChecked(this, &FMassiveLODSettingsLayout::GetLODTypeState, ESimplygonMassiveLOD::Aggregate)
						.Style(FEditorStyle::Get(), "Property.ToggleButton.End")
						.OnCheckStateChanged(this, &FMassiveLODSettingsLayout::OnLODTypeChanged, ESimplygonMassiveLOD::Aggregate)
						//.ToolTipText(LOCTEXT(""))
						[
							SNew(SHorizontalBox)

							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(3, 2)
							[
								SNew(SImage)
								.Image(FEditorStyle::GetBrush("Mobility.Static"))
							]

							+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								.VAlign(VAlign_Center)
								.HAlign(HAlign_Center)
								.Padding(6, 2)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("Aggregate", "Aggregate"))
									.Font(IDetailLayoutBuilder::GetDetailFont())
									//.ColorAndOpacity(this, &FSceneComponentDetails::GetMobilityTextColor, MobilityPropertyWeak, EComponentMobility::Stationary)
								]
						]
					]
			]
			
		];

	//Package name
	MainSettingsCategory.AddCustomRow(LOCTEXT("NewPackage", "New Package"))
		.NameContent()
		[
			SNew(SBox)
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFontBold())
				.Text(LOCTEXT("NewPackage", "New Package"))
			]
		]
	.ValueContent()
		.MinDesiredWidth(500.0f)
		.MaxDesiredWidth(500.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.Padding(0, 4, 0, 6)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0)
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Center)
				[
					SNew(SEditableTextBox)
					.Text(this, &FMassiveLODSettingsLayout::GetMassiveLODPackageName)
					.OnTextCommitted(this, &FMassiveLODSettingsLayout::OnPackageNameTextCommited)
				]
				+ SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Center)
					[
						SNew(SButton)
						.OnClicked(this, &FMassiveLODSettingsLayout::OnSelectPackageNameClicked)
						.Text(LOCTEXT("SelectPackageButton", "..."))
					]
			]
		];

	//Remeshing settings
	RemeshingSettingsWidgetVisibility = TAttribute<EVisibility>::Create( TAttribute<EVisibility>::FGetter::CreateSP(this, &FMassiveLODSettingsLayout::GetRemeshingVisibility) ) ;
	RemeshingSettingsWidget = MakeShareable( new FRemeshingSettingsLayout(RemeshingSettingsWidgetVisibility) );
	if (RemeshingSettingsWidget.IsValid())
	{
		MainSettingsCategory.AddCustomBuilder( RemeshingSettingsWidget.ToSharedRef() );
	}

	//Aggregate settings
	AggregateSettingsWidgetVisibility = TAttribute<EVisibility>::Create( TAttribute<EVisibility>::FGetter::CreateSP(this, &FMassiveLODSettingsLayout::GetAggregateVisibility) ) ;
	AggregateSettingsWidget = MakeShareable( new FAggregatorSettingsLayout(AggregateSettingsWidgetVisibility) );
	if(AggregateSettingsWidget.IsValid())
	{
		MainSettingsCategory.AddCustomBuilder( AggregateSettingsWidget.ToSharedRef() );
	}

	//Material settings
	MaterialLODSettingsWidget = MakeShareable( new FMaterialLODSettingsLayout(0,
		/*InAllowReuseUV = */ false,
		/*InAllowMultiMaterial = */ false,
		/*InShowVertexDataOption = */ true,
		/*InForceActive = */ true) );
	if(MaterialLODSettingsWidget.IsValid())
	{
		MainSettingsCategory.AddCustomBuilder( MaterialLODSettingsWidget.ToSharedRef() );
	}

	//Apply button
	MainSettingsCategory.AddCustomRow( LOCTEXT("Apply", "Apply") )
		.WholeRowContent()
		.HAlign(HAlign_Center)
		[
			SNew(SButton)
			.OnClicked(this, &FMassiveLODSettingsLayout::OnApply)
			[
				SNew( STextBlock )
				.Text(LOCTEXT("Apply", "Apply"))
				.Font( DetailBuilder.GetDetailFont() )
			]
		];
}

void FMassiveLODSettingsLayout::AddManagement(class IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& ManagementCategory =
		DetailBuilder.EditCategory( "Manage", LOCTEXT("Manage", "Manage"), ECategoryPriority::Uncommon );

	

	ManagementCategory.AddCustomRow( LOCTEXT("MassiveLODParenting", "Parent"))
		.NameContent()
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFontBold())
			.Text(LOCTEXT("MassiveLODParenting", "Parenting"))
		]
		.ValueContent()
		.MinDesiredWidth(500.0f)
		.MaxDesiredWidth(500.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.Padding(0, 3, 0, 3)
			[

				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.FillWidth(1.5f)
				.VAlign(VAlign_Center)
				[
					SNew(SEditableTextBox)
					.Font(IDetailLayoutBuilder::GetDetailFontBold())
					.Text(this, &FMassiveLODSettingsLayout::GetParentName)
					.IsEnabled(false)
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
					.OnClicked(this, &FMassiveLODSettingsLayout::OnSetParent)
					.ToolTipText(LOCTEXT("SetActiveParentToolTip", "Set selected actor (from level editor) as active parent"))
					.ContentPadding(4.0f)
					.ForegroundColor(FSlateColor::UseForeground())
					[
						SNew(SImage)
						.Image(FEditorStyle::GetBrush("PropertyWindow.Button_Use"))
						.ColorAndOpacity(FSlateColor::UseForeground())
					]
				]
				+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SButton)
						.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
						.OnClicked(this, &FMassiveLODSettingsLayout::OnAddChildToParent)
						.ToolTipText(LOCTEXT("AddChildToParentToolTip", "Add children to active parent"))
						.ContentPadding(4.0f)
						.ForegroundColor(FSlateColor::UseForeground())
						[
							SNew(SImage)
							.Image(FEditorStyle::GetBrush("PropertyWindow.Button_AddToArray"))
							.ColorAndOpacity(FSlateColor::UseForeground())
						]
					]
			]
#if ENABLE_UNLINKPARENT_BUTTON
			+ SVerticalBox::Slot()
			//.Padding(0, 3, 0, 3)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.MaxWidth(70.0f)
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.OnClicked(this, &FMassiveLODSettingsLayout::OnUnlinkSelection)
					[
						SNew(SBox)
						.HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("Unparent", "Unparent"))
							.ToolTipText(LOCTEXT("UnlinkSelectionToolTip", "Unparent selected actors that are influenced by MassiveLOD"))
							.Font(DetailBuilder.GetDetailFont())
						]
					]
				]
			]
#endif
		];

		/*ManagementCategory.AddCustomRow(LOCTEXT("UnParentSelection", "Unparent Selection"))
			.NameContent()
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFontBold())
				.Text(LOCTEXT("UnParentSelection", "Unparent Selection"))
			]
		.ValueContent()
			[
				SNew(SButton)
				.OnClicked(this, &FMassiveLODSettingsLayout::OnUnlinkSelection)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Unparent", "Unparent"))
					.ToolTipText(LOCTEXT("UnlinkSelectionToolTip", "Unlink selected actors that are influenced by MassiveLOD"))
					.Font(DetailBuilder.GetDetailFont())
				]
			];*/

}

ECheckBoxState FMassiveLODSettingsLayout::GetLODTypeState(ESimplygonMassiveLOD ButtonId) const
{
	return (CurrentLODType == ButtonId) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void FMassiveLODSettingsLayout::OnLODTypeChanged(ECheckBoxState NewRadioState, ESimplygonMassiveLOD RadioThatChanged)
{
	if (NewRadioState == ECheckBoxState::Checked)
	{
		CurrentLODType = RadioThatChanged;

		if (CurrentLODType == Aggregate)
		{
			bRemeshingSettingsWidgetVisibility = false;
			LODTypeString = TEXT("AggregateLOD");
		}
		else
		{
			bRemeshingSettingsWidgetVisibility = true;
			LODTypeString = TEXT("ProxyLOD");
		}

		//Refresh package name 
		GenerateNewMassiveLODPackageName();

		if (OnLODTypeSelectionChanged.IsBound())
		{

			OnLODTypeSelectionChanged.Execute(CurrentLODType);
		}
	}
}

void FMassiveLODSettingsLayout::GenerateNewMassiveLODPackageName()
{
	USelection* SelectedActors = GEditor->GetSelectedActors();
	MassiveLODPackageName.Empty();
	const FString BasePath = FPaths::GameContentDir() + TEXT("Simplygon/");

	// Iterate through selected actors and find first static mesh asset
	// Use this static mesh path as destination package name for a merged mesh
	for(FSelectionIterator Iter(*SelectedActors); Iter; ++Iter)
	{
		AActor* Actor = Cast<AActor>(*Iter);
		if (Actor)
		{
			TArray<UStaticMeshComponent*> SMComponets; 
			Actor->GetComponents<UStaticMeshComponent>(SMComponets);
			for (UStaticMeshComponent* Component : SMComponets)
			{
				if (Component->StaticMesh)
				{
					FString MeshName = Component->StaticMesh->GetName();

					MassiveLODPackageName = FPackageName::FilenameToLongPackageName(BasePath + MeshName + TEXT("/") + LODTypeString + TEXT("_") + MeshName);
					break;
				}
			}
		}

		if (!MassiveLODPackageName.IsEmpty())
		{
			break;
		}
	}

	if (MassiveLODPackageName.IsEmpty())
	{
		MassiveLODPackageName = FPackageName::FilenameToLongPackageName(BasePath + TEXT("MLOD"));
		MassiveLODPackageName = MakeUniqueObjectName(NULL, UPackage::StaticClass(), *MassiveLODPackageName).ToString();
	}
}

FText FMassiveLODSettingsLayout::GetMassiveLODPackageName() const
{
	return FText::FromString(MassiveLODPackageName);
}

void FMassiveLODSettingsLayout::OnPackageNameTextCommited(const FText& InText, ETextCommit::Type InCommitType)
{
	MassiveLODPackageName = InText.ToString();
}

FReply FMassiveLODSettingsLayout::OnSelectPackageNameClicked()
{
	TSharedRef<SDlgPickAssetPath> NewLayerDlg = 
		SNew(SDlgPickAssetPath)
		.Title(LOCTEXT("SelectMassiveLODPackage", "New Asset Name"))
		.DefaultAssetPath(FText::FromString(MassiveLODPackageName));

	if (NewLayerDlg->ShowModal() != EAppReturnType::Cancel)
	{
		MassiveLODPackageName = NewLayerDlg->GetFullAssetPath().ToString();
	}

	return FReply::Handled();
}

void FMassiveLODSettingsLayout::OnActorSelectionChanged(const TArray<UObject*>& NewSelection, bool bForceRefresh)
{
	GenerateNewMassiveLODPackageName();
}

EVisibility FMassiveLODSettingsLayout::GetRemeshingVisibility() const
{
	return bRemeshingSettingsWidgetVisibility ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility FMassiveLODSettingsLayout::GetAggregateVisibility() const
{
	return !bRemeshingSettingsWidgetVisibility ? EVisibility::Visible : EVisibility::Collapsed;
}

bool FMassiveLODSettingsLayout::IsRemeshingVisible()
{
	return bRemeshingSettingsWidgetVisibility;
}

bool FMassiveLODSettingsLayout::GetLevelEditorSelection(TArray<AActor*>& SelectedActors)
{
	bool bHasActors = false;
	bool bHasValidActors = true;

	USelection* ActorSelection = GEditor->GetSelectedActors();
	for(FSelectionIterator Iter(*ActorSelection); Iter; ++Iter)
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
		UE_LOG(LogSimplygonUtilities, Error, TEXT("No actors selected"));
		return false;
	}
	
	for (AActor* Actor : SelectedActors)
	{
		if (!Actor)
		{
			UE_LOG(LogSimplygonUtilities, Error, TEXT("Selection contains actors that are NULL"));
			return false;
		}
	}

	return bHasActors && bHasValidActors;
}

//Borrowed from MeshUtilites
bool PropagatePaintedColorsToRawMesh(UStaticMeshComponent* StaticMeshComponent, int32 LODIndex, FRawMesh& RawMesh, bool NegativeScale)
{
	UStaticMesh* StaticMesh = StaticMeshComponent->StaticMesh;

	if (StaticMesh->SourceModels.IsValidIndex(LODIndex) &&
		StaticMeshComponent->LODData.IsValidIndex(LODIndex) &&
		StaticMeshComponent->LODData[LODIndex].OverrideVertexColors != nullptr)	
	{
		FColorVertexBuffer& ColorVertexBuffer = *StaticMeshComponent->LODData[LODIndex].OverrideVertexColors;
		FStaticMeshSourceModel& SrcModel = StaticMesh->SourceModels[LODIndex];
		FStaticMeshRenderData& RenderData = *StaticMesh->RenderData;
		FStaticMeshLODResources& RenderModel = RenderData.LODResources[LODIndex];
		
		FIndexArrayView Indices = RenderModel.IndexBuffer.GetArrayView();

		if (Indices.Num() > 0 && 
			ColorVertexBuffer.GetNumVertices() == RenderModel.GetNumVertices())
		{
			int32 NumWedges = RawMesh.WedgeIndices.Num();
			if (Indices.Num() == NumWedges)
			{
				RawMesh.WedgeColors.SetNumUninitialized(NumWedges);

				for (int32 i = 0; i < NumWedges; ++i)
				{
					FColor WedgeColor = FColor::White;
					int32 Index = Indices[i];
					if (Index != INDEX_NONE)
					{
						WedgeColor = ColorVertexBuffer.VertexColor(Index);
					}

					// When mesh has negative scale, we have different corner order in triangles
					int32 TriIndex = i / 3;
					int32 CornerIndex = i % 3;
					const uint32 DestIndex = TriIndex * 3 + (NegativeScale ? (2 - CornerIndex) : CornerIndex);
					RawMesh.WedgeColors[DestIndex] = WedgeColor;
				}

				return true;
			}
			else
			{
				UE_LOG(LogSimplygonUtilities, Warning, TEXT("{%s} Wedge map size %d is wrong. Expected %d."), *StaticMesh->GetName(), RenderData.WedgeMap.Num(), RawMesh.WedgeIndices.Num());
			}
		}
	}

	return false;
}

bool GatherRenderDataFromLODResource_Depricated(FRawMesh* OutRawMesh, FStaticMeshLODResources* InLODResource)
{
	if (!InLODResource)
	{
		UE_LOG(LogSimplygonUtilities, Error, TEXT("LOD resource is NULL"));
		return false;
	}

	FStaticMeshLODResources* RenderData = InLODResource;

	uint32 TriangleCount = RenderData->GetNumTriangles();
	uint32 WedgeCount = TriangleCount * 3;
	uint32 VertexCount = RenderData->PositionVertexBuffer.GetNumVertices();
	int32 UVsCount = FMath::Min((int32)RenderData->VertexBuffer.GetNumTexCoords(), (int32)MAX_MESH_TEXTURE_COORDS);

	check(VertexCount == RenderData->VertexBuffer.GetNumVertices());
	FIndexArrayView Indices = RenderData->IndexBuffer.GetArrayView();

	// Vertex positions
	OutRawMesh->VertexPositions.Empty(VertexCount);
	OutRawMesh->VertexPositions.AddUninitialized(VertexCount);
	for (uint32 VertexIndex = 0; VertexIndex < VertexCount; ++VertexIndex)
	{
		OutRawMesh->VertexPositions[VertexIndex] = RenderData->PositionVertexBuffer.VertexPosition(VertexIndex);
	}

	// Per-Wedge Indices
	{
		OutRawMesh->WedgeIndices.Empty(WedgeCount);
		OutRawMesh->WedgeIndices.AddUninitialized(WedgeCount);
		for (uint32 WedgeIndex = 0; WedgeIndex < WedgeCount; ++WedgeIndex)
		{
			OutRawMesh->WedgeIndices[WedgeIndex] = Indices[WedgeIndex];
		}
	}

	// Per-Wedge Tangents
	{
		OutRawMesh->WedgeTangentZ.Empty(WedgeCount);
		OutRawMesh->WedgeTangentZ.AddUninitialized(WedgeCount);
		OutRawMesh->WedgeTangentX.Empty(WedgeCount);
		OutRawMesh->WedgeTangentX.AddUninitialized(WedgeCount);
		OutRawMesh->WedgeTangentY.Empty(WedgeCount);
		OutRawMesh->WedgeTangentY.AddUninitialized(WedgeCount);

		uint32 VertexIndex;
		for (uint32 WedgeIndex = 0; WedgeIndex < WedgeCount; ++WedgeIndex)
		{
			VertexIndex = Indices[WedgeIndex];
			OutRawMesh->WedgeTangentX[WedgeIndex] = (RenderData->VertexBuffer.VertexTangentX(VertexIndex)); //Tangents
			OutRawMesh->WedgeTangentY[WedgeIndex] = (RenderData->VertexBuffer.VertexTangentY(VertexIndex)); //Bitangents
			OutRawMesh->WedgeTangentZ[WedgeIndex] = (RenderData->VertexBuffer.VertexTangentZ(VertexIndex)); //Normals
		}
	}

	// Per-Wedge UVs 
	for (int32 UVIndex = 0; UVIndex < UVsCount; ++UVIndex)
	{
		OutRawMesh->WedgeTexCoords[UVIndex].Empty(WedgeCount);
		OutRawMesh->WedgeTexCoords[UVIndex].AddUninitialized(WedgeCount);

		uint32 VertexIndex;
		for (uint32 WedgeIndex = 0; WedgeIndex < WedgeCount; ++WedgeIndex)
		{
			VertexIndex = Indices[WedgeIndex];
			OutRawMesh->WedgeTexCoords[UVIndex][WedgeIndex] = RenderData->VertexBuffer.GetVertexUV(VertexIndex, UVIndex);
		}
	}

	//Per-Wedge Colors
	bool bHasVertexColors = RenderData->ColorVertexBuffer.GetNumVertices() > 0;
	if (bHasVertexColors)
	{
		OutRawMesh->WedgeColors.Empty(WedgeCount);
		OutRawMesh->WedgeColors.AddUninitialized(WedgeCount);
		uint32 VertexIndex;
		for (uint32 WedgeIndex = 0; WedgeIndex < WedgeCount; ++WedgeIndex)
		{
			VertexIndex = Indices[WedgeIndex];
			OutRawMesh->WedgeColors[WedgeIndex] = RenderData->ColorVertexBuffer.VertexColor(VertexIndex);
		}
	}

	//Per-face Material indices
	{
		OutRawMesh->FaceMaterialIndices.Empty(TriangleCount);
		OutRawMesh->FaceMaterialIndices.AddZeroed(TriangleCount);
		uint32 CurrentTriangleId = 0;
		for (int32 i = 0; i < RenderData->Sections.Num(); i++)
		{
			FStaticMeshSection& Section = RenderData->Sections[i];

			for (uint32 SectionTriIndex = 0; SectionTriIndex < Section.NumTriangles; ++SectionTriIndex)
			{
				OutRawMesh->FaceMaterialIndices[CurrentTriangleId++] = Section.MaterialIndex;
			}
		}
	}

	//SmoothingMasks
	OutRawMesh->FaceSmoothingMasks.Empty(TriangleCount);
	OutRawMesh->FaceSmoothingMasks.AddZeroed(TriangleCount);

	return true;
}

bool GatherRenderDataFromLODResource(FRawMesh* OutRawMesh, FStaticMeshLODResources* InLODResource, bool NegativeScale)
{
	if (!InLODResource)
	{
		UE_LOG(LogSimplygonUtilities, Error, TEXT("LOD resource is NULL"));
		return false;
	}

	FStaticMeshLODResources* RenderData = InLODResource;

	uint32 TriangleCount = RenderData->GetNumTriangles();
	uint32 WedgeCount = TriangleCount * 3;
	uint32 VertexCount = RenderData->PositionVertexBuffer.GetNumVertices();
	int32 UVsCount = FMath::Min((int32)RenderData->VertexBuffer.GetNumTexCoords(), (int32)MAX_MESH_TEXTURE_COORDS);

	check(VertexCount == RenderData->VertexBuffer.GetNumVertices());
	FIndexArrayView Indices = RenderData->IndexBuffer.GetArrayView();

	// Vertex positions
	OutRawMesh->VertexPositions.Empty(VertexCount);
	OutRawMesh->VertexPositions.AddUninitialized(VertexCount);
	for (uint32 VertexIndex = 0; VertexIndex < VertexCount; ++VertexIndex)
	{
		OutRawMesh->VertexPositions[VertexIndex] = RenderData->PositionVertexBuffer.VertexPosition(VertexIndex);
	}

	//Indices, Tangents, Vertex Colors
	{
		OutRawMesh->WedgeIndices.Empty(WedgeCount);
		OutRawMesh->WedgeIndices.AddUninitialized(WedgeCount);

		OutRawMesh->WedgeTangentZ.Empty(WedgeCount);
		OutRawMesh->WedgeTangentZ.AddUninitialized(WedgeCount);
		OutRawMesh->WedgeTangentX.Empty(WedgeCount);
		OutRawMesh->WedgeTangentX.AddUninitialized(WedgeCount);
		OutRawMesh->WedgeTangentY.Empty(WedgeCount); 
		OutRawMesh->WedgeTangentY.AddUninitialized(WedgeCount);

		bool bHasVertexColors = RenderData->ColorVertexBuffer.GetNumVertices() > 0;
		if (bHasVertexColors)
		{
			OutRawMesh->WedgeColors.Empty(WedgeCount);
			OutRawMesh->WedgeColors.AddUninitialized(WedgeCount);
		}

		for (uint32 TriIndex = 0; TriIndex < TriangleCount; ++TriIndex)
		{
			for (int32 CornerIndex = 0; CornerIndex < 3; ++CornerIndex)
			{
				int32 SourceIndex = Indices[TriIndex * 3 + CornerIndex];

				const uint32 DestIndex = TriIndex * 3 + (NegativeScale ? (2 - CornerIndex) : CornerIndex);
				
				OutRawMesh->WedgeIndices[DestIndex] = SourceIndex; //Indices

				OutRawMesh->WedgeTangentX[DestIndex] = (RenderData->VertexBuffer.VertexTangentX(SourceIndex)); //Tangents
				OutRawMesh->WedgeTangentY[DestIndex] = (RenderData->VertexBuffer.VertexTangentY(SourceIndex)); //Bitangents
				OutRawMesh->WedgeTangentZ[DestIndex] = (RenderData->VertexBuffer.VertexTangentZ(SourceIndex)); //Normals

				if (bHasVertexColors)
				{
					OutRawMesh->WedgeColors[DestIndex] = RenderData->ColorVertexBuffer.VertexColor(SourceIndex); //Vertex Colors
				}

			}
		}
	}

	// UVs 
	for (int32 UVIndex = 0; UVIndex < UVsCount; ++UVIndex)
	{
		OutRawMesh->WedgeTexCoords[UVIndex].Empty(WedgeCount);
		OutRawMesh->WedgeTexCoords[UVIndex].AddUninitialized(WedgeCount);

		uint32 VertexIndex;
		for (uint32 TriIndex = 0; TriIndex < TriangleCount; ++TriIndex)
		{
			for (int32 CornerIndex = 0; CornerIndex < 3; ++CornerIndex)
			{
				const uint32 DestIndex = NegativeScale ? (2 - CornerIndex) : CornerIndex;

				VertexIndex = Indices[TriIndex * 3 + CornerIndex];

				FVector2D UV = RenderData->VertexBuffer.GetVertexUV(VertexIndex, UVIndex);
				OutRawMesh->WedgeTexCoords[UVIndex][TriIndex * 3 + DestIndex].X = UV.X;
				OutRawMesh->WedgeTexCoords[UVIndex][TriIndex * 3 + DestIndex].Y = UV.Y;

				/*if (CornerIndex == 0)
				{
					OutRawMesh->WedgeTexCoords[UVIndex][TriIndex * 3 + CornerIndex] = FVector2D(0.0f, 0.0f);
				}
				else if (CornerIndex == 2)
				{
					OutRawMesh->WedgeTexCoords[UVIndex][TriIndex * 3 + CornerIndex] = FVector2D(1.0f, 0.0f);
				}
				else if (CornerIndex == 1)
				{
					OutRawMesh->WedgeTexCoords[UVIndex][TriIndex * 3 + CornerIndex] = FVector2D(1.0f, 1.0f);
				}*/



			}


		}

	}

	//Per-face Material indices
	{
		OutRawMesh->FaceMaterialIndices.Empty(TriangleCount);
		OutRawMesh->FaceMaterialIndices.AddZeroed(TriangleCount);
		uint32 CurrentTriangleId = 0;
		for(int32 i = 0; i < RenderData->Sections.Num(); i++)
		{
			FStaticMeshSection& Section = RenderData->Sections[i];

			for (uint32 SectionTriIndex = 0; SectionTriIndex < Section.NumTriangles; ++SectionTriIndex)
			{
				OutRawMesh->FaceMaterialIndices[CurrentTriangleId++] = Section.MaterialIndex;
			}
		}
	}

	//SmoothingMasks
	OutRawMesh->FaceSmoothingMasks.Empty(TriangleCount);
	OutRawMesh->FaceSmoothingMasks.AddZeroed(TriangleCount);

	return true;
}

bool GatherMaterialData(
	UStaticMeshComponent* InMeshComponent, 
	FMeshMaterialReductionData& OutMeshData,
	TArray<UMaterialInterface*>& OutMaterials,
	TArray<TArray<int32> >& RawMeshMaterialMap)
{
	if(!InMeshComponent)
	{
		UE_LOG(LogSimplygonUtilities, Error, TEXT("Input actor has mesh components that are NULL"));
		return false;
	}

	UStaticMesh* SrcMesh = InMeshComponent->StaticMesh;
	if(!SrcMesh)
	{
		UE_LOG(LogSimplygonUtilities, Error, TEXT("Static mesh is NULL"));
		return false;
	}

	UMaterialInterface* DefaultMaterial = Cast<UMaterialInterface>(UMaterial::GetDefaultMaterial(MD_Surface));

	//Need to store the unique material indices in order to re-map the material indices in each rawmesh
	TArray<int32> GlobalMaterialIndices;

	//Use the base mesh to harvest materials
	FStaticMeshLODResources* RenderData = &SrcMesh->RenderData->LODResources[0];

	for (const FStaticMeshSection& Section : RenderData->Sections) 
	{
		UMaterialInterface* MaterialToAdd = InMeshComponent->GetMaterial(Section.MaterialIndex);
		if (MaterialToAdd)
		{
			//Need to check if the resource exists..
			FMaterialResource* Resource = MaterialToAdd->GetMaterialResource(GMaxRHIFeatureLevel);
			if (!Resource)
			{
				MaterialToAdd = DefaultMaterial;
			}
		}
		else
		{
			MaterialToAdd = DefaultMaterial;
		}

		int32 MatId = OutMaterials.Add(MaterialToAdd);
		OutMeshData.NonFlattenMaterials.Add(MaterialToAdd);
		GlobalMaterialIndices.Add(MatId);
	}

	//Add a key as the current raw mesh along with its "global" material indices
	RawMeshMaterialMap.Add(GlobalMaterialIndices);

	return true;
}

bool GatherRawMeshData(
	UStaticMeshComponent* InMeshComponent, 
	TArray<struct FMeshMaterialReductionData*>& OutRawMeshes,
	ISimplygonUtilities& SimplygonUtilities)
{
	UStaticMesh* SrcMesh = InMeshComponent->StaticMesh;

	if(!SrcMesh)
	{
		UE_LOG(LogSimplygonUtilities, Error, TEXT("Static mesh is null"));
		return false;
	}

	if(SrcMesh->SourceModels.Num() < 1)
	{
		UE_LOG(LogSimplygonUtilities, Error, TEXT("No base render mesh found."));
		return false;
	}

	//Always access the base mesh
	FStaticMeshSourceModel& SrcModel = SrcMesh->SourceModels[0];
	FRawMesh* RawMesh ( new FRawMesh() );

	if ( !SrcModel.RawMeshBulkData->IsEmpty() )
	{
		//Find out if a negative scale has been applied to the object
		bool bNegativeScale = false;
		FVector Scale = InMeshComponent->ComponentToWorld.GetScale3D();
		int32 NegativeCount = 0;
		if (Scale[0] < 0) NegativeCount++;
		if (Scale[1] < 0) NegativeCount++;
		if (Scale[2] < 0) NegativeCount++;
		bNegativeScale = NegativeCount == 1 || NegativeCount == 3;

		//GatherRenderDataFromLODResource_Depricated(RawMesh, &SrcMesh->RenderData->LODResources[0]);
		GatherRenderDataFromLODResource(RawMesh, &SrcMesh->RenderData->LODResources[0], bNegativeScale);

		// Make sure the raw mesh is not irreparably malformed.
		if (!RawMesh->IsValidOrFixable())
		{
			UE_LOG(LogSimplygonUtilities, Error, TEXT("Raw mesh is corrupt for %s."), *(SrcMesh->GetName()) );
			return false;
		}

		//Transform the raw mesh to world space
		FTransform LocalToWorld = InMeshComponent->ComponentToWorld;
		
		//This should be applied to the tangents..
		/*FMatrix LocalToWorldInverseTranspose = LocalToWorld.ToMatrixWithScale();
		LocalToWorldInverseTranspose = LocalToWorldInverseTranspose.InverseFast().GetTransposed();*/
		
		FTransform LocalToWorldRot = FTransform::Identity;
		LocalToWorldRot.CopyRotationPart(LocalToWorld);
		
		for (FVector& Vertex : RawMesh->VertexPositions)
		{
			Vertex = LocalToWorld.TransformFVector4(Vertex);
		}

		for (FVector& Tangent : RawMesh->WedgeTangentX)
		{
			//Tangent = LocalToWorldInverseTranspose.TransformFVector4(Tangent);
			Tangent = LocalToWorldRot.TransformFVector4(Tangent);
		}

		for (FVector& Bitangent : RawMesh->WedgeTangentY)
		{
			//Bitangent = LocalToWorldInverseTranspose.TransformFVector4(Bitangent);
			Bitangent = LocalToWorldRot.TransformFVector4(Bitangent);
		}

		for (FVector& Normal : RawMesh->WedgeTangentZ)
		{
			//Normal = LocalToWorldInverseTranspose.TransformFVector4(Normal);
			Normal = LocalToWorldRot.TransformFVector4(Normal);
		}

		//Add vertex colors to the raw mesh
		PropagatePaintedColorsToRawMesh(InMeshComponent, 0, *RawMesh, bNegativeScale);

		if (!RawMesh->IsValid())
		{
			UE_LOG(LogSimplygonUtilities, Error, TEXT("Raw mesh is corrupt for %s"), *(SrcMesh->GetName()) );
			return false;
		}

		//Add the raw mesh to the collection of raw meshes. Store SrcMesh pointer for material analysis.
		FMeshMaterialReductionData* Data = SimplygonUtilities.CreateMeshReductionData(RawMesh, true);
		Data->bHasNegativeScale = bNegativeScale;
		Data->StaticMesh = SrcMesh;
		Data->StaticMeshComponent = InMeshComponent;
		OutRawMeshes.Add(Data);
	}

	return true;
}

float GetRawMeshSurfaceArea(const FRawMesh& RawMesh)
{
	float Area = 0;

	int32 NumFaces = RawMesh.WedgeIndices.Num() / 3;
	for (int32 FaceIndex = 0; FaceIndex < NumFaces; FaceIndex++)
	{
		const FVector& Corner1 = RawMesh.VertexPositions[RawMesh.WedgeIndices[FaceIndex * 3]];
		const FVector& Corner2 = RawMesh.VertexPositions[RawMesh.WedgeIndices[FaceIndex * 3 + 1]];
		const FVector& Corner3 = RawMesh.VertexPositions[RawMesh.WedgeIndices[FaceIndex * 3 + 2]];
		FVector Cross = FVector::CrossProduct(Corner2 - Corner1, Corner3 - Corner1);
		float TriangleArea = Cross.Size() / 2.0f;
		Area += TriangleArea;
	}

	return Area;
}

/*
 * Process all meshes which are going to be combined into a single mesh and find which materials could be reused
 * between meshes.
 */
void RemapMassiveLODMaterials(
	const TArray<UMaterialInterface*>& InMaterials,
	const TArray<FMeshMaterialReductionData*>& InMeshData,
	const TArray<TArray<int32> >& InMaterialMap,
	const FSimplygonMaterialLODSettings& InMaterialLODSettings,
	TArray<bool>& OutMeshShouldBakeVertexData,
	TArray<bool>& OutMeshHasNonRepeatingMaterial,
	TArray<bool>& OutMeshUseActorData,
	TArray<TArray<int32> >& OutMaterialMap,
	TArray<UMaterialInterface*>& OutMaterials)
{
	int32 NumMeshes = InMeshData.Num();

	// Gather material properties
	struct MaterialProps
	{
		int32 NumTexCoords;
		bool UseVertexColors;
		bool UseActorData;
		bool NonRepeating;

//		MaterialProps() : NumTexCoords(0), UseVertexColors(false), UseActorData(false), NonRepeating(false) {}
	};
	TMap<UMaterialInterface*, MaterialProps> Props;

	for (int32 MaterialIndex = 0; MaterialIndex < InMaterials.Num(); MaterialIndex++)
	{
		UMaterialInterface* Material = InMaterials[MaterialIndex];
		if (Props.Find(Material) != nullptr)
		{
			// This material was already processed.
			continue;
		}
		if (!InMaterialLODSettings.bBakeVertexData)
		{
			// We are not baking vertex data at all, don't analyze materials.
			Props.Add(Material);
			continue;
		}
		MaterialProps Data;
		FSimplygonUtilities::AnalyzeMaterial(Material, InMaterialLODSettings, Data.NumTexCoords, Data.UseVertexColors, Data.NonRepeating, Data.UseActorData);
		Props.Add(Material, Data);
	}

	// Check mesh properties.
	TArray<bool> MeshHasVertexColors;
	MeshHasVertexColors.AddZeroed(NumMeshes);
	TArray<bool> MeshHasLargeUV;
	MeshHasLargeUV.AddZeroed(NumMeshes);
	if (InMaterialLODSettings.bBakeVertexData)
	{
		for (int32 MeshIndex = 0; MeshIndex < NumMeshes; MeshIndex++)
		{
			const FRawMesh* Mesh = InMeshData[MeshIndex]->Mesh;
			// Check which meshes has vertex colors.
			bool bHasVertexColors = false;
			for (int32 WedgeIndex = 0; WedgeIndex < Mesh->WedgeColors.Num(); WedgeIndex++)
			{
				if (Mesh->WedgeColors[WedgeIndex] != FColor::White)
				{
					bHasVertexColors = true;
					break;
				}
			}
			MeshHasVertexColors[MeshIndex] = bHasVertexColors;
			// Check mesh UV range.
			bool bHasLargeUVs = false;
			for (int32 WedgeIndex = 0; WedgeIndex < Mesh->WedgeTexCoords[0].Num(); WedgeIndex++)
			{
				const FVector2D& TexCoord = Mesh->WedgeTexCoords[0][WedgeIndex];
				if (TexCoord.X < 0 || TexCoord.X > 1 || TexCoord.Y < 0 || TexCoord.Y > 1)
				{
					bHasLargeUVs = true;
					break;
				}
			}
			MeshHasLargeUV[MeshIndex] = bHasLargeUVs;
		}
	}

	// Build list of mesh's material properties.
	OutMeshShouldBakeVertexData.Empty();
	OutMeshShouldBakeVertexData.AddZeroed(NumMeshes);
	OutMeshHasNonRepeatingMaterial.Empty();
	OutMeshHasNonRepeatingMaterial.AddZeroed(NumMeshes);
	OutMeshUseActorData.Empty();
	OutMeshUseActorData.AddZeroed(NumMeshes);

	for (int32 MeshIndex = 0; MeshIndex < NumMeshes; MeshIndex++)
	{
		const TArray<int32>& MeshMaterialMap = InMaterialMap[MeshIndex];
		int32 NumTexCoords = 0;
		bool bUseVertexColors = false;
		bool bHasNonRepeatingMaterial = false;
		bool bUsePerActorData = false;
		// Accumulate data of all materials.
		for (int32 LocalMaterialIndex = 0; LocalMaterialIndex < MeshMaterialMap.Num(); LocalMaterialIndex++)
		{
			UMaterialInterface* Material = InMaterials[MeshMaterialMap[LocalMaterialIndex]];
			NumTexCoords = FMath::Max(NumTexCoords, Props[Material].NumTexCoords);
			bUseVertexColors |= Props[Material].UseVertexColors;
			bUsePerActorData |= Props[Material].UseActorData;
			bHasNonRepeatingMaterial |= Props[Material].NonRepeating;
		}
		// Take into account presence of vertex colors and large UVs in mesh.
		bUseVertexColors &= MeshHasVertexColors[MeshIndex];
		bHasNonRepeatingMaterial &= MeshHasLargeUV[MeshIndex];
		// Store data.
		MeshHasVertexColors[MeshIndex] = bUseVertexColors;
		if (InMaterialLODSettings.bBakeActorData)
		{
			OutMeshUseActorData[MeshIndex] = bUsePerActorData;
		}
		OutMeshShouldBakeVertexData[MeshIndex] = bUseVertexColors || (NumTexCoords >= 2);
		OutMeshHasNonRepeatingMaterial[MeshIndex] = bHasNonRepeatingMaterial;
	}

	// Build new material map.
	// Structure used to simplify material merging.
	struct MeshMaterialInfo
	{
		UMaterialInterface* Material;
		UStaticMesh* Mesh;
		bool bHasVertexColors;
		bool bHasNonRepeatingPattern;

		MeshMaterialInfo(UMaterialInterface* InMaterial, UStaticMesh* InMesh, bool bInHasVertexColors, bool bInHasNonRepeatingPattern)
		: Material(InMaterial)
		, Mesh(InMesh)
		, bHasVertexColors(bInHasVertexColors)
		, bHasNonRepeatingPattern(bInHasNonRepeatingPattern)
		{
		}

		bool operator==(const MeshMaterialInfo& Other) const
		{
			return Material == Other.Material && Mesh == Other.Mesh && bHasVertexColors == Other.bHasVertexColors
				&& bHasNonRepeatingPattern == Other.bHasNonRepeatingPattern;
		}
	};

	TArray<MeshMaterialInfo> MeshMaterialData;
	OutMaterialMap.Empty();
	for (int32 MeshIndex = 0; MeshIndex < NumMeshes; MeshIndex++)
	{
		const TArray<int32>& MeshMaterialMap = InMaterialMap[MeshIndex];
		TArray<int32>& NewMeshMaterialMap = *new (OutMaterialMap) TArray<int32>();
		UStaticMesh* StaticMesh = InMeshData[MeshIndex]->StaticMesh;

		if (!MeshHasVertexColors[MeshIndex] && !OutMeshUseActorData[MeshIndex])
		{
			// No vertex colors and no per-actor data - could merge materials with other meshes.
			bool bHasNonRepeatingPattern = OutMeshHasNonRepeatingMaterial[MeshIndex];
			if (!OutMeshShouldBakeVertexData[MeshIndex] && !bHasNonRepeatingPattern)
			{
				// Set to 'nullptr' if don't need to bake vertex data to be able to merge materials with any meshes
				// which don't require vertex data baking too. OutMeshShouldBakeVertexData could be 'true' if material
				// has multiple UVs in use.
				StaticMesh = nullptr;
			}

			for (int32 LocalMaterialIndex = 0; LocalMaterialIndex < MeshMaterialMap.Num(); LocalMaterialIndex++)
			{
				MeshMaterialInfo Data(InMaterials[MeshMaterialMap[LocalMaterialIndex]], StaticMesh, false, bHasNonRepeatingPattern);
				int32 Index = MeshMaterialData.Find(Data);
				if (Index == INDEX_NONE)
				{
					// Not found, add new entry.
					Index = MeshMaterialData.Add(Data);
				}
				NewMeshMaterialMap.Add(Index);
			}
		}
		else
		{
			// Mesh with vertex data baking, and with vertex colors - don't share materials at all.
			for (int32 LocalMaterialIndex = 0; LocalMaterialIndex < MeshMaterialMap.Num(); LocalMaterialIndex++)
			{
				MeshMaterialInfo Data(InMaterials[MeshMaterialMap[LocalMaterialIndex]], StaticMesh, true, false);
				int32 Index = MeshMaterialData.Add(Data);
				NewMeshMaterialMap.Add(Index);
			}
		}
	}

	// Build new material list - simply extract MeshMaterialData[i].Material.
	OutMaterials.Empty();
	OutMaterials.AddUninitialized(MeshMaterialData.Num());
	for (int32 MaterialIndex = 0; MaterialIndex < MeshMaterialData.Num(); MaterialIndex++)
	{
		OutMaterials[MaterialIndex] = MeshMaterialData[MaterialIndex].Material;
	}
}

/*
 * Analyze whole massive LOD scene and reduce size of materials which takes too little space.
 */
void ComputeMassiveLODMaterialDimensions(
	TArray<FMeshMaterialReductionData*>& MeshData,
	const TArray<UMaterialInterface*>& InMaterials,
	const TArray<TArray<int32> >& InMaterialMap,
	const FSimplygonMaterialLODSettings& InMaterialLODSettings,
	TArray<FIntPoint>& OutDimensions)
{
	// Compute areas of all meshes.
	float TotalArea = 0.0f;
	TArray<float> MeshArea;
	MeshArea.AddUninitialized(MeshData.Num());
	for (int32 MeshIndex = 0; MeshIndex < MeshData.Num(); MeshIndex++)
	{
		float Area = GetRawMeshSurfaceArea(*MeshData[MeshIndex]->Mesh);
		MeshArea[MeshIndex] = Area;
		TotalArea += Area;
	}

	// Compute desired material sizes.
	OutDimensions.SetNumZeroed(InMaterials.Num());
	const int32 InitialTextureWidth = FSimplygonMaterialLODSettings::GetTextureResolutionFromEnum(InMaterialLODSettings.TextureWidth);
	const int32 InitialTextureHeight = FSimplygonMaterialLODSettings::GetTextureResolutionFromEnum(InMaterialLODSettings.TextureHeight);
	// Don't shrink textures beyond this value.
	const int32 MinTextureSize = 64;

	for (int32 MaterialIndex = 0; MaterialIndex < InMaterials.Num(); MaterialIndex++)
	{
		// Find a largest mesh which uses this material.
		int32 LargestMeshIndex = -1;
		float LargestMeshArea = 0.0f;
		for (int32 MeshIndex = 0; MeshIndex < MeshData.Num(); MeshIndex++)
		{
			const TArray<int32>& GlobalMaterialIndices = InMaterialMap[MeshIndex];
			for (int32 LocalMaterialIndex = 0; LocalMaterialIndex < GlobalMaterialIndices.Num(); LocalMaterialIndex++)
			{
				if (GlobalMaterialIndices[LocalMaterialIndex] == MaterialIndex)
				{
					// This mesh uses the material.
					float Area = MeshArea[MeshIndex];
					if (Area >= LargestMeshArea)
					{
						LargestMeshArea = Area;
						LargestMeshIndex = MeshIndex; 
					}
					// Check next mesh.
					break;
				}
			}
		}
		float NormalizedArea = LargestMeshArea / TotalArea;
		int32 TextureWidth = InitialTextureWidth;
		int32 TextureHeight = InitialTextureHeight;
		int32 Fraction = 2;
		// 1/2 dimension (1/4 area) for texture which occupies 1/16 part of scene,
		// 1/4 dimension (1/16 area) for texture which occupies 1/64 part of scene,
		// 1/N dimension (1/N^2 area) for texture which occupies 1/(N^2*4) part of scene.
		while (NormalizedArea < 1.0f / (Fraction * Fraction * 4))
		{
			if (TextureWidth <= MinTextureSize || TextureHeight <= MinTextureSize)
			{
				// Texture is going to be too small, stop iterating.
				break;
			}
			TextureWidth >>= 1;
			TextureHeight >>= 1;
			Fraction *= 2;
		}
		OutDimensions[MaterialIndex] = FIntPoint(TextureWidth, TextureHeight);
	}
}

/*
 * Flatten all materials with baking vertex data when needed and taking into account material sharing.
 * All flatten materials will be put to the first item in MeshData array.
 * InMaterialMap could be modified after processing when new mesh UVs are generated.
 */
void FlattenMassiveLODMaterials(
	TArray<FMeshMaterialReductionData*>& MeshData,
	const TArray<UMaterialInterface*>& InMaterials,
	TArray<TArray<int32> >& InMaterialMap,
	const FSimplygonMaterialLODSettings& InMaterialLODSettings,
	const TArray<bool>& InMeshShouldBakeVertexData,
	const TArray<bool>& InMeshHasNonRepeatingMaterial,
	const TArray<bool>& InMeshUseActorData,
	TArray<FIntPoint>& InMaterialDimensions,
	ISimplygonUtilities& SimplygonUtilities)
{
	const int32 TextureWidth = FSimplygonMaterialLODSettings::GetTextureResolutionFromEnum(InMaterialLODSettings.TextureWidth);
	const int32 TextureHeight = FSimplygonMaterialLODSettings::GetTextureResolutionFromEnum(InMaterialLODSettings.TextureHeight);

	// Prepare container for cached shaders.
	TMap<UMaterialInterface*, FExportMaterialProxyCache> CachedShaders;
	TArray<UMaterialInterface*> MaterialParents;
	CachedShaders.Empty(InMaterials.Num());
	MaterialParents.AddZeroed(InMaterials.Num());
	// Pre-fill these data sets.
	for (int32 MaterialIndex = 0; MaterialIndex < InMaterials.Num(); MaterialIndex++)
	{
		UMaterialInterface* CurrentMaterial = InMaterials[MaterialIndex];
		UMaterialInterface* ParentMaterial = CurrentMaterial->GetMaterial();

		// Check if we already have cached compiled shader for this material.
		FExportMaterialProxyCache* CachedShader = CachedShaders.Find(CurrentMaterial);
		if (CachedShader == nullptr)
		{
			CachedShader = &CachedShaders.Add(CurrentMaterial);
		}
		MaterialParents[MaterialIndex] = ParentMaterial;
	}

	for (int32 MaterialIndex = 0; MaterialIndex < InMaterials.Num(); MaterialIndex++)
	{
		UMaterialInterface* CurrentMaterial = InMaterials[MaterialIndex];

		// Check if we already have cached compiled shader for this material.
		FExportMaterialProxyCache& CachedShader = CachedShaders[CurrentMaterial];

		// Find a mesh which uses this material.
		int32 UsedMeshIndex = 0;
		int32 LocalMaterialIndex = 0;
		FMeshMaterialReductionData* UsedMeshData = nullptr;
		for (int32 MeshIndex = 0; MeshIndex < MeshData.Num() && UsedMeshData == nullptr; MeshIndex++)
		{
			const TArray<int32>& GlobalMaterialIndices = InMaterialMap[MeshIndex];
			LocalMaterialIndex = GlobalMaterialIndices.Find(MaterialIndex);
			if (LocalMaterialIndex >= 0)
			{
				UsedMeshIndex = MeshIndex;
				UsedMeshData = MeshData[MeshIndex];
				break;
			}
		}
		check(UsedMeshData != nullptr);

		const FIntPoint& TextureSize = InMaterialDimensions[MaterialIndex];
		FFlattenMaterial* FlattenMaterial = FSimplygonUtilities::CreateFlattenMaterial(InMaterialLODSettings, TextureSize.X, TextureSize.Y);

		if (InMeshShouldBakeVertexData[UsedMeshIndex] || InMeshUseActorData[UsedMeshIndex])
		{
			// Generate new non-overlapping texture coordinates for mesh
			if (UsedMeshData->TexcoordBounds.bIsValid == false)
			{
				// Optimization: find if we already have UVs generated for the same UStaticMesh in scene
				for (int32 MeshIndex = 0; MeshIndex < MeshData.Num(); MeshIndex++)
				{
					FMeshMaterialReductionData* CheckData = MeshData[MeshIndex];
					if (CheckData->StaticMesh == UsedMeshData->StaticMesh &&
						CheckData->bHasNegativeScale == UsedMeshData->bHasNegativeScale &&
						CheckData->NewUVs.Num() > 0)
					{
						// Copy data
						UsedMeshData->TexcoordBounds = CheckData->TexcoordBounds;
						UsedMeshData->NewUVs = CheckData->NewUVs;
						break;
					}
				}
				if (UsedMeshData->NewUVs.Num() == 0)
				{
					// UVs wasn't built yet - build it. Use original texture dimensions used for while massive LOD scene.
					SimplygonUtilities.SimplygonGenerateUniqueUVs(*UsedMeshData->Mesh, TextureWidth, TextureHeight, UsedMeshData->NewUVs);
				}
			}
			// Generate FFlattenMaterial using mesh geometry
			FSimplygonUtilities::ExportMaterial(
				CurrentMaterial,
				UsedMeshData,
				LocalMaterialIndex,
				*FlattenMaterial,
				&CachedShader);
			// Put all separate materials into a single one
			if (LocalMaterialIndex > 0)
			{
				TArray<int32>& GlobalMaterialIndices = InMaterialMap[UsedMeshIndex];
				// Find first material for this mesh
				int32 FirstMaterialIndex = GlobalMaterialIndices[0];
				FFlattenMaterial& FirstMaterial = MeshData[0]->FlattenMaterials[FirstMaterialIndex];
				// Merge materials
				FSimplygonUtilities::MergeFlattenMaterials(FirstMaterial, *FlattenMaterial);
				// Release recently generated material and modify material map to point to single one
				FlattenMaterial->ReleaseData();
				GlobalMaterialIndices[LocalMaterialIndex] = FirstMaterialIndex;
			#if 0
				// Debug: save combined material
				if (LocalMaterialIndex == GlobalMaterialIndices.Num() - 1)
				{
					FString MaterialName = FString::Printf(TEXT("mesh%d-combined"), UsedMeshIndex);
					FSimplygonUtilities::SaveFlattenMaterial(FirstMaterial, *MaterialName);
				}
			#endif
			}
		}
		else if (InMeshHasNonRepeatingMaterial[UsedMeshIndex])
		{
			if (UsedMeshData->TexcoordBounds.bIsValid == false)
			{
				SimplygonUtilities.GetTextureCoordinateBoundsForRawMesh(*UsedMeshData->Mesh, UsedMeshData->TexcoordBounds);
			}
			// Generate simple FFlattenMaterial with TexcoordBounds information
			FSimplygonUtilities::ExportMaterial(
				CurrentMaterial,
				UsedMeshData->TexcoordBounds,
				*FlattenMaterial,
				&CachedShader);
			// Copy texcoord bounds for all other meshes (identical) which shares this material
			for (int32 MeshIndex = 0; MeshIndex < MeshData.Num(); MeshIndex++)
			{
				FMeshMaterialReductionData* Data = MeshData[MeshIndex];
				if (!Data->TexcoordBounds.bIsValid)
				{
					const TArray<int32>& GlobalMaterialIndices = InMaterialMap[MeshIndex];
					if (GlobalMaterialIndices.Find(MaterialIndex) >= 0)
					{
						// Current material is used in this mesh too, copy texcoord bounds for it
						// to produce correct massive LOD mesh
						Data->TexcoordBounds = UsedMeshData->TexcoordBounds;
					}
				}
			}

		}
		else
		{
			// Generate simple FFlattenMaterial
			FSimplygonUtilities::ExportMaterial(
				CurrentMaterial,
				*FlattenMaterial,
				&CachedShader);
		}
		// Adding all materials to the first mesh
		MeshData[0]->FlattenMaterials.Add(FlattenMaterial);
		check(MeshData[0]->FlattenMaterials.Num() == MaterialIndex + 1);

		// Check if this material will be used later. If not - release shader.
		int32 LastMaterialIndex = InMaterials.FindLast(CurrentMaterial);
		if (LastMaterialIndex <= MaterialIndex)
		{
			// This material is not used anymore. Check if we need to keep it as parent of another material.
			// Find first and last materials which use the same parent.
			UMaterialInterface* Parent = MaterialParents[MaterialIndex];
			int32 FirstParent = -1;
			int32 LastParent = -1;
			for (int32 CheckIndex = 0; CheckIndex < MaterialParents.Num(); CheckIndex++)
			{
				if (MaterialParents[CheckIndex] == Parent)
				{
					if (FirstParent == -1)
					{
						FirstParent = CheckIndex;
					}
					LastParent = CheckIndex;
				}
			}
			check(FirstParent != -1 && LastParent != -1);
			if (InMaterials[FirstParent] != CurrentMaterial)
			{
				// FirstParent material is used as storage for compiled shaders, and it is different from current one.
				// Current material could be released.
				CachedShader.Release();
			}
			if (LastParent <= MaterialIndex)
			{
				// This parent will not be used anymore, release FirstParent's cache because it will not be used anymore.
				// That could be CurrentMaterial, or another material.
				CachedShaders[InMaterials[FirstParent]].Release();
			}
		}
	}
	// Adjust emissive channels
	MeshData[0]->AdjustEmissiveChannels();
}

bool FMassiveLODSettingsLayout::GenerateMassiveLODMesh(
	const TArray<AActor*>& SourceActors,
	const struct FSimplygonRemeshingSettings& InMassiveLODSettings,
	FVector& OutProxyLocation,
	const FString& ProxyBasePackageName,
	TArray<UObject*>& OutAssetsToSync)
{
	TArray<UStaticMeshComponent*>					ComponentsToMerge;
	TArray<ALandscapeProxy*>						LandscapesToMerge;
	TArray<FMeshMaterialReductionData*>				InputDataArray;
	TArray<UMaterialInterface*>						StaticMeshMaterials;
	TArray<TArray<int32> >							MaterialMap;
	UMaterialInterface*								OutMassiveLODMaterial = NULL;
	FBox											ProxyBounds(0);

	IMeshUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");
	IMeshMaterialReduction* SimplygonInterface = MeshUtilities.GetMeshMaterialReductionInterface();
	ISimplygonUtilities& SimplygonUtilities = FModuleManager::Get().LoadModuleChecked<ISimplygonUtilities>("SimplygonUtilities");

	if (!SimplygonInterface || !SimplygonUtilities.IsAvailable())
	{
		UE_LOG(LogSimplygonUtilities, Error, TEXT("Failed to fetch Simplygon interface"));
		return false;
	}

	//Collect the components of the corresponding actor
	for (AActor* Actor : SourceActors)
	{
		ALandscapeProxy* LandscapeActor = Cast<ALandscapeProxy>(Actor);
		if (LandscapeActor)
		{
			LandscapesToMerge.Add(LandscapeActor);
		}
		else
		{
			TArray<UStaticMeshComponent*> Components;
			Actor->GetComponents<UStaticMeshComponent>(Components);
			// TODO: support instanced static meshes
			Components.RemoveAll([](UStaticMeshComponent* Val){ return Val->IsA(UInstancedStaticMeshComponent::StaticClass()); });
			//
			ComponentsToMerge.Append(Components);
		}
	}

	//Convert static mesh components
	MaterialMap.Empty(ComponentsToMerge.Num());
	for (int32 ComponentIndex = 0; ComponentIndex < ComponentsToMerge.Num(); ++ComponentIndex)
	{
		UStaticMeshComponent* MeshComponent = ComponentsToMerge[ ComponentIndex ];
		//Store the bounds for each component
		ProxyBounds	 += MeshComponent->Bounds.GetBox();

		bool bSuccess = GatherRawMeshData(MeshComponent, InputDataArray, SimplygonUtilities);

		if (bSuccess)
		{
			GatherMaterialData(MeshComponent, *InputDataArray.Last(), StaticMeshMaterials, MaterialMap);
		}
		check(InputDataArray.Num() == MaterialMap.Num());
	}

	bool bValid = InputDataArray.Num() > 0;
	if (!bValid)
	{
		UE_LOG(LogSimplygonUtilities, Error, TEXT("Number of valid meshes are zero, processing interrupted"));
		return false;
	}

	const FSimplygonMaterialLODSettings& MaterialLODSettings = InMassiveLODSettings.MaterialLODSettings;

	// Share materials whenever possible.
	TArray<bool> MeshShouldBakeVertexData;
	TArray<bool> MeshHasNonRepeatingMaterial;
	TArray<bool> MeshUseActorData;
	TArray<TArray<int32> > NewMaterialMap;
	TArray<UMaterialInterface*> NewStaticMeshMaterials;
	RemapMassiveLODMaterials(
		StaticMeshMaterials,
		InputDataArray,
		MaterialMap,
		MaterialLODSettings,
		MeshShouldBakeVertexData,
		MeshHasNonRepeatingMaterial,
		MeshUseActorData,
		NewMaterialMap,
		NewStaticMeshMaterials);
	// Use shared material data.
	Exchange(MaterialMap, NewMaterialMap);
	Exchange(StaticMeshMaterials, NewStaticMeshMaterials);

	// Put all materials to InputDataArray[0] and find resulting BlendMode. Note that massive lOD processors
	// doesn't have multiple output materials feature, so we are using InputDataArray[0] for resulting material.
	InputDataArray[0]->NonFlattenMaterials = StaticMeshMaterials;
	InputDataArray[0]->BuildOutputMaterialMap(MaterialLODSettings, false);

	// Remove component references for those meshes which don't need them
	for (int32 MeshIndex = 0; MeshIndex < InputDataArray.Num(); MeshIndex++)
	{
		if (!MeshUseActorData[MeshIndex])
		{
			InputDataArray[MeshIndex]->StaticMeshComponent = nullptr;
		}
	}

	// Compute preferred sizes of scene materials.
	TArray<FIntPoint> MaterialDimensions;
	ComputeMassiveLODMaterialDimensions(
		InputDataArray,
		StaticMeshMaterials,
		MaterialMap,
		MaterialLODSettings,
		MaterialDimensions);

	// Convert materials into flatten materials. Can't use FMeshMaterialReductionData::BuildFlattenMaterials
	// here because of binding materials to different meshes.
	FlattenMassiveLODMaterials(
		InputDataArray,
		StaticMeshMaterials,
		MaterialMap,
		MaterialLODSettings,
		MeshShouldBakeVertexData,
		MeshHasNonRepeatingMaterial,
		MeshUseActorData,
		MaterialDimensions,
		SimplygonUtilities);

	//Convert landscapes
	for (ALandscapeProxy* Landscape : LandscapesToMerge)
	{
		FRawMesh* LandscapeMesh = new FRawMesh();
		int32 MatIdx = StaticMeshMaterials.Num();

		if (Landscape->ExportToRawMesh(INDEX_NONE, *LandscapeMesh))
		{
			FMeshMaterialReductionData* Data = SimplygonUtilities.CreateMeshReductionData(LandscapeMesh, true);
			InputDataArray.Add(Data);
			// Landscape has one unique material
			FFlattenMaterial* LandscapeMaterial = new FFlattenMaterial();
			TArray<int32> RawMeshMaterialMap;
			RawMeshMaterialMap.Add(MatIdx);
			MatIdx++;
			MaterialMap.Add(RawMeshMaterialMap);
			// This is texture resolution for a landscape, probably need to be calculated using landscape size
			LandscapeMaterial->DiffuseSize = FIntPoint(1024, 1024);
			// FIXME: Landscape material exporter currently renders world space normal map, so it can't be merged with other meshes normal maps
			LandscapeMaterial->NormalSize = FIntPoint::ZeroValue;

			// Use only landscape primitives for texture flattening
			TSet<FPrimitiveComponentId> PrimitivesToHide;
			for (TObjectIterator<UPrimitiveComponent> It; It; ++It)
			{
				UPrimitiveComponent* PrimitiveComp = *It;
				const bool bTargetPrim = PrimitiveComp->GetOuter() == Landscape;
				if (!bTargetPrim && PrimitiveComp->IsRegistered() && PrimitiveComp->SceneProxy)
				{
					PrimitivesToHide.Add(PrimitiveComp->SceneProxy->GetPrimitiveComponentId());
				}
			}

			// Prepare landscape material and store data into FMeshMaterialReductionData
			FMaterialUtilities::ExportLandscapeMaterial(Landscape, PrimitivesToHide, *LandscapeMaterial);
			FBox2D DummyBounds;
			DummyBounds.Min.Set(0, 0);
			DummyBounds.Max.Set(1, 1);
			Data->TexcoordBounds = DummyBounds;
			Data->FlattenMaterials.Add(LandscapeMaterial);
			Data->NonFlattenMaterials.Add(Landscape->LandscapeMaterial);

			//Store the bounds for each component
			ProxyBounds+= Landscape->GetComponentsBoundingBox(true);
		}
		else
		{
			delete LandscapeMesh;
		}
	}

	//For each raw mesh, re-map the material indices from Local to Global material indices space
	for (int32 RawMeshIndex = 0; RawMeshIndex < InputDataArray.Num(); ++RawMeshIndex)
	{
		const TArray<int32>& GlobalMaterialIndices = MaterialMap[RawMeshIndex];
		TArray<int32>& MaterialIndices = InputDataArray[ RawMeshIndex ]->Mesh->FaceMaterialIndices;
		int32 MaterialIndicesCount = MaterialIndices.Num();

		for(int32 TriangleIndex = 0; TriangleIndex < MaterialIndicesCount ; ++TriangleIndex)
		{
			int32 LocalMaterialIndex = MaterialIndices[TriangleIndex];
			int32 GlobalMaterialIndex = GlobalMaterialIndices[LocalMaterialIndex];

			//Assign the new material index to the raw mesh
			MaterialIndices[TriangleIndex] = GlobalMaterialIndex;
		}
	}

	FRawMesh			OutRawMesh;
	FFlattenMaterial	ProxyFlattenMaterial;

	if (InMassiveLODSettings.bUseAggregateLOD)
	{
		SimplygonInterface->AggregateLOD(InputDataArray,
			InMassiveLODSettings, 
			OutRawMesh,
			ProxyFlattenMaterial); 
	}
	else
	{
		SimplygonInterface->ProxyLOD(InputDataArray,
			InMassiveLODSettings, 
			OutRawMesh,
			ProxyFlattenMaterial);
	}

	if (!OutRawMesh.IsValid())
	{
		UE_LOG(LogSimplygonUtilities, Error, TEXT("RawMesh for Massive LOD is corrupt"));
		return false;
	}

	//Transform the MassiveLOD
	OutProxyLocation = ProxyBounds.GetCenter();
	for(FVector& Vertex : OutRawMesh.VertexPositions)
	{
		Vertex-= OutProxyLocation;
	}

	// Purge unneeded UVs and rebuild lightmap UVs
	const int32 LightmapUVIndex = 1;
	const int32 LightmapResolution = 256;
	for (int32 UVIndex = 1; UVIndex < MAX_MESH_TEXTURE_COORDS; UVIndex++)
	{
		OutRawMesh.WedgeTexCoords[UVIndex].Empty();
	}
	SimplygonInterface->GenerateUniqueUVs(OutRawMesh, 0.8f, LightmapResolution, LightmapResolution, 3.0f, OutRawMesh.WedgeTexCoords[LightmapUVIndex]);

	//
	// Base asset name for a new assets
	//
	const FString AssetBaseName = FPackageName::GetShortName(ProxyBasePackageName);
	const FString AssetBasePath = FPackageName::IsShortPackageName(ProxyBasePackageName) ? 
		FPackageName::FilenameToLongPackageName(FPaths::GameContentDir()) : (FPackageName::GetLongPackagePath(ProxyBasePackageName) + TEXT("/"));

	// Construct proxy static mesh
	UPackage* MeshPackage = NULL;
	if (MeshPackage == nullptr)
	{
		MeshPackage = CreatePackage(NULL, *(AssetBasePath + TEXT("SM_") + AssetBaseName));
		MeshPackage->FullyLoad();
		MeshPackage->Modify();
	}

	FString UniqueName = MakeUniqueObjectName(MeshPackage, UTexture2D::StaticClass(), *(AssetBaseName)).ToString();

	// Construct proxy material
	OutMassiveLODMaterial = FMaterialUtilities::SgCreateMaterial(ProxyFlattenMaterial, MeshPackage, UniqueName, RF_Public | RF_Standalone);
	//SimplygonUtilities.SaveMaterial(OutMassiveLODMaterial); //todo: Should not "autosave" massiveLOD assets

	//Build a new StaticMesh for the MassiveLOD
	UStaticMesh* MassiveLODStaticMesh = NewObject<UStaticMesh>(MeshPackage, FName(*(TEXT("SM_") + UniqueName)), RF_Public | RF_Standalone);
	MassiveLODStaticMesh->InitResources();

	if (!MassiveLODStaticMesh)
	{
		UE_LOG(LogSimplygonUtilities, Error, TEXT("Failed to initialize new Static Mesh"));
		return false;
	}

	FString OutputPath = MassiveLODStaticMesh->GetPathName();

	//Make sure it has a new lighting guid
	MassiveLODStaticMesh->LightingGuid = FGuid::NewGuid();

	// Set it to use textured lightmaps. Note that Build Lighting will do the error-checking (texcoordindex exists for all LODs, etc).
	MassiveLODStaticMesh->LightMapResolution = LightmapResolution;
	MassiveLODStaticMesh->LightMapCoordinateIndex = LightmapUVIndex;

	FStaticMeshSourceModel* SrcModel = new(MassiveLODStaticMesh->SourceModels) FStaticMeshSourceModel();
	// Don't allow the engine to calculate stuff
	SrcModel->BuildSettings.bRecomputeNormals = false;
	SrcModel->BuildSettings.bRecomputeTangents = false;
	SrcModel->BuildSettings.bRemoveDegenerates = false;
	SrcModel->BuildSettings.bUseFullPrecisionUVs = false;
	SrcModel->BuildSettings.bGenerateLightmapUVs = false;
	SrcModel->RawMeshBulkData->SaveRawMesh(OutRawMesh);

	//Assign the MassiveLOD material to the StaticMesh
	MassiveLODStaticMesh->Materials.Add(OutMassiveLODMaterial);

	MassiveLODStaticMesh->Build();
	MassiveLODStaticMesh->PostEditChange();

	OutAssetsToSync.Add(MassiveLODStaticMesh);
	OutAssetsToSync.Add(OutMassiveLODMaterial);

	//Cleanup
	for (int32 Index = 0; Index < InputDataArray.Num(); ++Index)
	{
		delete InputDataArray[Index];
	}

	return true;
}

FReply FMassiveLODSettingsLayout::OnApply()
{
	TArray<AActor*> SelectedActors;
	if(GetLevelEditorSelection(SelectedActors))
	{
		FString ProgressTitle = TEXT("Simplygon: ");
		FString ProgressType = 
			CurrentLODType == Remeshing ? TEXT("Remeshing") : TEXT("Aggregating");
		ProgressTitle.Append(ProgressType);

		GWarn->BeginSlowTask( FText::Format( LOCTEXT("SimplygonProgressTitle", "{0}"),  FText::FromString(ProgressTitle) ), true);
		//FlushRenderingCommands();

		FVector ProxyLocation = FVector::ZeroVector;
		//Get the chosen settings
		FSimplygonRemeshingSettings InputSettings = 
			IsRemeshingVisible() ? RemeshingSettingsWidget->GetSettings() : AggregateSettingsWidget->GetSettings();

		//Update the materialLOD settings
		InputSettings.MaterialLODSettings = MaterialLODSettingsWidget->GetSettings();

		TArray<UObject*> AssetsToSync;
		if ( !GenerateMassiveLODMesh(SelectedActors, InputSettings, ProxyLocation, MassiveLODPackageName, AssetsToSync) )
		{
			UE_LOG(LogSimplygonUtilities, Error, TEXT("Failed to generate MassiveLOD mesh"));
			GWarn->EndSlowTask();
			return FReply::Handled();
		}

		//Get MassiveLOD mesh
		UStaticMesh* MassiveLODMesh = NULL;
		AssetsToSync.FindItemByClass(&MassiveLODMesh);
		if (!MassiveLODMesh)
		{
			UE_LOG(LogSimplygonUtilities, Error, TEXT("Could not find Massive LOD mesh"));
			GWarn->EndSlowTask();
			return FReply::Handled();
		}

		//Setup the new world actor
		UWorld* World = SelectedActors[0]->GetWorld();
		if (InputSettings.bUseMassiveLOD)
		{
			class ALODActor* NewActor = Cast<ALODActor>(World->SpawnActor(ALODActor::StaticClass(), &ProxyLocation, &FRotator::ZeroRotator));
			//NewActor->SubObjects = AssetsToSync;
			NewActor->SubActors = SelectedActors;
			NewActor->LODLevel = 1;
			float DrawDistance = 1000.0f;
			NewActor->LODDrawDistance = DrawDistance;
			NewActor->SetActorLabel(TEXT("MassiveLODActor"));

			NewActor->GetStaticMeshComponent()->StaticMesh = MassiveLODMesh;

			// now set as parent
			for (auto& Actor : SelectedActors)
			{
				Actor->SetLODParent(NewActor->GetStaticMeshComponent(), DrawDistance);
			}
		}
		else
		{
			AStaticMeshActor* WorldActor = Cast<AStaticMeshActor>(World->SpawnActor(AStaticMeshActor::StaticClass(), &ProxyLocation));
			WorldActor->StaticMeshComponent->StaticMesh = MassiveLODMesh;
			WorldActor->SetActorLabel(MassiveLODMesh->GetName());

			if (!WorldActor)
			{
				UE_LOG(LogSimplygonUtilities, Error, TEXT("Failed to create new world actor"));
				GWarn->EndSlowTask();
				return FReply::Handled();
			}

			//Select the new replacement actor
			GEditor->SelectNone(false, true, false);
			GEditor->SelectActor(WorldActor, true, true);
		}

		//Update the asset registry that a new static mash and material has been created
		if (AssetsToSync.Num())
		{
			FAssetRegistryModule& AssetRegistry = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
			int32 AssetCount = AssetsToSync.Num();
			for(UObject* asset : AssetsToSync)
			{
				AssetRegistry.AssetCreated(asset);
				GEditor->BroadcastObjectReimported(asset);
			}

			//Also notify the content browser that the new assets exists
			FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
			ContentBrowserModule.Get().SyncBrowserToAssets(AssetsToSync, true);
		}

		GWarn->EndSlowTask();
		FEditorDelegates::RefreshEditor.Broadcast();
	}

	return FReply::Handled();
}

FReply FMassiveLODSettingsLayout::OnUnlinkSelection()
{
#if ENABLE_UNLINKPARENT_BUTTON
	GWarn->BeginSlowTask( NSLOCTEXT("UnrealEd", "UnlinkingMassiveLOD", "Unlinking MassiveLOD..."), true );
	{
		const FScopedTransaction Transaction(LOCTEXT("UnparentSelection", "Unparent (MassiveLOD)"));
		TArray<AActor*> SelectedActors;
		if (!GetLevelEditorSelection(SelectedActors))
		{
			GWarn->EndSlowTask();
			return FReply::Handled();
		}

		AActor* AReplacementActor = NULL;
		TArray<AActor*> ReplacementActors;

		//Use the selected actors to find the replacement actor
		for (int32 SelectedActorsIndex = 0; SelectedActorsIndex < SelectedActors.Num(); ++SelectedActorsIndex)
		{
			AActor* Actor = SelectedActors[SelectedActorsIndex];

			TArray<UPrimitiveComponent*> ActorComponents;
			Actor->GetComponents(ActorComponents);
			for (int32 ComponentIndex = 0; ComponentIndex < ActorComponents.Num(); ++ComponentIndex)
			{
				UPrimitiveComponent* Component = Cast<UPrimitiveComponent>(ActorComponents[ComponentIndex]);
				if (Component)
				{
					//We can grab the replacement actor from the replacement primitive
					if (Component->ReplacementPrimitive)
					{
						AReplacementActor = Component->ReplacementPrimitive->GetOwner();
						ReplacementActors.AddUnique(Component->ReplacementPrimitive->GetOwner());
						break;
					}

					if (Component->MassiveLODSizeOnScreen > -1.0f)
					{
						AReplacementActor = Actor;
						ReplacementActors.AddUnique(Actor);
						break;
					}
				}
			}

		}

		if (!ReplacementActors.Num())
		{
			UE_LOG(LogSimplygonUtilities, Log, TEXT("Could not find any MassiveLOD parents"));
			GWarn->EndSlowTask();
			return FReply::Handled();
		}
			
		//Placeholder to put all the actors that are replaced by AReplacementActor
		TArray<AActor*> ActorsToUnlink;

		for (int32 ReplacementActorIndex = 0; ReplacementActorIndex < ReplacementActors.Num(); ReplacementActorIndex++)
		{
			//Find all the actors that are using this replacement actor
			TArray<AActor*> LevelActors = ReplacementActors[ReplacementActorIndex]->GetWorld()->GetCurrentLevel()->Actors;
			uint32 LevelActorsCount = LevelActors.Num();

			for (uint32 LevelActorIndex = 0; LevelActorIndex < LevelActorsCount; ++LevelActorIndex)
			{
				AActor* LevelActor = LevelActors[LevelActorIndex];
				if (!LevelActor)
				{
					continue;
				}

				TArray<UPrimitiveComponent*> LevelActorComponents;
				LevelActor->GetComponents(LevelActorComponents);
				for (int32 ComponentIndex = 0; ComponentIndex < LevelActorComponents.Num(); ++ComponentIndex)
				{
					UPrimitiveComponent* Component = Cast<UPrimitiveComponent>(LevelActorComponents[ComponentIndex]);
					if (!Component || !Component->ReplacementPrimitive)
					{
						continue;
					}

					//We can grab the replacement actor from the replacement primitive
					if (Component->ReplacementPrimitive->GetOwner() == ReplacementActors[ReplacementActorIndex])
					{
						ActorsToUnlink.Add(Component->GetOwner());
						break;
					}
				} 
			} 
		}

		//Needed for Undo
		for (AActor* Actor : ActorsToUnlink)
		{
			Actor->Modify();
		}

		for (AActor* Actor : ReplacementActors)
		{
			Actor->Modify();
		}

		//Detach children from parent (engine)
		GUnrealEd->AssignReplacementComponentsByActors(ActorsToUnlink, NULL);

		//Detach children from parent (outliner)
		for (AActor* ActorIdx : ActorsToUnlink)
		{
			ActorIdx->DetachRootComponentFromParent();
			ActorIdx->SetActorHiddenInGame(false);
			ActorIdx->SetIsTemporarilyHiddenInEditor(false);
			ActorIdx->GetRootComponent()->SetVisibility(true, true);
		}

		//Merge before reseting the MassiveLOD distances
		ActorsToUnlink.Append(ReplacementActors);

		for (int32 Index = 0; Index < ActorsToUnlink.Num(); ++Index)
		{
			AActor* ActorToReset = ActorsToUnlink[Index];

			//Reset the MassiveLOD distance for current replacement actor (LOD parent)
			TArray<UPrimitiveComponent*> ActorToResetComponents;
			ActorToReset->GetComponents(ActorToResetComponents);
			for (int32 ComponentIndex = 0; ComponentIndex < ActorToResetComponents.Num(); ++ComponentIndex)
			{
				UPrimitiveComponent* Component = Cast<UPrimitiveComponent>(ActorToResetComponents[ComponentIndex]);
				if (Component)
				{
					Component->MassiveLODSizeOnScreen = -1.0f;
				}
			}
			ActorToReset->PostEditChange();
		}
	}
	GWarn->EndSlowTask();
#endif
	
	return FReply::Handled();
}

FText FMassiveLODSettingsLayout::GetParentName() const
{
	if (!ParentActor.IsValid())
	{
		return FText::FromString(TEXT("No Active Parent"));
	}

	return ParentName;
}

FReply FMassiveLODSettingsLayout::OnSetParent()
{
	//Get Level Editor Selection
	TArray<AActor*> SelectedActors;
	if (!GetLevelEditorSelection(SelectedActors))
	{
		return FReply::Handled();
	}

	//Only support one parent at the moment
	bool bSingleActor = SelectedActors.Num() == 1;
	if (!bSingleActor)
	{
		UE_LOG(LogSimplygonUtilities, Error, TEXT("Make sure to select one actor as parent"));
		return FReply::Handled();
	}

	//Cache a pointer to parent actor for later use
	ParentActor = SelectedActors[0];
	if (!ParentActor.IsValid())
	{
		UE_LOG(LogSimplygonUtilities, Error, TEXT("Selection corrupt"));
		return FReply::Handled();
	}

	//Cache the parent name
	FString OutName;
	ParentActor->GetName(OutName);
	ParentName = FText::FromString(OutName);
	
	return FReply::Handled();
}

FReply FMassiveLODSettingsLayout::OnAddChildToParent()
{
	const FScopedTransaction Transaction(LOCTEXT("AddChildren", "Add children (MassiveLOD)"));
	if (!ParentActor.IsValid())
	{
		return FReply::Handled();
	}

	UWorld* LevelWorld = ParentActor->GetWorld();
	if (!LevelWorld)
	{
		return FReply::Handled();
	}

	//Get Level Editor Selection
	TArray<AActor*> SelectedActors;
	if (!GetLevelEditorSelection(SelectedActors))
	{
		return FReply::Handled();
	}

	//Needed for Undo (transactions)
	for (AActor* Actor : SelectedActors)
	{
		Actor->Modify();
	}
	ParentActor->Modify();
	LevelWorld->Modify();

	//Translate parent (MassiveLOD) to children location
	FBox MergedBounds = FBox(0);
	for (AActor* Actor : SelectedActors)
	{
		MergedBounds += Actor->GetComponentsBoundingBox();
	}

	if (ParentActor.IsValid())
	{
		FVector Goal = MergedBounds.GetCenter();
		FVector PivotOffset = ParentActor->GetRootComponent()->Bounds.Origin - ParentActor->GetActorLocation();
		FVector ToGoal = Goal - ParentActor->GetActorLocation();
		ParentActor->EditorApplyTranslation(ToGoal - PivotOffset, false, false, false);
	}

	//Alternative option to translate the asset..
	/*FQuat oldRotation = Actor->GetTransform().GetRotation();
	FTransform translateTransform;
	translateTransform.SetTranslationAndScale3D(Origin - PivotOffset, Actor->GetActorScale3D());
	translateTransform.SetRotation(oldRotation);
	Actor->SetActorTransform(translateTransform);*/

	//Convert parent to LODActor
	FVector ParentLocation = ParentActor->GetActorLocation();
	class ALODActor* NewActor = Cast<ALODActor>(LevelWorld->SpawnActor(ALODActor::StaticClass(), &ParentLocation, &FRotator::ZeroRotator));
	//NewActor->SubObjects = OutAssets;
	NewActor->SubActors = SelectedActors;
	NewActor->LODLevel = 1; 
	float DrawDistance = 1000;
	NewActor->LODDrawDistance = DrawDistance;

	TArray<UStaticMeshComponent*> Components;
	ParentActor->GetComponents<UStaticMeshComponent>(Components);
	NewActor->GetStaticMeshComponent()->StaticMesh = Components[0]->StaticMesh;

	// now set as parent
	for (auto& Actor : SelectedActors)
	{
		Actor->SetLODParent(NewActor->GetStaticMeshComponent(), DrawDistance);
	}

	FEditorDelegates::RefreshEditor.Broadcast();

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE