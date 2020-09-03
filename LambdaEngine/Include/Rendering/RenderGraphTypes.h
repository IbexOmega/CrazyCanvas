#pragma once
#include "LambdaEngine.h"
#include "PipelineStateManager.h"

#include "Core/API/PipelineState.h"
#include "Core/API/GraphicsTypes.h"

#include "Containers/String.h"

namespace LambdaEngine
{
	constexpr const char* RENDER_GRAPH_IMGUI_STAGE_NAME			= "RENDER_STAGE_IMGUI";

	constexpr const char* RENDER_GRAPH_BACK_BUFFER_ATTACHMENT   = "BACK_BUFFER_TEXTURE";

	constexpr const char* FULLSCREEN_QUAD_VERTEX_BUFFER		    = "FULLSCREEN_QUAD_VERTEX_BUFFER";

	constexpr const char* PER_FRAME_BUFFER					    = "PER_FRAME_BUFFER";
	constexpr const char* SCENE_LIGHTS_BUFFER					= "SCENE_LIGHTS_BUFFER";

	constexpr const char* SCENE_MAT_PARAM_BUFFER				= "SCENE_MAT_PARAM_BUFFER";
	constexpr const char* SCENE_VERTEX_BUFFER					= "SCENE_VERTEX_BUFFER";
	constexpr const char* SCENE_INDEX_BUFFER					= "SCENE_INDEX_BUFFER";
	constexpr const char* SCENE_PRIMARY_INSTANCE_BUFFER			= "SCENE_PRIMARY_INSTANCE_BUFFER";
	constexpr const char* SCENE_SECONDARY_INSTANCE_BUFFER		= "SCENE_SECONDARY_INSTANCE_BUFFER";
	constexpr const char* SCENE_INDIRECT_ARGS_BUFFER			= "SCENE_INDIRECT_ARGS_BUFFER";
	constexpr const char* SCENE_TLAS							= "SCENE_TLAS";

	constexpr const char* SCENE_ALBEDO_MAPS					    = "SCENE_ALBEDO_MAPS";
	constexpr const char* SCENE_NORMAL_MAPS					    = "SCENE_NORMAL_MAPS";
	constexpr const char* SCENE_AO_MAPS						    = "SCENE_AO_MAPS";
	constexpr const char* SCENE_ROUGHNESS_MAPS				    = "SCENE_ROUGHNESS_MAPS";
	constexpr const char* SCENE_METALLIC_MAPS					= "SCENE_METALLIC_MAPS";

	enum class ERenderGraphPipelineStageType : uint8
	{
		NONE			= 0,
		RENDER			= 1,
		SYNCHRONIZATION = 2,
	};

	enum class ERenderGraphResourceType : uint8
	{
		NONE					= 0,
		TEXTURE					= 1,
		BUFFER					= 2,
		ACCELERATION_STRUCTURE	= 3,
	};

	enum class ERenderGraphResourceBindingType : uint8
	{
		NONE							= 0,
		ACCELERATION_STRUCTURE			= 1,	//READ
		CONSTANT_BUFFER					= 2,	//READ
		COMBINED_SAMPLER				= 3,	//READ
		UNORDERED_ACCESS_READ			= 4,	//READ
		UNORDERED_ACCESS_WRITE			= 5,	//WRITE
		UNORDERED_ACCESS_READ_WRITE		= 6,	//READ & WRITE
		ATTACHMENT						= 7,	//WRITE
		PRESENT							= 8,	//READ
		DRAW_RESOURCE					= 9,	//READ
	};

	enum class ERenderStageDrawType : uint8
	{
		NONE					= 0,
		SCENE_INDIRECT			= 1,
		FULLSCREEN_QUAD			= 2,
	};

	enum class ERenderStageDimensionType : uint8
	{
		NONE					= 0,
		CONSTANT				= 1,
		EXTERNAL				= 2,
		RELATIVE				= 3,
		RELATIVE_1D				= 4,
	};

	/*-----------------------------------------------------------------Resource Structs Begin-----------------------------------------------------------------*/

	struct RenderGraphResourceDesc 
	{
		String							Name					= "";
		
