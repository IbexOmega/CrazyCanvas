#include "Rendering/Core/Vulkan/ShaderVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"

#include <cstdio>

#include "Log/Log.h"

namespace LambdaEngine
{
	ShaderVK::ShaderVK(const GraphicsDeviceVK* pDevice) 
		: TDeviceChild(pDevice)
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
		createInfo.codeSize	= pDesc->Source.size();
		createInfo.pCode	= reinterpret_cast<const uint32_t*>(pDesc->Source.data());

		VkResult result = vkCreateShaderModule(m_pDevice->Device, &createInfo, nullptr, &m_Module);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "[ShaderVK]: Failed to create ShaderModule");
			return false;
		}
		else
		{
			m_Desc = *pDesc;
			SetName(pDesc->DebugName);

			D_LOG_MESSAGE("[ShaderVK]: Created ShaderModule");
			
			return true;
		}
	}

	void ShaderVK::SetName(const String& debugName)
	{
		m_pDevice->SetVulkanObjectName(debugName, reinterpret_cast<uint64>(m_Module), VK_OBJECT_TYPE_SHADER_MODULE);
		m_Desc.DebugName = debugName;
	}
}
