#pragma once
#include "GraphicsTypes.h"
#include "IPipelineState.h"

namespace LambdaEngine
{
	FORCEINLINE ECommandQueueType ConvertPipelineStateTypeToQueue(EPipelineStateType pipelineStateType)
	{
		switch (pipelineStateType)
		{
		case EPipelineStateType::PIPELINE_GRAPHICS:		return ECommandQueueType::COMMAND_QUEUE_GRAPHICS;
		case EPipelineStateType::PIPELINE_COMPUTE:		return ECommandQueueType::COMMAND_QUEUE_COMPUTE;
		case EPipelineStateType::PIPELINE_RAY_TRACING:	return ECommandQueueType::COMMAND_QUEUE_COMPUTE;
		case EPipelineStateType::NONE:			
		default:								return ECommandQueueType::COMMAND_QUEUE_NONE;
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

	FORCEINLINE bool CheckValidDescriptorCount(const DescriptorCountDesc& count)
	{
		return
			(count.DescriptorSetCount						> 0) &&
			(count.AccelerationStructureDescriptorCount		> 0) &&
			(count.ConstantBufferDescriptorCount			> 0) &&
			(count.SamplerDescriptorCount					> 0) &&
			(count.TextureCombinedSamplerDescriptorCount	> 0) &&
			(count.TextureDescriptorCount					> 0) &&
			(count.UnorderedAccessBufferDescriptorCount		> 0) &&
			(count.UnorderedAccessTextureDescriptorCount	> 0);
	}

	FORCEINLINE uint32 TextureFormatStride(EFormat format)
	{
		switch (format)
		{		
        case EFormat::FORMAT_D24_UNORM_S8_UINT:
        case EFormat::FORMAT_R8G8B8A8_UNORM:
        case EFormat::FORMAT_B8G8R8A8_UNORM:
        case EFormat::FORMAT_R8G8B8A8_SNORM:		return 4;
		case EFormat::FORMAT_R32G32_SFLOAT:
        case EFormat::FORMAT_R16G16B16A16_SFLOAT:	return 8;
        default:                                    return 0;
        }
	}
}
