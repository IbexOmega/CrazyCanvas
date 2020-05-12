#pragma once
#include "Rendering/Core/API/GraphicsTypes.h"

#include "Vulkan.h"

#define LOG_VULKAN_ERROR(result, ...) \
    LOG_ERROR(__VA_ARGS__); \
    LOG_ERROR("%s CODE: %s", LambdaEngine::VkResultToString(result), LambdaEngine::GetVkErrorString(result)) \

namespace LambdaEngine
{
    inline uint32 FindMemoryType(VkPhysicalDevice physicalDevice, uint32 typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties = {};
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32 i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        return UINT32_MAX;
    }

    inline VkFormat ConvertFormat(EFormat format)
    {
        switch (format)
        {
		case EFormat::FORMAT_R32G32_SFLOAT:			return VK_FORMAT_R32G32_SFLOAT;
        case EFormat::FORMAT_R8G8B8A8_UNORM:		return VK_FORMAT_R8G8B8A8_UNORM;
        case EFormat::FORMAT_B8G8R8A8_UNORM:		return VK_FORMAT_B8G8R8A8_UNORM;
        case EFormat::FORMAT_R8G8B8A8_SNORM:		return VK_FORMAT_R8G8B8A8_SNORM;
        case EFormat::FORMAT_R16G16B16A16_SFLOAT:	return VK_FORMAT_R16G16B16A16_SFLOAT;
		case EFormat::FORMAT_D24_UNORM_S8_UINT:		return VK_FORMAT_D24_UNORM_S8_UINT;
        default:                                    return VK_FORMAT_UNDEFINED;
        }
    }

	inline VkIndexType ConvertIndexType(EIndexType indexType)
	{
		switch (indexType)
		{
		case EIndexType::UINT16:		return VK_INDEX_TYPE_UINT16;
		case EIndexType::UINT32:		return VK_INDEX_TYPE_UINT32;
		default:						return VK_INDEX_TYPE_MAX_ENUM;
		}
	}

    inline VkSampleCountFlagBits ConvertSamples(uint32 samples)
    {
        switch (samples)
        {
        case 1:     return VK_SAMPLE_COUNT_1_BIT;
        case 2:     return VK_SAMPLE_COUNT_2_BIT;
        case 4:     return VK_SAMPLE_COUNT_4_BIT;
        case 8:     return VK_SAMPLE_COUNT_8_BIT;
        case 16:    return VK_SAMPLE_COUNT_16_BIT;
        case 32:    return VK_SAMPLE_COUNT_32_BIT;
        case 64:    return VK_SAMPLE_COUNT_64_BIT;
        default:    return VkSampleCountFlagBits(0);
        }
    }

	inline VkAttachmentLoadOp ConvertLoadOp(ELoadOp loadOp)
	{
		switch (loadOp)
		{
		case ELoadOp::CLEAR:		return VK_ATTACHMENT_LOAD_OP_CLEAR;
		case ELoadOp::LOAD:			return VK_ATTACHMENT_LOAD_OP_LOAD;
		case ELoadOp::DONT_CARE:	return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		default:					return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		}
	}

	inline VkAttachmentStoreOp ConvertStoreOp(EStoreOp storeOp)
	{
		switch (storeOp)
		{
		case EStoreOp::STORE:		return VK_ATTACHMENT_STORE_OP_STORE;
		case EStoreOp::DONT_CARE:	return VK_ATTACHMENT_STORE_OP_DONT_CARE;
		default:					return VK_ATTACHMENT_STORE_OP_DONT_CARE;
		}
	}

	inline VkFilter ConvertFilter(EFilter filter)
	{
		switch (filter)
		{
		case EFilter::NEAREST:		return VK_FILTER_NEAREST;
		case EFilter::LINEAR:		return VK_FILTER_LINEAR;
		default:					return VK_FILTER_LINEAR;
		}
	}

	inline VkSamplerMipmapMode ConvertMipmapMode(EMipmapMode mipmapMode)
	{
		switch (mipmapMode)
		{
		case EMipmapMode::NEAREST:		return VK_SAMPLER_MIPMAP_MODE_NEAREST;
		case EMipmapMode::LINEAR:		return VK_SAMPLER_MIPMAP_MODE_LINEAR;
		default:						return VK_SAMPLER_MIPMAP_MODE_LINEAR;
		}
	}

	inline VkSamplerAddressMode ConvertAddressMode(EAddressMode addressMode)
	{
		switch (addressMode)
		{
		case EAddressMode::REPEAT:					return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case EAddressMode::MIRRORED_REPEAT:			return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		case EAddressMode::CLAMP_TO_EDGE:			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case EAddressMode::CLAMP_TO_BORDER:			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		case EAddressMode::MIRRORED_CLAMP_TO_EDGE:	return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
		default:									return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		}
	}

    inline VkDescriptorType ConvertDescriptorType(EDescriptorType descriptorType)
    {
        switch (descriptorType)
        {
        case EDescriptorType::DESCRIPTOR_SHADER_RESOURCE_TEXTURE:		    return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case EDescriptorType::DESCRIPTOR_SHADER_RESOURCE_COMBINED_SAMPLER:	return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case EDescriptorType::DESCRIPTOR_UNORDERED_ACCESS_TEXTURE:			return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case EDescriptorType::DESCRIPTOR_UNORDERED_ACCESS_BUFFER:			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case EDescriptorType::DESCRIPTOR_CONSTANT_BUFFER:			        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case EDescriptorType::DESCRIPTOR_ACCELERATION_STRUCTURE:			return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        case EDescriptorType::DESCRIPTOR_SAMPLER:                           return VK_DESCRIPTOR_TYPE_SAMPLER;
        case EDescriptorType::DESCRIPTOR_UNKNOWN:
        default: return VkDescriptorType(0);
        }
    }

	inline VkShaderStageFlagBits ConvertShaderStageFlag(FShaderStageFlags shaderType)
	{
		switch (shaderType)
		{
		case SHADER_STAGE_FLAG_MESH_SHADER:			return VK_SHADER_STAGE_MESH_BIT_NV;
        case SHADER_STAGE_FLAG_TASK_SHADER:			return VK_SHADER_STAGE_TASK_BIT_NV;
		case SHADER_STAGE_FLAG_VERTEX_SHADER:		return VK_SHADER_STAGE_VERTEX_BIT;
		case SHADER_STAGE_FLAG_GEOMETRY_SHADER:		return VK_SHADER_STAGE_GEOMETRY_BIT;
		case SHADER_STAGE_FLAG_HULL_SHADER:			return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		case SHADER_STAGE_FLAG_DOMAIN_SHADER:		return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		case SHADER_STAGE_FLAG_PIXEL_SHADER:		return VK_SHADER_STAGE_FRAGMENT_BIT;
		case SHADER_STAGE_FLAG_COMPUTE_SHADER:		return VK_SHADER_STAGE_COMPUTE_BIT;
		case SHADER_STAGE_FLAG_RAYGEN_SHADER:		return VK_SHADER_STAGE_RAYGEN_BIT_NV;
		case SHADER_STAGE_FLAG_INTERSECT_SHADER:	return VK_SHADER_STAGE_INTERSECTION_BIT_NV;
		case SHADER_STAGE_FLAG_ANY_HIT_SHADER:		return VK_SHADER_STAGE_ANY_HIT_BIT_NV;
		case SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER:	return VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;
		case SHADER_STAGE_FLAG_MISS_SHADER:			return VK_SHADER_STAGE_MISS_BIT_NV;
        default:                                    return VK_SHADER_STAGE_ALL;
		}
	}

