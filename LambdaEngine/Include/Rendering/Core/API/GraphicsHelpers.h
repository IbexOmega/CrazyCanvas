#pragma once
#include "GraphicsTypes.h"

#include "Containers/String.h"

namespace LambdaEngine
{
	FORCEINLINE ECommandQueueType ConvertPipelineStateTypeToQueue(EPipelineStateType pipelineStateType)
	{
		switch (pipelineStateType)
		{
		case EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS:		return ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS;
		case EPipelineStateType::PIPELINE_STATE_TYPE_COMPUTE:		return ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE;
		case EPipelineStateType::PIPELINE_STATE_TYPE_RAY_TRACING:	return ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE;
		case EPipelineStateType::PIPELINE_STATE_TYPE_NONE:			
		default:													return ECommandQueueType::COMMAND_QUEUE_TYPE_NONE;
		}
	}

	FORCEINLINE FPipelineStageFlags ConvertShaderStageToPipelineStage(FShaderStageFlags shaderStage)
	{
		switch (shaderStage)
		{
			case FShaderStageFlags::SHADER_STAGE_FLAG_MESH_SHADER:			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_MESH_SHADER;
			case FShaderStageFlags::SHADER_STAGE_FLAG_TASK_SHADER:			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_TASK_SHADER;
			case FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER:		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_VERTEX_SHADER;
			case FShaderStageFlags::SHADER_STAGE_FLAG_GEOMETRY_SHADER:		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_GEOMETRY_SHADER;
			case FShaderStageFlags::SHADER_STAGE_FLAG_HULL_SHADER:			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_HULL_SHADER;
			case FShaderStageFlags::SHADER_STAGE_FLAG_DOMAIN_SHADER:		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_DOMAIN_SHADER;
			case FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER:			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_PIXEL_SHADER;
			case FShaderStageFlags::SHADER_STAGE_FLAG_COMPUTE_SHADER:		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_COMPUTE_SHADER;
			case FShaderStageFlags::SHADER_STAGE_FLAG_RAYGEN_SHADER:		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER;
			case FShaderStageFlags::SHADER_STAGE_FLAG_INTERSECT_SHADER:		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER;
			case FShaderStageFlags::SHADER_STAGE_FLAG_ANY_HIT_SHADER:		return FPipelineStageFlags::PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER;
			case FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER:	return FPipelineStageFlags::PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER;
			case FShaderStageFlags::SHADER_STAGE_FLAG_MISS_SHADER:			return FPipelineStageFlags::PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER;
			default:														return FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP;
		}
	}

