#pragma once
#include "GraphicsTypes.h"
#include "IPipelineState.h"

namespace LambdaEngine
{
	FORCEINLINE ECommandQueueType ConvertPipelineStateTypeToQueue(EPipelineStateType pipelineStateType)
	{
		switch (pipelineStateType)
		{
		case EPipelineStateType::NONE:			return ECommandQueueType::COMMAND_QUEUE_NONE;
		case EPipelineStateType::GRAPHICS:		return ECommandQueueType::COMMAND_QUEUE_GRAPHICS;
		case EPipelineStateType::COMPUTE:		return ECommandQueueType::COMMAND_QUEUE_COMPUTE;
		case EPipelineStateType::RAY_TRACING:	return ECommandQueueType::COMMAND_QUEUE_COMPUTE;
		}
	}

	FPipelineStageFlags FORCEINLINE ConvertShaderStageToPipelineStage(FShaderStageFlags shaderStage)
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
			default:														return FPipelineStageFlags::PIPELINE_STAGE_FLAG_UNKNOWN;
		}
	}
}