    inline uint32 ConvertShaderStageMask(uint32 shaderTypeMask)
    {
        uint32 vkShaderTypeMask = 0;

        vkShaderTypeMask |= (shaderTypeMask & SHADER_STAGE_FLAG_MESH_SHADER)        ? VK_SHADER_STAGE_MESH_BIT_NV                   : 0;
        vkShaderTypeMask |= (shaderTypeMask & SHADER_STAGE_FLAG_TASK_SHADER)        ? VK_SHADER_STAGE_TASK_BIT_NV                   : 0;
        vkShaderTypeMask |= (shaderTypeMask & SHADER_STAGE_FLAG_VERTEX_SHADER)      ? VK_SHADER_STAGE_VERTEX_BIT                    : 0;
        vkShaderTypeMask |= (shaderTypeMask & SHADER_STAGE_FLAG_GEOMETRY_SHADER)    ? VK_SHADER_STAGE_GEOMETRY_BIT                  : 0;
        vkShaderTypeMask |= (shaderTypeMask & SHADER_STAGE_FLAG_HULL_SHADER)        ? VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT      : 0;
        vkShaderTypeMask |= (shaderTypeMask & SHADER_STAGE_FLAG_DOMAIN_SHADER)      ? VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT   : 0;
        vkShaderTypeMask |= (shaderTypeMask & SHADER_STAGE_FLAG_PIXEL_SHADER)       ? VK_SHADER_STAGE_FRAGMENT_BIT                  : 0;
        vkShaderTypeMask |= (shaderTypeMask & SHADER_STAGE_FLAG_COMPUTE_SHADER)     ? VK_SHADER_STAGE_COMPUTE_BIT                   : 0;
        vkShaderTypeMask |= (shaderTypeMask & SHADER_STAGE_FLAG_RAYGEN_SHADER)      ? VK_SHADER_STAGE_RAYGEN_BIT_NV                 : 0;
        vkShaderTypeMask |= (shaderTypeMask & SHADER_STAGE_FLAG_INTERSECT_SHADER)   ? VK_SHADER_STAGE_INTERSECTION_BIT_NV           : 0;
        vkShaderTypeMask |= (shaderTypeMask & SHADER_STAGE_FLAG_ANY_HIT_SHADER)     ? VK_SHADER_STAGE_ANY_HIT_BIT_NV                : 0;
        vkShaderTypeMask |= (shaderTypeMask & SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER) ? VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV            : 0;
        vkShaderTypeMask |= (shaderTypeMask & SHADER_STAGE_FLAG_MISS_SHADER)        ? VK_SHADER_STAGE_MISS_BIT_NV                   : 0;

        return vkShaderTypeMask;
    }

    inline VkQueryType ConvertQueryType(EQueryType queryType)
    {
        switch (queryType)
        {
        case EQueryType::QUERY_TYPE_TIMESTAMP:			    return VK_QUERY_TYPE_TIMESTAMP;
        case EQueryType::QUERY_TYPE_OCCLUSION:			    return VK_QUERY_TYPE_OCCLUSION;
        case EQueryType::QUERY_TYPE_PIPELINE_STATISTICS:	return VK_QUERY_TYPE_PIPELINE_STATISTICS;
        case EQueryType::NONE:
        default: return VkQueryType(0);
        }
    }

    inline VkQueryPipelineStatisticFlagBits ConvertQueryPipelineStatisticsFlag(FQueryPipelineStatisticsFlag pipelineStatisticsFlag)
    {
        switch (pipelineStatisticsFlag)
        {
        case QUERY_PIPELINE_STATISTICS_FLAG_INPUT_ASSEMBLY_VERTICES:                        return VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT;
        case QUERY_PIPELINE_STATISTICS_FLAG_INPUT_ASSEMBLY_PRIMITIVES:                      return VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT;
        case QUERY_PIPELINE_STATISTICS_FLAG_VERTEX_SHADER_INVOCATIONS:                      return VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT;
        case QUERY_PIPELINE_STATISTICS_FLAG_GEOMETRY_SHADER_INVOCATIONS:                    return VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT;
        case QUERY_PIPELINE_STATISTICS_FLAG_GEOMETRY_SHADER_PRIMITIVES:                     return VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT;
        case QUERY_PIPELINE_STATISTICS_FLAG_CLIPPING_INVOCATIONS:                           return VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT;
        case QUERY_PIPELINE_STATISTICS_FLAG_CLIPPING_PRIMITIVES:                            return VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT;
        case QUERY_PIPELINE_STATISTICS_FLAG_FRAGMENT_SHADER_INVOCATIONS:                    return VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT;
        case QUERY_PIPELINE_STATISTICS_FLAG_TESSELLATION_CONTROL_SHADER_PATCHES:            return VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT;
        case QUERY_PIPELINE_STATISTICS_FLAG_TESSELLATION_EVALUATION_SHADER_INVOCATIONS:     return VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT;
        case QUERY_PIPELINE_STATISTICS_FLAG_COMPUTE_SHADER_INVOCATIONS:                     return VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;
        case QUERY_PIPELINE_STATISTICS_FLAG_NONE:                                                   
        default: return VkQueryPipelineStatisticFlagBits(0);
        }
    }

    inline uint32 ConvertQueryPipelineStatisticsMask(uint32 pipelineStatisticsMask)
    {
        uint32 vkPipelineStatisticsMask = 0;

        vkPipelineStatisticsMask |= (pipelineStatisticsMask & QUERY_PIPELINE_STATISTICS_FLAG_INPUT_ASSEMBLY_VERTICES)                       ? VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT                       : 0;
        vkPipelineStatisticsMask |= (pipelineStatisticsMask & QUERY_PIPELINE_STATISTICS_FLAG_INPUT_ASSEMBLY_PRIMITIVES)                     ? VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT                     : 0;
        vkPipelineStatisticsMask |= (pipelineStatisticsMask & QUERY_PIPELINE_STATISTICS_FLAG_VERTEX_SHADER_INVOCATIONS)                     ? VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT                     : 0;
        vkPipelineStatisticsMask |= (pipelineStatisticsMask & QUERY_PIPELINE_STATISTICS_FLAG_GEOMETRY_SHADER_INVOCATIONS)                   ? VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT                   : 0;
        vkPipelineStatisticsMask |= (pipelineStatisticsMask & QUERY_PIPELINE_STATISTICS_FLAG_GEOMETRY_SHADER_PRIMITIVES)                    ? VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT                    : 0;
        vkPipelineStatisticsMask |= (pipelineStatisticsMask & QUERY_PIPELINE_STATISTICS_FLAG_CLIPPING_INVOCATIONS)                          ? VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT                          : 0;
        vkPipelineStatisticsMask |= (pipelineStatisticsMask & QUERY_PIPELINE_STATISTICS_FLAG_CLIPPING_PRIMITIVES)                           ? VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT                           : 0;
        vkPipelineStatisticsMask |= (pipelineStatisticsMask & QUERY_PIPELINE_STATISTICS_FLAG_FRAGMENT_SHADER_INVOCATIONS)                   ? VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT                   : 0;
        vkPipelineStatisticsMask |= (pipelineStatisticsMask & QUERY_PIPELINE_STATISTICS_FLAG_TESSELLATION_CONTROL_SHADER_PATCHES)           ? VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT           : 0;
        vkPipelineStatisticsMask |= (pipelineStatisticsMask & QUERY_PIPELINE_STATISTICS_FLAG_TESSELLATION_EVALUATION_SHADER_INVOCATIONS)    ? VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT    : 0;
        vkPipelineStatisticsMask |= (pipelineStatisticsMask & QUERY_PIPELINE_STATISTICS_FLAG_COMPUTE_SHADER_INVOCATIONS)                    ? VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT                    : 0;

        return vkPipelineStatisticsMask;
    }