	FORCEINLINE FShaderStageFlags ConvertPipelineStageToShaderStage(FPipelineStageFlags pipelineStage)
	{
		switch (pipelineStage)
		{
		case FPipelineStageFlags::PIPELINE_STAGE_FLAG_UNKNOWN:							return FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
		case FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP:								return FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
		case FPipelineStageFlags::PIPELINE_STAGE_FLAG_BOTTOM:							return FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
		case FPipelineStageFlags::PIPELINE_STAGE_FLAG_DRAW_INDIRECT:					return FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
		case FPipelineStageFlags::PIPELINE_STAGE_FLAG_VERTEX_INPUT:						return FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER;
		case FPipelineStageFlags::PIPELINE_STAGE_FLAG_VERTEX_SHADER:					return FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER;
		case FPipelineStageFlags::PIPELINE_STAGE_FLAG_HULL_SHADER:						return FShaderStageFlags::SHADER_STAGE_FLAG_HULL_SHADER;
		case FPipelineStageFlags::PIPELINE_STAGE_FLAG_DOMAIN_SHADER:					return FShaderStageFlags::SHADER_STAGE_FLAG_DOMAIN_SHADER;
		case FPipelineStageFlags::PIPELINE_STAGE_FLAG_GEOMETRY_SHADER:					return FShaderStageFlags::SHADER_STAGE_FLAG_GEOMETRY_SHADER;
		case FPipelineStageFlags::PIPELINE_STAGE_FLAG_PIXEL_SHADER:						return FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER;
		case FPipelineStageFlags::PIPELINE_STAGE_FLAG_EARLY_FRAGMENT_TESTS:				return FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
		case FPipelineStageFlags::PIPELINE_STAGE_FLAG_LATE_FRAGMENT_TESTS:				return FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
		case FPipelineStageFlags::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT:				return FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
		case FPipelineStageFlags::PIPELINE_STAGE_FLAG_COMPUTE_SHADER:					return FShaderStageFlags::SHADER_STAGE_FLAG_COMPUTE_SHADER;
		case FPipelineStageFlags::PIPELINE_STAGE_FLAG_COPY:								return FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
		case FPipelineStageFlags::PIPELINE_STAGE_FLAG_HOST:								return FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
		case FPipelineStageFlags::PIPELINE_STAGE_FLAG_STREAM_OUTPUT:					return FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
		case FPipelineStageFlags::PIPELINE_STAGE_FLAG_CONDITIONAL_RENDERING:			return FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
		case FPipelineStageFlags::PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER:				return FShaderStageFlags::SHADER_STAGE_FLAG_RAYGEN_SHADER;
		case FPipelineStageFlags::PIPELINE_STAGE_FLAG_ACCELERATION_STRUCTURE_BUILD:		return FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
		case FPipelineStageFlags::PIPELINE_STAGE_FLAG_SHADING_RATE_TEXTURE:				return FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
		case FPipelineStageFlags::PIPELINE_STAGE_FLAG_TASK_SHADER:						return FShaderStageFlags::SHADER_STAGE_FLAG_TASK_SHADER;
		case FPipelineStageFlags::PIPELINE_STAGE_FLAG_MESH_SHADER:						return FShaderStageFlags::SHADER_STAGE_FLAG_MESH_SHADER;
		}
	}

	FORCEINLINE FShaderStageFlags GetFirstShaderStageInMask(uint32 shaderStageMask)
	{
		FShaderStageFlags firstStage = FShaderStageFlags::SHADER_STAGE_FLAG_NONE;

		if (shaderStageMask & FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER)		firstStage = SHADER_STAGE_FLAG_PIXEL_SHADER;

		if (shaderStageMask & FShaderStageFlags::SHADER_STAGE_FLAG_MESH_SHADER)			firstStage = SHADER_STAGE_FLAG_MESH_SHADER;
		if (shaderStageMask & FShaderStageFlags::SHADER_STAGE_FLAG_TASK_SHADER)			firstStage = SHADER_STAGE_FLAG_TASK_SHADER;

		if (shaderStageMask & FShaderStageFlags::SHADER_STAGE_FLAG_DOMAIN_SHADER)		firstStage = SHADER_STAGE_FLAG_DOMAIN_SHADER;
		if (shaderStageMask & FShaderStageFlags::SHADER_STAGE_FLAG_HULL_SHADER)			firstStage = SHADER_STAGE_FLAG_HULL_SHADER;
		if (shaderStageMask & FShaderStageFlags::SHADER_STAGE_FLAG_GEOMETRY_SHADER)		firstStage = SHADER_STAGE_FLAG_GEOMETRY_SHADER;
		if (shaderStageMask & FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER)		firstStage = SHADER_STAGE_FLAG_VERTEX_SHADER;

		if (shaderStageMask & FShaderStageFlags::SHADER_STAGE_FLAG_COMPUTE_SHADER)		firstStage = SHADER_STAGE_FLAG_COMPUTE_SHADER;

		if (shaderStageMask & FShaderStageFlags::SHADER_STAGE_FLAG_MISS_SHADER)			firstStage = SHADER_STAGE_FLAG_MISS_SHADER;
		if (shaderStageMask & FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER)	firstStage = SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER;
		if (shaderStageMask & FShaderStageFlags::SHADER_STAGE_FLAG_ANY_HIT_SHADER)		firstStage = SHADER_STAGE_FLAG_ANY_HIT_SHADER;
		if (shaderStageMask & FShaderStageFlags::SHADER_STAGE_FLAG_INTERSECT_SHADER)	firstStage = SHADER_STAGE_FLAG_INTERSECT_SHADER;
		if (shaderStageMask & FShaderStageFlags::SHADER_STAGE_FLAG_RAYGEN_SHADER)		firstStage = SHADER_STAGE_FLAG_RAYGEN_SHADER;

		return firstStage;
	}

