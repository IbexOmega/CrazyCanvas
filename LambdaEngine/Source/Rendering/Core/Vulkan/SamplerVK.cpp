#include "Rendering/Core/Vulkan/SamplerVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	SamplerVK::SamplerVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice)
		, m_Sampler(VK_NULL_HANDLE)
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

	bool SamplerVK::Init(const SamplerDesc* pDesc)
	{
		VkSamplerCreateInfo samplerCreateInfo = {};
		samplerCreateInfo.sType						= VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.pNext						= nullptr;
		samplerCreateInfo.flags						= 0;
		samplerCreateInfo.minFilter					= ConvertFilter(pDesc->MinFilter);
		samplerCreateInfo.magFilter					= ConvertFilter(pDesc->MagFilter);
		samplerCreateInfo.mipmapMode				= ConvertMipmapMode(pDesc->MipmapMode);
		samplerCreateInfo.addressModeU				= ConvertAddressMode(pDesc->AddressModeU);
		samplerCreateInfo.addressModeV				= ConvertAddressMode(pDesc->AddressModeV);
		samplerCreateInfo.addressModeW				= ConvertAddressMode(pDesc->AddressModeW);
		samplerCreateInfo.mipLodBias				= pDesc->MipLODBias;
		samplerCreateInfo.anisotropyEnable			= pDesc->AnisotropyEnabled ? VK_TRUE : VK_FALSE;
		samplerCreateInfo.maxAnisotropy				= pDesc->MaxAnisotropy;
		samplerCreateInfo.compareEnable				= VK_FALSE;
		samplerCreateInfo.compareOp					= VK_COMPARE_OP_ALWAYS;
		samplerCreateInfo.minLod					= pDesc->MinLOD;
		samplerCreateInfo.maxLod					= pDesc->MaxLOD;
		samplerCreateInfo.borderColor				= ConvertBorderColor(pDesc->borderColor);
		samplerCreateInfo.unnormalizedCoordinates	= VK_FALSE;

		VkResult result = vkCreateSampler(m_pDevice->Device, &samplerCreateInfo, nullptr, &m_Sampler);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "vkCreateSampler failed");
			return false;
		}
		else
		{
			m_Desc = *pDesc;
			SetName(pDesc->DebugName);

			LOG_DEBUG("Created sampler");

			return true;
		}
	}

	void SamplerVK::SetName(const String& debugName)
	{
		m_pDevice->SetVulkanObjectName(debugName, reinterpret_cast<uint64>(m_Sampler), VK_OBJECT_TYPE_SAMPLER);
		m_Desc.DebugName = debugName;
	}
}