    inline VkPipelineStageFlagBits ConvertPipelineStage(FPipelineStageFlags pipelineStage)
    {
        switch (pipelineStage)
        {
        case PIPELINE_STAGE_FLAG_TOP:                          return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        case PIPELINE_STAGE_FLAG_BOTTOM:                       return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        case PIPELINE_STAGE_FLAG_DRAW_INDIRECT:                return VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        case PIPELINE_STAGE_FLAG_VERTEX_INPUT:                 return VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
        case PIPELINE_STAGE_FLAG_VERTEX_SHADER:                return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
        case PIPELINE_STAGE_FLAG_HULL_SHADER:                  return VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
        case PIPELINE_STAGE_FLAG_DOMAIN_SHADER:                return VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
        case PIPELINE_STAGE_FLAG_GEOMETRY_SHADER:              return VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
        case PIPELINE_STAGE_FLAG_PIXEL_SHADER:                 return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        case PIPELINE_STAGE_FLAG_EARLY_FRAGMENT_TESTS:         return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        case PIPELINE_STAGE_FLAG_LATE_FRAGMENT_TESTS:          return VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        case PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT:         return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        case PIPELINE_STAGE_FLAG_COMPUTE_SHADER:               return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        case PIPELINE_STAGE_FLAG_COPY:                         return VK_PIPELINE_STAGE_TRANSFER_BIT;
        case PIPELINE_STAGE_FLAG_HOST:                         return VK_PIPELINE_STAGE_HOST_BIT;
        case PIPELINE_STAGE_FLAG_STREAM_OUTPUT:                return VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT;
        case PIPELINE_STAGE_FLAG_CONDITIONAL_RENDERING:        return VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT;
        case PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER:           return VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
        case PIPELINE_STAGE_FLAG_ACCELERATION_STRUCTURE_BUILD: return VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
        case PIPELINE_STAGE_FLAG_SHADING_RATE_TEXTURE:         return VK_PIPELINE_STAGE_SHADING_RATE_IMAGE_BIT_NV;
        case PIPELINE_STAGE_FLAG_TASK_SHADER:                  return VK_PIPELINE_STAGE_TASK_SHADER_BIT_NV;
        case PIPELINE_STAGE_FLAG_MESH_SHADER:                  return VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV;
        case PIPELINE_STAGE_FLAG_UNKNOWN:
        default: return VkPipelineStageFlagBits(0);
        }
    }

    inline uint32 ConvertPipelineStageMask(uint32 pipelineStageMask)
    {
        uint32 result = 0;
        if (pipelineStageMask & PIPELINE_STAGE_FLAG_TOP)
            result |= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        if (pipelineStageMask & PIPELINE_STAGE_FLAG_BOTTOM)
            result |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        if (pipelineStageMask & PIPELINE_STAGE_FLAG_DRAW_INDIRECT)
            result |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        if (pipelineStageMask & PIPELINE_STAGE_FLAG_VERTEX_INPUT)
            result |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
        if (pipelineStageMask & PIPELINE_STAGE_FLAG_VERTEX_SHADER)
            result |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
        if (pipelineStageMask & PIPELINE_STAGE_FLAG_HULL_SHADER)
            result |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
        if (pipelineStageMask & PIPELINE_STAGE_FLAG_DOMAIN_SHADER)
            result |= VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
        if (pipelineStageMask & PIPELINE_STAGE_FLAG_GEOMETRY_SHADER)
            result |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
        if (pipelineStageMask & PIPELINE_STAGE_FLAG_PIXEL_SHADER)
            result |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        if (pipelineStageMask & PIPELINE_STAGE_FLAG_EARLY_FRAGMENT_TESTS)
            result |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        if (pipelineStageMask & PIPELINE_STAGE_FLAG_LATE_FRAGMENT_TESTS)
            result |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        if (pipelineStageMask & PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT)
            result |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        if (pipelineStageMask & PIPELINE_STAGE_FLAG_COMPUTE_SHADER)
            result |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        if (pipelineStageMask & PIPELINE_STAGE_FLAG_COPY)
            result |= VK_PIPELINE_STAGE_TRANSFER_BIT;
        if (pipelineStageMask & PIPELINE_STAGE_FLAG_HOST)
            result |= VK_PIPELINE_STAGE_HOST_BIT;
        if (pipelineStageMask & PIPELINE_STAGE_FLAG_STREAM_OUTPUT)
            result |= VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT;
        if (pipelineStageMask & PIPELINE_STAGE_FLAG_CONDITIONAL_RENDERING)
            result |= VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT;
        if (pipelineStageMask & PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER)
            result |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
        if (pipelineStageMask & PIPELINE_STAGE_FLAG_ACCELERATION_STRUCTURE_BUILD)
            result |= VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
        if (pipelineStageMask & PIPELINE_STAGE_FLAG_SHADING_RATE_TEXTURE)
            result |= VK_PIPELINE_STAGE_SHADING_RATE_IMAGE_BIT_NV;
        if (pipelineStageMask & PIPELINE_STAGE_FLAG_TASK_SHADER)
            result |= VK_PIPELINE_STAGE_TASK_SHADER_BIT_NV;
        if (pipelineStageMask & PIPELINE_STAGE_FLAG_MESH_SHADER)
            result |= VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV;
        
        return result;
    }
	inline uint32 ConvertMemoryAccessFlags(uint32 accessFlags)
	{
        uint32 vkAccessFlags = 0;

		vkAccessFlags |= (accessFlags & MEMORY_ACCESS_FLAG_INDIRECT_COMMAND_READ)					? VK_ACCESS_INDIRECT_COMMAND_READ_BIT : 0;
		vkAccessFlags |= (accessFlags & MEMORY_ACCESS_FLAG_INDEX_READ)								? VK_ACCESS_INDEX_READ_BIT : 0;
		vkAccessFlags |= (accessFlags & MEMORY_ACCESS_FLAG_VERTEX_ATTRIBUTE_READ)					? VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT : 0;
		vkAccessFlags |= (accessFlags & MEMORY_ACCESS_FLAG_CONSTANT_BUFFER_READ)							? VK_ACCESS_UNIFORM_READ_BIT : 0;
		vkAccessFlags |= (accessFlags & MEMORY_ACCESS_FLAG_INPUT_ATTACHMENT_READ)					? VK_ACCESS_INPUT_ATTACHMENT_READ_BIT : 0;
		vkAccessFlags |= (accessFlags & MEMORY_ACCESS_FLAG_SHADER_READ)							    ? VK_ACCESS_SHADER_READ_BIT : 0;
		vkAccessFlags |= (accessFlags & MEMORY_ACCESS_FLAG_SHADER_WRITE)							? VK_ACCESS_SHADER_WRITE_BIT : 0;
		vkAccessFlags |= (accessFlags & MEMORY_ACCESS_FLAG_COLOR_ATTACHMENT_READ)					? VK_ACCESS_COLOR_ATTACHMENT_READ_BIT : 0;
		vkAccessFlags |= (accessFlags & MEMORY_ACCESS_FLAG_COLOR_ATTACHMENT_WRITE)					? VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT : 0;
		vkAccessFlags |= (accessFlags & MEMORY_ACCESS_FLAG_DEPTH_STENCIL_ATTACHMENT_READ)			? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT : 0;
		vkAccessFlags |= (accessFlags & MEMORY_ACCESS_FLAG_DEPTH_STENCIL_ATTACHMENT_WRITE)			? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : 0;
		vkAccessFlags |= (accessFlags & MEMORY_ACCESS_FLAG_TRANSFER_READ)							? VK_ACCESS_TRANSFER_READ_BIT : 0;
		vkAccessFlags |= (accessFlags & MEMORY_ACCESS_FLAG_TRANSFER_WRITE)							? VK_ACCESS_TRANSFER_WRITE_BIT : 0;
		vkAccessFlags |= (accessFlags & MEMORY_ACCESS_FLAG_HOST_READ)								? VK_ACCESS_HOST_READ_BIT : 0;
		vkAccessFlags |= (accessFlags & MEMORY_ACCESS_FLAG_HOST_WRITE)								? VK_ACCESS_HOST_WRITE_BIT : 0;
		vkAccessFlags |= (accessFlags & MEMORY_ACCESS_FLAG_MEMORY_READ)							    ? VK_ACCESS_MEMORY_READ_BIT : 0;
		vkAccessFlags |= (accessFlags & MEMORY_ACCESS_FLAG_MEMORY_WRITE)							? VK_ACCESS_MEMORY_WRITE_BIT : 0;
		vkAccessFlags |= (accessFlags & MEMORY_ACCESS_FLAG_TRANSFORM_FEEDBACK_WRITE)				? VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT : 0;
		vkAccessFlags |= (accessFlags & MEMORY_ACCESS_FLAG_TRANSFORM_FEEDBACK_COUNTER_READ)		    ? VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT : 0;
		vkAccessFlags |= (accessFlags & MEMORY_ACCESS_FLAG_TRANSFORM_FEEDBACK_COUNTER_WRITE)		? VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT : 0;
		vkAccessFlags |= (accessFlags & MEMORY_ACCESS_FLAG_CONDITIONAL_RENDERING_READ)				? VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT : 0;
		vkAccessFlags |= (accessFlags & MEMORY_ACCESS_FLAG_COLOR_ATTACHMENT_READ_NONCOHERENT)		? VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT : 0;
		vkAccessFlags |= (accessFlags & MEMORY_ACCESS_FLAG_ACCELERATION_STRUCTURE_READ)			    ? VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR : 0;
		vkAccessFlags |= (accessFlags & MEMORY_ACCESS_FLAG_ACCELERATION_STRUCTURE_WRITE)			? VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR : 0;
		vkAccessFlags |= (accessFlags & MEMORY_ACCESS_FLAG_SHADING_RATE_IMAGE_READ)				    ? VK_ACCESS_SHADING_RATE_IMAGE_READ_BIT_NV : 0;
		vkAccessFlags |= (accessFlags & MEMORY_ACCESS_FLAG_FRAGMENT_DENSITY_MAP_READ)				? VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT : 0;
		vkAccessFlags |= (accessFlags & MEMORY_ACCESS_FLAG_COMMAND_PREPROCESS_READ)				    ? VK_ACCESS_COMMAND_PREPROCESS_READ_BIT_NV : 0;
		vkAccessFlags |= (accessFlags & MEMORY_ACCESS_FLAG_COMMAND_PREPROCESS_WRITE)				? VK_ACCESS_COMMAND_PREPROCESS_WRITE_BIT_NV : 0;

		return vkAccessFlags;
	}

