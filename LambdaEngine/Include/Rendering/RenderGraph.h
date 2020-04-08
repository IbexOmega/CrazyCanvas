#pragma once

#include "LambdaEngine.h"

#include "Rendering/Core/API/IPipelineState.h"

#include <unordered_map>

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
			uint32														GlobalIndex = 0;
		};

		struct InternalRenderStageInputAttachment
		{
			const RenderStageInputAttachment*			pAttachment = nullptr;
			std::vector<InternalRenderStage*>			RenderStages;
			uint32										GlobalIndex = 0;
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
		bool IsInputTemporal(const RenderStage& renderStage, const RenderStageInputAttachment* pInputAttachment);
		bool CompatibleAttachmentNames(const RenderStageInputAttachment* pInputAttachment, const RenderStageOutputAttachment* pOutputAttachment);
		bool CompatibleAttachmentTypes(const RenderStageInputAttachment* pInputAttachment, const RenderStageOutputAttachment* pOutputAttachment);

		void WriteGraphVizDeclarations(
			FILE* pFile,
			bool declareExternalInputs,
			const std::unordered_map<const char*, InternalRenderStage>&	internalRenderStages,
			const std::unordered_map<const char*, InternalRenderStageInputAttachment>& internalInputAttachments,
			const std::unordered_map<const char*, InternalRenderStageInputAttachment>& temporalInternalInputAttachments,
			const std::unordered_map<const char*, InternalRenderStageExternalInputAttachment>& internalExternalInputAttachments,
			const std::unordered_map<const char*, InternalRenderStageOutputAttachment>& internalOutputAttachments);

		void WriteGraphVizDefinitions(
			FILE* pFile,
			bool externalInputsDeclared,
			bool linkExternalInputs,
			const std::unordered_map<const char*, InternalRenderStage>& internalRenderStages,
			const std::unordered_map<const char*, InternalRenderStageInputAttachment>& internalInputAttachments,
			const std::unordered_map<const char*, InternalRenderStageInputAttachment>& temporalInternalInputAttachments,
			const std::unordered_map<const char*, InternalRenderStageExternalInputAttachment>& internalExternalInputAttachments,
			const std::unordered_map<const char*, InternalRenderStageOutputAttachment>& internalOutputAttachments);
	};
}