#pragma once

#include "LambdaEngine.h"
#include "Core/API/GraphicsTypes.h"
#include "Core/API/IPipelineState.h"
#include "PipelineStateManager.h"

namespace LambdaEngine
{
	class ICustomRenderer;

	constexpr const char* RENDER_GRAPH_IMGUI_STAGE_NAME			= "RENDER_STAGE_IMGUI";

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

	struct RenderStageAttachment //Todo: Rename to Resource
	{
		const char*			pName				= "";
		EAttachmentType		Type				= EAttachmentType::NONE;
		uint32				ShaderStages		= SHADER_STAGE_FLAG_NONE;
		uint32				SubResourceCount	= 1;
		bool				TemporalInput		= false;
		EFormat				TextureFormat		= EFormat::NONE;
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
		const char*					pName				= "Render Stage";
		RenderStageAttachment*		pAttachments		= nullptr;
		uint32						AttachmentCount		= 0;
		RenderStagePushConstants	PushConstants		= {};

		EPipelineStateType			PipelineType		= EPipelineStateType::NONE;
		bool						UsesCustomRenderer	= false;
		uint32						Priority			= UINT32_MAX;

		union
		{
			struct
			{
				GraphicsManagedPipelineStateDesc*		pGraphicsDesc;
				ERenderStageDrawType					DrawType;
				const char*								pIndexBufferName;
				const char*								pMeshIndexBufferName;
			} GraphicsPipeline;

			struct
			{
				ComputeManagedPipelineStateDesc*		pComputeDesc;
			} ComputePipeline;

			struct
			{
				RayTracingManagedPipelineStateDesc*		pRayTracingDesc;
			} RayTracingPipeline;

			struct
			{
				ICustomRenderer*						pCustomRenderer;
			} CustomRenderer;
		};
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

	FORCEINLINE EAttachmentAccessType GetAttachmentAccessType(EAttachmentType attachmentType)
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

	FORCEINLINE EDescriptorType GetAttachmentDescriptorType(EAttachmentType attachmentType)
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

	FORCEINLINE bool AttachmentsNeedsDescriptor(EAttachmentType attachmentType)
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

	FORCEINLINE ESimpleResourceType GetSimpleType(EAttachmentType attachmentType)
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

	FORCEINLINE FMemoryAccessFlags ConvertAttachmentTypeToMemoryAccessFlags(EAttachmentType attachmentType)
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
		case EAttachmentType::OUTPUT_COLOR:											return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_WRITE; //FMemoryAccessFlags::MEMORY_ACCESS_FLAG_COLOR_ATTACHMENT_WRITE;
		case EAttachmentType::OUTPUT_DEPTH_STENCIL:									return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_WRITE; //FMemoryAccessFlags::MEMORY_ACCESS_FLAG_DEPTH_STENCIL_ATTACHMENT_WRITE;

