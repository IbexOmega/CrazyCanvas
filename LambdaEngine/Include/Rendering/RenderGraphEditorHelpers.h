#pragma once

#include "RenderGraphTypes.h"
#include "Rendering/Core/API/GraphicsTypes.h"

namespace LambdaEngine
{
	constexpr const char* TEXTURE_FORMAT_NAMES[] =
	{
		"FORMAT_R8G8B8A8_UNORM",
		"FORMAT_R8G8B8A8_SNORM",
		"FORMAT_B8G8R8A8_UNORM",
		"FORMAT_R8_UNORM",
		"FORMAT_R16_UNORM",
		"FORMAT_R16_SFLOAT",
		"FORMAT_R16G16_SFLOAT",
		"FORMAT_R16G16B16A16_SFLOAT",
		"FORMAT_R32G32_SFLOAT",
		"FORMAT_R11G11B10_SFLOAT",
		"FORMAT_R32G32B32A32_UINT",
		"FORMAT_R32G32B32A32_SFLOAT",
		"FORMAT_D24_UNORM_S8_UINT",
	};

	static EFormat TextureFormatIndexToFormat(int32 index)
	{
		switch (index)
		{
		case 0:		return EFormat::FORMAT_R8G8B8A8_UNORM;
		case 1:		return EFormat::FORMAT_R8G8B8A8_SNORM;
		case 2:		return EFormat::FORMAT_B8G8R8A8_UNORM;
		case 3:		return EFormat::FORMAT_R8_UNORM;
		case 4:		return EFormat::FORMAT_R16_UNORM;
		case 5:		return EFormat::FORMAT_R16_SFLOAT;
		case 6:		return EFormat::FORMAT_R16G16_SFLOAT;
		case 7:		return EFormat::FORMAT_R16G16B16A16_SFLOAT;
		case 8:		return EFormat::FORMAT_R32G32_SFLOAT;
		case 9:		return EFormat::FORMAT_R11G11B10_UFLOAT;
		case 10:	return EFormat::FORMAT_R32G32B32A32_UINT;
		case 11:	return EFormat::FORMAT_R32G32B32A32_SFLOAT;
		case 12:	return EFormat::FORMAT_D24_UNORM_S8_UINT;
		}

		return EFormat::FORMAT_NONE;
	}

	static int32 TextureFormatToFormatIndex(EFormat format)
	{
		switch (format)
		{
		case EFormat::FORMAT_R8G8B8A8_UNORM:		return 0;
		case EFormat::FORMAT_R8G8B8A8_SNORM:		return 1;
		case EFormat::FORMAT_B8G8R8A8_UNORM:		return 2;
		case EFormat::FORMAT_R8_UNORM:				return 3;
		case EFormat::FORMAT_R16_UNORM:				return 4;
		case EFormat::FORMAT_R16_SFLOAT:			return 5;
		case EFormat::FORMAT_R16G16_SFLOAT:			return 6;
		case EFormat::FORMAT_R16G16B16A16_SFLOAT:	return 7;
		case EFormat::FORMAT_R32G32_SFLOAT:			return 8;
		case EFormat::FORMAT_R11G11B10_UFLOAT:		return 9;
		case EFormat::FORMAT_R32G32B32A32_UINT:		return 10;
		case EFormat::FORMAT_R32G32B32A32_SFLOAT:	return 11;
		case EFormat::FORMAT_D24_UNORM_S8_UINT:		return 12;
		}

		return -1;
	}

	constexpr const char* DIMENSION_NAMES[] =
	{
		"CONSTANT",
		"EXTERNAL",
		"RELATIVE",
		"RELATIVE_1D",
	};

	ERenderGraphDimensionType DimensionTypeIndexToDimensionType(int32 index)
	{
		switch (index)
		{
		case 0: return ERenderGraphDimensionType::CONSTANT;
		case 1: return ERenderGraphDimensionType::EXTERNAL;
		case 2: return ERenderGraphDimensionType::RELATIVE;
		case 3: return ERenderGraphDimensionType::RELATIVE_1D;
		}

		return ERenderGraphDimensionType::NONE;
	}

	int32 DimensionTypeToDimensionTypeIndex(ERenderGraphDimensionType dimensionType)
	{
		switch (dimensionType)
		{
		case ERenderGraphDimensionType::CONSTANT:		return 0;
		case ERenderGraphDimensionType::EXTERNAL:		return 1;
		case ERenderGraphDimensionType::RELATIVE:		return 2;
		case ERenderGraphDimensionType::RELATIVE_1D:	return 3;
		}

		return -1;
	}

	constexpr const char* SAMPLER_NAMES[] =
	{
		"LINEAR",
		"NEAREST",
	};

	ERenderGraphSamplerType SamplerTypeIndexToSamplerType(int32 index)
	{
		switch (index)
		{
		case 0: return ERenderGraphSamplerType::LINEAR;
		case 1: return ERenderGraphSamplerType::NEAREST;
		}

		return ERenderGraphSamplerType::NONE;
	}

	int32 SamplerTypeToSamplerTypeIndex(ERenderGraphSamplerType samplerType)
	{
		switch (samplerType)
		{
		case ERenderGraphSamplerType::LINEAR:	return 0;
		case ERenderGraphSamplerType::NEAREST:	return 1;
		}

		return -1;
	}

	constexpr const char* MEMORY_TYPE_NAMES[] =
	{
		"MEMORY_TYPE_CPU_VISIBLE",
		"MEMORY_TYPE_GPU",
	};

