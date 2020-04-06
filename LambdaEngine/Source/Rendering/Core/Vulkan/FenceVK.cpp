#include "Log/Log.h"

#include "Rendering/Core/Vulkan/FenceVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

namespace LambdaEngine
{
	FenceVK::FenceVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice),
        m_Desc()
	{
	}
	
	FenceVK::~FenceVK()
	{
		if (m_Semaphore != VK_NULL_HANDLE)
		{
			vkDestroySemaphore(m_pDevice->Device, m_Semaphore, nullptr);
			m_Semaphore = VK_NULL_HANDLE;
		}
	}
	
	bool FenceVK::Init(const FenceDesc& desc)
	{
		VkSemaphoreTypeCreateInfo typeInfo = { };
		typeInfo.sType			= VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
		typeInfo.pNext			= nullptr;
		typeInfo.semaphoreType	= VK_SEMAPHORE_TYPE_TIMELINE;
        typeInfo.initialValue	= desc.InitalValue;

		VkSemaphoreCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		info.pNext = (const void*)&typeInfo;
		info.flags = 0;

		VkResult result = vkCreateSemaphore(m_pDevice->Device, &info, nullptr, &m_Semaphore);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR("[FenceVK]: Failed to create semaphore", result);
			return false;
		}
        
        SetName(desc.pName);
        m_Desc = desc;
		return true;
	}

	void FenceVK::Wait(uint64 signalValue, uint64 timeOut) const
	{
		if (m_pDevice->vkWaitSemaphores)
		{
			VkSemaphoreWaitInfo waitInfo = {};
			waitInfo.sType			= VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
			waitInfo.pNext			= nullptr;
			waitInfo.flags			= 0;
			waitInfo.semaphoreCount = 1;
			waitInfo.pSemaphores	= &m_Semaphore;
			waitInfo.pValues		= &signalValue;

			VkResult result = m_pDevice->vkWaitSemaphores(m_pDevice->Device, &waitInfo, timeOut);
			LOG_MESSAGE("[FenceVK::Wait]: Return %s", VkResultToString(result));
		}
	}

	void FenceVK::Signal(uint64 signalValue)
	{
		if (m_pDevice->vkSignalSemaphore)
		{
			VkSemaphoreSignalInfo signalInfo = { };
			signalInfo.sType		= VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
			signalInfo.pNext		= nullptr;
			signalInfo.value		= signalValue;
			signalInfo.semaphore	= m_Semaphore;

			VkResult result = m_pDevice->vkSignalSemaphore(m_pDevice->Device, &signalInfo);
			if (result != VK_SUCCESS)
			{
				LOG_VULKAN_ERROR("[FenceVK]: Failed to signal semaphore", result);
			}
		}
	}

	uint64 FenceVK::GetValue() const
	{
		uint64 value = 0xffffffffffffffff;
		if (m_pDevice->vkGetSemaphoreCounterValue)
		{
			VkResult result = m_pDevice->vkGetSemaphoreCounterValue(m_pDevice->Device, m_Semaphore, &value);
			if (result != VK_SUCCESS)
			{
				LOG_VULKAN_ERROR("[FenceVK]: Failed to retrive fence-value", result);
			}
		}

		return value;
	}
	
	void FenceVK::SetName(const char* pName)
	{
		m_pDevice->SetVulkanObjectName(pName, (uint64)m_Semaphore, VK_OBJECT_TYPE_SEMAPHORE);
	}
}
