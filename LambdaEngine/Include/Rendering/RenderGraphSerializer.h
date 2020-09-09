#pragma once

#include "Rendering/RenderGraphTypes.h"
#include "Rendering/Core/API/GraphicsHelpers.h"

#include "Containers/THashTable.h"
#include "Containers/TArray.h"
#include "Containers/TSet.h"

namespace LambdaEngine
{
	class LAMBDA_API RenderGraphSerializer
	{
	public:
		static bool SaveRenderGraphToFile(
			const String& renderGraphName,
			const TArray<RenderGraphResourceDesc>& resources,
			const THashTable<int32, String>& renderStageNameByInputAttributeIndex,
			const THashTable<String, EditorRenderStageDesc>& renderStagesByName,
			const THashTable<int32, EditorRenderGraphResourceState>& resourceStatesByHalfAttributeIndex,
			const THashTable<int32, EditorRenderGraphResourceLink>& resourceStateLinksByLinkIndex,
			const TArray<EditorResourceStateGroup>& resourceStateGroups,
			const EditorFinalOutput& finalOutput);

		static bool LoadFromFile(
			const String& renderGraphFileName,
			TArray<RenderGraphResourceDesc>& resources,
			THashTable<int32, String>& renderStageNameByInputAttributeIndex,
			THashTable<String, EditorRenderStageDesc>& renderStagesByName,
			THashTable<int32, EditorRenderGraphResourceState>& resourceStatesByHalfAttributeIndex,
			THashTable<int32, EditorRenderGraphResourceLink>& resourceStateLinksByLinkIndex,
			TArray<EditorResourceStateGroup>& resourceStateGroups,
			EditorFinalOutput& finalOutput,
			int32& nextNodeID,
			int32& nextAttributeID,
			int32& nextLinkID);

		static bool LoadAndParse(RenderGraphStructureDesc* pRenderGraphStructureDesc, const String& renderGraphFileName, bool imGuiEnabled);

	private:
		static bool FixLinkForPreviouslyLoadedResourceState(
			EditorRenderGraphResourceState* pResourceState,
			int32 attributeIndex,
			THashTable<int32, EditorRenderGraphResourceState>& loadedResourceStatesByHalfAttributeIndex,
			THashTable<int32, EditorRenderGraphResourceLink>& loadedResourceStateLinks,
			TArray<int32>& unfinishedLinksAwaitingStage,
			int32& nextLinkID);

		static void CreateLinkForLoadedResourceState(
			EditorRenderGraphResourceState* pResourceState,
			int32 attributeIndex,
			String& srcStageName,
			TArray<EditorResourceStateGroup>& loadedResourceStateGroups,
			THashTable<String, EditorRenderStageDesc>& loadedRenderStagesByName,
			THashTable<int32, EditorRenderGraphResourceState>& loadedResourceStatesByHalfAttributeIndex,
			THashTable<int32, EditorRenderGraphResourceLink>& loadedResourceStateLinks,
			THashTable<String, TArray<int32>>& unfinishedLinks,
			int32& nextLinkID);
	};
}