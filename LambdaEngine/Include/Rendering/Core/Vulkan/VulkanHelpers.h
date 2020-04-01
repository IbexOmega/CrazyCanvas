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

	inline bool CreateShadeModule(VkDevice device, VkShaderModule shaderModule, const char* pSource, uint32 sourceSize)
	{
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.pNext    = nullptr;
		createInfo.flags    = 0;
		createInfo.codeSize = sourceSize;
		createInfo.pCode    = reinterpret_cast<const uint32_t*>(pSource);

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
        {            
            return false;
        }

		shaderStageInfo = {};

		shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfo.pNext = nullptr;
		shaderStageInfo.flags = 0;
		shaderStageInfo.stage = ConvertShaderType(shaderDesc.Type);
		shaderStageInfo.module = shaderModule;
		shaderStageInfo.pName = shaderDesc.pEntryPoint;
		shaderStageInfo.pSpecializationInfo = &specializationInfo;
        
        return true;
	}

	inline const char* VkFormatString(VkFormat format)
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
}