	EMemoryType MemoryTypeIndexToMemoryType(int32 index)
	{
		switch (index)
		{
		case 0: return EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
		case 1: return EMemoryType::MEMORY_TYPE_GPU;
		}

		return EMemoryType::MEMORY_TYPE_NONE;
	}

	int32 MemoryTypeToMemoryTypeIndex(EMemoryType memoryType)
	{
		switch (memoryType)
		{
		case EMemoryType::MEMORY_TYPE_CPU_VISIBLE:	return 0;
		case EMemoryType::MEMORY_TYPE_GPU:			return 1;
		}

		return -1;
	}

	constexpr const char* PRIMITIVE_TOPOLOGY_NAMES[] =
	{
		"PRIMITIVE_TOPOLOGY_TRIANGLE_LIST",
		"PRIMITIVE_TOPOLOGY_LINE_LIST",
		"PRIMITIVE_TOPOLOGY_POINT_LIST",
	};

	EPrimitiveTopology PrimitiveTopologyIndexToPrimitiveTopology(int32 index)
	{
		switch (index)
		{
		case 0: return EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		case 1: return EPrimitiveTopology::PRIMITIVE_TOPOLOGY_LINE_LIST;
		case 2: return EPrimitiveTopology::PRIMITIVE_TOPOLOGY_POINT_LIST;
		}

		return EPrimitiveTopology::PRIMITIVE_TOPOLOGY_NONE;
	}

	int32 PrimitiveTopologyToPrimitiveTopologyIndex(EPrimitiveTopology primitiveTopology)
	{
		switch (primitiveTopology)
		{
		case EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:	return 0;
		case EPrimitiveTopology::PRIMITIVE_TOPOLOGY_LINE_LIST:		return 1;
		case EPrimitiveTopology::PRIMITIVE_TOPOLOGY_POINT_LIST:		return 2;
		}

		return -1;
	}

	constexpr const char* POLYGON_MODE_NAMES[] =
	{
		"POLYGON_MODE_FILL",
		"POLYGON_MODE_LINE",
		"POLYGON_MODE_POINT",
	};

	EPolygonMode PolygonModeIndexToPolygonMode(int32 index)
	{
		switch (index)
		{
		case 0: return EPolygonMode::POLYGON_MODE_FILL;
		case 1: return EPolygonMode::POLYGON_MODE_LINE;
		case 2: return EPolygonMode::POLYGON_MODE_POINT;
		}

		return EPolygonMode::POLYGON_MODE_NONE;
	}

	int32 PolygonModeToPolygonModeIndex(EPolygonMode primitiveTopology)
	{
		switch (primitiveTopology)
		{
		case EPolygonMode::POLYGON_MODE_FILL:	return 0;
		case EPolygonMode::POLYGON_MODE_LINE:	return 1;
		case EPolygonMode::POLYGON_MODE_POINT:	return 2;
		}

		return -1;
	}

	constexpr const char* CULL_MODE_NAMES[] =
	{
		"CULL_MODE_BACK",
		"CULL_MODE_FRONT",
		"CULL_MODE_NONE",
	};

	ECullMode CullModeIndexToCullMode(int32 index)
	{
		switch (index)
		{
		case 0: return ECullMode::CULL_MODE_BACK;
		case 1: return ECullMode::CULL_MODE_FRONT;
		case 2: return ECullMode::CULL_MODE_NONE;
		}

		return ECullMode::CULL_MODE_NONE;
	}

	int32 CullModeToCullModeIndex(ECullMode primitiveTopology)
	{
		switch (primitiveTopology)
		{
		case ECullMode::CULL_MODE_BACK:		return 0;
		case ECullMode::CULL_MODE_FRONT:	return 1;
		case ECullMode::CULL_MODE_NONE:		return 2;
		}

		return -1;
	}

	constexpr const char* TRIGGER_TYPE_NAMES[] =
	{
		"DISABLED",
		"EVERY",
		"TRIGGERED",
	};

	ERenderStageExecutionTrigger TriggerTypeIndexToTriggerType(int32 index)
	{
		switch (index)
		{
		case 0: return ERenderStageExecutionTrigger::DISABLED;
		case 1: return ERenderStageExecutionTrigger::EVERY;
		case 2: return ERenderStageExecutionTrigger::TRIGGERED;
		}

		return ERenderStageExecutionTrigger::NONE;
	}

	int32 TriggerTypeToTriggerTypeIndex(ERenderStageExecutionTrigger primitiveTopology)
	{
		switch (primitiveTopology)
		{
		case ERenderStageExecutionTrigger::DISABLED:	return 0;
		case ERenderStageExecutionTrigger::EVERY:		return 1;
		case ERenderStageExecutionTrigger::TRIGGERED:	return 2;
		}

		return -1;
	}

	constexpr const char* DRAW_TYPE_NAMES[] =
	{
		"SCENE_INSTANCES",
		"FULLSCREEN_QUAD",
		"CUBE",
	};

	ERenderStageDrawType DrawTypeIndexToDrawType(int32 index)
	{
		switch (index)
		{
		case 0: return ERenderStageDrawType::SCENE_INSTANCES;
		case 1: return ERenderStageDrawType::FULLSCREEN_QUAD;
		case 2: return ERenderStageDrawType::CUBE;
		}

		return ERenderStageDrawType::NONE;
	}

	int32 DrawTypeToDrawTypeIndex(ERenderStageDrawType drawType)
	{
		switch (drawType)
		{
		case ERenderStageDrawType::SCENE_INSTANCES:	return 0;
		case ERenderStageDrawType::FULLSCREEN_QUAD:	return 1;
		case ERenderStageDrawType::CUBE:			return 2;
		}

		return -1;
	}
}