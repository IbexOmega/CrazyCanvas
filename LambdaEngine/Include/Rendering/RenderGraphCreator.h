#pragma once

#include "LambdaEngine.h"

#include "Rendering/Core/API/IPipelineState.h"

#include <unordered_map>
#include <set>

namespace LambdaEngine
{
	enum class EInputAttachmentType
	{
		UNKNOWN			= 0,
		TEXTURE			= 1,
		BUFFER			= 2,
	};

	enum class EExternalInputAttachmentType
	{
		UNKNOWN			= 0,
		TEXTURE			= 1,
		BUFFER			= 2,
		UNIFORM			= 3,
	};

	enum class EOutputAttachmentType
	{
		UNKNOWN			= 0,
		TEXTURE			= 1,
		BUFFER			= 2,
		DEPTH_STENCIL	= 3,
	};

	enum class EPipelineStageType
	{
		NONE			= 0,
		RENDER			= 1,
		SYNCHRONIZATION = 2,
	};

	enum class EAttachmentSynchronizationType
	{
		NONE					= 0,
		TRANSITION_FOR_WRITE	= 1,
		TRANSITION_FOR_READ		= 2,
		OWNERSHIP_CHANGE		= 3,
	};

	enum class EAttachmentState
	{
		NONE	= 0,
		READ	= 1,
		WRITE	= 2
	};

	struct RenderStageInputAttachment
	{
		const char* pName					= "Input Render Stage Attachment";
		EInputAttachmentType Type			= EInputAttachmentType::UNKNOWN;
	};

	struct RenderStageExternalInputAttachment
	{
		const char* pName					= "External Input Render Stage Attachment";
		EExternalInputAttachmentType Type	= EExternalInputAttachmentType::UNKNOWN;
		uint32 DescriptorCount				= 1;
	};

	struct RenderStageOutputAttachment
	{
		const char* pName					= "Output Render Stage Attachment";
		EOutputAttachmentType Type			= EOutputAttachmentType::UNKNOWN;
	};

	struct RenderStage
	{
		const char*									pName							= "Render Stage";
		const RenderStageInputAttachment*			pInputAttachments				= nullptr;
		const RenderStageExternalInputAttachment*	pExternalInputAttachments		= nullptr;
		const RenderStageOutputAttachment*			pOutputAttachments				= nullptr;
		uint32										InputAttachmentCount			= 0;
		uint32										ExtenalInputAttachmentCount		= 0;
		uint32										OutputAttachmentCount			= 0;

		EPipelineStateType							PipelineType					= EPipelineStateType::NONE;

		union
		{
			GraphicsPipelineStateDesc*		pGraphicsDesc;
			ComputePipelineStateDesc*		pComputeDesc;
			RayTracingPipelineStateDesc*	pRayTracingDesc;
		} Pipeline;
	};

	struct RenderGraphDesc
	{
		const char* pName					= "Render Graph";
		bool CreateDebugGraph				= false;
		const RenderStage* pRenderStages	= nullptr;
		uint32 RenderStageCount				= 0;
	};

	struct AttachmentSynchronization
	{
		EAttachmentSynchronizationType Type = EAttachmentSynchronizationType::NONE;
		EPipelineStateType FromQueueOwner	= EPipelineStateType::NONE;
		EPipelineStateType ToQueueOwner		= EPipelineStateType::NONE;

		union
		{
			struct
			{
				RenderStageOutputAttachment		FromAttachment;
				RenderStageInputAttachment		ToAttachment;
			} OutputToInput;

			struct
			{
				RenderStageInputAttachment		FromAttachment;
				RenderStageOutputAttachment		ToAttachment;
			} InputToOutput;
		};
		
	};

	struct SynchronizationStage
	{
		std::vector<AttachmentSynchronization> Synchronizations;
	};

	struct PipelineStage
	{
		EPipelineStageType Type		= EPipelineStageType::NONE;
		uint32 StageIndex			= 0;
	};

	constexpr char* RENDER_GRAPH_BACK_BUFFER = "BACK_BUFFER_TEXTURE";

	class LAMBDA_API RenderGraphCreator
	{
		struct InternalRenderStageInputAttachment;
		struct InternalRenderStageExternalInputAttachment;
		struct InternalRenderStageOutputAttachment;