		ERenderGraphResourceType		Type					= ERenderGraphResourceType::NONE;
		bool							BackBufferBound			= false;
		uint32							SubResourceCount		= 1;
		bool							IsOfArrayType			= false;
		EFormat							TextureFormat			= EFormat::FORMAT_NONE;

		bool							External				= false;
		bool							Temporal				= false;
	};

	/*-----------------------------------------------------------------Resource Structs End / Render Stage Structs Begin-----------------------------------------------------------------*/

	struct GraphicsShaderNames
	{
		String TaskShaderName		= "";
		String MeshShaderName		= "";

		String VertexShaderName		= "";
		String GeometryShaderName	= "";
		String HullShaderName		= "";
		String DomainShaderName		= "";

		String PixelShaderName		= "";
	};

	struct RayTracingShaderNames
	{
		String			RaygenShaderName			= "";
		String			pMissShaderNames[MAX_MISS_SHADER_COUNT];
		String			pClosestHitShaderNames[MAX_CLOSEST_HIT_SHADER_COUNT];
		uint32			MissShaderCount			= 0;
		uint32			ClosestHitShaderCount	= 0;
	};

	struct RenderGraphResourceState
	{
		String	ResourceName			= "";
		ERenderGraphResourceBindingType BindingType = ERenderGraphResourceBindingType::NONE;

		struct
		{
			bool										PrevSameFrame		= true;
			ERenderGraphResourceBindingType	PrevBindingType		= ERenderGraphResourceBindingType::NONE;
			ERenderGraphResourceBindingType	NextBindingType		= ERenderGraphResourceBindingType::NONE;
		} AttachmentSynchronizations; //If this resource state is transitioned using a renderpass, that information is stored here
	};

	struct RenderStageParameters
	{
		ERenderStageDimensionType	XDimType		= ERenderStageDimensionType::NONE;
		ERenderStageDimensionType	YDimType		= ERenderStageDimensionType::NONE;
		ERenderStageDimensionType	ZDimType		= ERenderStageDimensionType::NONE;

		float32						XDimVariable	= 0.0f;
		float32						YDimVariable	= 0.0f;
		float32						ZDimVariable	= 0.0f;
	};

	struct RenderStageDesc
	{
		String						Name				= "";
		EPipelineStateType			Type				= EPipelineStateType::PIPELINE_STATE_TYPE_NONE;
		bool						CustomRenderer		= false;
		bool						Enabled				= true;
		uint32						Weight				= 0;
		RenderStageParameters		Parameters			= {};

		TArray<RenderGraphResourceState> ResourceStates;

		struct
		{
			GraphicsShaderNames			Shaders;
			ERenderStageDrawType		DrawType;
			String						IndexBufferName;
			String						IndirectArgsBufferName;
			bool						DepthTestEnabled;
		} Graphics;

		struct
		{	
			String						ShaderName	= "";
		} Compute;

		struct
		{
			RayTracingShaderNames		Shaders;
		} RayTracing;
	};

	/*-----------------------------------------------------------------Render Stage Structs End / Synchronization Stage Structs Begin-----------------------------------------------------------------*/

	struct RenderGraphResourceSynchronizationDesc
	{
		String							PrevRenderStage		= "";
		String							NextRenderStage		= "";
		String							ResourceName		= "";
		ECommandQueueType				PrevQueue			= ECommandQueueType::COMMAND_QUEUE_TYPE_NONE;
		ECommandQueueType				NextQueue			= ECommandQueueType::COMMAND_QUEUE_TYPE_NONE;
		ERenderGraphResourceBindingType	PrevBindingType		= ERenderGraphResourceBindingType::NONE;
		ERenderGraphResourceBindingType	NextBindingType		= ERenderGraphResourceBindingType::NONE;
	};

	struct SynchronizationStageDesc
	{
		TArray<RenderGraphResourceSynchronizationDesc> Synchronizations;
	};

	/*-----------------------------------------------------------------Synchronization Stage Structs End / Pipeline Stage Structs Begin-----------------------------------------------------------------*/

	struct PipelineStageDesc
	{
		ERenderGraphPipelineStageType Type = ERenderGraphPipelineStageType::NONE;
		uint32 StageIndex		= 0;
	};

	/*-----------------------------------------------------------------Pipeline Stage Structs End-----------------------------------------------------------------*/

