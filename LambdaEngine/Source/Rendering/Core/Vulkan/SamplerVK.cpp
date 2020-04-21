#include "Rendering/Core/Vulkan/SamplerVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	SamplerVK::SamplerVK(const GraphicsDeviceVK* pDevice) :
		TDeviceChild(pDevice),
		m_Sampler(VK_NULL_HANDLE)
	{
	}

	SamplerVK::~SamplerVK()
	{
		if (m_Sampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(m_pDevice->Device, m_Sampler, nullptr);
			m_Sampler = VK_NULL_HANDLE;
		}
	}

	bool SamplerVK::Init(const SamplerDesc& desc)
	{
		VkSamplerCreateInfo samplerCreateInfo = {};
		samplerCreateInfo.sType					    = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.pNext					    = nullptr;
		samplerCreateInfo.flags					    = 0;
		samplerCreateInfo.minFilter				    = ConvertFilter(desc.MinFilter);
		samplerCreateInfo.magFilter				    = ConvertFilter(desc.MagFilter);
		samplerCreateInfo.mipmapMode			    = ConvertMipmapMode(desc.MipmapMode);
		samplerCreateInfo.addressModeU			    = ConvertAddressMode(desc.AddressModeU);
		samplerCreateInfo.addressModeV			    = ConvertAddressMode(desc.AddressModeV);
		samplerCreateInfo.addressModeW			    = ConvertAddressMode(desc.AddressModeW);
		samplerCreateInfo.mipLodBias			    = desc.MipLODBias;
		samplerCreateInfo.anisotropyEnable		    = desc.AnisotropyEnabled ? VK_TRUE : VK_FALSE;
		samplerCreateInfo.maxAnisotropy			    = desc.MaxAnisotropy;
		samplerCreateInfo.compareEnable			    = VK_FALSE;
		samplerCreateInfo.compareOp				    = VK_COMPARE_OP_ALWAYS;
		samplerCreateInfo.minLod				    = desc.MinLOD;
		samplerCreateInfo.maxLod				    = desc.MaxLOD;
		samplerCreateInfo.borderColor			    = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		samplerCreateInfo.unnormalizedCoordinates   = VK_FALSE;

		VkResult result = vkCreateSampler(m_pDevice->Device, &samplerCreateInfo, nullptr, &m_Sampler);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "[SamplerVK]: vkCreateSampler failed");
			return false;
		}
		else
		{
			m_Desc = desc;
			SetName(desc.pName);
			
			return true;
		}
	}

	void SamplerVK::SetName(const char* pName)
	{
		if (pName)
		{
			TDeviceChild::SetName(pName);
			m_pDevice->SetVulkanObjectName(pName, (uint64)m_Sampler, VK_OBJECT_TYPE_SAMPLER);

			m_Desc.pName = m_pDebugName;
		}
	}
}
