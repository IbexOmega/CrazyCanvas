#pragma once
#include "LambdaEngine.h"

#include "Core/API/PipelineState.h"

#include "PipelineStateManager.h"

namespace LambdaEngine
{
	constexpr const char* RENDER_GRAPH_BACK_BUFFER_ATTACHMENT   = "BACK_BUFFER_TEXTURE";

	constexpr const char* FULLSCREEN_QUAD_VERTEX_BUFFER		    = "FULLSCREEN_QUAD_VERTEX_BUFFER";

	constexpr const char* PER_FRAME_BUFFER					    = "PER_FRAME_BUFFER";

	constexpr const char* SCENE_MAT_PARAM_BUFFER				= "SCENE_MAT_PARAM_BUFFER";
	constexpr const char* SCENE_VERTEX_BUFFER					= "SCENE_VERTEX_BUFFER";
	constexpr const char* SCENE_INDEX_BUFFER					= "SCENE_INDEX_BUFFER";
	constexpr const char* SCENE_INSTANCE_BUFFER				    = "SCENE_INSTANCE_BUFFER";
	constexpr const char* SCENE_MESH_INDEX_BUFFER				= "SCENE_MESH_INDEX_BUFFER";

	constexpr const char* SCENE_ALBEDO_MAPS					    = "SCENE_ALBEDO_MAPS";
	constexpr const char* SCENE_NORMAL_MAPS					    = "SCENE_NORMAL_MAPS";
	constexpr const char* SCENE_AO_MAPS						    = "SCENE_AO_MAPS";
	constexpr const char* SCENE_ROUGHNESS_MAPS				    = "SCENE_ROUGHNESS_MAPS";
	constexpr const char* SCENE_METALLIC_MAPS					= "SCENE_METALLIC_MAPS";

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
		OWNERSHIP_CHANGE_WRITE	= 3,
		OWNERSHIP_CHANGE_READ	= 4,
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
		NONE						= 0,
		TEXTURE						= 1,
		BUFFER						= 2,
		ACCELERATION_STRUCTURE		= 3,
		COLOR_ATTACHMENT			= 4,
		DEPTH_STENCIL_ATTACHMENT	= 5,
	};

	enum class ERenderStageResourceType : uint8
	{
		NONE			= 0,
		ATTACHMENT		= 1,
		PUSH_CONSTANTS	= 2,
	};

	enum class ERenderStageDrawType : uint8
	{
		NONE					= 0,
		SCENE_INDIRECT			= 1,
		FULLSCREEN_QUAD			= 2,
	};

	struct RenderStageAttachment
	{
		const char*			pName				= "";
		EAttachmentType		Type				= EAttachmentType::NONE;
		uint32				ShaderStages		= SHADER_STAGE_FLAG_NONE;
		uint32				SubResourceCount	= 1;
		EFormat				TextureFormat		= EFormat::FORMAT_NONE;
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
		RenderStageAttachment* pAttachments			= nullptr;
		uint32 AttachmentCount						= 0;
		RenderStagePushConstants PushConstants		= {};

		EPipelineStateType PipelineType				= EPipelineStateType::PIPELINE_STATE_TYPE_NONE;

		union
		{
			struct
			{
				ManagedGraphicsPipelineStateDesc*		pGraphicsDesc;
				ERenderStageDrawType					DrawType;
				const char*								pIndexBufferName;
				const char*								pMeshIndexBufferName;
			} GraphicsPipeline;

			struct
			{
				ManagedComputePipelineStateDesc*		pComputeDesc;
			} ComputePipeline;

			struct
			{
				ManagedRayTracingPipelineStateDesc*		pRayTracingDesc;
			} RayTracingPipeline;
		};
	};

	struct AttachmentSynchronizationDesc
	{
		EAttachmentSynchronizationType	Type			= EAttachmentSynchronizationType::NONE;
		EPipelineStateType				FromQueueOwner	= EPipelineStateType::PIPELINE_STATE_TYPE_NONE;
		EPipelineStateType				ToQueueOwner	= EPipelineStateType::PIPELINE_STATE_TYPE_NONE;
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
		case EAttachmentType::INPUT_SHADER_RESOURCE_TEXTURE:						return EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_TEXTURE;
		case EAttachmentType::INPUT_SHADER_RESOURCE_COMBINED_SAMPLER:				return EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		case EAttachmentType::INPUT_UNORDERED_ACCESS_TEXTURE:						return EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_TEXTURE;
		case EAttachmentType::INPUT_UNORDERED_ACCESS_BUFFER:						return EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;

		case EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_TEXTURE:				return EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_TEXTURE;
		case EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER:		return EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		case EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_TEXTURE:				return EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_TEXTURE;
		case EAttachmentType::EXTERNAL_INPUT_CONSTANT_BUFFER:						return EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER;
		case EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER:				return EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		case EAttachmentType::EXTERNAL_INPUT_ACCELERATION_STRUCTURE:				return EDescriptorType::DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE;

		case EAttachmentType::OUTPUT_UNORDERED_ACCESS_TEXTURE:						return EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_TEXTURE;
		case EAttachmentType::OUTPUT_UNORDERED_ACCESS_BUFFER:						return EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		case EAttachmentType::OUTPUT_COLOR:											return EDescriptorType::DESCRIPTOR_TYPE_UNKNOWN;
		case EAttachmentType::OUTPUT_DEPTH_STENCIL:									return EDescriptorType::DESCRIPTOR_TYPE_UNKNOWN;

		default:																	return EDescriptorType::DESCRIPTOR_TYPE_UNKNOWN;
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
		case EAttachmentType::OUTPUT_COLOR:											return ESimpleResourceType::COLOR_ATTACHMENT;
		case EAttachmentType::OUTPUT_DEPTH_STENCIL:									return ESimpleResourceType::DEPTH_STENCIL_ATTACHMENT;

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

	FMemoryAccessFlags FORCEINLINE ConvertAttachmentTypeToMemoryAccessFlags(EAttachmentType attachmentType)
	{
		switch (attachmentType)
		{
		case EAttachmentType::INPUT_SHADER_RESOURCE_TEXTURE:						return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_SHADER_READ;
		case EAttachmentType::INPUT_SHADER_RESOURCE_COMBINED_SAMPLER:				return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_SHADER_READ;
		case EAttachmentType::INPUT_UNORDERED_ACCESS_TEXTURE:						return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_READ;
		case EAttachmentType::INPUT_UNORDERED_ACCESS_BUFFER:						return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_READ;

		case EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_TEXTURE:				return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_SHADER_READ;
		case EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER:		return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_SHADER_READ;
		case EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_TEXTURE:				return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_READ;
		case EAttachmentType::EXTERNAL_INPUT_CONSTANT_BUFFER:						return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_CONSTANT_BUFFER_READ;
		case EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER:				return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_READ;
		case EAttachmentType::EXTERNAL_INPUT_ACCELERATION_STRUCTURE:				return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_ACCELERATION_STRUCTURE_READ;

		case EAttachmentType::OUTPUT_UNORDERED_ACCESS_TEXTURE:						return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
		case EAttachmentType::OUTPUT_UNORDERED_ACCESS_BUFFER:						return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
		case EAttachmentType::OUTPUT_COLOR:											return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_COLOR_ATTACHMENT_WRITE;
		case EAttachmentType::OUTPUT_DEPTH_STENCIL:									return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_DEPTH_STENCIL_ATTACHMENT_WRITE;

		default:																	return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_UNKNOWN;
		}
	}
}
