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

		union Pipeline
		{
			GraphicsPipelineDesc	GraphicsDesc;
			ComputePipelineDesc		ComputeDesc;
			RayTracingPipelineDesc	RayTracingDesc;
		};
	};

	struct RenderGraphDesc
	{
		const char* pName					= "Render Graph";
		bool CreateDebugGraph				= false;
		const RenderStage* pRenderStages	= nullptr;
		uint32 RenderStageCount				= 0;
	};

	class LAMBDA_API RenderGraph
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
		DECL_REMOVE_COPY(RenderGraph);
		DECL_REMOVE_MOVE(RenderGraph);

		RenderGraph();
		~RenderGraph();

		bool Init(const RenderGraphDesc& desc);

	private:
		/*
		* Parses everything to internal structures, creates bidirectional connections, separates temporal inputs from non-temporal inputs
		*/
		bool ParseInitialStages(const RenderGraphDesc& desc);
		/*
		* Connects output resources to input resources, thereby marking resource transitions, also discovers input resources that miss output resources
		*/
		bool ConnectOutputsToInputs();
		/*
		* For each renderpass, go through its ascendants and add 1 to their weights
		*/
		bool WeightRenderStages();
		void RecursivelyWeightAncestors(InternalRenderStage* pRenderStage);
		/*
		* Sort render stages
		*/
		bool SortRenderStages();

		bool IsInputTemporal(const RenderStage& renderStage, const RenderStageInputAttachment* pInputAttachment);
		bool CompatibleAttachmentNames(const RenderStageInputAttachment* pInputAttachment, const RenderStageOutputAttachment* pOutputAttachment);
		bool CompatibleAttachmentTypes(const RenderStageInputAttachment* pInputAttachment, const RenderStageOutputAttachment* pOutputAttachment);
		bool AreRenderStagesRelated(const InternalRenderStage* pRenderStageAncestor, const InternalRenderStage* pRenderStageDescendant);

		bool WriteGraphViz(bool declareExternalInputs, bool linkExternalInputs);
		void WriteGraphVizRenderStageOrderDeclarations(FILE* pFile);
		void WriteGraphVizRenderStageOrderDefinitions(FILE* pFile);
		void WriteGraphVizCompleteDeclarations(FILE* pFile, bool declareExternalInputs);
		void WriteGraphVizCompleteDefinitions(FILE* pFile, bool externalInputsDeclared, bool linkExternalInputs);

		void SanitizeString(char* pString, uint32 numCharacters);

	private:
		const char* m_pName;

		std::unordered_map<const char*, InternalRenderStage>						m_ParsedRenderStages;
		std::unordered_map<const char*, InternalRenderStageInputAttachment>			m_ParsedInputAttachments;
		std::unordered_map<const char*, InternalRenderStageInputAttachment>			m_ParsedTemporalInputAttachments;
		std::unordered_map<const char*, InternalRenderStageExternalInputAttachment> m_ParsedExternalInputAttachments;
		std::unordered_map<const char*, InternalRenderStageOutputAttachment>		m_ParsedOutputAttachments;

		std::vector<const InternalRenderStage*>										m_SortedInternalRenderStages;
		std::vector<const RenderStage*>												m_SortedRenderStages;
	};
}