		struct InternalRenderStage
		{
			const RenderStage* pRenderStage								= nullptr;
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
			const RenderStageInputAttachment*			pAttachment = nullptr;
			std::vector<InternalRenderStage*>			RenderStages;
			uint32										GlobalIndex = 0;

			const InternalRenderStageOutputAttachment*	pConnectedAttachment = nullptr;
		};

		struct InternalRenderStageExternalInputAttachment
		{
			const RenderStageExternalInputAttachment*	pAttachment = nullptr;
			std::vector<InternalRenderStage*>			RenderStages;
			uint32										GlobalIndex = 0;
		};

		struct InternalRenderStageOutputAttachment
		{
			const RenderStageOutputAttachment*			pAttachment = nullptr;
			std::vector<InternalRenderStage*>			RenderStages;
			uint32										GlobalIndex = 0;

			const InternalRenderStageInputAttachment*	pConnectedAttachment = nullptr;
		};

	public:
		DECL_STATIC_CLASS(RenderGraphCreator);

		static bool Create(const RenderGraphDesc& desc);

	private:
		/*
		* Parses everything to internal structures, creates bidirectional connections, separates temporal inputs from non-temporal inputs
		*/
		static bool ParseInitialStages(
		const RenderGraphDesc& desc,
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
			std::vector<RenderStage>&								sortedRenderStages,
			std::vector<SynchronizationStage>&						sortedSynchronizationStages,
			std::vector<PipelineStage>&								sortedPipelineStages);
		/*
		* Removes unnecessary synchronizations
		*/
		static bool PruneUnnecessarySynchronizations(
			std::vector<SynchronizationStage>&		sortedSynchronizationStages,
			std::vector<PipelineStage>&				sortedPipelineStages);

		static bool IsInputTemporal(const RenderStage& renderStage, const RenderStageInputAttachment* pInputAttachment);
		static bool AttachmentsEqual(const RenderStageInputAttachment* pInputAttachment, const RenderStageOutputAttachment* pOutputAttachment);
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
			std::vector<RenderStage>&														sortedRenderStages,
			std::vector<SynchronizationStage>&												sortedSynchronizationStages,
			std::vector<PipelineStage>&														sortedPipelineStages);
		static void WriteGraphVizPipelineStages(
			FILE* pFile,
			std::unordered_map<const char*, InternalRenderStage>&							parsedRenderStages,
			std::unordered_map<const char*, InternalRenderStageInputAttachment>&			parsedInputAttachments,
			std::unordered_map<const char*, InternalRenderStageInputAttachment>&			parsedTemporalInputAttachments,
			std::unordered_map<const char*, InternalRenderStageExternalInputAttachment>&	parsedExternalInputAttachments,
			std::unordered_map<const char*, InternalRenderStageOutputAttachment>&			parsedOutputAttachments,
			std::vector<const InternalRenderStage*>&										sortedInternalRenderStages,
			std::vector<RenderStage>&														sortedRenderStages,
			std::vector<SynchronizationStage>&												sortedSynchronizationStages,
			std::vector<PipelineStage>&														sortedPipelineStages);
		static void WriteGraphVizCompleteDeclarations(
			FILE* pFile, 
			bool declareExternalInputs,
			std::unordered_map<const char*, InternalRenderStage>&							parsedRenderStages,
			std::unordered_map<const char*, InternalRenderStageInputAttachment>&			parsedInputAttachments,
			std::unordered_map<const char*, InternalRenderStageInputAttachment>&			parsedTemporalInputAttachments,
			std::unordered_map<const char*, InternalRenderStageExternalInputAttachment>&	parsedExternalInputAttachments,
			std::unordered_map<const char*, InternalRenderStageOutputAttachment>&			parsedOutputAttachments,
			std::vector<const InternalRenderStage*>&										sortedInternalRenderStages,
			std::vector<RenderStage>&														sortedRenderStages,
			std::vector<SynchronizationStage>&												sortedSynchronizationStages,
			std::vector<PipelineStage>&														sortedPipelineStages);
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
			std::vector<RenderStage>& sortedRenderStages,
			std::vector<SynchronizationStage>& sortedSynchronizationStages,
			std::vector<PipelineStage>& sortedPipelineStages);

		static void SanitizeString(char* pString, uint32 numCharacters);
		static void ConcatPipelineStateToString(char* pStringBuffer, EPipelineStateType pipelineState);
	};
}
