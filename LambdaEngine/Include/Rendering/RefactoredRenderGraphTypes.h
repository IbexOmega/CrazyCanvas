#pragma once

#include "LambdaEngine.h"
#include "Core/API/GraphicsTypes.h"
#include "Core/API/IPipelineState.h"
#include "PipelineStateManager.h"

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
	constexpr const char* SCENE_INSTANCE_BUFFER				    = "SCENE_INSTANCE_BUFFER";
	constexpr const char* SCENE_INDIRECT_ARGS_BUFFER			= "SCENE_INDIRECT_ARGS_BUFFER";
	constexpr const char* SCENE_TLAS							= "SCENE_TLAS";

	constexpr const char* SCENE_ALBEDO_MAPS					    = "SCENE_ALBEDO_MAPS";
	constexpr const char* SCENE_NORMAL_MAPS					    = "SCENE_NORMAL_MAPS";
	constexpr const char* SCENE_AO_MAPS						    = "SCENE_AO_MAPS";
	constexpr const char* SCENE_ROUGHNESS_MAPS				    = "SCENE_ROUGHNESS_MAPS";
	constexpr const char* SCENE_METALLIC_MAPS					= "SCENE_METALLIC_MAPS";

	enum class ERefactoredRenderGraphPipelineStageType : uint8
	{
		NONE			= 0,
		RENDER			= 1,
		SYNCHRONIZATION = 2,
	};

	enum class ERefactoredRenderGraphResourceType : uint8
	{
		NONE					= 0,
		TEXTURE					= 1,
		BUFFER					= 2,
		ACCELERATION_STRUCTURE	= 3,
	};

	enum class ERefactoredRenderGraphResourceBindingType : uint8
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

	enum class ERefactoredRenderStageDrawType : uint8
	{
		NONE					= 0,
		SCENE_INDIRECT			= 1,
		FULLSCREEN_QUAD			= 2,
	};

	/*-----------------------------------------------------------------Resource Structs Begin-----------------------------------------------------------------*/

	struct RefactoredResourceDesc 
	{
		String							Name					= "";
		
		ERefactoredRenderGraphResourceType		Type					= ERefactoredRenderGraphResourceType::NONE;
		uint32							SubResourceCount		= 1;

		EFormat							TextureFormat			= EFormat::NONE; //Todo: How to solve?

		bool							External				= false;
		bool							Temporal				= false;
	};

	/*-----------------------------------------------------------------Resource Structs End / Render Stage Structs Begin-----------------------------------------------------------------*/

	struct RefactoredGraphicsShaders
	{
		String TaskShaderName		= "";
		String MeshShaderName		= "";

		String VertexShaderName		= "";
		String GeometryShaderName	= "";
		String HullShaderName		= "";
		String DomainShaderName		= "";

		String PixelShaderName		= "";
	};

	struct RefactoredRayTracingShaders
	{
		String			RaygenShaderName			= "";
		String			pMissShaderNames[MAX_MISS_SHADER_COUNT];
		String			pClosestHitShaderNames[MAX_CLOSEST_HIT_SHADER_COUNT];
		uint32			MissShaderCount			= 0;
		uint32			ClosestHitShaderCount	= 0;
	};

	struct RefactoredResourceState
	{
		String	ResourceName			= "";
		ERefactoredRenderGraphResourceBindingType BindingType = ERefactoredRenderGraphResourceBindingType::NONE;

		struct
		{
			bool										PrevSameFrame		= true;
			ERefactoredRenderGraphResourceBindingType	PrevBindingType		= ERefactoredRenderGraphResourceBindingType::NONE;
			ERefactoredRenderGraphResourceBindingType	NextBindingType		= ERefactoredRenderGraphResourceBindingType::NONE;
		} AttachmentSynchronizations; //If this resource state is transitioned using a renderpass, that information is stored here
	};

	struct RefactoredRenderStageDesc
	{
		String						Name			= "";
		EPipelineStateType			Type			= EPipelineStateType::NONE;
		bool						CustomRenderer	= false;
		bool						Enabled			= true;

		TArray<RefactoredResourceState>				ResourceStates;

		uint32						Weight = 0;

		struct
		{
			RefactoredGraphicsShaders				Shaders;
			ERefactoredRenderStageDrawType		DrawType;
			String						IndexBufferName;
			String						IndirectArgsBufferName;
		} Graphics;

		struct
		{	
			String						ShaderName	= "";
		} Compute;

		struct
		{
			RefactoredRayTracingShaders			Shaders;
		} RayTracing;
	};

	/*-----------------------------------------------------------------Render Stage Structs End / Synchronization Stage Structs Begin-----------------------------------------------------------------*/

	struct RefactoredResourceSynchronizationDesc
	{
		String					PrevRenderStage = "";
		String					NextRenderStage	= "";
		String					ResourceName	= "";
		ECommandQueueType		PrevQueue		= ECommandQueueType::COMMAND_QUEUE_NONE;
		ECommandQueueType		NextQueue		= ECommandQueueType::COMMAND_QUEUE_NONE;
		ERefactoredRenderGraphResourceBindingType	PrevBindingType		= ERefactoredRenderGraphResourceBindingType::NONE;
		ERefactoredRenderGraphResourceBindingType	NextBindingType		= ERefactoredRenderGraphResourceBindingType::NONE;
	};

	struct RefactoredSynchronizationStageDesc
	{
		TArray<RefactoredResourceSynchronizationDesc> Synchronizations;
	};

	/*-----------------------------------------------------------------Synchronization Stage Structs End / Pipeline Stage Structs Begin-----------------------------------------------------------------*/

	struct RefactoredPipelineStageDesc
	{
		ERefactoredRenderGraphPipelineStageType Type = ERefactoredRenderGraphPipelineStageType::NONE;
		uint32 StageIndex		= 0;
	};

	/*-----------------------------------------------------------------Pipeline Stage Structs End-----------------------------------------------------------------*/

	struct RefactoredRenderGraphStructure
	{
		TArray<RefactoredResourceDesc>				ResourceDescriptions;
		TArray<RefactoredRenderStageDesc>			RenderStageDescriptions;
		TArray<RefactoredSynchronizationStageDesc>	SynchronizationStageDescriptions;
		TArray<RefactoredPipelineStageDesc>			PipelineStageDescriptions;
	};

	FORCEINLINE bool ResourceStateNeedsDescriptor(ERefactoredRenderGraphResourceBindingType bindingType)
	{
		switch (bindingType)
		{
		case ERefactoredRenderGraphResourceBindingType::ACCELERATION_STRUCTURE:			return true;
		case ERefactoredRenderGraphResourceBindingType::CONSTANT_BUFFER:				return true;
		case ERefactoredRenderGraphResourceBindingType::COMBINED_SAMPLER:				return true;
		case ERefactoredRenderGraphResourceBindingType::UNORDERED_ACCESS_READ:			return true;
		case ERefactoredRenderGraphResourceBindingType::UNORDERED_ACCESS_WRITE:			return true;
		case ERefactoredRenderGraphResourceBindingType::UNORDERED_ACCESS_READ_WRITE:	return true;
		case ERefactoredRenderGraphResourceBindingType::ATTACHMENT:						return false;
		case ERefactoredRenderGraphResourceBindingType::PRESENT:						return false;
		case ERefactoredRenderGraphResourceBindingType::DRAW_RESOURCE:					return false;

		default:															return false;
		}
	}

	FORCEINLINE uint32 CreateShaderStageMask(const RefactoredRenderStageDesc* pRenderStageDesc)
	{
		uint32 mask = 0;

		if (pRenderStageDesc->Type == EPipelineStateType::GRAPHICS)
		{
			if (pRenderStageDesc->Graphics.Shaders.TaskShaderName.size()		> 0)	mask |= FShaderStageFlags::SHADER_STAGE_FLAG_TASK_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.MeshShaderName.size()		> 0)	mask |= FShaderStageFlags::SHADER_STAGE_FLAG_MESH_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.VertexShaderName.size()		> 0)	mask |= FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.GeometryShaderName.size()	> 0)	mask |= FShaderStageFlags::SHADER_STAGE_FLAG_GEOMETRY_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.HullShaderName.size()		> 0)	mask |= FShaderStageFlags::SHADER_STAGE_FLAG_HULL_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.DomainShaderName.size()		> 0)	mask |= FShaderStageFlags::SHADER_STAGE_FLAG_DOMAIN_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.PixelShaderName.size()		> 0)	mask |= FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER;
		}
		else if (pRenderStageDesc->Type == EPipelineStateType::COMPUTE)
		{
			if (pRenderStageDesc->Compute.ShaderName.size()						> 0)	mask |= FShaderStageFlags::SHADER_STAGE_FLAG_COMPUTE_SHADER;
		}
		else if (pRenderStageDesc->Type == EPipelineStateType::RAY_TRACING)
		{
			if (pRenderStageDesc->RayTracing.Shaders.RaygenShaderName.size()	> 0)	mask |= FShaderStageFlags::SHADER_STAGE_FLAG_RAYGEN_SHADER;
			if (pRenderStageDesc->RayTracing.Shaders.ClosestHitShaderCount		> 0)	mask |= FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER;
			if (pRenderStageDesc->RayTracing.Shaders.MissShaderCount			> 0)	mask |= FShaderStageFlags::SHADER_STAGE_FLAG_MISS_SHADER;
		}

		return mask;
	}

	FORCEINLINE EDescriptorType CalculateResourceStateDescriptorType(ERefactoredRenderGraphResourceType resourceType, ERefactoredRenderGraphResourceBindingType bindingType)
	{
		if (resourceType == ERefactoredRenderGraphResourceType::TEXTURE)
		{
			switch (bindingType)
			{
			case ERefactoredRenderGraphResourceBindingType::COMBINED_SAMPLER:				return EDescriptorType::DESCRIPTOR_SHADER_RESOURCE_COMBINED_SAMPLER;
			case ERefactoredRenderGraphResourceBindingType::UNORDERED_ACCESS_READ:			return EDescriptorType::DESCRIPTOR_UNORDERED_ACCESS_TEXTURE;
			case ERefactoredRenderGraphResourceBindingType::UNORDERED_ACCESS_WRITE:			return EDescriptorType::DESCRIPTOR_UNORDERED_ACCESS_TEXTURE;
			case ERefactoredRenderGraphResourceBindingType::UNORDERED_ACCESS_READ_WRITE:	return EDescriptorType::DESCRIPTOR_UNORDERED_ACCESS_TEXTURE;
			}
		}
		else if (resourceType == ERefactoredRenderGraphResourceType::BUFFER)
		{
			switch (bindingType)
			{
			case ERefactoredRenderGraphResourceBindingType::CONSTANT_BUFFER:				return EDescriptorType::DESCRIPTOR_CONSTANT_BUFFER;
			case ERefactoredRenderGraphResourceBindingType::UNORDERED_ACCESS_READ:			return EDescriptorType::DESCRIPTOR_UNORDERED_ACCESS_BUFFER;
			case ERefactoredRenderGraphResourceBindingType::UNORDERED_ACCESS_WRITE:			return EDescriptorType::DESCRIPTOR_UNORDERED_ACCESS_BUFFER;
			case ERefactoredRenderGraphResourceBindingType::UNORDERED_ACCESS_READ_WRITE:	return EDescriptorType::DESCRIPTOR_UNORDERED_ACCESS_BUFFER;
			}
		}
		else if (resourceType == ERefactoredRenderGraphResourceType::ACCELERATION_STRUCTURE)
		{
			switch (bindingType)
			{
			case ERefactoredRenderGraphResourceBindingType::ACCELERATION_STRUCTURE:			return EDescriptorType::DESCRIPTOR_ACCELERATION_STRUCTURE;
			case ERefactoredRenderGraphResourceBindingType::UNORDERED_ACCESS_READ:			return EDescriptorType::DESCRIPTOR_UNORDERED_ACCESS_BUFFER;
			case ERefactoredRenderGraphResourceBindingType::UNORDERED_ACCESS_WRITE:			return EDescriptorType::DESCRIPTOR_UNORDERED_ACCESS_BUFFER;
			case ERefactoredRenderGraphResourceBindingType::UNORDERED_ACCESS_READ_WRITE:	return EDescriptorType::DESCRIPTOR_UNORDERED_ACCESS_BUFFER;
			}
		}

		return EDescriptorType::DESCRIPTOR_UNKNOWN;
	}

	FORCEINLINE ETextureState CalculateResourceTextureState(ERefactoredRenderGraphResourceType resourceType, ERefactoredRenderGraphResourceBindingType bindingType, EFormat format)
	{
		if (resourceType == ERefactoredRenderGraphResourceType::TEXTURE)
		{
			switch (bindingType)
			{
			case ERefactoredRenderGraphResourceBindingType::COMBINED_SAMPLER:				return format != EFormat::FORMAT_D24_UNORM_S8_UINT ? ETextureState::TEXTURE_STATE_SHADER_READ_ONLY : ETextureState::TEXTURE_STATE_DEPTH_STENCIL_READ_ONLY;
			case ERefactoredRenderGraphResourceBindingType::UNORDERED_ACCESS_READ:			return ETextureState::TEXTURE_STATE_GENERAL;
			case ERefactoredRenderGraphResourceBindingType::UNORDERED_ACCESS_WRITE:			return ETextureState::TEXTURE_STATE_GENERAL;
			case ERefactoredRenderGraphResourceBindingType::UNORDERED_ACCESS_READ_WRITE:	return ETextureState::TEXTURE_STATE_GENERAL;
			case ERefactoredRenderGraphResourceBindingType::ATTACHMENT:						return format != EFormat::FORMAT_D24_UNORM_S8_UINT ? ETextureState::TEXTURE_STATE_RENDER_TARGET : ETextureState::TEXTURE_STATE_DEPTH_STENCIL_ATTACHMENT;
			case ERefactoredRenderGraphResourceBindingType::PRESENT:						return ETextureState::TEXTURE_STATE_PRESENT;
			}
		}

		return ETextureState::TEXTURE_STATE_UNKNOWN;
	}

	FORCEINLINE uint32 CalculateResourceAccessFlags(ERefactoredRenderGraphResourceBindingType bindingType)
	{
		switch (bindingType)
		{
			case ERefactoredRenderGraphResourceBindingType::ACCELERATION_STRUCTURE:			return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_ACCELERATION_STRUCTURE_READ;
			case ERefactoredRenderGraphResourceBindingType::CONSTANT_BUFFER:				return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_CONSTANT_BUFFER_READ;
			case ERefactoredRenderGraphResourceBindingType::COMBINED_SAMPLER:				return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_SHADER_READ;
			case ERefactoredRenderGraphResourceBindingType::UNORDERED_ACCESS_READ:			return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_READ;
			case ERefactoredRenderGraphResourceBindingType::UNORDERED_ACCESS_WRITE:			return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
			case ERefactoredRenderGraphResourceBindingType::UNORDERED_ACCESS_READ_WRITE:	return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_READ | FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
			case ERefactoredRenderGraphResourceBindingType::ATTACHMENT:						return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
			case ERefactoredRenderGraphResourceBindingType::PRESENT:						return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_READ;
			case ERefactoredRenderGraphResourceBindingType::DRAW_RESOURCE:					return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_READ;																
		}

		return FMemoryAccessFlags::MEMORY_ACCESS_FLAG_UNKNOWN;
	}

	FORCEINLINE FPipelineStageFlags FindEarliestPipelineStage(const RefactoredRenderStageDesc* pRenderStageDesc)
	{
		uint32 shaderStageMask = 0;

		if (pRenderStageDesc->Type == EPipelineStateType::GRAPHICS)
		{
			if (pRenderStageDesc->Graphics.Shaders.TaskShaderName.size()		> 0)		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_TASK_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.MeshShaderName.size()		> 0)		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_MESH_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.VertexShaderName.size()		> 0)		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_VERTEX_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.GeometryShaderName.size()	> 0)		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_GEOMETRY_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.HullShaderName.size()		> 0)		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_HULL_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.DomainShaderName.size()		> 0)		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_DOMAIN_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.PixelShaderName.size()		> 0)		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_PIXEL_SHADER;
		}
		else if (pRenderStageDesc->Type == EPipelineStateType::COMPUTE)
		{
			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_COMPUTE_SHADER;
		}
		else if (pRenderStageDesc->Type == EPipelineStateType::RAY_TRACING)
		{
			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER;
		}

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
	}

	FORCEINLINE FPipelineStageFlags FindLastPipelineStage(const RefactoredRenderStageDesc* pRenderStageDesc)
	{
		uint32 shaderStageMask = 0;

		if (pRenderStageDesc->Type == EPipelineStateType::GRAPHICS)
		{
			if (pRenderStageDesc->Graphics.Shaders.PixelShaderName.size() > 0)		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_PIXEL_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.DomainShaderName.size() > 0)		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_DOMAIN_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.HullShaderName.size() > 0)		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_HULL_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.GeometryShaderName.size() > 0)	return FPipelineStageFlags::PIPELINE_STAGE_FLAG_GEOMETRY_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.VertexShaderName.size() > 0)		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_VERTEX_INPUT;
			if (pRenderStageDesc->Graphics.Shaders.MeshShaderName.size() > 0)		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_MESH_SHADER;
			if (pRenderStageDesc->Graphics.Shaders.TaskShaderName.size() > 0)		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_TASK_SHADER;
		}
		else if (pRenderStageDesc->Type == EPipelineStateType::COMPUTE)
		{
			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_COMPUTE_SHADER;
		}
		else if (pRenderStageDesc->Type == EPipelineStateType::RAY_TRACING)
		{
			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER;
		}

		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_UNKNOWN;
	}

	//FORCEINLINE uint32 CreateShaderStageMask(const RenderStageDesc* pRenderStageDesc)
	//{
	//	uint32 shaderStageMask = 0;

	//	if (pRenderStageDesc->PipelineType == EPipelineStateType::GRAPHICS)
	//	{
	//		shaderStageMask |= (pRenderStageDesc->GraphicsPipeline.pGraphicsDesc->MeshShader		!= GUID_NONE)	? FShaderStageFlags::SHADER_STAGE_FLAG_MESH_SHADER		: 0;

	//		shaderStageMask |= (pRenderStageDesc->GraphicsPipeline.pGraphicsDesc->VertexShader		!= GUID_NONE)	? FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER	: 0;
	//		shaderStageMask |= (pRenderStageDesc->GraphicsPipeline.pGraphicsDesc->GeometryShader	!= GUID_NONE)	? FShaderStageFlags::SHADER_STAGE_FLAG_GEOMETRY_SHADER	: 0;
	//		shaderStageMask |= (pRenderStageDesc->GraphicsPipeline.pGraphicsDesc->HullShader		!= GUID_NONE)	? FShaderStageFlags::SHADER_STAGE_FLAG_HULL_SHADER		: 0;
	//		shaderStageMask |= (pRenderStageDesc->GraphicsPipeline.pGraphicsDesc->DomainShader		!= GUID_NONE)	? FShaderStageFlags::SHADER_STAGE_FLAG_DOMAIN_SHADER	: 0;

	//		shaderStageMask |= (pRenderStageDesc->GraphicsPipeline.pGraphicsDesc->PixelShader		!= GUID_NONE)	? FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER		: 0;
	//	}
	//	else if (pRenderStageDesc->PipelineType == EPipelineStateType::COMPUTE)
	//	{
	//		shaderStageMask |= FShaderStageFlags::SHADER_STAGE_FLAG_COMPUTE_SHADER;
	//	}
	//	else if (pRenderStageDesc->PipelineType == EPipelineStateType::RAY_TRACING)
	//	{
	//		shaderStageMask |= FShaderStageFlags::SHADER_STAGE_FLAG_RAYGEN_SHADER;
	//		shaderStageMask |= FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER;
	//		shaderStageMask |= FShaderStageFlags::SHADER_STAGE_FLAG_MISS_SHADER;
	//	}

	//	return shaderStageMask;
	//}

	FORCEINLINE String RenderStageDrawTypeToString(ERefactoredRenderStageDrawType drawType)
	{
		switch (drawType)
		{
		case ERefactoredRenderStageDrawType::SCENE_INDIRECT:		return "SCENE_INDIRECT";
		case ERefactoredRenderStageDrawType::FULLSCREEN_QUAD:		return "FULLSCREEN_QUAD";
		default:													return "NONE";
		}
	}

	FORCEINLINE ERefactoredRenderStageDrawType RenderStageDrawTypeFromString(const String& string)
	{
		if (string == "SCENE_INDIRECT")		return ERefactoredRenderStageDrawType::SCENE_INDIRECT;
		if (string == "FULLSCREEN_QUAD")	return ERefactoredRenderStageDrawType::FULLSCREEN_QUAD;
		return ERefactoredRenderStageDrawType::NONE;
	}
}
