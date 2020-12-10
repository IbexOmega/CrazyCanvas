#pragma once
#include "LambdaEngine.h"

namespace LambdaEngine
{
	/*
	* Constants
	*/
	constexpr const uint32 MAX_COLOR_ATTACHMENTS			= 8;
	constexpr const uint32 MAX_SUBPASSES					= 16;
	constexpr const uint32 MAX_SUBPASS_DEPENDENCIES			= 16;
	constexpr const uint32 MAX_VERTEX_INPUT_ATTACHMENTS		= 8;
	constexpr const uint32 MAX_ATTRIBUTES_PER_VERTEX		= 8;
	constexpr const uint32 MAX_IMAGE_BARRIERS				= 16;
	constexpr const uint32 MAX_MEMORY_BARRIERS				= 16;
	constexpr const uint32 MAX_BUFFER_BARRIERS				= 16;
	constexpr const uint32 MAX_VIEWPORTS					= 8;
	constexpr const uint32 MAX_VERTEX_BUFFERS				= 32;
	constexpr const uint32 MAX_DESCRIPTOR_BINDINGS			= 32;
	constexpr const uint32 MAX_CONSTANT_RANGES				= 16;
	constexpr const uint32 MAX_IMMUTABLE_SAMPLERS			= 32;
	constexpr const uint32 MAX_HIT_SHADER_COUNT				= 8;
	constexpr const uint32 MAX_MISS_SHADER_COUNT			= 8;
	constexpr const uint32 MAX_PUSH_CONSTANT_SIZE			= 128;
	constexpr const uint32 PARTIALLY_BOUND_DESCRIPTOR_COUNT	= 1024;
	constexpr const uint32 EXTERNAL_SUBPASS					= UINT32_MAX;
	constexpr const uint32 BACK_BUFFER_COUNT				= 3;

	// Determines if a resource should be allocated by a deviceallocator or via a seperate allocation
	constexpr const uint32 LARGE_TEXTURE_ALLOCATION_SIZE				= MEGA_BYTE(64);
	constexpr const uint32 LARGE_BUFFER_ALLOCATION_SIZE					= MEGA_BYTE(8);
	constexpr const uint32 LARGE_ACCELERATION_STRUCTURE_ALLOCATION_SIZE	= MEGA_BYTE(8);

	/*
	* Enums
	*/
	enum class EMemoryType : uint8
	{
		MEMORY_TYPE_NONE			= 0,
		MEMORY_TYPE_CPU_VISIBLE		= 1,
		MEMORY_TYPE_GPU				= 2,
	};

	enum class EFormat : uint8
	{
		FORMAT_NONE						= 0,
		
		FORMAT_R8_UNORM					= 1,

		FORMAT_R16_UNORM				= 2,
		FORMAT_R16_SFLOAT				= 3,
		FORMAT_R32_SFLOAT				= 4,

		FORMAT_R16G16_SFLOAT			= 5,
		FORMAT_R16G16_SNORM				= 6,
		
		FORMAT_R10G10B10A2_UNORM		= 7,

		FORMAT_R32G32_SFLOAT			= 8,

		FORMAT_B10G11R11_UFLOAT			= 9,
		
		FORMAT_R8G8B8A8_UNORM			= 10,
		FORMAT_B8G8R8A8_UNORM			= 11,
		FORMAT_R8G8B8A8_SNORM			= 12,
		
		FORMAT_R16G16B16A16_SFLOAT		= 13,
		FORMAT_R16G16B16A16_SNORM		= 14,
		FORMAT_R16G16B16A16_UNORM		= 15,
		
		FORMAT_R32G32B32A32_SFLOAT		= 16,
		FORMAT_R32G32B32A32_UINT		= 17,
		
		FORMAT_D24_UNORM_S8_UINT		= 18,

		FORMAT_R8_UINT					= 19,
		FORMAT_R8G8_UINT				= 20,
		FORMAT_R32G32_UINT				= 21,
	};

	enum class EIndexType
	{
		INDEX_TYPE_NONE		= 0,
		INDEX_TYPE_UINT16	= 1,
		INDEX_TYPE_UINT32	= 2,
	};