    inline VkImageLayout ConvertTextureState(ETextureState textureState)
    {
        switch (textureState)
        {
            case ETextureState::TEXTURE_STATE_GENERAL:								return VK_IMAGE_LAYOUT_GENERAL;
            case ETextureState::TEXTURE_STATE_RENDER_TARGET:						return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            case ETextureState::TEXTURE_STATE_DEPTH_STENCIL_ATTACHMENT:				return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            case ETextureState::TEXTURE_STATE_DEPTH_STENCIL_READ_ONLY:				return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            case ETextureState::TEXTURE_STATE_SHADER_READ_ONLY:						return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            case ETextureState::TEXTURE_STATE_COPY_SRC:								return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            case ETextureState::TEXTURE_STATE_COPY_DST:								return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            case ETextureState::TEXTURE_STATE_PREINITIALIZED:						return VK_IMAGE_LAYOUT_PREINITIALIZED;
            case ETextureState::TEXTURE_STATE_DEPTH_READ_ONLY_STENCIL_ATTACHMENT:	return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
            case ETextureState::TEXTURE_STATE_DEPTH_ATTACHMENT_STENCIL_READ_ONLY:	return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
            case ETextureState::TEXTURE_STATE_DEPTH_ATTACHMENT:						return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            case ETextureState::TEXTURE_STATE_DEPTH_READ_ONLY:						return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
            case ETextureState::TEXTURE_STATE_STENCIL_ATTACHMENT:					return VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
            case ETextureState::TEXTURE_STATE_STENCIL_READ_ONLY:					return VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL;
            case ETextureState::TEXTURE_STATE_PRESENT:								return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            case ETextureState::TEXTURE_STATE_SHADING_RATE:							return VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV;
            case ETextureState::TEXTURE_STATE_DONT_CARE:
            case ETextureState::TEXTURE_STATE_UNKNOWN:
            default: return VK_IMAGE_LAYOUT_UNDEFINED;
        }
    }

	inline uint32 ConvertColorComponentMask(uint32 mask)
	{
		uint32 vkColorComponentBits = 0;

		vkColorComponentBits |= (mask & COLOR_COMPONENT_FLAG_R) ? VK_COLOR_COMPONENT_R_BIT : 0;
		vkColorComponentBits |= (mask & COLOR_COMPONENT_FLAG_G) ? VK_COLOR_COMPONENT_G_BIT : 0;
		vkColorComponentBits |= (mask & COLOR_COMPONENT_FLAG_B) ? VK_COLOR_COMPONENT_B_BIT : 0;
		vkColorComponentBits |= (mask & COLOR_COMPONENT_FLAG_A) ? VK_COLOR_COMPONENT_A_BIT : 0;

		return vkColorComponentBits;
	}

	inline VkVertexInputRate ConvertVertexInputRate(EVertexInputRate inputRate)
	{
		switch (inputRate)
		{
		case EVertexInputRate::PER_VERTEX:							return VK_VERTEX_INPUT_RATE_VERTEX;
		case EVertexInputRate::PER_INSTANCE:						return VK_VERTEX_INPUT_RATE_INSTANCE;
		case EVertexInputRate::NONE:
		default: return VK_VERTEX_INPUT_RATE_MAX_ENUM;
		}
	}

