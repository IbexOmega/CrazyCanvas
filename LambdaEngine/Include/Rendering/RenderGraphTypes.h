#pragma once

#include "LambdaEngine.h"
#include "Core/API/GraphicsTypes.h"
#include "Core/API/IPipelineState.h"

namespace LambdaEngine
{
	constexpr char* RENDER_GRAPH_BACK_BUFFER_ATTACHMENT = "BACK_BUFFER_TEXTURE";

	enum class EPipelineStageType : uint8
	{
		NONE			= 0,
		RENDER			= 1,
		SYNCHRONIZATION = 2,
	};

	enum class EAttachmentSynchronizationType : uint8
	{
		NONE					= 0,
		TRANSITION_FOR_WRITE	= 1,
		TRANSITION_FOR_READ		= 2,
		OWNERSHIP_CHANGE		= 3,
	};

	enum class EAttachmentState : uint8
	{
		NONE	= 0,
		READ	= 1,
		WRITE	= 2
	};

	enum class EAttachmentAccessType : uint8
	{
		NONE			= 0,
		INPUT			= 1,
		EXTERNAL_INPUT	= 2,
		OUTPUT			= 3,
	};

	enum class EAttachmentType : uint8
	{
		NONE												= 0,

		INPUT_SHADER_RESOURCE_TEXTURE						= 1,
		INPUT_SHADER_RESOURCE_COMBINED_SAMPLER				= 2,
		INPUT_UNORDERED_ACCESS_TEXTURE						= 3,
		INPUT_UNORDERED_ACCESS_BUFFER						= 4,

		EXTERNAL_INPUT_SHADER_RESOURCE_TEXTURE				= 5,
		EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER		= 6,
		EXTERNAL_INPUT_UNORDERED_ACCESS_TEXTURE				= 7,
		EXTERNAL_INPUT_CONSTANT_BUFFER						= 8,
		EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER				= 9,
		EXTERNAL_INPUT_ACCELERATION_STRUCTURE				= 10,

		OUTPUT_UNORDERED_ACCESS_TEXTURE						= 11,
		OUTPUT_UNORDERED_ACCESS_BUFFER						= 12,
		OUTPUT_COLOR										= 13,
		OUTPUT_DEPTH_STENCIL								= 14,
	};

	enum class ESimpleResourceType : uint8
	{
		NONE					= 0,
		TEXTURE					= 1,
		BUFFER					= 2,
		ACCELERATION_STRUCTURE	= 3,
	};

	enum class ERenderStageResourceType : uint8
	{
		NONE			= 0,
		ATTACHMENT		= 1,
		PUSH_CONSTANTS	= 2,
	};

	struct RenderStageAttachment
	{
		const char* pName				= "";
		EAttachmentType Type			= EAttachmentType::NONE;
		FShaderStageFlags StageMask		= SHADER_STAGE_FLAG_NONE;
		uint32 DescriptorCount			= 1;
	};

	struct RenderStagePushConstants
	{
		const char* pName				= "";
		uint32 DataSize					= 0;
	};

	struct RenderStageResourceDesc
	{
		ERenderStageResourceType Type	= ERenderStageResourceType::NONE;

		union
		{
			const RenderStageAttachment* pAttachmentDesc;
			const RenderStagePushConstants* pPushConstantsDesc;
		};
	};

	struct RenderStageDesc
	{
		const char* pName							= "Render Stage";
		const RenderStageAttachment* pAttachments	= nullptr;
		uint32 AttachmentCount						= 0;
		RenderStagePushConstants PushConstants		= {};

		EPipelineStateType PipelineType				= EPipelineStateType::NONE;

		union
		{
			GraphicsPipelineStateDesc*		pGraphicsDesc;
			ComputePipelineStateDesc*		pComputeDesc;
			RayTracingPipelineStateDesc*	pRayTracingDesc;
		} Pipeline;
	};

	struct AttachmentSynchronizationDesc
	{
		EAttachmentSynchronizationType	Type			= EAttachmentSynchronizationType::NONE;
		EPipelineStateType				FromQueueOwner	= EPipelineStateType::NONE;
		EPipelineStateType				ToQueueOwner	= EPipelineStateType::NONE;
		RenderStageAttachment			FromAttachment;
		RenderStageAttachment			ToAttachment;
	};

	struct SynchronizationStageDesc
	{
		std::vector<AttachmentSynchronizationDesc> Synchronizations;
	};

	struct PipelineStageDesc
	{
		EPipelineStageType Type = EPipelineStageType::NONE;
		uint32 StageIndex		= 0;
	};