	enum class ECommandQueueType : uint8
	{
		COMMAND_QUEUE_TYPE_UNKNOWN	= 0,
		COMMAND_QUEUE_TYPE_NONE		= 1,
		COMMAND_QUEUE_TYPE_COMPUTE	= 2,
		COMMAND_QUEUE_TYPE_GRAPHICS	= 3,
		COMMAND_QUEUE_TYPE_COPY		= 4,
	};

	enum class ECommandListType : uint8
	{
		COMMAND_LIST_TYPE_UNKNOWN		= 0,
		COMMAND_LIST_TYPE_PRIMARY		= 1,
		COMMAND_LIST_TYPE_SECONDARY		= 2
	};

	enum class EPrimitiveTopology : uint16
	{
		PRIMITIVE_TOPOLOGY_NONE				= 0,
		PRIMITIVE_TOPOLOGY_TRIANGLE_LIST	= 1,
		PRIMITIVE_TOPOLOGY_LINE_LIST		= 2,
		PRIMITIVE_TOPOLOGY_POINT_LIST		= 3,
		PRIMITIVE_TOPOLOGY_PATCH_LIST		= 4
	};

	enum class EPolygonMode : uint8
	{
		POLYGON_MODE_NONE	= 0,
		POLYGON_MODE_FILL	= 1,
		POLYGON_MODE_LINE	= 2,
		POLYGON_MODE_POINT	= 3
	};

	enum class ECullMode : uint8
	{
		CULL_MODE_NONE	= 0,
		CULL_MODE_BACK	= 1,
		CULL_MODE_FRONT	= 2,
	};

	enum class EShaderLang : uint32
	{
		SHADER_LANG_NONE	= 0,
		SHADER_LANG_SPIRV	= 1,
		SHADER_LANG_GLSL	= 2,
	};

	enum class ELoadOp : uint8
	{
		LOAD_OP_NONE		= 0,
		LOAD_OP_LOAD		= 1,
		LOAD_OP_CLEAR		= 2,
		LOAD_OP_DONT_CARE	= 3,
	};

	enum class EStoreOp : uint8
	{
		STORE_OP_NONE		= 0,
		STORE_OP_STORE		= 1,
		STORE_OP_DONT_CARE	= 2,
	};

	enum class EFilterType : uint8
	{
		FILTER_TYPE_NONE		= 0,
		FILTER_TYPE_NEAREST		= 1,
		FILTER_TYPE_LINEAR		= 2,
	};

	enum class EMipmapMode : uint8
	{
		MIPMAP_MODE_NONE		= 0,
		MIPMAP_MODE_NEAREST		= 1,
		MIPMAP_MODE_LINEAR		= 2,
	};

	enum class ESamplerAddressMode : uint8
	{
		SAMPLER_ADDRESS_MODE_NONE					= 0,
		SAMPLER_ADDRESS_MODE_REPEAT					= 1,
		SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT		= 2,
		SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE			= 3,
		SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER		= 4,
		SAMPLER_ADDRESS_MODE_MIRRORED_CLAMP_TO_EDGE	= 5,
	};

	enum class ESamplerBorderColor : uint8
	{
		SAMPLER_BORDER_COLOR_NONE = 0,
		SAMPLER_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK = 1,
		SAMPLER_BORDER_COLOR_INT_TRANSPARENT_BLACK = 2,
		SAMPLER_BORDER_COLOR_FLOAT_OPAQUE_BLACK = 3,
		SAMPLER_BORDER_COLOR_INT_OPAQUE_BLACK = 4,
		SAMPLER_BORDER_COLOR_FLOAT_OPAQUE_WHITE = 5,
		SAMPLER_BORDER_COLOR_INT_OPAQUE_WHITE = 6,
	};


	enum class ETextureState : uint32
	{
		TEXTURE_STATE_UNKNOWN								= 0,
		TEXTURE_STATE_DONT_CARE								= 1,
		TEXTURE_STATE_GENERAL								= 2,
		TEXTURE_STATE_RENDER_TARGET							= 3,
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

	enum class EDescriptorType : uint32
	{
		DESCRIPTOR_TYPE_UNKNOWN								= 0,
		DESCRIPTOR_TYPE_SHADER_RESOURCE_TEXTURE				= 1,
		DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER	= 3,
		DESCRIPTOR_TYPE_UNORDERED_ACCESS_TEXTURE			= 2,
		DESCRIPTOR_TYPE_CONSTANT_BUFFER						= 4,
		DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER				= 5,
		DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE				= 6,
		DESCRIPTOR_TYPE_SAMPLER								= 7,
	};