	inline const char* VkFormatToString(VkFormat format)
	{
        switch (format)
        {
        case VK_FORMAT_R4G4_UNORM_PACK8:                            return "VK_FORMAT_R4G4_UNORM_PACK8";
        case VK_FORMAT_R4G4B4A4_UNORM_PACK16:                       return "VK_FORMAT_R4G4B4A4_UNORM_PACK16";
        case VK_FORMAT_B4G4R4A4_UNORM_PACK16:                       return "VK_FORMAT_B4G4R4A4_UNORM_PACK16";
        case VK_FORMAT_R5G6B5_UNORM_PACK16:                         return "VK_FORMAT_R5G6B5_UNORM_PACK16";
        case VK_FORMAT_B5G6R5_UNORM_PACK16:                         return "VK_FORMAT_B5G6R5_UNORM_PACK16";
        case VK_FORMAT_R5G5B5A1_UNORM_PACK16:                       return "VK_FORMAT_R5G5B5A1_UNORM_PACK16";
        case VK_FORMAT_B5G5R5A1_UNORM_PACK16:                       return "VK_FORMAT_B5G5R5A1_UNORM_PACK16";
        case VK_FORMAT_A1R5G5B5_UNORM_PACK16:                       return "VK_FORMAT_A1R5G5B5_UNORM_PACK16";
        case VK_FORMAT_R8_UNORM:                                    return "VK_FORMAT_R8_UNORM";
        case VK_FORMAT_R8_SNORM:                                    return "VK_FORMAT_R8_SNORM";
        case VK_FORMAT_R8_USCALED:                                  return "VK_FORMAT_R8_USCALED";
        case VK_FORMAT_R8_SSCALED:                                  return "VK_FORMAT_R8_SSCALED";
        case VK_FORMAT_R8_UINT:                                     return "VK_FORMAT_R8_UINT";
        case VK_FORMAT_R8_SINT:                                     return "VK_FORMAT_R8_SINT";
        case VK_FORMAT_R8_SRGB:                                     return "VK_FORMAT_R8_SRGB";
        case VK_FORMAT_R8G8_UNORM:                                  return "VK_FORMAT_R8G8_UNORM";
        case VK_FORMAT_R8G8_SNORM:                                  return "VK_FORMAT_R8G8_SNORM";
        case VK_FORMAT_R8G8_USCALED:                                return "VK_FORMAT_R8G8_USCALED";
        case VK_FORMAT_R8G8_SSCALED:                                return "VK_FORMAT_R8G8_SSCALED";
        case VK_FORMAT_R8G8_UINT:                                   return "VK_FORMAT_R8G8_UINT";
        case VK_FORMAT_R8G8_SINT:                                   return "VK_FORMAT_R8G8_SINT";
        case VK_FORMAT_R8G8_SRGB:                                   return "VK_FORMAT_R8G8_SRGB";
        case VK_FORMAT_R8G8B8_UNORM:                                return "VK_FORMAT_R8G8B8_UNORM";
        case VK_FORMAT_R8G8B8_SNORM:                                return "VK_FORMAT_R8G8B8_SNORM";
        case VK_FORMAT_R8G8B8_USCALED:                              return "VK_FORMAT_R8G8B8_USCALED";
        case VK_FORMAT_R8G8B8_SSCALED:                              return "VK_FORMAT_R8G8B8_SSCALED";
        case VK_FORMAT_R8G8B8_UINT:                                 return "VK_FORMAT_R8G8B8_UINT";
        case VK_FORMAT_R8G8B8_SINT:                                 return "VK_FORMAT_R8G8B8_SINT";
        case VK_FORMAT_R8G8B8_SRGB:                                 return "VK_FORMAT_R8G8B8_SRGB";
        case VK_FORMAT_B8G8R8_UNORM:                                return "VK_FORMAT_B8G8R8_UNORM";
        case VK_FORMAT_B8G8R8_SNORM:                                return "VK_FORMAT_B8G8R8_SNORM";
        case VK_FORMAT_B8G8R8_USCALED:                              return "VK_FORMAT_B8G8R8_USCALED";
        case VK_FORMAT_B8G8R8_SSCALED:                              return "VK_FORMAT_B8G8R8_SSCALED";
        case VK_FORMAT_B8G8R8_UINT:                                 return "VK_FORMAT_B8G8R8_UINT";
        case VK_FORMAT_B8G8R8_SINT:                                 return "VK_FORMAT_B8G8R8_SINT";
        case VK_FORMAT_B8G8R8_SRGB:                                 return "VK_FORMAT_B8G8R8_SRGB";
        case VK_FORMAT_R8G8B8A8_UNORM:                              return "VK_FORMAT_R8G8B8A8_UNORM";
        case VK_FORMAT_R8G8B8A8_SNORM:                              return "VK_FORMAT_R8G8B8A8_SNORM";
        case VK_FORMAT_R8G8B8A8_USCALED:                            return "VK_FORMAT_R8G8B8A8_USCALED";
        case VK_FORMAT_R8G8B8A8_SSCALED:                            return "VK_FORMAT_R8G8B8A8_SSCALED";
        case VK_FORMAT_R8G8B8A8_UINT:                               return "VK_FORMAT_R8G8B8A8_UINT";
        case VK_FORMAT_R8G8B8A8_SINT:                               return "VK_FORMAT_R8G8B8A8_SINT";
        case VK_FORMAT_R8G8B8A8_SRGB:                               return "VK_FORMAT_R8G8B8A8_SRGB";
        case VK_FORMAT_B8G8R8A8_UNORM:                              return "VK_FORMAT_B8G8R8A8_UNORM";
        case VK_FORMAT_B8G8R8A8_SNORM:                              return "VK_FORMAT_B8G8R8A8_SNORM";
        case VK_FORMAT_B8G8R8A8_USCALED:                            return "VK_FORMAT_B8G8R8A8_USCALED";
        case VK_FORMAT_B8G8R8A8_SSCALED:                            return "VK_FORMAT_B8G8R8A8_SSCALED";
        case VK_FORMAT_B8G8R8A8_UINT:                               return "VK_FORMAT_B8G8R8A8_UINT";
        case VK_FORMAT_B8G8R8A8_SINT:                               return "VK_FORMAT_B8G8R8A8_SINT";
        case VK_FORMAT_B8G8R8A8_SRGB:                               return "VK_FORMAT_B8G8R8A8_SRGB";
        case VK_FORMAT_A8B8G8R8_UNORM_PACK32:                       return "VK_FORMAT_A8B8G8R8_UNORM_PACK32";
        case VK_FORMAT_A8B8G8R8_SNORM_PACK32:                       return "VK_FORMAT_A8B8G8R8_SNORM_PACK32";
        case VK_FORMAT_A8B8G8R8_USCALED_PACK32:                     return "VK_FORMAT_A8B8G8R8_USCALED_PACK32";
        case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:                     return "VK_FORMAT_A8B8G8R8_SSCALED_PACK32";
        case VK_FORMAT_A8B8G8R8_UINT_PACK32:                        return "VK_FORMAT_A8B8G8R8_UINT_PACK32";
        case VK_FORMAT_A8B8G8R8_SINT_PACK32:                        return "VK_FORMAT_A8B8G8R8_SINT_PACK32";
        case VK_FORMAT_A8B8G8R8_SRGB_PACK32:                        return "VK_FORMAT_A8B8G8R8_SRGB_PACK32";
        case VK_FORMAT_A2R10G10B10_UNORM_PACK32:                    return "VK_FORMAT_A2R10G10B10_UNORM_PACK32";
        case VK_FORMAT_A2R10G10B10_SNORM_PACK32:                    return "VK_FORMAT_A2R10G10B10_SNORM_PACK32";
        case VK_FORMAT_A2R10G10B10_USCALED_PACK32:                  return "VK_FORMAT_A2R10G10B10_USCALED_PACK32";
        case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:                  return "VK_FORMAT_A2R10G10B10_SSCALED_PACK32";
        case VK_FORMAT_A2R10G10B10_UINT_PACK32:                     return "VK_FORMAT_A2R10G10B10_UINT_PACK32";
        case VK_FORMAT_A2R10G10B10_SINT_PACK32:                     return "VK_FORMAT_A2R10G10B10_SINT_PACK32";
        case VK_FORMAT_A2B10G10R10_UNORM_PACK32:                    return "VK_FORMAT_A2B10G10R10_UNORM_PACK32";
        case VK_FORMAT_A2B10G10R10_SNORM_PACK32:                    return "VK_FORMAT_A2B10G10R10_SNORM_PACK32";
        case VK_FORMAT_A2B10G10R10_USCALED_PACK32:                  return "VK_FORMAT_A2B10G10R10_USCALED_PACK32";
        case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:                  return "VK_FORMAT_A2B10G10R10_SSCALED_PACK32";
        case VK_FORMAT_A2B10G10R10_UINT_PACK32:                     return "VK_FORMAT_A2B10G10R10_UINT_PACK32";
        case VK_FORMAT_A2B10G10R10_SINT_PACK32:                     return "VK_FORMAT_A2B10G10R10_SINT_PACK32";
        case VK_FORMAT_R16_UNORM:                                   return "VK_FORMAT_R16_UNORM";
        case VK_FORMAT_R16_SNORM:                                   return "VK_FORMAT_R16_SNORM";
        case VK_FORMAT_R16_USCALED:                                 return "VK_FORMAT_R16_USCALED";
        case VK_FORMAT_R16_SSCALED:                                 return "VK_FORMAT_R16_SSCALED";
        case VK_FORMAT_R16_UINT:                                    return "VK_FORMAT_R16_UINT";
        case VK_FORMAT_R16_SINT:                                    return "VK_FORMAT_R16_SINT";
        case VK_FORMAT_R16_SFLOAT:                                  return "VK_FORMAT_R16_SFLOAT";
        case VK_FORMAT_R16G16_UNORM:                                return "VK_FORMAT_R16G16_UNORM";
        case VK_FORMAT_R16G16_SNORM:                                return "VK_FORMAT_R16G16_SNORM";
        case VK_FORMAT_R16G16_USCALED:                              return "VK_FORMAT_R16G16_USCALED";
        case VK_FORMAT_R16G16_SSCALED:                              return "VK_FORMAT_R16G16_SSCALED";
        case VK_FORMAT_R16G16_UINT:                                 return "VK_FORMAT_R16G16_UINT";
        case VK_FORMAT_R16G16_SINT:                                 return "VK_FORMAT_R16G16_SINT";
        case VK_FORMAT_R16G16_SFLOAT:                               return "VK_FORMAT_R16G16_SFLOAT";
        case VK_FORMAT_R16G16B16_UNORM:                             return "VK_FORMAT_R16G16B16_UNORM";
        case VK_FORMAT_R16G16B16_SNORM:                             return "VK_FORMAT_R16G16B16_SNORM";
        case VK_FORMAT_R16G16B16_USCALED:                           return "VK_FORMAT_R16G16B16_USCALED";
        case VK_FORMAT_R16G16B16_SSCALED:                           return "VK_FORMAT_R16G16B16_SSCALED";
        case VK_FORMAT_R16G16B16_UINT:                              return "VK_FORMAT_R16G16B16_UINT";
        case VK_FORMAT_R16G16B16_SINT:                              return "VK_FORMAT_R16G16B16_SINT";
        case VK_FORMAT_R16G16B16_SFLOAT:                            return "VK_FORMAT_R16G16B16_SFLOAT";
        case VK_FORMAT_R16G16B16A16_UNORM:                          return "VK_FORMAT_R16G16B16A16_UNORM";
        case VK_FORMAT_R16G16B16A16_SNORM:                          return "VK_FORMAT_R16G16B16A16_SNORM";
        case VK_FORMAT_R16G16B16A16_USCALED:                        return "VK_FORMAT_R16G16B16A16_USCALED";
        case VK_FORMAT_R16G16B16A16_SSCALED:                        return "VK_FORMAT_R16G16B16A16_SSCALED";
        case VK_FORMAT_R16G16B16A16_UINT:                           return "VK_FORMAT_R16G16B16A16_UINT";
        case VK_FORMAT_R16G16B16A16_SINT:                           return "VK_FORMAT_R16G16B16A16_SINT";
        case VK_FORMAT_R16G16B16A16_SFLOAT:                         return "VK_FORMAT_R16G16B16A16_SFLOAT";
        case VK_FORMAT_R32_UINT:                                    return "VK_FORMAT_R32_UINT";
        case VK_FORMAT_R32_SINT:                                    return "VK_FORMAT_R32_SINT";
        case VK_FORMAT_R32_SFLOAT:                                  return "VK_FORMAT_R32_SFLOAT";
        case VK_FORMAT_R32G32_UINT:                                 return "VK_FORMAT_R32G32_UINT";
        case VK_FORMAT_R32G32_SINT:                                 return "VK_FORMAT_R32G32_SINT";
        case VK_FORMAT_R32G32_SFLOAT:                               return "VK_FORMAT_R32G32_SFLOAT";
        case VK_FORMAT_R32G32B32_UINT:                              return "VK_FORMAT_R32G32B32_UINT";
        case VK_FORMAT_R32G32B32_SINT:                              return "VK_FORMAT_R32G32B32_SINT";
        case VK_FORMAT_R32G32B32_SFLOAT:                            return "VK_FORMAT_R32G32B32_SFLOAT";
        case VK_FORMAT_R32G32B32A32_UINT:                           return "VK_FORMAT_R32G32B32A32_UINT";
        case VK_FORMAT_R32G32B32A32_SINT:                           return "VK_FORMAT_R32G32B32A32_SINT";
        case VK_FORMAT_R32G32B32A32_SFLOAT:                         return "VK_FORMAT_R32G32B32A32_SFLOAT";
        case VK_FORMAT_R64_UINT:                                    return "VK_FORMAT_R64_UINT";
        case VK_FORMAT_R64_SINT:                                    return "VK_FORMAT_R64_SINT";
        case VK_FORMAT_R64_SFLOAT:                                  return "VK_FORMAT_R64_SFLOAT";
        case VK_FORMAT_R64G64_UINT:                                 return "VK_FORMAT_R64G64_UINT";
        case VK_FORMAT_R64G64_SINT:                                 return "VK_FORMAT_R64G64_SINT";
        case VK_FORMAT_R64G64_SFLOAT:                               return "VK_FORMAT_R64G64_SFLOAT";
        case VK_FORMAT_R64G64B64_UINT:                              return "VK_FORMAT_R64G64B64_UINT";
        case VK_FORMAT_R64G64B64_SINT:                              return "VK_FORMAT_R64G64B64_SINT";
        case VK_FORMAT_R64G64B64_SFLOAT:                            return "VK_FORMAT_R64G64B64_SFLOAT";
        case VK_FORMAT_R64G64B64A64_UINT:                           return "VK_FORMAT_R64G64B64A64_UINT";
        case VK_FORMAT_R64G64B64A64_SINT:                           return "VK_FORMAT_R64G64B64A64_SINT";
        case VK_FORMAT_R64G64B64A64_SFLOAT:                         return "VK_FORMAT_R64G64B64A64_SFLOAT";
        case VK_FORMAT_B10G11R11_UFLOAT_PACK32:                     return "VK_FORMAT_B10G11R11_UFLOAT_PACK32";
        case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:                      return "VK_FORMAT_E5B9G9R9_UFLOAT_PACK32";
        case VK_FORMAT_D16_UNORM:                                   return "VK_FORMAT_D16_UNORM";
        case VK_FORMAT_X8_D24_UNORM_PACK32:                         return "VK_FORMAT_X8_D24_UNORM_PACK32";
        case VK_FORMAT_D32_SFLOAT:                                  return "VK_FORMAT_D32_SFLOAT";
        case VK_FORMAT_S8_UINT:                                     return "VK_FORMAT_S8_UINT";
        case VK_FORMAT_D16_UNORM_S8_UINT:                           return "VK_FORMAT_D16_UNORM_S8_UINT";
        case VK_FORMAT_D24_UNORM_S8_UINT:                           return "VK_FORMAT_D24_UNORM_S8_UINT";
        case VK_FORMAT_D32_SFLOAT_S8_UINT:                          return "VK_FORMAT_D32_SFLOAT_S8_UINT";
        case VK_FORMAT_BC1_RGB_UNORM_BLOCK:                         return "VK_FORMAT_BC1_RGB_UNORM_BLOCK";
        case VK_FORMAT_BC1_RGB_SRGB_BLOCK:                          return "VK_FORMAT_BC1_RGB_SRGB_BLOCK";
        case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:                        return "VK_FORMAT_BC1_RGBA_UNORM_BLOCK";
        case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:                         return "VK_FORMAT_BC1_RGBA_SRGB_BLOCK";
        case VK_FORMAT_BC2_UNORM_BLOCK:                             return "VK_FORMAT_BC2_UNORM_BLOCK";
        case VK_FORMAT_BC2_SRGB_BLOCK:                              return "VK_FORMAT_BC2_SRGB_BLOCK";
        case VK_FORMAT_BC3_UNORM_BLOCK:                             return "VK_FORMAT_BC3_UNORM_BLOCK";
        case VK_FORMAT_BC3_SRGB_BLOCK:                              return "VK_FORMAT_BC3_SRGB_BLOCK";
        case VK_FORMAT_BC4_UNORM_BLOCK:                             return "VK_FORMAT_BC4_UNORM_BLOCK";
        case VK_FORMAT_BC4_SNORM_BLOCK:                             return "VK_FORMAT_BC4_SNORM_BLOCK";
        case VK_FORMAT_BC5_UNORM_BLOCK:                             return "VK_FORMAT_BC5_UNORM_BLOCK";
        case VK_FORMAT_BC5_SNORM_BLOCK:                             return "VK_FORMAT_BC5_SNORM_BLOCK";
        case VK_FORMAT_BC6H_UFLOAT_BLOCK:                           return "VK_FORMAT_BC6H_UFLOAT_BLOCK";
        case VK_FORMAT_BC6H_SFLOAT_BLOCK:                           return "VK_FORMAT_BC6H_SFLOAT_BLOCK";
        case VK_FORMAT_BC7_UNORM_BLOCK:                             return "VK_FORMAT_BC7_UNORM_BLOCK";
        case VK_FORMAT_BC7_SRGB_BLOCK:                              return "VK_FORMAT_BC7_SRGB_BLOCK";
        case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:                     return "VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK";
        case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:                      return "VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK";
        case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:                   return "VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK";
        case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:                    return "VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK";
        case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:                   return "VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK";
        case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:                    return "VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK";
        case VK_FORMAT_EAC_R11_UNORM_BLOCK:                         return "VK_FORMAT_EAC_R11_UNORM_BLOCK";
        case VK_FORMAT_EAC_R11_SNORM_BLOCK:                         return "VK_FORMAT_EAC_R11_SNORM_BLOCK";
        case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:                      return "VK_FORMAT_EAC_R11G11_UNORM_BLOCK";
        case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:                      return "VK_FORMAT_EAC_R11G11_SNORM_BLOCK";
        case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:                        return "VK_FORMAT_ASTC_4x4_UNORM_BLOCK";
        case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:                         return "VK_FORMAT_ASTC_4x4_SRGB_BLOCK";
        case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:                        return "VK_FORMAT_ASTC_5x4_UNORM_BLOCK";
        case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:                         return "VK_FORMAT_ASTC_5x4_SRGB_BLOCK";
        case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:                        return "VK_FORMAT_ASTC_5x5_UNORM_BLOCK";
        case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:                         return "VK_FORMAT_ASTC_5x5_SRGB_BLOCK";
        case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:                        return "VK_FORMAT_ASTC_6x5_UNORM_BLOCK";
        case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:                         return "VK_FORMAT_ASTC_6x5_SRGB_BLOCK";
        case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:                        return "VK_FORMAT_ASTC_6x6_UNORM_BLOCK";
        case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:                         return "VK_FORMAT_ASTC_6x6_SRGB_BLOCK";
        case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:                        return "VK_FORMAT_ASTC_8x5_UNORM_BLOCK";
        case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:                         return "VK_FORMAT_ASTC_8x5_SRGB_BLOCK";
        case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:                        return "VK_FORMAT_ASTC_8x6_UNORM_BLOCK";
        case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:                         return "VK_FORMAT_ASTC_8x6_SRGB_BLOCK";
        case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:                        return "VK_FORMAT_ASTC_8x8_UNORM_BLOCK";
        case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:                         return "VK_FORMAT_ASTC_8x8_SRGB_BLOCK";
        case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:                       return "VK_FORMAT_ASTC_10x5_UNORM_BLOCK";
        case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:                        return "VK_FORMAT_ASTC_10x5_SRGB_BLOCK";
        case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:                       return "VK_FORMAT_ASTC_10x6_UNORM_BLOCK";
        case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:                        return "VK_FORMAT_ASTC_10x6_SRGB_BLOCK";
        case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:                       return "VK_FORMAT_ASTC_10x8_UNORM_BLOCK";
        case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:                        return "VK_FORMAT_ASTC_10x8_SRGB_BLOCK";
        case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:                      return "VK_FORMAT_ASTC_10x10_UNORM_BLOCK";
        case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:                       return "VK_FORMAT_ASTC_10x10_SRGB_BLOCK";
        case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:                      return "VK_FORMAT_ASTC_12x10_UNORM_BLOCK";
        case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:                       return "VK_FORMAT_ASTC_12x10_SRGB_BLOCK";
        case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:                      return "VK_FORMAT_ASTC_12x12_UNORM_BLOCK";
        case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:                       return "VK_FORMAT_ASTC_12x12_SRGB_BLOCK";
        case VK_FORMAT_G8B8G8R8_422_UNORM:                          return "VK_FORMAT_G8B8G8R8_422_UNORM";
        case VK_FORMAT_B8G8R8G8_422_UNORM:                          return "VK_FORMAT_B8G8R8G8_422_UNORM";
        case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:                   return "VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM";
        case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:                    return "VK_FORMAT_G8_B8R8_2PLANE_420_UNORM";
        case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:                   return "VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM";
        case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:                    return "VK_FORMAT_G8_B8R8_2PLANE_422_UNORM";
        case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:                   return "VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM";
        case VK_FORMAT_R10X6_UNORM_PACK16:                          return "VK_FORMAT_R10X6_UNORM_PACK16";
        case VK_FORMAT_R10X6G10X6_UNORM_2PACK16:                    return "VK_FORMAT_R10X6G10X6_UNORM_2PACK16";
        case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16:          return "VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16";
        case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16:      return "VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16";
        case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16:      return "VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16";
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:  return "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16";
        case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:   return "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16";
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:  return "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16";
        case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:   return "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16";
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:  return "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16";
        case VK_FORMAT_R12X4_UNORM_PACK16:                          return "VK_FORMAT_R12X4_UNORM_PACK16";
        case VK_FORMAT_R12X4G12X4_UNORM_2PACK16:                    return "VK_FORMAT_R12X4G12X4_UNORM_2PACK16";
        case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16:          return "VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16";
        case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16:      return "VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16";
        case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16:      return "VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16";
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:  return "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16";
        case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:   return "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16";
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:  return "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16";
        case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:   return "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16";
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:  return "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16";
        case VK_FORMAT_G16B16G16R16_422_UNORM:                      return "VK_FORMAT_G16B16G16R16_422_UNORM";
        case VK_FORMAT_B16G16R16G16_422_UNORM:                      return "VK_FORMAT_B16G16R16G16_422_UNORM";
        case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:                return "VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM";
        case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM:                 return "VK_FORMAT_G16_B16R16_2PLANE_420_UNORM";
        case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:                return "VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM";
        case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM:                 return "VK_FORMAT_G16_B16R16_2PLANE_422_UNORM";
        case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM:                return "VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM";
        case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:                 return "VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG";
        case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:                 return "VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG";
        case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:                 return "VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG";
        case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:                 return "VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG";
        case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:                  return "VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG";
        case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:                  return "VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG";
        case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:                  return "VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG";
        case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:                  return "VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG";
        case VK_FORMAT_UNDEFINED:
        default: return "VK_FORMAT_UNDEFINED";
        }
	}

