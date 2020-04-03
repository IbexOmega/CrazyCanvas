#include "Log/Log.h"

#include "Rendering/Core/Vulkan/FenceVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

namespace LambdaEngine
{
	FenceVK::FenceVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice)
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
	
	bool FenceVK::Init(uint64 initalValue)
	{
		VkSemaphoreTypeCreateInfo typeInfo = { };
		typeInfo.sType			= VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
		typeInfo.pNext			= nullptr;
		typeInfo.semaphoreType	= VK_SEMAPHORE_TYPE_TIMELINE;
		typeInfo.initialValue	= initalValue;

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

		return true;
	}

	void FenceVK::Wait(uint64 signalValue, uint64 timeOut) const
	{
		VkSemaphoreWaitInfo waitInfo = {};
		waitInfo.sType			= VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
		waitInfo.pNext			= nullptr;
		waitInfo.flags			= 0;
		waitInfo.semaphoreCount = 1;
		waitInfo.pSemaphores	= &m_Semaphore;
		waitInfo.pValues		= &signalValue;

		vkWaitSemaphores(m_pDevice->Device, &waitInfo, timeOut);
	}

	void FenceVK::Signal(uint64 signalValue)
	{
		VkSemaphoreSignalInfo signalInfo = { };
		signalInfo.sType		= VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
		signalInfo.pNext		= nullptr;
		signalInfo.value		= signalValue;
		signalInfo.semaphore	= m_Semaphore;

        VkResult result = VK_SUCCESS;//vkSignalSemaphore(m_pDevice->Device, &signalInfo);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR("[FenceVK]: Failed to signal semaphore", result);
		}
	}

	uint64 FenceVK::GetValue() const
	{
		uint64 value;
		VkResult result = vkGetSemaphoreCounterValue(m_pDevice->Device, m_Semaphore, &value);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR("[FenceVK]: Failed to retrive fence-value", result);
			return 0xffffffffffffffff;
		}

		return value;
	}
	
	void FenceVK::SetName(const char* pName)
	{
		m_pDevice->SetVulkanObjectName(pName, (uint64)m_Semaphore, VK_OBJECT_TYPE_SEMAPHORE);
	}
}