	enum class EQueryType : uint8
	{
		QUERY_TYPE_NONE					= 0,
		QUERY_TYPE_TIMESTAMP			= 1,
		QUERY_TYPE_OCCLUSION			= 2,
		QUERY_TYPE_PIPELINE_STATISTICS	= 3,
	};

	enum class EBlendOp : uint8
	{
		BLEND_OP_NONE		= 0,
		BLEND_OP_ADD		= 1,
		BLEND_OP_SUB		= 2,
		BLEND_OP_REV_SUB	= 3,
		BLEND_OP_MIN		= 4,
		BLEND_OP_MAX		= 5,
	};

	enum class ELogicOp : int8
	{
		LOGIC_OP_NONE			= 0,
		LOGIC_OP_CLEAR			= 1,
		LOGIC_OP_AND			= 2,
		LOGIC_OP_AND_REVERSE	= 3,
		LOGIC_OP_COPY			= 4,
		LOGIC_OP_AND_INVERTED	= 5,
		LOGIC_OP_NO_OP			= 6,
		LOGIC_OP_XOR			= 7,
		LOGIC_OP_OR				= 8,
		LOGIC_OP_NOR			= 9,
		LOGIC_OP_EQUIVALENT		= 10,
		LOGIC_OP_INVERT			= 11,
		LOGIC_OP_OR_REVERSE		= 12,
		LOGIC_OP_COPY_INVERTED	= 13,
		LOGIC_OP_OR_INVERTED	= 14,
		LOGIC_OP_NAND			= 15,
		LOGIC_OP_SET			= 16,
	};

	enum class EBlendFactor : uint8
	{
		BLEND_FACTOR_NONE				= 0,
		BLEND_FACTOR_ZERO				= 1,
		BLEND_FACTOR_ONE				= 2,
		BLEND_FACTOR_SRC_COLOR			= 3,
		BLEND_FACTOR_INV_SRC_COLOR		= 4,
		BLEND_FACTOR_DST_COLOR			= 5,
		BLEND_FACTOR_INV_DST_COLOR		= 6,
		BLEND_FACTOR_SRC_ALPHA			= 7,
		BLEND_FACTOR_INV_SRC_ALPHA		= 8,
		BLEND_FACTOR_DST_ALPHA			= 9,
		BLEND_FACTOR_INV_DST_ALPHA		= 10,
		BLEND_FACTOR_CONSTANT_COLOR		= 11,
		BLEND_FACTOR_INV_CONSTANT_COLOR	= 12,
		BLEND_FACTOR_CONSTANT_ALPHA		= 13,
		BLEND_FACTOR_INV_CONSTANT_ALPHA	= 14,
		BLEND_FACTOR_SRC_ALPHA_SATURATE	= 15,
		BLEND_FACTOR_SRC1_COLOR			= 16,
		BLEND_FACTOR_INV_SRC1_COLOR		= 17,
		BLEND_FACTOR_SRC1_ALPHA			= 18,
		BLEND_FACTOR_INV_SRC1_ALPHA		= 19,
	};

	enum class ECompareOp
	{
		COMPARE_OP_NEVER			= 0,
		COMPARE_OP_LESS				= 1,
		COMPARE_OP_EQUAL			= 2,
		COMPARE_OP_LESS_OR_EQUAL	= 3,
		COMPARE_OP_GREATER			= 4,
		COMPARE_OP_NOT_EQUAL		= 5,
		COMPARE_OP_GREATER_OR_EQUAL	= 6,
		COMPARE_OP_ALWAYS			= 7,
	};

	enum class EStencilOp
	{
		STENCIL_OP_KEEP					= 0,
		STENCIL_OP_ZERO					= 1,
		STENCIL_OP_REPLACE				= 2,
		STENCIL_OP_INCREMENT_AND_CLAMP	= 3,
		STENCIL_OP_DECREMENT_AND_CLAMP	= 4,
		STENCIL_OP_INVERT				= 5,
		STENCIL_OP_INCREMENT_AND_WRAP	= 6,
		STENCIL_OP_DECREMENT_AND_WRAP	= 7,
	};

