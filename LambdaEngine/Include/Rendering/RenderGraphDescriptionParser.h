#pragma once

#include "LambdaEngine.h"

#include "RenderGraphTypes.h"
#include "RenderGraph.h"

#include <unordered_map>
#include <set>

namespace LambdaEngine
{
	constexpr char* RENDER_GRAPH_BACK_BUFFER = "BACK_BUFFER_TEXTURE";

	class LAMBDA_API RenderGraphDescriptionParser
	{
		struct InternalRenderStageInputAttachment;
		struct InternalRenderStageExternalInputAttachment;
		struct InternalRenderStageOutputAttachment;

		struct InternalRenderStage
		{
			const RenderStageDesc* pRenderStage								= nullptr;
			std::vector<InternalRenderStageInputAttachment*>			InputAttachments;
			std::vector<InternalRenderStageInputAttachment*>			TemporalInputAttachments;
			std::vector<InternalRenderStageExternalInputAttachment*>	ExternalInputAttachments;
			std::vector<InternalRenderStageOutputAttachment*>			OutputAttachments;
			std::set<InternalRenderStage*>								ParentRenderStages;
			std::set<InternalRenderStage*>								ChildRenderStages;
			uint32														GlobalIndex = 0;
			uint32														Weight = 0;
		};

		struct InternalRenderStageInputAttachment
		{
			const RenderStageAttachment*				pAttachment = nullptr;
			std::vector<InternalRenderStage*>			RenderStages;
			uint32										GlobalIndex = 0;

			const InternalRenderStageOutputAttachment*	pConnectedAttachment = nullptr;
		};

		struct InternalRenderStageExternalInputAttachment
		{
			const RenderStageAttachment*				pAttachment = nullptr;
			std::vector<InternalRenderStage*>			RenderStages;
			uint32										GlobalIndex = 0;
		};

		struct InternalRenderStageOutputAttachment
		{
			const RenderStageAttachment*				pAttachment = nullptr;
			std::vector<InternalRenderStage*>			RenderStages;
			uint32										GlobalIndex = 0;

			const InternalRenderStageInputAttachment*	pConnectedAttachment = nullptr;
		};

	public:
		DECL_STATIC_CLASS(RenderGraphDescriptionParser);

		/*
		* Goes through the RenderGraphDesc and creates the underlying RenderGraphStructure, consisting of the vectors sortedRenderStages, sortedSynchronizationStages, sortedPipelineStages
		*	desc - The RenderGraph Description
		*	sortedRenderStageDescriptions - (Output) Sorted Render Stage Descriptions in order of execution
		*	sortedSynchronizationStageDescriptions - (Output) Sorted Resource Synchronization Stage Descriptions in order of execution
		*	sortedPipelineStageDescriptions - (Output) Sorted Pipeline Stage Descriptions in order of execution, each PipelineStage is either a RenderStage or a SynchronizationStage
		* return - true if the parsing succeeded, otherwise false
		*/
		static bool Parse(
			const RenderGraphDesc& desc,
			std::vector<RenderStageDesc>& sortedRenderStageDescriptions,
			std::vector<SynchronizationStageDesc>& sortedSynchronizationStageDescriptions,
			std::vector<PipelineStageDesc>& sortedPipelineStageDescriptions);

	private:
		/*
		* Goes through each Render Stage and sorts its attachments into three groups, either Input, External Input or Output
		*/
		static bool SortRenderStagesAttachments(
			const RenderGraphDesc& desc,
			std::unordered_map<const char*, std::vector<const RenderStageAttachment*>>&		renderStagesInputAttachments,
			std::unordered_map<const char*, std::vector<const RenderStageAttachment*>>&		renderStagesExternalInputAttachments,
			std::unordered_map<const char*, std::vector<const RenderStageAttachment*>>&		renderStagesOutputAttachments);
		/*
		* Parses everything to internal structures, creates bidirectional connections, separates temporal inputs from non-temporal inputs
		*/
		static bool ParseInitialStages(
			const RenderGraphDesc& desc,
			std::unordered_map<const char*, std::vector<const RenderStageAttachment*>>& renderStagesInputAttachments,
			std::unordered_map<const char*, std::vector<const RenderStageAttachment*>>& renderStagesExternalInputAttachments,
			std::unordered_map<const char*, std::vector<const RenderStageAttachment*>>&			renderStagesOutputAttachments,
			std::unordered_map<const char*, InternalRenderStage>&							parsedRenderStages,
			std::unordered_map<const char*, InternalRenderStageInputAttachment>&			parsedInputAttachments,
			std::unordered_map<const char*, InternalRenderStageInputAttachment>&			parsedTemporalInputAttachments,
			std::unordered_map<const char*, InternalRenderStageExternalInputAttachment>&	parsedExternalInputAttachments,
			std::unordered_map<const char*, InternalRenderStageOutputAttachment>&			parsedOutputAttachments);
		/*
		* Connects output resources to input resources, thereby marking resource transitions, also discovers input resources that miss output resources
		*/
		static bool ConnectOutputsToInputs(
			std::unordered_map<const char*, InternalRenderStageInputAttachment>&	parsedInputAttachments,
			std::unordered_map<const char*, InternalRenderStageOutputAttachment>&	parsedOutputAttachments);
		/*
		* For each renderpass, go through its ascendants and add 1 to their weights
		*/
		static bool WeightRenderStages(std::unordered_map<const char*, InternalRenderStage>& parsedRenderStages);
		static void RecursivelyWeightAncestors(InternalRenderStage* pRenderStage);
		/*
		* Sorts Render Stages and Creates Synchronization Stages
		*/
		static bool SortPipelineStages(
			std::unordered_map<const char*, InternalRenderStage>&	parsedRenderStages,
			std::vector<const InternalRenderStage*>&				sortedInternalRenderStages,
			std::vector<RenderStageDesc>&								sortedRenderStages,
			std::vector<SynchronizationStageDesc>&						sortedSynchronizationStages,
			std::vector<PipelineStageDesc>&								sortedPipelineStages);
		/*
		* Removes unnecessary synchronizations
		*/
		static bool PruneUnnecessarySynchronizations(
			std::vector<SynchronizationStageDesc>&		sortedSynchronizationStages,
			std::vector<PipelineStageDesc>&				sortedPipelineStages);