    inline const char* VkResultToString(VkResult result)
    {
        switch (result)
        {
        case VK_SUCCESS:											return "VK_SUCCESS";
        case VK_NOT_READY:											return "VK_NOT_READY";
        case VK_TIMEOUT:											return "VK_TIMEOUT";
        case VK_EVENT_SET:											return "VK_EVENT_SET";
        case VK_EVENT_RESET:										return "VK_EVENT_RESET";
        case VK_INCOMPLETE:											return "VK_INCOMPLETE";
        case VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT:				return "VK_PIPELINE_COMPILE_REQUIRED_EXT";
        case VK_ERROR_OUT_OF_HOST_MEMORY:							return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:							return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED:						return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST:									return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED:							return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT:							return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT:						return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT:							return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER:							return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS:								return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:							return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL:								return "VK_ERROR_FRAGMENTED_POOL";
        case VK_ERROR_OUT_OF_POOL_MEMORY:							return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:						return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case VK_ERROR_SURFACE_LOST_KHR:								return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:						return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_SUBOPTIMAL_KHR:										return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR:								return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:						return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_VALIDATION_FAILED_EXT:						return "VK_ERROR_VALIDATION_FAILED_EXT";
        case VK_ERROR_INVALID_SHADER_NV:							return "VK_ERROR_INVALID_SHADER_NV";
        case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
        case VK_ERROR_FRAGMENTATION:							    return "VK_ERROR_FRAGMENTATION_EXT";
        case VK_ERROR_NOT_PERMITTED_EXT:							return "VK_ERROR_NOT_PERMITTED_EXT";
        case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT:					return "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT";
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:			return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
        case VK_ERROR_INCOMPATIBLE_VERSION_KHR:                     return "VK_ERROR_INCOMPATIBLE_VERSION_KHR";
        case VK_THREAD_IDLE_KHR:                                    return "VK_THREAD_IDLE_KHR";
        case VK_THREAD_DONE_KHR:                                    return "VK_THREAD_DONE_KHR";
        case VK_OPERATION_DEFERRED_KHR:                             return "VK_OPERATION_DEFERRED_KHR";
        case VK_OPERATION_NOT_DEFERRED_KHR:                         return "VK_OPERATION_NOT_DEFERRED_KHR";
        case VK_ERROR_UNKNOWN:
        default: return "VK_ERROR_UNKNOWN";
        }
    }