		default:																	return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_UNKNOWN;
		}
	}

	FORCEINLINE FPipelineStageFlags FindEarliestPipelineStage(const RenderStageDesc* pRenderStageDesc)
	{
		uint32 shaderStageMask = 0;

		if (pRenderStageDesc->PipelineType == EPipelineStateType::GRAPHICS)
		{
			if (pRenderStageDesc->GraphicsPipeline.pGraphicsDesc->MeshShader		!= GUID_NONE)	return FPipelineStageFlags::PIPELINE_STAGE_FLAG_MESH_SHADER;
			if (pRenderStageDesc->GraphicsPipeline.pGraphicsDesc->VertexShader		!= GUID_NONE)	return FPipelineStageFlags::PIPELINE_STAGE_FLAG_VERTEX_INPUT;
			if (pRenderStageDesc->GraphicsPipeline.pGraphicsDesc->GeometryShader	!= GUID_NONE)	return FPipelineStageFlags::PIPELINE_STAGE_FLAG_GEOMETRY_SHADER;
			if (pRenderStageDesc->GraphicsPipeline.pGraphicsDesc->HullShader		!= GUID_NONE)	return FPipelineStageFlags::PIPELINE_STAGE_FLAG_HULL_SHADER;
			if (pRenderStageDesc->GraphicsPipeline.pGraphicsDesc->DomainShader		!= GUID_NONE)	return FPipelineStageFlags::PIPELINE_STAGE_FLAG_DOMAIN_SHADER;
			if (pRenderStageDesc->GraphicsPipeline.pGraphicsDesc->PixelShader		!= GUID_NONE)	return FPipelineStageFlags::PIPELINE_STAGE_FLAG_PIXEL_SHADER;
		}
		else if (pRenderStageDesc->PipelineType == EPipelineStateType::COMPUTE)
		{
			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_COMPUTE_SHADER;
		}
		else if (pRenderStageDesc->PipelineType == EPipelineStateType::RAY_TRACING)
		{
			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER;
		}

		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_UNKNOWN;
	}

	FORCEINLINE FPipelineStageFlags FindLastPipelineStage(const RenderStageDesc* pRenderStageDesc)
	{
		uint32 shaderStageMask = 0;

		if (pRenderStageDesc->PipelineType == EPipelineStateType::GRAPHICS)
		{
			if (pRenderStageDesc->GraphicsPipeline.pGraphicsDesc->PixelShader		!= GUID_NONE)	return FPipelineStageFlags::PIPELINE_STAGE_FLAG_PIXEL_SHADER;
			if (pRenderStageDesc->GraphicsPipeline.pGraphicsDesc->DomainShader		!= GUID_NONE)	return FPipelineStageFlags::PIPELINE_STAGE_FLAG_DOMAIN_SHADER;
			if (pRenderStageDesc->GraphicsPipeline.pGraphicsDesc->HullShader		!= GUID_NONE)	return FPipelineStageFlags::PIPELINE_STAGE_FLAG_HULL_SHADER;
			if (pRenderStageDesc->GraphicsPipeline.pGraphicsDesc->GeometryShader	!= GUID_NONE)	return FPipelineStageFlags::PIPELINE_STAGE_FLAG_GEOMETRY_SHADER;
			if (pRenderStageDesc->GraphicsPipeline.pGraphicsDesc->VertexShader		!= GUID_NONE)	return FPipelineStageFlags::PIPELINE_STAGE_FLAG_VERTEX_INPUT;
			if (pRenderStageDesc->GraphicsPipeline.pGraphicsDesc->MeshShader		!= GUID_NONE)	return FPipelineStageFlags::PIPELINE_STAGE_FLAG_MESH_SHADER;
		}
		else if (pRenderStageDesc->PipelineType == EPipelineStateType::COMPUTE)
		{
			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_COMPUTE_SHADER;
		}
		else if (pRenderStageDesc->PipelineType == EPipelineStateType::RAY_TRACING)
		{
			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER;
		}

		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_UNKNOWN;
	}

	FORCEINLINE uint32 CreateShaderStageMask(const RenderStageDesc* pRenderStageDesc)
	{
		uint32 shaderStageMask = 0;

		if (pRenderStageDesc->PipelineType == EPipelineStateType::GRAPHICS)
		{
			shaderStageMask |= (pRenderStageDesc->GraphicsPipeline.pGraphicsDesc->MeshShader		!= GUID_NONE)	? FShaderStageFlags::SHADER_STAGE_FLAG_MESH_SHADER		: 0;

			shaderStageMask |= (pRenderStageDesc->GraphicsPipeline.pGraphicsDesc->VertexShader		!= GUID_NONE)	? FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER	: 0;
			shaderStageMask |= (pRenderStageDesc->GraphicsPipeline.pGraphicsDesc->GeometryShader	!= GUID_NONE)	? FShaderStageFlags::SHADER_STAGE_FLAG_GEOMETRY_SHADER	: 0;
			shaderStageMask |= (pRenderStageDesc->GraphicsPipeline.pGraphicsDesc->HullShader		!= GUID_NONE)	? FShaderStageFlags::SHADER_STAGE_FLAG_HULL_SHADER		: 0;
			shaderStageMask |= (pRenderStageDesc->GraphicsPipeline.pGraphicsDesc->DomainShader		!= GUID_NONE)	? FShaderStageFlags::SHADER_STAGE_FLAG_DOMAIN_SHADER	: 0;

			shaderStageMask |= (pRenderStageDesc->GraphicsPipeline.pGraphicsDesc->PixelShader		!= GUID_NONE)	? FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER		: 0;
		}
		else if (pRenderStageDesc->PipelineType == EPipelineStateType::COMPUTE)
		{
			shaderStageMask |= FShaderStageFlags::SHADER_STAGE_FLAG_COMPUTE_SHADER;
		}
		else if (pRenderStageDesc->PipelineType == EPipelineStateType::RAY_TRACING)
		{
			shaderStageMask |= FShaderStageFlags::SHADER_STAGE_FLAG_RAYGEN_SHADER;
			shaderStageMask |= FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER;
			shaderStageMask |= FShaderStageFlags::SHADER_STAGE_FLAG_MISS_SHADER;
		}

		return shaderStageMask;
	}
}
