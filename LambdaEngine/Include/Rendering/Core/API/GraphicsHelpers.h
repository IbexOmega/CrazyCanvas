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
}