	enum class ETextureViewType : uint8
	{
		TEXTURE_VIEW_TYPE_NONE			= 0,
		TEXTURE_VIEW_TYPE_1D			= 1,
		TEXTURE_VIEW_TYPE_2D			= 2,
		TEXTURE_VIEW_TYPE_3D			= 3,
		TEXTURE_VIEW_TYPE_CUBE			= 4,
		TEXTURE_VIEW_TYPE_CUBE_ARRAY	= 5,
		TEXTURE_VIEW_TYPE_2D_ARRAY		= 6,
	};

	enum class EPipelineStateType : uint8
	{
		PIPELINE_STATE_TYPE_NONE			= 0,
		PIPELINE_STATE_TYPE_GRAPHICS		= 1,
		PIPELINE_STATE_TYPE_COMPUTE			= 2,
		PIPELINE_STATE_TYPE_RAY_TRACING		= 3,
	};

	typedef uint32 FShaderStageFlags;
	enum FShaderStageFlag : FShaderStageFlags
	{
		SHADER_STAGE_FLAG_NONE					= 0,
		SHADER_STAGE_FLAG_MESH_SHADER			= FLAG(0),
		SHADER_STAGE_FLAG_TASK_SHADER			= FLAG(1),
		SHADER_STAGE_FLAG_VERTEX_SHADER			= FLAG(2),
		SHADER_STAGE_FLAG_GEOMETRY_SHADER		= FLAG(3),
		SHADER_STAGE_FLAG_HULL_SHADER			= FLAG(4),
		SHADER_STAGE_FLAG_DOMAIN_SHADER			= FLAG(5),
		SHADER_STAGE_FLAG_PIXEL_SHADER			= FLAG(6),
		SHADER_STAGE_FLAG_COMPUTE_SHADER		= FLAG(7),
		SHADER_STAGE_FLAG_RAYGEN_SHADER			= FLAG(8),
		SHADER_STAGE_FLAG_INTERSECT_SHADER		= FLAG(9),
		SHADER_STAGE_FLAG_ANY_HIT_SHADER		= FLAG(10),
		SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER	= FLAG(11),
		SHADER_STAGE_FLAG_MISS_SHADER			= FLAG(12),
		SHADER_STAGE_FLAG_ALL					= FLAG(13),
	};

	typedef uint32 FBufferFlags;
	enum FBufferFlag : FBufferFlags
	{
		BUFFER_FLAG_NONE								= 0,
		BUFFER_FLAG_VERTEX_BUFFER						= FLAG(1),
		BUFFER_FLAG_INDEX_BUFFER						= FLAG(2),
		BUFFER_FLAG_UNORDERED_ACCESS_BUFFER				= FLAG(3),
		BUFFER_FLAG_CONSTANT_BUFFER						= FLAG(4),
		BUFFER_FLAG_COPY_DST							= FLAG(5),
		BUFFER_FLAG_COPY_SRC							= FLAG(5),
		BUFFER_FLAG_ACCELERATIONS_STRUCTURE_STORAGE		= FLAG(6),
		BUFFER_FLAG_ACCELERATIONS_STRUCTURE_BUILD_INPUT	= FLAG(7),
		BUFFER_FLAG_SHADER_BINDING_TABLE				= FLAG(8),
		BUFFER_FLAG_INDIRECT_BUFFER						= FLAG(9),
	};

	typedef uint16 FTextureFlags;
	enum FTextureFlag : FTextureFlags
	{
		TEXTURE_FLAG_NONE				= 0,
		TEXTURE_FLAG_RENDER_TARGET		= FLAG(1),
		TEXTURE_FLAG_SHADER_RESOURCE	= FLAG(2),
		TEXTURE_FLAG_UNORDERED_ACCESS	= FLAG(3),
		TEXTURE_FLAG_DEPTH_STENCIL		= FLAG(4),
		TEXTURE_FLAG_COPY_SRC			= FLAG(5),
		TEXTURE_FLAG_COPY_DST			= FLAG(6),
		TEXTURE_FLAG_CUBE_COMPATIBLE	= FLAG(7)
	};

	typedef uint32 FTextureViewFlags;
	enum FTextureViewFlag : FTextureViewFlags
	{
		TEXTURE_VIEW_FLAG_NONE				= 0,
		TEXTURE_VIEW_FLAG_RENDER_TARGET		= FLAG(1),
		TEXTURE_VIEW_FLAG_DEPTH_STENCIL		= FLAG(2),
		TEXTURE_VIEW_FLAG_UNORDERED_ACCESS	= FLAG(3),
		TEXTURE_VIEW_FLAG_SHADER_RESOURCE	= FLAG(4),
	};