    inline const char* VkPresentatModeToString(VkPresentModeKHR mode)
    {
        switch (mode)
        {
        case VK_PRESENT_MODE_IMMEDIATE_KHR:                 return "VK_PRESENT_MODE_IMMEDIATE_KHR";
        case VK_PRESENT_MODE_MAILBOX_KHR:                   return "VK_PRESENT_MODE_MAILBOX_KHR";
        case VK_PRESENT_MODE_FIFO_KHR:                      return "VK_PRESENT_MODE_FIFO_KHR";
        case VK_PRESENT_MODE_FIFO_RELAXED_KHR:              return "VK_PRESENT_MODE_FIFO_RELAXED_KHR";
        case VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR:     return "VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR";
        case VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR: return "VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR";
        default: return "Unknown VkPresentModeKHR";
        }
    }

    inline const char* GetVkErrorString(VkResult result)
    {
        switch (result)
        {
        case VK_SUCCESS:											return "Command successfully completed";
        case VK_NOT_READY:											return "A fence or query has not yet completed";
        case VK_TIMEOUT:											return "A wait operation has not completed in the specified time";
        case VK_EVENT_SET:											return "An event is signaled";
        case VK_EVENT_RESET:										return "An event is unsignaled";
        case VK_INCOMPLETE:											return "A return array was too small for the result";
        case VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT:				return "A requested pipeline creation would have required compilation, but the application requested compilation to not be performed.";
        case VK_ERROR_OUT_OF_HOST_MEMORY:							return "A host memory allocation has failed.";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:							return "A device memory allocation has failed.";
        case VK_ERROR_INITIALIZATION_FAILED:						return "Initialization of an object could not be completed for implementation-specific reasons.";
        case VK_ERROR_DEVICE_LOST:									return "The logical or physical device has been lost.";
        case VK_ERROR_MEMORY_MAP_FAILED:							return "Mapping of a memory object has failed.";
        case VK_ERROR_LAYER_NOT_PRESENT:							return "A requested layer is not present or could not be loaded.";
        case VK_ERROR_EXTENSION_NOT_PRESENT:						return "A requested extension is not supported.";
        case VK_ERROR_FEATURE_NOT_PRESENT:							return "A requested feature is not supported.";
        case VK_ERROR_INCOMPATIBLE_DRIVER:							return "The requested version of Vulkan is not supported by the driver or is otherwise incompatible for implementation-specific reasons.";
        case VK_ERROR_TOO_MANY_OBJECTS:								return "Too many objects of the type have already been created.";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:							return "A requested format is not supported on this device.";
        case VK_ERROR_FRAGMENTED_POOL:								return "A descriptor pool creation has failed due to fragmentation.";
        case VK_ERROR_OUT_OF_POOL_MEMORY:							return "A pool memory allocation has failed. ";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:						return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case VK_ERROR_SURFACE_LOST_KHR:								return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:						return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_SUBOPTIMAL_KHR:										return "A swapchain no longer matches the surface properties exactly, but can still be used to present to the surface successfully.";
        case VK_ERROR_OUT_OF_DATE_KHR:								return "A surface has changed in such a way that it is no longer compatible with the swapchain, and further presentation requests using the swapchain will fail. Applications must query the new surface properties and recreate their swapchain if they wish to continue presenting to the surface.";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:						return "The display used by a swapchain does not use the same presentable image layout, or is incompatible in a way that prevents sharing an image.";
        case VK_ERROR_VALIDATION_FAILED_EXT:						return "VK_ERROR_VALIDATION_FAILED_EXT";
        case VK_ERROR_INVALID_SHADER_NV:							return "One or more shaders failed to compile or link. ";
        case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
        case VK_ERROR_FRAGMENTATION:							    return "A descriptor pool creation has failed due to fragmentation.";
        case VK_ERROR_NOT_PERMITTED_EXT:							return "VK_ERROR_NOT_PERMITTED_EXT";
        case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT:					return "A buffer creation failed because the requested address is not available.";
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:			return "An operation on a swapchain created with VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT failed as it did not have exlusive full-screen access. This may occur due to implementation-dependent reasons, outside of the application�s control.";
        case VK_ERROR_INCOMPATIBLE_VERSION_KHR:                     return "VK_ERROR_INCOMPATIBLE_VERSION_KHR";
        case VK_THREAD_IDLE_KHR:                                    return "A deferred operation is not complete but there is currently no work for this thread to do at the time of this call.";
        case VK_THREAD_DONE_KHR:                                    return "A deferred operation is not complete but there is no work remaining to assign to additional threads.";
        case VK_OPERATION_DEFERRED_KHR:                             return "A deferred operation was requested and at least some of the work was deferred.";
        case VK_OPERATION_NOT_DEFERRED_KHR:                         return "A deferred operation was requested and no operations were deferred.";
        case VK_ERROR_UNKNOWN:
        default: return "An unknown error has occurred; either the application has provided invalid input, or an implementation failure has occurred.";
        }
    }
}