	struct RenderGraphStructureDesc
	{
		TArray<RenderGraphResourceDesc>		ResourceDescriptions;
		TArray<RenderStageDesc>				RenderStageDescriptions;
		TArray<SynchronizationStageDesc>	SynchronizationStageDescriptions;
		TArray<PipelineStageDesc>			PipelineStageDescriptions;
	};

	FORCEINLINE bool ResourceStateNeedsDescriptor(ERenderGraphResourceBindingType bindingType)
	{
		switch (bindingType)
		{
		case ERenderGraphResourceBindingType::ACCELERATION_STRUCTURE:			return true;
		case ERenderGraphResourceBindingType::CONSTANT_BUFFER:				return true;
		case ERenderGraphResourceBindingType::COMBINED_SAMPLER:				return true;
		case ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ:			return true;
		case ERenderGraphResourceBindingType::UNORDERED_ACCESS_WRITE:			return true;
		case ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ_WRITE:	return true;
		case ERenderGraphResourceBindingType::ATTACHMENT:						return false;
		case ERenderGraphResourceBindingType::PRESENT:						return false;
		case ERenderGraphResourceBindingType::DRAW_RESOURCE:					return false;

		default:															return false;
		}
	}

	FORCEINLINE uint32 CreateShaderStageMask(const RenderStageDesc* pRenderStageDesc)
	{
		uint32 mask = 0;

		if (pRenderStageDesc->Type == EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS)
		{
			if (pRenderStageDesc->Graphics.Shaders.TaskShaderName.size()		> 0)	mask |= FShaderStageFlags::SHADER_STAGE_FLAG_TASK_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.MeshShaderName.size()		> 0)	mask |= FShaderStageFlags::SHADER_STAGE_FLAG_MESH_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.VertexShaderName.size()		> 0)	mask |= FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.GeometryShaderName.size()	> 0)	mask |= FShaderStageFlags::SHADER_STAGE_FLAG_GEOMETRY_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.HullShaderName.size()		> 0)	mask |= FShaderStageFlags::SHADER_STAGE_FLAG_HULL_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.DomainShaderName.size()		> 0)	mask |= FShaderStageFlags::SHADER_STAGE_FLAG_DOMAIN_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.PixelShaderName.size()		> 0)	mask |= FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER;
		}
		else if (pRenderStageDesc->Type == EPipelineStateType::PIPELINE_STATE_TYPE_COMPUTE)
		{
			if (pRenderStageDesc->Compute.ShaderName.size()						> 0)	mask |= FShaderStageFlags::SHADER_STAGE_FLAG_COMPUTE_SHADER;
		}
		else if (pRenderStageDesc->Type == EPipelineStateType::PIPELINE_STATE_TYPE_RAY_TRACING)
		{
			if (pRenderStageDesc->RayTracing.Shaders.RaygenShaderName.size()	> 0)	mask |= FShaderStageFlags::SHADER_STAGE_FLAG_RAYGEN_SHADER;
			if (pRenderStageDesc->RayTracing.Shaders.ClosestHitShaderCount		> 0)	mask |= FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER;
			if (pRenderStageDesc->RayTracing.Shaders.MissShaderCount			> 0)	mask |= FShaderStageFlags::SHADER_STAGE_FLAG_MISS_SHADER;
		}

		return mask;
	}

	FORCEINLINE EDescriptorType CalculateResourceStateDescriptorType(ERenderGraphResourceType resourceType, ERenderGraphResourceBindingType bindingType)
	{
		if (resourceType == ERenderGraphResourceType::TEXTURE)
		{
			switch (bindingType)
			{
			case ERenderGraphResourceBindingType::COMBINED_SAMPLER:					return EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
			case ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ:			return EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_TEXTURE;
			case ERenderGraphResourceBindingType::UNORDERED_ACCESS_WRITE:			return EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_TEXTURE;
			case ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ_WRITE:		return EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_TEXTURE;
			}
		}
		else if (resourceType == ERenderGraphResourceType::BUFFER)
		{
			switch (bindingType)
			{
			case ERenderGraphResourceBindingType::CONSTANT_BUFFER:					return EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER;
			case ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ:			return EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
			case ERenderGraphResourceBindingType::UNORDERED_ACCESS_WRITE:			return EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
			case ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ_WRITE:		return EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
			}
		}
		else if (resourceType == ERenderGraphResourceType::ACCELERATION_STRUCTURE)
		{
			switch (bindingType)
			{
			case ERenderGraphResourceBindingType::ACCELERATION_STRUCTURE:			return EDescriptorType::DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE;
			case ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ:			return EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
			case ERenderGraphResourceBindingType::UNORDERED_ACCESS_WRITE:			return EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
			case ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ_WRITE:		return EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
			}
		}

		return EDescriptorType::DESCRIPTOR_TYPE_UNKNOWN;
	}

	FORCEINLINE ETextureState CalculateResourceTextureState(ERenderGraphResourceType resourceType, ERenderGraphResourceBindingType bindingType, EFormat format)
	{
		if (resourceType == ERenderGraphResourceType::TEXTURE)
		{
			switch (bindingType)
			{
			case ERenderGraphResourceBindingType::COMBINED_SAMPLER:					return ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;
			case ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ:			return ETextureState::TEXTURE_STATE_GENERAL;
			case ERenderGraphResourceBindingType::UNORDERED_ACCESS_WRITE:			return ETextureState::TEXTURE_STATE_GENERAL;
			case ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ_WRITE:		return ETextureState::TEXTURE_STATE_GENERAL;
			case ERenderGraphResourceBindingType::ATTACHMENT:						return format != EFormat::FORMAT_D24_UNORM_S8_UINT ? ETextureState::TEXTURE_STATE_RENDER_TARGET : ETextureState::TEXTURE_STATE_DEPTH_STENCIL_ATTACHMENT;
			case ERenderGraphResourceBindingType::PRESENT:							return ETextureState::TEXTURE_STATE_PRESENT;
			}
		}

		return ETextureState::TEXTURE_STATE_UNKNOWN;
	}

	FORCEINLINE uint32 CalculateResourceAccessFlags(ERenderGraphResourceBindingType bindingType)
	{
		switch (bindingType)
		{
			case ERenderGraphResourceBindingType::ACCELERATION_STRUCTURE:			return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_ACCELERATION_STRUCTURE_READ;
			case ERenderGraphResourceBindingType::CONSTANT_BUFFER:					return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_CONSTANT_BUFFER_READ;
			case ERenderGraphResourceBindingType::COMBINED_SAMPLER:					return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_SHADER_READ;
			case ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ:			return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_READ;
			case ERenderGraphResourceBindingType::UNORDERED_ACCESS_WRITE:			return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
			case ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ_WRITE:		return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_READ | FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
			case ERenderGraphResourceBindingType::ATTACHMENT:						return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
			case ERenderGraphResourceBindingType::PRESENT:							return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_READ;
			case ERenderGraphResourceBindingType::DRAW_RESOURCE:					return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_READ;																
		}

		return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_UNKNOWN;
	}

	FORCEINLINE FPipelineStageFlags FindEarliestPipelineStage(const RenderStageDesc* pRenderStageDesc)
	{
		if (pRenderStageDesc->Type == EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS)
		{
			if (pRenderStageDesc->Graphics.Shaders.TaskShaderName.size()		> 0)		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_TASK_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.MeshShaderName.size()		> 0)		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_MESH_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.VertexShaderName.size()		> 0)		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_VERTEX_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.GeometryShaderName.size()	> 0)		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_GEOMETRY_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.HullShaderName.size()		> 0)		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_HULL_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.DomainShaderName.size()		> 0)		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_DOMAIN_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.PixelShaderName.size()		> 0)		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_PIXEL_SHADER;
		}
		else if (pRenderStageDesc->Type == EPipelineStateType::PIPELINE_STATE_TYPE_COMPUTE)
		{
			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_COMPUTE_SHADER;
		}
		else if (pRenderStageDesc->Type == EPipelineStateType::PIPELINE_STATE_TYPE_RAY_TRACING)
		{
			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER;
		}

		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_UNKNOWN;
	}

	FORCEINLINE FPipelineStageFlags FindEarliestCompatiblePipelineStage(uint32 pipelineStageMask, ECommandQueueType commandQueueType)
	{
		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP)								return FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP;

		if (commandQueueType == ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS)
		{
			if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_DRAW_INDIRECT)					return FPipelineStageFlags::PIPELINE_STAGE_FLAG_DRAW_INDIRECT;
			if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_VERTEX_INPUT)					return FPipelineStageFlags::PIPELINE_STAGE_FLAG_VERTEX_INPUT;
			if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_VERTEX_SHADER)					return FPipelineStageFlags::PIPELINE_STAGE_FLAG_VERTEX_SHADER;
			if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_HULL_SHADER)					return FPipelineStageFlags::PIPELINE_STAGE_FLAG_HULL_SHADER;
			if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_DOMAIN_SHADER)					return FPipelineStageFlags::PIPELINE_STAGE_FLAG_DOMAIN_SHADER;
			if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_GEOMETRY_SHADER)				return FPipelineStageFlags::PIPELINE_STAGE_FLAG_GEOMETRY_SHADER;
			if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_PIXEL_SHADER)					return FPipelineStageFlags::PIPELINE_STAGE_FLAG_PIXEL_SHADER;
			if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_EARLY_FRAGMENT_TESTS)			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_EARLY_FRAGMENT_TESTS;
			if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_LATE_FRAGMENT_TESTS)			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_LATE_FRAGMENT_TESTS;
			if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT)			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;

			if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_STREAM_OUTPUT)					return FPipelineStageFlags::PIPELINE_STAGE_FLAG_STREAM_OUTPUT;
			if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_CONDITIONAL_RENDERING)			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_CONDITIONAL_RENDERING;
			if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_SHADING_RATE_TEXTURE)			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_SHADING_RATE_TEXTURE;

			if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_TASK_SHADER)					return FPipelineStageFlags::PIPELINE_STAGE_FLAG_TASK_SHADER;
			if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_MESH_SHADER)					return FPipelineStageFlags::PIPELINE_STAGE_FLAG_MESH_SHADER;
		}
		else if (commandQueueType == ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE)
		{
			if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_COMPUTE_SHADER)				return FPipelineStageFlags::PIPELINE_STAGE_FLAG_COMPUTE_SHADER;
			if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER)			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER;
			if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_ACCELERATION_STRUCTURE_BUILD)	return FPipelineStageFlags::PIPELINE_STAGE_FLAG_ACCELERATION_STRUCTURE_BUILD;
		}

		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_BOTTOM)							return FPipelineStageFlags::PIPELINE_STAGE_FLAG_BOTTOM;

		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_UNKNOWN;
	}

	FORCEINLINE FPipelineStageFlags FindEarliestPipelineStage(uint32 pipelineStageMask)
	{
		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP)							return FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP;

		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_DRAW_INDIRECT)					return FPipelineStageFlags::PIPELINE_STAGE_FLAG_DRAW_INDIRECT;
		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_VERTEX_INPUT)					return FPipelineStageFlags::PIPELINE_STAGE_FLAG_VERTEX_INPUT;
		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_VERTEX_SHADER)					return FPipelineStageFlags::PIPELINE_STAGE_FLAG_VERTEX_SHADER;
		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_HULL_SHADER)					return FPipelineStageFlags::PIPELINE_STAGE_FLAG_HULL_SHADER;
		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_DOMAIN_SHADER)					return FPipelineStageFlags::PIPELINE_STAGE_FLAG_DOMAIN_SHADER;
		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_GEOMETRY_SHADER)				return FPipelineStageFlags::PIPELINE_STAGE_FLAG_GEOMETRY_SHADER;
		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_PIXEL_SHADER)					return FPipelineStageFlags::PIPELINE_STAGE_FLAG_PIXEL_SHADER;
		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_EARLY_FRAGMENT_TESTS)			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_EARLY_FRAGMENT_TESTS;
		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_LATE_FRAGMENT_TESTS)			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_LATE_FRAGMENT_TESTS;
		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT)			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;

		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_COMPUTE_SHADER)				return FPipelineStageFlags::PIPELINE_STAGE_FLAG_COMPUTE_SHADER;

		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_COPY)							return FPipelineStageFlags::PIPELINE_STAGE_FLAG_COPY;
		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_HOST)							return FPipelineStageFlags::PIPELINE_STAGE_FLAG_HOST;

		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_STREAM_OUTPUT)					return FPipelineStageFlags::PIPELINE_STAGE_FLAG_STREAM_OUTPUT;

		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_CONDITIONAL_RENDERING)			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_CONDITIONAL_RENDERING;

		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER)			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER;
		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_ACCELERATION_STRUCTURE_BUILD)	return FPipelineStageFlags::PIPELINE_STAGE_FLAG_ACCELERATION_STRUCTURE_BUILD;

		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_SHADING_RATE_TEXTURE)			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_SHADING_RATE_TEXTURE;

		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_TASK_SHADER)					return FPipelineStageFlags::PIPELINE_STAGE_FLAG_TASK_SHADER;
		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_MESH_SHADER)					return FPipelineStageFlags::PIPELINE_STAGE_FLAG_MESH_SHADER;

		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_BOTTOM)						return FPipelineStageFlags::PIPELINE_STAGE_FLAG_BOTTOM;

		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_UNKNOWN;
	}

	FORCEINLINE FPipelineStageFlags FindLastPipelineStage(uint32 pipelineStageMask)
	{
		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_BOTTOM)						return FPipelineStageFlags::PIPELINE_STAGE_FLAG_BOTTOM;

		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_MESH_SHADER)					return FPipelineStageFlags::PIPELINE_STAGE_FLAG_MESH_SHADER;
		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_TASK_SHADER)					return FPipelineStageFlags::PIPELINE_STAGE_FLAG_TASK_SHADER;

		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_SHADING_RATE_TEXTURE)			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_SHADING_RATE_TEXTURE;

		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_ACCELERATION_STRUCTURE_BUILD)	return FPipelineStageFlags::PIPELINE_STAGE_FLAG_ACCELERATION_STRUCTURE_BUILD;
		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER)			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER;

		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_CONDITIONAL_RENDERING)			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_CONDITIONAL_RENDERING;

		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_STREAM_OUTPUT)					return FPipelineStageFlags::PIPELINE_STAGE_FLAG_STREAM_OUTPUT;

		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_HOST)							return FPipelineStageFlags::PIPELINE_STAGE_FLAG_HOST;
		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_COPY)							return FPipelineStageFlags::PIPELINE_STAGE_FLAG_COPY;

		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_COMPUTE_SHADER)				return FPipelineStageFlags::PIPELINE_STAGE_FLAG_COMPUTE_SHADER;

		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT)			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;
		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_LATE_FRAGMENT_TESTS)			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_LATE_FRAGMENT_TESTS;
		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_EARLY_FRAGMENT_TESTS)			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_EARLY_FRAGMENT_TESTS;
		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_PIXEL_SHADER)					return FPipelineStageFlags::PIPELINE_STAGE_FLAG_PIXEL_SHADER;
		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_GEOMETRY_SHADER)				return FPipelineStageFlags::PIPELINE_STAGE_FLAG_GEOMETRY_SHADER;
		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_DOMAIN_SHADER)					return FPipelineStageFlags::PIPELINE_STAGE_FLAG_DOMAIN_SHADER;
		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_HULL_SHADER)					return FPipelineStageFlags::PIPELINE_STAGE_FLAG_HULL_SHADER;
		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_VERTEX_SHADER)					return FPipelineStageFlags::PIPELINE_STAGE_FLAG_VERTEX_SHADER;
		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_VERTEX_INPUT)					return FPipelineStageFlags::PIPELINE_STAGE_FLAG_VERTEX_INPUT;
		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_DRAW_INDIRECT)					return FPipelineStageFlags::PIPELINE_STAGE_FLAG_DRAW_INDIRECT;

		if (pipelineStageMask & FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP)							return FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP;

		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_UNKNOWN;
	}

	FORCEINLINE FPipelineStageFlags FindLastPipelineStage(const RenderStageDesc* pRenderStageDesc)
	{
		if (pRenderStageDesc->Type == EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS)
		{
			if (pRenderStageDesc->Graphics.Shaders.PixelShaderName.size() > 0)		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_PIXEL_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.DomainShaderName.size() > 0)		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_DOMAIN_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.HullShaderName.size() > 0)		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_HULL_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.GeometryShaderName.size() > 0)	return FPipelineStageFlags::PIPELINE_STAGE_FLAG_GEOMETRY_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.VertexShaderName.size() > 0)		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_VERTEX_INPUT;
			if (pRenderStageDesc->Graphics.Shaders.MeshShaderName.size() > 0)		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_MESH_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.TaskShaderName.size() > 0)		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_TASK_SHADER;
		}
		else if (pRenderStageDesc->Type == EPipelineStateType::PIPELINE_STATE_TYPE_COMPUTE)
		{
			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_COMPUTE_SHADER;
		}
		else if (pRenderStageDesc->Type == EPipelineStateType::PIPELINE_STATE_TYPE_RAY_TRACING)
		{
			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER;
		}

		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_UNKNOWN;
	}

	FORCEINLINE bool IsReadOnly(ERenderGraphResourceBindingType bindingType)
	{
		switch (bindingType)
		{
			case ERenderGraphResourceBindingType::ACCELERATION_STRUCTURE:		return true;
			case ERenderGraphResourceBindingType::CONSTANT_BUFFER:				return true;
			case ERenderGraphResourceBindingType::COMBINED_SAMPLER:				return true;
			case ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ:		return true;
			case ERenderGraphResourceBindingType::UNORDERED_ACCESS_WRITE:		return false;
			case ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ_WRITE:	return false;
			case ERenderGraphResourceBindingType::ATTACHMENT:					return false;
			case ERenderGraphResourceBindingType::PRESENT:						return true;
			case ERenderGraphResourceBindingType::DRAW_RESOURCE:				return true;
		}

		return false;
	}

	FORCEINLINE String RenderStageDrawTypeToString(ERenderStageDrawType drawType)
	{
		switch (drawType)
		{
		case ERenderStageDrawType::SCENE_INDIRECT:		return "SCENE_INDIRECT";
		case ERenderStageDrawType::FULLSCREEN_QUAD:		return "FULLSCREEN_QUAD";
		default:													return "NONE";
		}
	}

	FORCEINLINE ERenderStageDrawType RenderStageDrawTypeFromString(const String& string)
	{
		if (string == "SCENE_INDIRECT")		return ERenderStageDrawType::SCENE_INDIRECT;
		if (string == "FULLSCREEN_QUAD")	return ERenderStageDrawType::FULLSCREEN_QUAD;
		return ERenderStageDrawType::NONE;
	}

	FORCEINLINE String BindingTypeToShortString(ERenderGraphResourceBindingType bindingType)
	{
		switch (bindingType)
		{
		case ERenderGraphResourceBindingType::ACCELERATION_STRUCTURE:		return "AS";
		case ERenderGraphResourceBindingType::CONSTANT_BUFFER:				return "CONST_BUF";
		case ERenderGraphResourceBindingType::COMBINED_SAMPLER:				return "COMB_SMPL";
		case ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ:		return "UA_R";
		case ERenderGraphResourceBindingType::UNORDERED_ACCESS_WRITE:		return "UA_W";
		case ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ_WRITE:	return "UA_RW";
		case ERenderGraphResourceBindingType::ATTACHMENT:					return "ATTACHMENT";
		case ERenderGraphResourceBindingType::PRESENT:						return "PRESENT";
		case ERenderGraphResourceBindingType::DRAW_RESOURCE:				return "DR";
		}

		return "UNKNOWN";
	}

	FORCEINLINE String BindingTypeToString(ERenderGraphResourceBindingType bindingType)
	{
		switch (bindingType)
		{
		case ERenderGraphResourceBindingType::ACCELERATION_STRUCTURE:		return "ACCELERATION_STRUCTURE";
		case ERenderGraphResourceBindingType::CONSTANT_BUFFER:				return "CONSTANT_BUFFER";
		case ERenderGraphResourceBindingType::COMBINED_SAMPLER:				return "COMBINED_SAMPLER";
		case ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ:		return "UNORDERED_ACCESS_R";
		case ERenderGraphResourceBindingType::UNORDERED_ACCESS_WRITE:		return "UNORDERED_ACCESS_W";
		case ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ_WRITE:	return "UNORDERED_ACCESS_RW";
		case ERenderGraphResourceBindingType::ATTACHMENT:					return "ATTACHMENT";
		case ERenderGraphResourceBindingType::PRESENT:						return "PRESENT";
		case ERenderGraphResourceBindingType::DRAW_RESOURCE:				return "DRAW_RESOURCE";
		}

		return "UNKNOWN";
	}

	FORCEINLINE ERenderGraphResourceBindingType ResourceStateBindingTypeFromString(const String& string)
	{
		if (string == "ACCELERATION_STRUCTURE")		return ERenderGraphResourceBindingType::ACCELERATION_STRUCTURE;
		if (string == "CONSTANT_BUFFER")			return ERenderGraphResourceBindingType::CONSTANT_BUFFER;
		if (string == "COMBINED_SAMPLER")			return ERenderGraphResourceBindingType::COMBINED_SAMPLER;
		if (string == "UNORDERED_ACCESS_R")			return ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ;
		if (string == "UNORDERED_ACCESS_W")			return ERenderGraphResourceBindingType::UNORDERED_ACCESS_WRITE;
		if (string == "UNORDERED_ACCESS_RW")		return ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ_WRITE;
		if (string == "ATTACHMENT")					return ERenderGraphResourceBindingType::ATTACHMENT;
		if (string == "PRESENT")					return ERenderGraphResourceBindingType::PRESENT;
		if (string == "DRAW_RESOURCE")				return ERenderGraphResourceBindingType::DRAW_RESOURCE;
		return ERenderGraphResourceBindingType::NONE;
	}


	FORCEINLINE String RenderStageTypeToString(EPipelineStateType type)
	{
		switch (type)
		{
		case EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS:						return "GRAPHICS";
		case EPipelineStateType::PIPELINE_STATE_TYPE_COMPUTE:						return "COMPUTE";
		case EPipelineStateType::PIPELINE_STATE_TYPE_RAY_TRACING:					return "RAY_TRACING";
		default:												return "NONE";
		}
	}

	FORCEINLINE EPipelineStateType RenderStageTypeFromString(const String& string)
	{
		if		(string == "GRAPHICS")		return EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS;
		else if (string == "COMPUTE")		return EPipelineStateType::PIPELINE_STATE_TYPE_COMPUTE;
		else if (string == "RAY_TRACING")	return EPipelineStateType::PIPELINE_STATE_TYPE_RAY_TRACING;

		return EPipelineStateType::PIPELINE_STATE_TYPE_NONE;
	}

	FORCEINLINE String RenderGraphResourceTypeToString(ERenderGraphResourceType type)
	{
		switch (type)
		{
		case ERenderGraphResourceType::TEXTURE:						return "TEXTURE";
		case ERenderGraphResourceType::BUFFER:						return "BUFFER";
		case ERenderGraphResourceType::ACCELERATION_STRUCTURE:		return "ACCELERATION_STRUCTURE";
		default:													return "NONE";
		}
	}

	FORCEINLINE ERenderGraphResourceType RenderGraphResourceTypeFromString(const String& string)
	{
		if		(string == "TEXTURE")					return ERenderGraphResourceType::TEXTURE;
		else if (string == "BUFFER")					return ERenderGraphResourceType::BUFFER;
		else if (string == "ACCELERATION_STRUCTURE")	return ERenderGraphResourceType::ACCELERATION_STRUCTURE;

		return ERenderGraphResourceType::NONE;
	}

	FORCEINLINE String RenderStageDimensionTypeToString(ERenderStageDimensionType dimensionType)
	{
		switch (dimensionType)
		{
			case ERenderStageDimensionType::CONSTANT:		return "CONSTANT";
			case ERenderStageDimensionType::RELATIVE:		return "RELATIVE";
;			case ERenderStageDimensionType::EXTERNAL:		return "EXTERNAL";
			case ERenderStageDimensionType::RELATIVE_1D:	return "RELATIVE_1D";
			default:										return "NONE";
		}
	}

	FORCEINLINE ERenderStageDimensionType RenderStageDimensionTypeFromString(const String& string)
	{
		if		(string == "CONSTANT")		return ERenderStageDimensionType::CONSTANT;
		else if (string == "RELATIVE")		return ERenderStageDimensionType::RELATIVE;
		else if (string == "EXTERNAL")		return ERenderStageDimensionType::EXTERNAL;
		else if (string == "RELATIVE_1D")	return ERenderStageDimensionType::RELATIVE_1D;

		return ERenderStageDimensionType::NONE;
	}
}