	typedef uint32 FPipelineStageFlags;
	enum FPipelineStageFlag : FPipelineStageFlags
	{
		PIPELINE_STAGE_FLAG_UNKNOWN							= 0,
		PIPELINE_STAGE_FLAG_TOP								= FLAG(1),
		PIPELINE_STAGE_FLAG_BOTTOM							= FLAG(2),
		PIPELINE_STAGE_FLAG_DRAW_INDIRECT					= FLAG(3),
		PIPELINE_STAGE_FLAG_VERTEX_INPUT					= FLAG(4),
		PIPELINE_STAGE_FLAG_VERTEX_SHADER					= FLAG(5),
		PIPELINE_STAGE_FLAG_HULL_SHADER						= FLAG(6),
		PIPELINE_STAGE_FLAG_DOMAIN_SHADER					= FLAG(7),
		PIPELINE_STAGE_FLAG_GEOMETRY_SHADER					= FLAG(8),
		PIPELINE_STAGE_FLAG_PIXEL_SHADER					= FLAG(9),
		PIPELINE_STAGE_FLAG_EARLY_FRAGMENT_TESTS			= FLAG(10),
		PIPELINE_STAGE_FLAG_LATE_FRAGMENT_TESTS				= FLAG(11),
		PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT			= FLAG(12),
		PIPELINE_STAGE_FLAG_COMPUTE_SHADER					= FLAG(13),
		PIPELINE_STAGE_FLAG_COPY							= FLAG(14),
		PIPELINE_STAGE_FLAG_HOST							= FLAG(15),
		PIPELINE_STAGE_FLAG_STREAM_OUTPUT					= FLAG(16),
		PIPELINE_STAGE_FLAG_CONDITIONAL_RENDERING			= FLAG(17),
		PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER				= FLAG(18),
		PIPELINE_STAGE_FLAG_ACCELERATION_STRUCTURE_BUILD	= FLAG(19),
		PIPELINE_STAGE_FLAG_SHADING_RATE_TEXTURE			= FLAG(20),
		PIPELINE_STAGE_FLAG_TASK_SHADER						= FLAG(21),
		PIPELINE_STAGE_FLAG_MESH_SHADER						= FLAG(22),

		PIPELINE_STAGE_FLAG_ALL_STAGES						= FLAG(23),
	};

	typedef uint32 FMemoryAccessFlags;
	enum FMemoryAccessFlag : FMemoryAccessFlags
	{
		MEMORY_ACCESS_FLAG_UNKNOWN								= 0,
		MEMORY_ACCESS_FLAG_INDIRECT_COMMAND_READ				= FLAG(1),
		MEMORY_ACCESS_FLAG_INDEX_READ							= FLAG(2),
		MEMORY_ACCESS_FLAG_VERTEX_ATTRIBUTE_READ				= FLAG(3),
		MEMORY_ACCESS_FLAG_CONSTANT_BUFFER_READ					= FLAG(4),
		MEMORY_ACCESS_FLAG_INPUT_ATTACHMENT_READ				= FLAG(5),
		MEMORY_ACCESS_FLAG_SHADER_READ							= FLAG(6),
		MEMORY_ACCESS_FLAG_SHADER_WRITE							= FLAG(7),
		MEMORY_ACCESS_FLAG_COLOR_ATTACHMENT_READ				= FLAG(8),
		MEMORY_ACCESS_FLAG_COLOR_ATTACHMENT_WRITE				= FLAG(9),
		MEMORY_ACCESS_FLAG_DEPTH_STENCIL_ATTACHMENT_READ		= FLAG(10),
		MEMORY_ACCESS_FLAG_DEPTH_STENCIL_ATTACHMENT_WRITE		= FLAG(11),
		MEMORY_ACCESS_FLAG_TRANSFER_READ						= FLAG(12),
		MEMORY_ACCESS_FLAG_TRANSFER_WRITE						= FLAG(13),
		MEMORY_ACCESS_FLAG_HOST_READ							= FLAG(14),
		MEMORY_ACCESS_FLAG_HOST_WRITE							= FLAG(15),
		MEMORY_ACCESS_FLAG_MEMORY_READ							= FLAG(16),
		MEMORY_ACCESS_FLAG_MEMORY_WRITE							= FLAG(17),
		MEMORY_ACCESS_FLAG_TRANSFORM_FEEDBACK_WRITE				= FLAG(18),
		MEMORY_ACCESS_FLAG_TRANSFORM_FEEDBACK_COUNTER_READ		= FLAG(19),
		MEMORY_ACCESS_FLAG_TRANSFORM_FEEDBACK_COUNTER_WRITE		= FLAG(20),
		MEMORY_ACCESS_FLAG_CONDITIONAL_RENDERING_READ			= FLAG(21),
		MEMORY_ACCESS_FLAG_COLOR_ATTACHMENT_READ_NONCOHERENT	= FLAG(22),
		MEMORY_ACCESS_FLAG_ACCELERATION_STRUCTURE_READ			= FLAG(23),
		MEMORY_ACCESS_FLAG_ACCELERATION_STRUCTURE_WRITE			= FLAG(24),
		MEMORY_ACCESS_FLAG_SHADING_RATE_IMAGE_READ				= FLAG(25),
		MEMORY_ACCESS_FLAG_FRAGMENT_DENSITY_MAP_READ			= FLAG(26),
		MEMORY_ACCESS_FLAG_COMMAND_PREPROCESS_READ				= FLAG(27),
		MEMORY_ACCESS_FLAG_COMMAND_PREPROCESS_WRITE				= FLAG(28),
	};

