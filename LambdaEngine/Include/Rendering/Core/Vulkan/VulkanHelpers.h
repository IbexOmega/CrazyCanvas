#pragma once
#include "Rendering/Core/API/GraphicsTypes.h"

#include "Vulkan.h"

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
        case EFormat::R8G8B8A8_UNORM:   return VK_FORMAT_R8G8B8A8_UNORM;
        case EFormat::B8G8R8A8_UNORM:   return VK_FORMAT_B8G8R8A8_UNORM;
        default:                        return VK_FORMAT_UNDEFINED;
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
}