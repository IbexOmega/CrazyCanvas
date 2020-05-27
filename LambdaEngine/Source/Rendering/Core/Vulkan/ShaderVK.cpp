#include "Rendering/Core/Vulkan/ShaderVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"

#include <cstdio>

#include "Log/Log.h"

namespace LambdaEngine
{
	ShaderVK::ShaderVK(const GraphicsDeviceVK* pDevice) 
		: TDeviceChild(pDevice),
		m_Desc()
	{
	}

	ShaderVK::~ShaderVK()
	{
		if (m_Module != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(m_pDevice->Device, m_Module, nullptr);
			m_Module = VK_NULL_HANDLE;
		}
	}

	bool ShaderVK::Init(const ShaderDesc* pDesc)
	{
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType	= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.pNext	= nullptr;
		createInfo.flags	= 0;
		createInfo.codeSize	= pDesc->SourceSize;
		createInfo.pCode	= reinterpret_cast<const uint32_t*>(pDesc->pSource);

		VkResult result = vkCreateShaderModule(m_pDevice->Device, &createInfo, nullptr, &m_Module);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "[ShaderVK]: Failed to create ShaderModule");
			return false;
		}
		else
		{
            memcpy(&m_Desc, pDesc, sizeof(m_Desc));
			SetName(pDesc->pName);

            D_LOG_MESSAGE("[ShaderVK]: Created ShaderModule");
            
			return true;
		}
	}

	void ShaderVK::SetName(const char* pName)
	{
		if (pName)
		{
			TDeviceChild::SetName(pName);
			m_pDevice->SetVulkanObjectName(pName, (uint64)m_Module, VK_OBJECT_TYPE_SHADER_MODULE);

			m_Desc.pName = m_pDebugName;
		}
	}
}
