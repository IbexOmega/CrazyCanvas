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

	FORCEINLINE FPipelineStageFlag ConvertShaderStageToPipelineStage(FShaderStageFlags shaderStage)
	{
		switch (shaderStage)
		{
			case FShaderStageFlag::SHADER_STAGE_FLAG_MESH_SHADER:			return FPipelineStageFlag::PIPELINE_STAGE_FLAG_MESH_SHADER;
			case FShaderStageFlag::SHADER_STAGE_FLAG_TASK_SHADER:			return FPipelineStageFlag::PIPELINE_STAGE_FLAG_TASK_SHADER;
			case FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER:			return FPipelineStageFlag::PIPELINE_STAGE_FLAG_VERTEX_SHADER;
			case FShaderStageFlag::SHADER_STAGE_FLAG_GEOMETRY_SHADER:		return FPipelineStageFlag::PIPELINE_STAGE_FLAG_GEOMETRY_SHADER;
			case FShaderStageFlag::SHADER_STAGE_FLAG_HULL_SHADER:			return FPipelineStageFlag::PIPELINE_STAGE_FLAG_HULL_SHADER;
			case FShaderStageFlag::SHADER_STAGE_FLAG_DOMAIN_SHADER:			return FPipelineStageFlag::PIPELINE_STAGE_FLAG_DOMAIN_SHADER;
			case FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER:			return FPipelineStageFlag::PIPELINE_STAGE_FLAG_PIXEL_SHADER;
			case FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER:		return FPipelineStageFlag::PIPELINE_STAGE_FLAG_COMPUTE_SHADER;
			case FShaderStageFlag::SHADER_STAGE_FLAG_RAYGEN_SHADER:			return FPipelineStageFlag::PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER;
			case FShaderStageFlag::SHADER_STAGE_FLAG_INTERSECT_SHADER:		return FPipelineStageFlag::PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER;
			case FShaderStageFlag::SHADER_STAGE_FLAG_ANY_HIT_SHADER:		return FPipelineStageFlag::PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER;
			case FShaderStageFlag::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER:	return FPipelineStageFlag::PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER;
			case FShaderStageFlag::SHADER_STAGE_FLAG_MISS_SHADER:			return FPipelineStageFlag::PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER;
			default:														return FPipelineStageFlag::PIPELINE_STAGE_FLAG_TOP;
		}
	}

	FORCEINLINE FShaderStageFlag ConvertPipelineStageToShaderStage(FPipelineStageFlags pipelineStage)
	{
		switch (pipelineStage)
		{
		case FPipelineStageFlag::PIPELINE_STAGE_FLAG_UNKNOWN:							return FShaderStageFlag::SHADER_STAGE_FLAG_NONE;
		case FPipelineStageFlag::PIPELINE_STAGE_FLAG_TOP:								return FShaderStageFlag::SHADER_STAGE_FLAG_NONE;
		case FPipelineStageFlag::PIPELINE_STAGE_FLAG_BOTTOM:							return FShaderStageFlag::SHADER_STAGE_FLAG_NONE;
		case FPipelineStageFlag::PIPELINE_STAGE_FLAG_DRAW_INDIRECT:						return FShaderStageFlag::SHADER_STAGE_FLAG_NONE;
		case FPipelineStageFlag::PIPELINE_STAGE_FLAG_VERTEX_INPUT:						return FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER;
		case FPipelineStageFlag::PIPELINE_STAGE_FLAG_VERTEX_SHADER:						return FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER;
		case FPipelineStageFlag::PIPELINE_STAGE_FLAG_HULL_SHADER:						return FShaderStageFlag::SHADER_STAGE_FLAG_HULL_SHADER;
		case FPipelineStageFlag::PIPELINE_STAGE_FLAG_DOMAIN_SHADER:						return FShaderStageFlag::SHADER_STAGE_FLAG_DOMAIN_SHADER;
		case FPipelineStageFlag::PIPELINE_STAGE_FLAG_GEOMETRY_SHADER:					return FShaderStageFlag::SHADER_STAGE_FLAG_GEOMETRY_SHADER;
		case FPipelineStageFlag::PIPELINE_STAGE_FLAG_PIXEL_SHADER:						return FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;
		case FPipelineStageFlag::PIPELINE_STAGE_FLAG_EARLY_FRAGMENT_TESTS:				return FShaderStageFlag::SHADER_STAGE_FLAG_NONE;
		case FPipelineStageFlag::PIPELINE_STAGE_FLAG_LATE_FRAGMENT_TESTS:				return FShaderStageFlag::SHADER_STAGE_FLAG_NONE;
		case FPipelineStageFlag::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT:				return FShaderStageFlag::SHADER_STAGE_FLAG_NONE;
		case FPipelineStageFlag::PIPELINE_STAGE_FLAG_COMPUTE_SHADER:					return FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;
		case FPipelineStageFlag::PIPELINE_STAGE_FLAG_COPY:								return FShaderStageFlag::SHADER_STAGE_FLAG_NONE;
		case FPipelineStageFlag::PIPELINE_STAGE_FLAG_HOST:								return FShaderStageFlag::SHADER_STAGE_FLAG_NONE;
		case FPipelineStageFlag::PIPELINE_STAGE_FLAG_STREAM_OUTPUT:						return FShaderStageFlag::SHADER_STAGE_FLAG_NONE;
		case FPipelineStageFlag::PIPELINE_STAGE_FLAG_CONDITIONAL_RENDERING:				return FShaderStageFlag::SHADER_STAGE_FLAG_NONE;
		case FPipelineStageFlag::PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER:				return FShaderStageFlag::SHADER_STAGE_FLAG_RAYGEN_SHADER;
		case FPipelineStageFlag::PIPELINE_STAGE_FLAG_ACCELERATION_STRUCTURE_BUILD:		return FShaderStageFlag::SHADER_STAGE_FLAG_NONE;
		case FPipelineStageFlag::PIPELINE_STAGE_FLAG_SHADING_RATE_TEXTURE:				return FShaderStageFlag::SHADER_STAGE_FLAG_NONE;
		case FPipelineStageFlag::PIPELINE_STAGE_FLAG_TASK_SHADER:						return FShaderStageFlag::SHADER_STAGE_FLAG_TASK_SHADER;
		case FPipelineStageFlag::PIPELINE_STAGE_FLAG_MESH_SHADER:						return FShaderStageFlag::SHADER_STAGE_FLAG_MESH_SHADER;
		}
	}

	FORCEINLINE FShaderStageFlag GetFirstShaderStageInMask(uint32 shaderStageMask)
	{
		FShaderStageFlag firstStage = FShaderStageFlag::SHADER_STAGE_FLAG_NONE;

		if (shaderStageMask & FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER)			firstStage = SHADER_STAGE_FLAG_PIXEL_SHADER;

		if (shaderStageMask & FShaderStageFlag::SHADER_STAGE_FLAG_MESH_SHADER)			firstStage = SHADER_STAGE_FLAG_MESH_SHADER;
		if (shaderStageMask & FShaderStageFlag::SHADER_STAGE_FLAG_TASK_SHADER)			firstStage = SHADER_STAGE_FLAG_TASK_SHADER;

		if (shaderStageMask & FShaderStageFlag::SHADER_STAGE_FLAG_DOMAIN_SHADER)		firstStage = SHADER_STAGE_FLAG_DOMAIN_SHADER;
		if (shaderStageMask & FShaderStageFlag::SHADER_STAGE_FLAG_HULL_SHADER)			firstStage = SHADER_STAGE_FLAG_HULL_SHADER;
		if (shaderStageMask & FShaderStageFlag::SHADER_STAGE_FLAG_GEOMETRY_SHADER)		firstStage = SHADER_STAGE_FLAG_GEOMETRY_SHADER;
		if (shaderStageMask & FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER)		firstStage = SHADER_STAGE_FLAG_VERTEX_SHADER;

		if (shaderStageMask & FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER)		firstStage = SHADER_STAGE_FLAG_COMPUTE_SHADER;

		if (shaderStageMask & FShaderStageFlag::SHADER_STAGE_FLAG_MISS_SHADER)			firstStage = SHADER_STAGE_FLAG_MISS_SHADER;
		if (shaderStageMask & FShaderStageFlag::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER)	firstStage = SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER;
		if (shaderStageMask & FShaderStageFlag::SHADER_STAGE_FLAG_ANY_HIT_SHADER)		firstStage = SHADER_STAGE_FLAG_ANY_HIT_SHADER;
		if (shaderStageMask & FShaderStageFlag::SHADER_STAGE_FLAG_INTERSECT_SHADER)		firstStage = SHADER_STAGE_FLAG_INTERSECT_SHADER;
		if (shaderStageMask & FShaderStageFlag::SHADER_STAGE_FLAG_RAYGEN_SHADER)		firstStage = SHADER_STAGE_FLAG_RAYGEN_SHADER;

		return firstStage;
	}

	FORCEINLINE FShaderStageFlag GetLastShaderStageInMask(uint32 shaderStageMask)
	{
		FShaderStageFlag lastStage = FShaderStageFlag::SHADER_STAGE_FLAG_NONE;

		if (shaderStageMask & FShaderStageFlag::SHADER_STAGE_FLAG_TASK_SHADER)			lastStage = SHADER_STAGE_FLAG_TASK_SHADER;
		if (shaderStageMask & FShaderStageFlag::SHADER_STAGE_FLAG_MESH_SHADER)			lastStage = SHADER_STAGE_FLAG_MESH_SHADER;

		if (shaderStageMask & FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER)		lastStage = SHADER_STAGE_FLAG_VERTEX_SHADER;
		if (shaderStageMask & FShaderStageFlag::SHADER_STAGE_FLAG_GEOMETRY_SHADER)		lastStage = SHADER_STAGE_FLAG_GEOMETRY_SHADER;
		if (shaderStageMask & FShaderStageFlag::SHADER_STAGE_FLAG_HULL_SHADER)			lastStage = SHADER_STAGE_FLAG_HULL_SHADER;
		if (shaderStageMask & FShaderStageFlag::SHADER_STAGE_FLAG_DOMAIN_SHADER)		lastStage = SHADER_STAGE_FLAG_DOMAIN_SHADER;

		if (shaderStageMask & FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER)		lastStage = SHADER_STAGE_FLAG_PIXEL_SHADER;

		if (shaderStageMask & FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER)		lastStage = SHADER_STAGE_FLAG_COMPUTE_SHADER;

		if (shaderStageMask & FShaderStageFlag::SHADER_STAGE_FLAG_RAYGEN_SHADER)		lastStage = SHADER_STAGE_FLAG_RAYGEN_SHADER;
		if (shaderStageMask & FShaderStageFlag::SHADER_STAGE_FLAG_INTERSECT_SHADER)	lastStage = SHADER_STAGE_FLAG_INTERSECT_SHADER;
		if (shaderStageMask & FShaderStageFlag::SHADER_STAGE_FLAG_ANY_HIT_SHADER)		lastStage = SHADER_STAGE_FLAG_ANY_HIT_SHADER;
		if (shaderStageMask & FShaderStageFlag::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER)	lastStage = SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER;
		if (shaderStageMask & FShaderStageFlag::SHADER_STAGE_FLAG_MISS_SHADER)			lastStage = SHADER_STAGE_FLAG_MISS_SHADER;

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
        case EFormat::FORMAT_R8G8B8A8_SNORM:		
		case EFormat::FORMAT_R16G16_SFLOAT:			
		case EFormat::FORMAT_R11G11B10_SFLOAT:		return 4;
		case EFormat::FORMAT_R32G32_SFLOAT:
        case EFormat::FORMAT_R16G16B16A16_SFLOAT:	return 8;
        case EFormat::FORMAT_R32G32B32A32_SFLOAT:	
        case EFormat::FORMAT_R32G32B32A32_UINT:		return 16;
        default:                                    return 0;
        }
	}

	FORCEINLINE const char* TextureFormatToString(EFormat format)
	{
		switch (format)
		{
		case EFormat::FORMAT_R16_UNORM:				return "FORMAT_R16_UNORM";
		case EFormat::FORMAT_R16_SFLOAT:			return "FORMAT_R16_SFLOAT";
		case EFormat::FORMAT_R8G8B8A8_UNORM:		return "FORMAT_R8G8B8A8_UNORM";
		case EFormat::FORMAT_B8G8R8A8_UNORM:		return "FORMAT_B8G8R8A8_UNORM";
		case EFormat::FORMAT_R8G8B8A8_SNORM:		return "FORMAT_R8G8B8A8_SNORM";
		case EFormat::FORMAT_R16G16_SFLOAT:			return "FORMAT_R16G16_SFLOAT";
		case EFormat::FORMAT_R32G32_SFLOAT:			return "FORMAT_R32G32_SFLOAT";
		case EFormat::FORMAT_R11G11B10_SFLOAT:		return "FORMAT_R11G11B10_SFLOAT";
		case EFormat::FORMAT_R16G16B16A16_SFLOAT:	return "FORMAT_R16G16B16A16_SFLOAT";
		case EFormat::FORMAT_R32G32B32A32_SFLOAT:	return "FORMAT_R32G32B32A32_SFLOAT";
		case EFormat::FORMAT_R32G32B32A32_UINT:		return "FORMAT_R32G32B32A32_UINT";
		case EFormat::FORMAT_D24_UNORM_S8_UINT:		return "FORMAT_D24_UNORM_S8_UINT";
		default:                                    return "FORMAT_NONE";
		}
	}

	FORCEINLINE EFormat TextureFormatFromString(const String& string)
	{
		if		(string == "FORMAT_R16_UNORM")				return EFormat::FORMAT_R16_UNORM;
		else if (string == "FORMAT_R16_SFLOAT")				return EFormat::FORMAT_R16_SFLOAT;
		else if (string == "FORMAT_R8G8B8A8_UNORM")			return EFormat::FORMAT_R8G8B8A8_UNORM;
		else if (string == "FORMAT_B8G8R8A8_UNORM")			return EFormat::FORMAT_B8G8R8A8_UNORM;
		else if (string == "FORMAT_R8G8B8A8_SNORM")			return EFormat::FORMAT_R8G8B8A8_SNORM;
		else if (string == "FORMAT_R16G16_SFLOAT")			return EFormat::FORMAT_R16G16_SFLOAT;
		else if (string == "FORMAT_R32G32_SFLOAT")			return EFormat::FORMAT_R32G32_SFLOAT;
		else if (string == "FORMAT_R11G11B10_SFLOAT")		return EFormat::FORMAT_R11G11B10_SFLOAT;
		else if (string == "FORMAT_R16G16B16A16_SFLOAT")	return EFormat::FORMAT_R16G16B16A16_SFLOAT;
		else if (string == "FORMAT_R32G32B32A32_SFLOAT")	return EFormat::FORMAT_R32G32B32A32_SFLOAT;
		else if (string == "FORMAT_R32G32B32A32_UINT")		return EFormat::FORMAT_R32G32B32A32_UINT;
		else if	(string == "FORMAT_D24_UNORM_S8_UINT")		return EFormat::FORMAT_D24_UNORM_S8_UINT;

		return EFormat::FORMAT_NONE;
	}

	FORCEINLINE const char* CommandQueueToString(ECommandQueueType commandQueue)
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

	FORCEINLINE const char* MemoryTypeToString(EMemoryType memoryType)
	{
		switch (memoryType)
		{
		case EMemoryType::MEMORY_TYPE_CPU_VISIBLE:		return "MEMORY_TYPE_CPU_VISIBLE";
		case EMemoryType::MEMORY_TYPE_GPU:				return "MEMORY_TYPE_GPU";
		default:										return "NONE";
		}
	}

	FORCEINLINE EMemoryType MemoryTypeFromString(const String& string)
	{
		if (string == "MEMORY_TYPE_CPU_VISIBLE")		return EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
		if (string == "MEMORY_TYPE_GPU")				return EMemoryType::MEMORY_TYPE_GPU;

		return EMemoryType::MEMORY_TYPE_NONE;
	}

	FORCEINLINE const char* PrimitiveTopologyToString(EPrimitiveTopology primitiveTopology)
	{
		switch (primitiveTopology)
		{
		case EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:	return "PRIMITIVE_TOPOLOGY_TRIANGLE_LIST";
		case EPrimitiveTopology::PRIMITIVE_TOPOLOGY_LINE_LIST:		return "PRIMITIVE_TOPOLOGY_LINE_LIST";
		case EPrimitiveTopology::PRIMITIVE_TOPOLOGY_POINT_LIST:		return "PRIMITIVE_TOPOLOGY_POINT_LIST";
		default:													return "NONE";
		}
	}

	FORCEINLINE EPrimitiveTopology PrimitiveTopologyFromString(const String& string)
	{
		if (string == "PRIMITIVE_TOPOLOGY_TRIANGLE_LIST")	return EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		if (string == "PRIMITIVE_TOPOLOGY_LINE_LIST")		return EPrimitiveTopology::PRIMITIVE_TOPOLOGY_LINE_LIST;
		if (string == "PRIMITIVE_TOPOLOGY_POINT_LIST")		return EPrimitiveTopology::PRIMITIVE_TOPOLOGY_POINT_LIST;

		return EPrimitiveTopology::PRIMITIVE_TOPOLOGY_NONE;
	}

	FORCEINLINE const char* PolygonModeToString(EPolygonMode polygonMode)
	{
		switch (polygonMode)
		{
		case EPolygonMode::POLYGON_MODE_FILL:	return "POLYGON_MODE_FILL";
		case EPolygonMode::POLYGON_MODE_LINE:	return "POLYGON_MODE_LINE";
		case EPolygonMode::POLYGON_MODE_POINT:	return "POLYGON_MODE_POINT";
		default:								return "NONE";
		}
	}

	FORCEINLINE EPolygonMode PolygonModeFromString(const String& string)
	{
		if (string == "POLYGON_MODE_FILL")		return EPolygonMode::POLYGON_MODE_FILL;
		if (string == "POLYGON_MODE_LINE")		return EPolygonMode::POLYGON_MODE_LINE;
		if (string == "POLYGON_MODE_POINT")		return EPolygonMode::POLYGON_MODE_POINT;

		return EPolygonMode::POLYGON_MODE_NONE;
	}

	FORCEINLINE const char* CullModeToString(ECullMode cullMode)
	{
		switch (cullMode)
		{
		case ECullMode::CULL_MODE_NONE:		return "CULL_MODE_NONE";
		case ECullMode::CULL_MODE_BACK:		return "CULL_MODE_BACK";
		case ECullMode::CULL_MODE_FRONT:	return "CULL_MODE_FRONT";
		default:							return "NONE";
		}
	}

	FORCEINLINE ECullMode CullModeFromString(const String& string)
	{
		if (string == "CULL_MODE_NONE")		return ECullMode::CULL_MODE_NONE;
		if (string == "CULL_MODE_BACK")		return ECullMode::CULL_MODE_BACK;
		if (string == "CULL_MODE_FRONT")	return ECullMode::CULL_MODE_FRONT;
		return ECullMode::CULL_MODE_NONE;
	}
}