		static bool IsInputTemporal(const std::vector<const RenderStageAttachment*>& renderStageOutputAttachments, const RenderStageAttachment* pInputAttachment);
		static bool AttachmentsEqual(const RenderStageAttachment* pInputAttachment, const RenderStageAttachment* pOutputAttachment);
		static bool AreRenderStagesRelated(const InternalRenderStage* pRenderStageAncestor, const InternalRenderStage* pRenderStageDescendant);
		static bool IsAttachmentReserved(const char* pAttachmentName);

		static bool WriteGraphViz(
			const char* pName, 
			bool declareExternalInputs, 
			bool linkExternalInputs,
			std::unordered_map<const char*, InternalRenderStage>&							parsedRenderStages,
			std::unordered_map<const char*, InternalRenderStageInputAttachment>&			parsedInputAttachments,
			std::unordered_map<const char*, InternalRenderStageInputAttachment>&			parsedTemporalInputAttachments,
			std::unordered_map<const char*, InternalRenderStageExternalInputAttachment>&	parsedExternalInputAttachments,
			std::unordered_map<const char*, InternalRenderStageOutputAttachment>&			parsedOutputAttachments,
			std::vector<const InternalRenderStage*>&										sortedInternalRenderStages,
			std::vector<RenderStageDesc>&														sortedRenderStages,
			std::vector<SynchronizationStageDesc>&												sortedSynchronizationStages,
			std::vector<PipelineStageDesc>&														sortedPipelineStages);
		static void WriteGraphVizPipelineStages(
			FILE* pFile,
			std::unordered_map<const char*, InternalRenderStage>&							parsedRenderStages,
			std::unordered_map<const char*, InternalRenderStageInputAttachment>&			parsedInputAttachments,
			std::unordered_map<const char*, InternalRenderStageInputAttachment>&			parsedTemporalInputAttachments,
			std::unordered_map<const char*, InternalRenderStageExternalInputAttachment>&	parsedExternalInputAttachments,
			std::unordered_map<const char*, InternalRenderStageOutputAttachment>&			parsedOutputAttachments,
			std::vector<const InternalRenderStage*>&										sortedInternalRenderStages,
			std::vector<RenderStageDesc>&														sortedRenderStages,
			std::vector<SynchronizationStageDesc>&												sortedSynchronizationStages,
			std::vector<PipelineStageDesc>&														sortedPipelineStages);
		static void WriteGraphVizCompleteDeclarations(
			FILE* pFile, 
			bool declareExternalInputs,
			std::unordered_map<const char*, InternalRenderStage>&							parsedRenderStages,
			std::unordered_map<const char*, InternalRenderStageInputAttachment>&			parsedInputAttachments,
			std::unordered_map<const char*, InternalRenderStageInputAttachment>&			parsedTemporalInputAttachments,
			std::unordered_map<const char*, InternalRenderStageExternalInputAttachment>&	parsedExternalInputAttachments,
			std::unordered_map<const char*, InternalRenderStageOutputAttachment>&			parsedOutputAttachments,
			std::vector<const InternalRenderStage*>&										sortedInternalRenderStages,
			std::vector<RenderStageDesc>&														sortedRenderStages,
			std::vector<SynchronizationStageDesc>&												sortedSynchronizationStages,
			std::vector<PipelineStageDesc>&														sortedPipelineStages);
		static void WriteGraphVizCompleteDefinitions(
			FILE* pFile, 
			bool externalInputsDeclared, 
			bool linkExternalInputs,
			std::unordered_map<const char*, InternalRenderStage>& parsedRenderStages,
			std::unordered_map<const char*, InternalRenderStageInputAttachment>& parsedInputAttachments,
			std::unordered_map<const char*, InternalRenderStageInputAttachment>& parsedTemporalInputAttachments,
			std::unordered_map<const char*, InternalRenderStageExternalInputAttachment>& parsedExternalInputAttachments,
			std::unordered_map<const char*, InternalRenderStageOutputAttachment>& parsedOutputAttachments,
			std::vector<const InternalRenderStage*>& sortedInternalRenderStages,
			std::vector<RenderStageDesc>& sortedRenderStages,
			std::vector<SynchronizationStageDesc>& sortedSynchronizationStages,
			std::vector<PipelineStageDesc>& sortedPipelineStages);

		static void SanitizeString(char* pString, uint32 numCharacters);
		static void ConcatPipelineStateToString(char* pStringBuffer, EPipelineStateType pipelineState);
	};
}