	typedef uint32 FColorComponentFlags;
	enum FColorComponentFlag : FColorComponentFlags
	{
		COLOR_COMPONENT_FLAG_NONE	= 0,
		COLOR_COMPONENT_FLAG_ALL	= 0xffffffff,
		COLOR_COMPONENT_FLAG_R		= FLAG(0),
		COLOR_COMPONENT_FLAG_G		= FLAG(1),
		COLOR_COMPONENT_FLAG_B		= FLAG(2),
		COLOR_COMPONENT_FLAG_A		= FLAG(3),
	};

	typedef uint32 FAccelerationStructureFlags;
	enum FAccelerationStructureFlag : FAccelerationStructureFlags
	{
		ACCELERATION_STRUCTURE_FLAG_NONE			= 0,
		ACCELERATION_STRUCTURE_FLAG_ALLOW_UPDATE	= FLAG(1),
	};

	typedef uint32 FQueryPipelineStatisticsFlags;
	enum FQueryPipelineStatisticsFlag : FQueryPipelineStatisticsFlags
	{
		QUERY_PIPELINE_STATISTICS_FLAG_NONE											= 0,
		QUERY_PIPELINE_STATISTICS_FLAG_INPUT_ASSEMBLY_VERTICES						= FLAG(1),
		QUERY_PIPELINE_STATISTICS_FLAG_INPUT_ASSEMBLY_PRIMITIVES					= FLAG(2),
		QUERY_PIPELINE_STATISTICS_FLAG_VERTEX_SHADER_INVOCATIONS					= FLAG(3),
		QUERY_PIPELINE_STATISTICS_FLAG_GEOMETRY_SHADER_INVOCATIONS					= FLAG(4),
		QUERY_PIPELINE_STATISTICS_FLAG_GEOMETRY_SHADER_PRIMITIVES					= FLAG(5),
		QUERY_PIPELINE_STATISTICS_FLAG_CLIPPING_INVOCATIONS							= FLAG(6),
		QUERY_PIPELINE_STATISTICS_FLAG_CLIPPING_PRIMITIVES							= FLAG(7),
		QUERY_PIPELINE_STATISTICS_FLAG_FRAGMENT_SHADER_INVOCATIONS					= FLAG(8),
		QUERY_PIPELINE_STATISTICS_FLAG_TESSELLATION_CONTROL_SHADER_PATCHES			= FLAG(9),
		QUERY_PIPELINE_STATISTICS_FLAG_TESSELLATION_EVALUATION_SHADER_INVOCATIONS	= FLAG(10),
		QUERY_PIPELINE_STATISTICS_FLAG_COMPUTE_SHADER_INVOCATIONS					= FLAG(11),
	};