	EAttachmentAccessType FORCEINLINE GetAttachmentAccessType(EAttachmentType attachmentType)
	{
		switch (attachmentType)
		{
		case EAttachmentType::INPUT_SHADER_RESOURCE_TEXTURE:						return EAttachmentAccessType::INPUT;
		case EAttachmentType::INPUT_SHADER_RESOURCE_COMBINED_SAMPLER:				return EAttachmentAccessType::INPUT;
		case EAttachmentType::INPUT_UNORDERED_ACCESS_TEXTURE:						return EAttachmentAccessType::INPUT;
		case EAttachmentType::INPUT_UNORDERED_ACCESS_BUFFER:						return EAttachmentAccessType::INPUT;

		case EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_TEXTURE:				return EAttachmentAccessType::EXTERNAL_INPUT;
		case EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER:		return EAttachmentAccessType::EXTERNAL_INPUT;
		case EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_TEXTURE:				return EAttachmentAccessType::EXTERNAL_INPUT;
		case EAttachmentType::EXTERNAL_INPUT_CONSTANT_BUFFER:						return EAttachmentAccessType::EXTERNAL_INPUT;
		case EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER:				return EAttachmentAccessType::EXTERNAL_INPUT;
		case EAttachmentType::EXTERNAL_INPUT_ACCELERATION_STRUCTURE:				return EAttachmentAccessType::EXTERNAL_INPUT;

		case EAttachmentType::OUTPUT_UNORDERED_ACCESS_TEXTURE:						return EAttachmentAccessType::OUTPUT;
		case EAttachmentType::OUTPUT_UNORDERED_ACCESS_BUFFER:						return EAttachmentAccessType::OUTPUT;
		case EAttachmentType::OUTPUT_COLOR:											return EAttachmentAccessType::OUTPUT;
		case EAttachmentType::OUTPUT_DEPTH_STENCIL:									return EAttachmentAccessType::OUTPUT;

		default:																	return EAttachmentAccessType::NONE;
		}
	}

	EDescriptorType FORCEINLINE GetAttachmentDescriptorType(EAttachmentType attachmentType)
	{
		switch (attachmentType)
		{
		case EAttachmentType::INPUT_SHADER_RESOURCE_TEXTURE:						return EDescriptorType::DESCRIPTOR_SHADER_RESOURCE_TEXTURE;
		case EAttachmentType::INPUT_SHADER_RESOURCE_COMBINED_SAMPLER:				return EDescriptorType::DESCRIPTOR_SHADER_RESOURCE_COMBINED_SAMPLER;
		case EAttachmentType::INPUT_UNORDERED_ACCESS_TEXTURE:						return EDescriptorType::DESCRIPTOR_UNORDERED_ACCESS_TEXTURE;
		case EAttachmentType::INPUT_UNORDERED_ACCESS_BUFFER:						return EDescriptorType::DESCRIPTOR_UNORDERED_ACCESS_BUFFER;

		case EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_TEXTURE:				return EDescriptorType::DESCRIPTOR_SHADER_RESOURCE_TEXTURE;
		case EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER:		return EDescriptorType::DESCRIPTOR_SHADER_RESOURCE_COMBINED_SAMPLER;
		case EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_TEXTURE:				return EDescriptorType::DESCRIPTOR_UNORDERED_ACCESS_TEXTURE;
		case EAttachmentType::EXTERNAL_INPUT_CONSTANT_BUFFER:						return EDescriptorType::DESCRIPTOR_CONSTANT_BUFFER;
		case EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER:				return EDescriptorType::DESCRIPTOR_UNORDERED_ACCESS_BUFFER;
		case EAttachmentType::EXTERNAL_INPUT_ACCELERATION_STRUCTURE:				return EDescriptorType::DESCRIPTOR_ACCELERATION_STRUCTURE;

		case EAttachmentType::OUTPUT_UNORDERED_ACCESS_TEXTURE:						return EDescriptorType::DESCRIPTOR_UNORDERED_ACCESS_TEXTURE;
		case EAttachmentType::OUTPUT_UNORDERED_ACCESS_BUFFER:						return EDescriptorType::DESCRIPTOR_UNORDERED_ACCESS_BUFFER;
		case EAttachmentType::OUTPUT_COLOR:											return EDescriptorType::DESCRIPTOR_UNKNOWN;
		case EAttachmentType::OUTPUT_DEPTH_STENCIL:									return EDescriptorType::DESCRIPTOR_UNKNOWN;

		default:																	return EDescriptorType::DESCRIPTOR_UNKNOWN;
		}
	}

