#pragma once

#include "Rendering/RenderGraphTypes.h"
#include "Rendering/Core/API/GraphicsHelpers.h"

#include "Containers/THashTable.h"
#include "Containers/TArray.h"
#include "Containers/TSet.h"

#include <map>

namespace LambdaEngine
{
	class LAMBDA_API RenderGraphParser
	{
	public:

		static bool ParseRenderGraph(
			RenderGraphStructureDesc* pParsedStructure,
			const TArray<RenderGraphResourceDesc>& resources,
			const THashTable<String, EditorRenderStageDesc>& renderStagesByName,
			const THashTable<int32, EditorRenderGraphResourceState>& resourceStatesByHalfAttributeIndex,
			const THashTable<int32, EditorRenderGraphResourceLink>& resourceStateLinksByLinkIndex,
			const EditorFinalOutput& finalOutput,
			bool generateImGuiStage);

		static RenderGraphResourceDesc CreateBackBufferResource();

	private:
		static bool RecursivelyWeightParentRenderStages(
			THashTable<String, EditorRenderStageDesc>::const_iterator childRenderStageIt,
			const THashTable<String, EditorRenderStageDesc>& renderStagesByName,
			const THashTable<int32, EditorRenderGraphResourceState>& resourceStatesByHalfAttributeIndex,
			const THashTable<int32, EditorRenderGraphResourceLink>& resourceStateLinksByLinkIndex,
			THashTable<String, int32>& renderStageWeightsByName);

		static bool FindAndCreateSynchronization(
			const TArray<RenderGraphResourceDesc>& resources,
			const THashTable<int32, EditorRenderGraphResourceState>& resourceStatesByHalfAttributeIndex,
			std::multimap<uint32, const EditorRenderStageDesc*>::reverse_iterator currentOrderedRenderStageIt,
			const std::multimap<uint32, const EditorRenderStageDesc*>& orderedMappedRenderStages,
			THashTable<int32, EditorRenderGraphResourceState>::const_iterator currentResourceStateIt,
			SynchronizationStageDesc* pSynchronizationStage,
			bool generateImGuiStage);

		static bool CapturedByImGui(TArray<RenderGraphResourceDesc>::ConstIterator resourceIt);
		static void CreateParsedRenderStage(const THashTable<int32, EditorRenderGraphResourceState>& resourceStatesByHalfAttributeIndex, RenderStageDesc* pDstRenderStage, const EditorRenderStageDesc* pSrcRenderStage);
	};
}