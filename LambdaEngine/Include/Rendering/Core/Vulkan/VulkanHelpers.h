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
        case EFormat::R8G8B8A8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
        case EFormat::B8G8R8A8_UNORM: return VK_FORMAT_B8G8R8A8_UNORM;
        default: return VK_FORMAT_UNDEFINED;
        }
    }

	inline bool CreateShadeModule(VkDevice device, VkShaderModule shaderModule, const char* pSource, uint32 sourceSize)
	{
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.codeSize = sourceSize;
		createInfo.pCode = reinterpret_cast<const uint32_t*>(pSource);

		VkResult result = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
		return result == VK_SUCCESS;
	}

	inline VkShaderStageFlagBits ConvertShaderType(EShaderType shaderType)
	{
		switch (shaderType)
		{
		case EShaderType::MESH_SHADER:			return VK_SHADER_STAGE_MESH_BIT_NV;
		case EShaderType::VERTEX_SHADER:		return VK_SHADER_STAGE_VERTEX_BIT;
		case EShaderType::GEOMETRY_SHADER:		return VK_SHADER_STAGE_GEOMETRY_BIT;
		case EShaderType::HULL_SHADER:			return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		case EShaderType::DOMAIN_SHADER:		return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		case EShaderType::PIXEL_SHADER:			return VK_SHADER_STAGE_FRAGMENT_BIT;
		case EShaderType::COMPUTE_SHADER:		return VK_SHADER_STAGE_COMPUTE_BIT;
		case EShaderType::RAYGEN_SHADER:		return VK_SHADER_STAGE_RAYGEN_BIT_NV;
		case EShaderType::INTERSECT_SHADER:		return VK_SHADER_STAGE_INTERSECTION_BIT_NV;
		case EShaderType::ANY_HIT_SHADER:		return VK_SHADER_STAGE_ANY_HIT_BIT_NV;
		case EShaderType::CLOSEST_HIT_SHADER:	return VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;
		case EShaderType::MISS_SHADER:			return VK_SHADER_STAGE_MISS_BIT_NV;
		}

		return VK_SHADER_STAGE_ALL;
	}
	
	inline void CreateSpecializationInfo(
		VkSpecializationInfo& specializationInfo,
		std::vector<VkSpecializationMapEntry>& specializationEntries,
		const ShaderDesc& shaderDesc)
	{
		specializationInfo = {};

		for (uint32 i = 0; i < shaderDesc.Constants.size(); i++)
		{
			VkSpecializationMapEntry specializationEntry = {};
			specializationEntry.constantID = i;
			specializationEntry.offset = i * sizeof(ShaderConstant);
			specializationEntry.size = sizeof(ShaderConstant);
			specializationEntries.push_back(specializationEntry);
		}

		specializationInfo.mapEntryCount = (uint32)specializationEntries.size();
		specializationInfo.pMapEntries = specializationEntries.data();
		specializationInfo.dataSize = shaderDesc.Constants.size() * sizeof(ShaderConstant);
		specializationInfo.pData = shaderDesc.Constants.data();
	}

	inline bool CreateShaderStageInfo(VkDevice device, VkPipelineShaderStageCreateInfo& shaderStageInfo, const ShaderDesc& shaderDesc, const VkSpecializationInfo& specializationInfo)
	{
		VkShaderModule shaderModule = VK_NULL_HANDLE;

		if (!CreateShadeModule(device, shaderModule, shaderDesc.pSource, shaderDesc.SourceSize))
			return false;

		shaderStageInfo = {};

		shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfo.pNext = nullptr;
		shaderStageInfo.flags = 0;
		shaderStageInfo.stage = ConvertShaderType(shaderDesc.Type);
		shaderStageInfo.module = shaderModule;
		shaderStageInfo.pName = shaderDesc.pEntryPoint;
		shaderStageInfo.pSpecializationInfo = &specializationInfo;
	}
}