	FORCEINLINE FShaderStageFlags GetLastShaderStageInMask(uint32 shaderStageMask)
	{
		FShaderStageFlags lastStage = FShaderStageFlags::SHADER_STAGE_FLAG_NONE;

		if (shaderStageMask & FShaderStageFlags::SHADER_STAGE_FLAG_TASK_SHADER)			lastStage = SHADER_STAGE_FLAG_TASK_SHADER;
		if (shaderStageMask & FShaderStageFlags::SHADER_STAGE_FLAG_MESH_SHADER)			lastStage = SHADER_STAGE_FLAG_MESH_SHADER;

		if (shaderStageMask & FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER)		lastStage = SHADER_STAGE_FLAG_VERTEX_SHADER;
		if (shaderStageMask & FShaderStageFlags::SHADER_STAGE_FLAG_GEOMETRY_SHADER)		lastStage = SHADER_STAGE_FLAG_GEOMETRY_SHADER;
		if (shaderStageMask & FShaderStageFlags::SHADER_STAGE_FLAG_HULL_SHADER)			lastStage = SHADER_STAGE_FLAG_HULL_SHADER;
		if (shaderStageMask & FShaderStageFlags::SHADER_STAGE_FLAG_DOMAIN_SHADER)		lastStage = SHADER_STAGE_FLAG_DOMAIN_SHADER;

		if (shaderStageMask & FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER)		lastStage = SHADER_STAGE_FLAG_PIXEL_SHADER;

		if (shaderStageMask & FShaderStageFlags::SHADER_STAGE_FLAG_COMPUTE_SHADER)		lastStage = SHADER_STAGE_FLAG_COMPUTE_SHADER;

		if (shaderStageMask & FShaderStageFlags::SHADER_STAGE_FLAG_RAYGEN_SHADER)		lastStage = SHADER_STAGE_FLAG_RAYGEN_SHADER;
		if (shaderStageMask & FShaderStageFlags::SHADER_STAGE_FLAG_INTERSECT_SHADER)	lastStage = SHADER_STAGE_FLAG_INTERSECT_SHADER;
		if (shaderStageMask & FShaderStageFlags::SHADER_STAGE_FLAG_ANY_HIT_SHADER)		lastStage = SHADER_STAGE_FLAG_ANY_HIT_SHADER;
		if (shaderStageMask & FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER)	lastStage = SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER;
		if (shaderStageMask & FShaderStageFlags::SHADER_STAGE_FLAG_MISS_SHADER)			lastStage = SHADER_STAGE_FLAG_MISS_SHADER;

		return lastStage;
	}

	FORCEINLINE uint32 TextureFormatStride(EFormat format)
	{
		switch (format)
		{		
        case EFormat::FORMAT_R16_SFLOAT:				
        case EFormat::FORMAT_R16_UNORM:				return 2;
        case EFormat::FORMAT_D24_UNORM_S8_UINT:
        case EFormat::FORMAT_R8G8B8A8_UNORM:
        case EFormat::FORMAT_B8G8R8A8_UNORM:
        case EFormat::FORMAT_R8G8B8A8_SNORM:		return 4;
		case EFormat::FORMAT_R32G32_SFLOAT:
        case EFormat::FORMAT_R16G16B16A16_SFLOAT:	return 8;
        case EFormat::FORMAT_R32G32B32A32_SFLOAT:	
        case EFormat::FORMAT_R32G32B32A32_UINT:		return 16;
        default:                                    return 0;
        }
	}