	typedef uint32 FAccelerationStructureInstanceFlags;
	enum FAccelerationStructureInstanceFlag : FAccelerationStructureInstanceFlags
	{
		RAY_TRACING_INSTANCE_FLAG_NONE				= 0,
		RAY_TRACING_INSTANCE_FLAG_CULLING_DISABLED	= FLAG(0),
		RAY_TRACING_INSTANCE_FLAG_FRONT_CCW			= FLAG(1),
		RAY_TRACING_INSTANCE_FLAG_FORCE_OPAQUE		= FLAG(2),
		RAY_TRACING_INSTANCE_FLAG_FORCE_NO_OPAQUE	= FLAG(3),
	};

	typedef uint32 FDescriptorSetLayoutsFlags;
	enum FDescriptorSetLayoutsFlag : FDescriptorSetLayoutsFlags
	{
		DESCRIPTOR_SET_LAYOUT_FLAG_NONE						= 0,
		DESCRIPTOR_SET_LAYOUT_FLAG_PUSH_DESCRIPTOR			= FLAG(0),
		DESCRIPTOR_SET_LAYOUT_FLAG_UPDATE_AFTER_BIND_POOL	= FLAG(1),
	};

	typedef uint32 FDescriptorHeapFlags;
	enum FDescriptorHeapFlag : FDescriptorHeapFlags
	{
		DESCRIPTOR_HEAP_FLAG_NONE					= 0,
		DESCRIPTOR_HEAP_FLAG_PUSH_DESCRIPTOR		= FLAG(0),
		DESCRIPTOR_HEAP_FLAG_UPDATE_AFTER_BIND_POOL	= FLAG(1),
	};

	typedef uint32 FExtraDynamicStateFlags;
	enum FExtraDynamicStateFlag : FExtraDynamicStateFlags
	{
		EXTRA_DYNAMIC_STATE_FLAG_NONE				= 0,
		EXTRA_DYNAMIC_STATE_FLAG_STENCIL_ENABLE		= FLAG(1),
		EXTRA_DYNAMIC_STATE_FLAG_STENCIL_OP			= FLAG(2),
		EXTRA_DYNAMIC_STATE_FLAG_STENCIL_REFERENCE	= FLAG(3),
		EXTRA_DYNAMIC_STATE_FLAG_LINE_WIDTH			= FLAG(4),
	};

	typedef uint32 FDescriptorSetLayoutBindingFlags;
	enum FDescriptorSetLayoutBindingFlag : FDescriptorSetLayoutBindingFlags
	{
		DESCRIPTOR_SET_LAYOUT_BINDING_FLAG_NONE				= 0,
		DESCRIPTOR_SET_LAYOUT_BINDING_FLAG_PARTIALLY_BOUND	= FLAG(0),
	};

	enum class EVertexInputRate : uint8
	{
		VERTEX_INPUT_NONE			= 0,
		VERTEX_INPUT_PER_VERTEX		= 1,
		VERTEX_INPUT_PER_INSTANCE	= 2,
	};

	enum class EStencilFace : uint8
	{
		STENCIL_FACE_FRONT			= 0,
		STENCIL_FACE_BACK			= 1,
		STENCIL_FACE_FRONT_AND_BACK	= 2,
	};

	/*
	* Structs
	*/

	struct Viewport
	{
		float32 MinDepth	= 0.0f;
		float32 MaxDepth	= 1.0f;
		float32 Width		= 0.0f;
		float32 Height		= 0.0f;
		float32 x			= 0.0f;
		float32 y			= 0.0f;
	};

	struct ScissorRect
	{
		uint32 Width	= 0;
		uint32 Height	= 0;
		int32 x			= 0;
		int32 y			= 0;
	};

	struct StencilOpStateDesc
	{
		EStencilOp	FailOp			= EStencilOp::STENCIL_OP_ZERO;
		EStencilOp	PassOp			= EStencilOp::STENCIL_OP_ZERO;
		EStencilOp	DepthFailOp		= EStencilOp::STENCIL_OP_ZERO;
		ECompareOp	CompareOp		= ECompareOp::COMPARE_OP_LESS_OR_EQUAL;
		uint32		CompareMask		= 0x00000000;
		uint32		WriteMask		= 0x00000000;
		uint32		Reference		= 0x00000000;
	};

	struct SBTRecord
	{
		uint64	VertexBufferAddress		= 0;
		uint64	IndexBufferAddress		= 0;
	};
}
