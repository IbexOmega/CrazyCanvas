#pragma once
#include "LambdaEngine.h"

#include <vector>

namespace LambdaEngine
{
    enum class EMemoryType : uint8
    {
        NONE        = 0,
        CPU_MEMORY  = 1,
        GPU_MEMORY  = 2,
    };

    enum class EFormat : uint8
    {
        NONE            = 0,
        R8G8B8A8_UNORM  = 1,
        B8G8R8A8_UNORM  = 2,
    };

	enum class EShaderType : uint32
	{
		NONE				= 0,
		MESH_SHADER			= BIT(0),
		VERTEX_SHADER		= BIT(1),
		GEOMETRY_SHADER		= BIT(2),
		HULL_SHADER			= BIT(3),
		DOMAIN_SHADER		= BIT(4),
		PIXEL_SHADER		= BIT(5),
		COMPUTE_SHADER		= BIT(6),
		RAYGEN_SHADER		= BIT(7),
		INTERSECT_SHADER	= BIT(8),
		ANY_HIT_SHADER		= BIT(9),
		CLOSEST_HIT_SHADER	= BIT(10),
		MISS_SHADER			= BIT(11),
	};

	enum class ECommandQueueType : uint8
	{
		COMMAND_QUEUE_UNKNOWN	= 0,
		COMMAND_QUEUE_IGNORE	= 1,
		COMMAND_QUEUE_COMPUTE	= 2,
		COMMAND_QUEUE_GRAPHICS	= 3,
		COMMAND_QUEUE_COPY		= 4,
	};

	enum class ECommandListType : uint8
	{
		COMMANDLIST_UNKNOWN		= 0,
		COMMANDLIST_PRIMARY		= 1,
		COMMANDLIST_SECONDARY	= 2
	};

	enum class EShaderLang : uint32
	{
		NONE	= 0,
		SPIRV	= BIT(0),
	};

	enum ETextureFlags : uint32
	{
		TEXTURE_FLAG_NONE				= 0,
		TEXTURE_FLAG_RENDER_TARGET		= FLAG(1),
		TEXTURE_FLAG_SHADER_RESOURCE	= FLAG(2),
		TEXTURE_FLAG_UNORDERED_ACCESS	= FLAG(3),
		TEXTURE_FLAG_DEPTH_STENCIL		= FLAG(4),
		TEXTURE_FLAG_COPY_SRC			= FLAG(5),
		TEXTURE_FLAG_COPY_DST			= FLAG(6),
	};

	enum EPipelineStage : uint32
	{
		PIPELINE_STAGE_UNKNOWN						= 0,
		PIPELINE_STAGE_TOP							= BIT(1),
		PIPELINE_STAGE_BOTTOM						= BIT(2),
		PIPELINE_STAGE_DRAW_INDIRECT				= BIT(3),
		PIPELINE_STAGE_VERTEX_INPUT					= BIT(4),
		PIPELINE_STAGE_VERTEX_SHADER				= BIT(5),
		PIPELINE_STAGE_HULL_SHADER					= BIT(6),
		PIPELINE_STAGE_DOMAIN_SHADER				= BIT(7),
		PIPELINE_STAGE_GEOMETRY_SHADER				= BIT(8),
		PIPELINE_STAGE_PIXEL_SHADER					= BIT(9),
		PIPELINE_STAGE_EARLY_FRAGMENT_TESTS			= BIT(10),
		PIPELINE_STAGE_LATE_FRAGMENT_TESTS			= BIT(11),
		PIPELINE_STAGE_RENDER_TARGET_OUTPUT			= BIT(12),
		PIPELINE_STAGE_COMPUTE_SHADER				= BIT(13),
		PIPELINE_STAGE_COPY							= BIT(14),
		PIPELINE_STAGE_HOST							= BIT(15),
		PIPELINE_STAGE_STREAM_OUTPUT				= BIT(16),
		PIPELINE_STAGE_CONDITIONAL_RENDERING		= BIT(17),
		PIPELINE_STAGE_RAY_TRACING_SHADER			= BIT(18),
		PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD = BIT(19),
		PIPELINE_STAGE_SHADING_RATE_TEXTURE			= BIT(20),
		PIPELINE_STAGE_TASK_SHADER					= BIT(21),
		PIPELINE_STAGE_MESH_SHADER					= BIT(22),
	};

	enum ETextureState : uint32
	{
		TEXTURE_STATE_UNKNOWN								= 0,
		TEXTURE_STATE_DONT_CARE								= 1,
		TEXTURE_STATE_GENERAL								= 2,
		TEXTURE_STATE_COLOR_ATTACHMENT						= 3,
		TEXTURE_STATE_DEPTH_STENCIL_ATTACHMENT				= 4,
		TEXTURE_STATE_DEPTH_STENCIL_READ_ONLY				= 5,
		TEXTURE_STATE_SHADER_READ_ONLY						= 6,
		TEXTURE_STATE_COPY_SRC								= 7,
		TEXTURE_STATE_COPY_DST								= 8,
		TEXTURE_STATE_PREINITIALIZED						= 9,
		TEXTURE_STATE_DEPTH_READ_ONLY_STENCIL_ATTACHMENT	= 10,
		TEXTURE_STATE_DEPTH_ATTACHMENT_STENCIL_READ_ONLY	= 11,
		TEXTURE_STATE_DEPTH_ATTACHMENT						= 12,
		TEXTURE_STATE_DEPTH_READ_ONLY						= 13,
		TEXTURE_STATE_STENCIL_ATTACHMENT					= 14,
		TEXTURE_STATE_STENCIL_READ_ONLY						= 15,
		TEXTURE_STATE_PRESENT								= 16,
		TEXTURE_STATE_SHADING_RATE							= 17,
	};

	union ShaderConstant
	{
		byte Data[4];
		float Float;
		int32 Integer;
	};

	struct ShaderDesc
	{
		const char* pSource = nullptr;
		uint32 SourceSize = 0;
		const char* pEntryPoint = "main";
		EShaderType Type = EShaderType::NONE;
		EShaderLang Lang = EShaderLang::NONE;
		std::vector<ShaderConstant> Constants;
	};

	struct Viewport
	{
		float MinDepth	= 0.0f;
		float MaxDepth	= 0.0f;
		float Width		= 0.0f;
		float Height	= 0.0f;
		float TopX		= 0.0f;
		float TopY		= 0.0f;
	};

	struct ScissorRect
	{
		uint32 Width	= 0;
		uint32 Height	= 0;
		uint32 TopX		= 0;
		uint32 TopY		= 0;
	};
	
	struct GraphicsObject
	{
		GUID_Lambda Mesh;
		GUID_Lambda Material;
	};
}