	FORCEINLINE String TextureFormatToString(EFormat format)
	{
		switch (format)
		{
		case EFormat::FORMAT_R16_UNORM:				return "R16_UNORM";
		case EFormat::FORMAT_R16_SFLOAT:			return "R16_SFLOAT";
		case EFormat::FORMAT_R8G8B8A8_UNORM:		return "R8G8B8A8_UNORM";
		case EFormat::FORMAT_B8G8R8A8_UNORM:		return "B8G8R8A8_UNORM";
		case EFormat::FORMAT_R8G8B8A8_SNORM:		return "R8G8B8A8_SNORM";
		case EFormat::FORMAT_R32G32_SFLOAT:			return "R32G32_SFLOAT";
		case EFormat::FORMAT_R16G16B16A16_SFLOAT:	return "R16G16B16A16_SFLOAT";
		case EFormat::FORMAT_R32G32B32A32_SFLOAT:	return "R32G32B32A32_SFLOAT";
		case EFormat::FORMAT_R32G32B32A32_UINT:		return "R32G32B32A32_UINT";
		case EFormat::FORMAT_D24_UNORM_S8_UINT:		return "D24_UNORM_S8_UINT";
		default:                                    return "NONE";
		}
	}

	FORCEINLINE EFormat TextureFormatFromString(const String& string)
	{
		if (string == "R16_UNORM")					return EFormat::FORMAT_R16_UNORM;
		if (string == "R16_SFLOAT")					return EFormat::FORMAT_R16_SFLOAT;
		else if (string == "R8G8B8A8_UNORM")		return EFormat::FORMAT_R8G8B8A8_UNORM;
		else if (string == "B8G8R8A8_UNORM")		return EFormat::FORMAT_B8G8R8A8_UNORM;
		else if (string == "R8G8B8A8_SNORM")		return EFormat::FORMAT_R8G8B8A8_SNORM;
		else if (string == "R32G32_SFLOAT")			return EFormat::FORMAT_R32G32_SFLOAT;
		else if (string == "R16G16B16A16_SFLOAT")	return EFormat::FORMAT_R16G16B16A16_SFLOAT;
		else if (string == "R32G32B32A32_SFLOAT")	return EFormat::FORMAT_R32G32B32A32_SFLOAT;
		else if (string == "R32G32B32A32_UINT")		return EFormat::FORMAT_R32G32B32A32_UINT;
		else if	(string == "D24_UNORM_S8_UINT")		return EFormat::FORMAT_D24_UNORM_S8_UINT;

		return EFormat::FORMAT_NONE;
	}

	FORCEINLINE String CommandQueueToString(ECommandQueueType commandQueue)
	{
		switch (commandQueue)
		{
		case ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS:	return "GRAPHICS QUEUE";
		case ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE:		return "COMPUTE QUEUE";
		case ECommandQueueType::COMMAND_QUEUE_TYPE_COPY:		return "COPY QUEUE";
		case ECommandQueueType::COMMAND_QUEUE_TYPE_UNKNOWN:		return "UNKNOWN QUEUE";
		case ECommandQueueType::COMMAND_QUEUE_TYPE_NONE:
		default:												return "NONE";
		}
	}

	FORCEINLINE String MemoryTypeToString(EMemoryType memoryType)
	{
		switch (memoryType)
		{
		case EMemoryType::MEMORY_TYPE_CPU_VISIBLE:		return "MEMORY_TYPE_CPU_VISIBLE";
		case EMemoryType::MEMORY_TYPE_GPU:				return "MEMORY_TYPE_GPU";
		default:										return "NONE";
		}
	}

	FORCEINLINE EMemoryType MemoryTypeFromString(const String string)
	{
		if (string == "MEMORY_TYPE_CPU_VISIBLE")		return EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
		if (string == "MEMORY_TYPE_GPU")				return EMemoryType::MEMORY_TYPE_GPU;

		return EMemoryType::MEMORY_TYPE_NONE;
	}
}