	bool FORCEINLINE AttachmentsNeedsDescriptor(EAttachmentType attachmentType)
	{
		switch (attachmentType)
		{
		case EAttachmentType::INPUT_SHADER_RESOURCE_TEXTURE:						return true;
		case EAttachmentType::INPUT_SHADER_RESOURCE_COMBINED_SAMPLER:				return true;
		case EAttachmentType::INPUT_UNORDERED_ACCESS_TEXTURE:						return true;
		case EAttachmentType::INPUT_UNORDERED_ACCESS_BUFFER:						return true;

		case EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_TEXTURE:				return true;
		case EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER:		return true;
		case EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_TEXTURE:				return true;
		case EAttachmentType::EXTERNAL_INPUT_CONSTANT_BUFFER:						return true;
		case EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER:				return true;
		case EAttachmentType::EXTERNAL_INPUT_ACCELERATION_STRUCTURE:				return true;

		case EAttachmentType::OUTPUT_UNORDERED_ACCESS_TEXTURE:						return true;
		case EAttachmentType::OUTPUT_UNORDERED_ACCESS_BUFFER:						return true;
		case EAttachmentType::OUTPUT_COLOR:											return false;
		case EAttachmentType::OUTPUT_DEPTH_STENCIL:									return false;

		default:																	return false;
		}
	}

	ESimpleResourceType FORCEINLINE GetSimpleType(EAttachmentType attachmentType)
	{
		switch (attachmentType)
		{
		case EAttachmentType::INPUT_SHADER_RESOURCE_TEXTURE:						return ESimpleResourceType::TEXTURE;
		case EAttachmentType::INPUT_SHADER_RESOURCE_COMBINED_SAMPLER:				return ESimpleResourceType::TEXTURE;
		case EAttachmentType::INPUT_UNORDERED_ACCESS_TEXTURE:						return ESimpleResourceType::TEXTURE;
		case EAttachmentType::INPUT_UNORDERED_ACCESS_BUFFER:						return ESimpleResourceType::BUFFER;

		case EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_TEXTURE:				return ESimpleResourceType::TEXTURE;
		case EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER:		return ESimpleResourceType::TEXTURE;
		case EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_TEXTURE:				return ESimpleResourceType::TEXTURE;
		case EAttachmentType::EXTERNAL_INPUT_CONSTANT_BUFFER:						return ESimpleResourceType::BUFFER;
		case EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER:				return ESimpleResourceType::BUFFER;
		case EAttachmentType::EXTERNAL_INPUT_ACCELERATION_STRUCTURE:				return ESimpleResourceType::ACCELERATION_STRUCTURE;

		case EAttachmentType::OUTPUT_UNORDERED_ACCESS_TEXTURE:						return ESimpleResourceType::TEXTURE;
		case EAttachmentType::OUTPUT_UNORDERED_ACCESS_BUFFER:						return ESimpleResourceType::BUFFER;
		case EAttachmentType::OUTPUT_COLOR:											return ESimpleResourceType::TEXTURE;
		case EAttachmentType::OUTPUT_DEPTH_STENCIL:									return ESimpleResourceType::TEXTURE;

		default:																	return ESimpleResourceType::NONE;
		}
	}

	ETextureState FORCEINLINE ConvertAttachmentTypeToTextureState(EAttachmentType attachmentType)
	{
		switch (attachmentType)
		{
		case EAttachmentType::INPUT_SHADER_RESOURCE_TEXTURE:						return ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;
		case EAttachmentType::INPUT_SHADER_RESOURCE_COMBINED_SAMPLER:				return ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;
		case EAttachmentType::INPUT_UNORDERED_ACCESS_TEXTURE:						return ETextureState::TEXTURE_STATE_GENERAL;
		case EAttachmentType::INPUT_UNORDERED_ACCESS_BUFFER:						return ETextureState::TEXTURE_STATE_UNKNOWN;

		case EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_TEXTURE:				return ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;
		case EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER:		return ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;
		case EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_TEXTURE:				return ETextureState::TEXTURE_STATE_GENERAL;
		case EAttachmentType::EXTERNAL_INPUT_CONSTANT_BUFFER:						return ETextureState::TEXTURE_STATE_UNKNOWN;
		case EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER:				return ETextureState::TEXTURE_STATE_UNKNOWN;
		case EAttachmentType::EXTERNAL_INPUT_ACCELERATION_STRUCTURE:				return ETextureState::TEXTURE_STATE_UNKNOWN;

		case EAttachmentType::OUTPUT_UNORDERED_ACCESS_TEXTURE:						return ETextureState::TEXTURE_STATE_GENERAL;
		case EAttachmentType::OUTPUT_UNORDERED_ACCESS_BUFFER:						return ETextureState::TEXTURE_STATE_UNKNOWN;
		case EAttachmentType::OUTPUT_COLOR:											return ETextureState::TEXTURE_STATE_RENDER_TARGET;
		case EAttachmentType::OUTPUT_DEPTH_STENCIL:									return ETextureState::TEXTURE_STATE_DEPTH_STENCIL_ATTACHMENT;

		default:																	return ETextureState::TEXTURE_STATE_UNKNOWN;
		}
	}

	bool FORCEINLINE IsAttachmentReserved(const char* pAttachmentName)
	{
		if (strcmp(pAttachmentName, RENDER_GRAPH_BACK_BUFFER_ATTACHMENT) == 0) return true;

		return false;
	}
}