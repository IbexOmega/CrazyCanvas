#include "Log/Log.h"

#include "Rendering/Core/Vulkan/FenceTimelineVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

namespace LambdaEngine
{
	FenceTimelineVK::FenceTimelineVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice),
        m_Desc()
	{
	}
	
	FenceTimelineVK::~FenceTimelineVK()
	{
		if (m_Semaphore != VK_NULL_HANDLE)
		{
			vkDestroySemaphore(m_pDevice->Device, m_Semaphore, nullptr);
			m_Semaphore = VK_NULL_HANDLE;
		}
	}
	
	bool FenceTimelineVK::Init(const FenceDesc* pDesc)
	{
		VkSemaphoreTypeCreateInfo typeInfo = { };
		typeInfo.sType			= VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
		typeInfo.pNext			= nullptr;
		typeInfo.semaphoreType	= VK_SEMAPHORE_TYPE_TIMELINE;
        typeInfo.initialValue	= pDesc->InitalValue;

		VkSemaphoreCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		info.pNext = (const void*)&typeInfo;
		info.flags = 0;

		VkResult result = vkCreateSemaphore(m_pDevice->Device, &info, nullptr, &m_Semaphore);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "[FenceVK]: Failed to create semaphore");
			return false;
		}
        else
        {
            memcpy(&m_Desc, pDesc, sizeof(m_Desc));
            SetName(pDesc->pName);
            return true;
        }
	}

	void FenceTimelineVK::Wait(uint64 waitValue, uint64 timeOut) const
	{
		VkSemaphoreWaitInfo waitInfo = {};
		waitInfo.sType			= VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
		waitInfo.pNext			= nullptr;
		waitInfo.flags			= 0;
		waitInfo.semaphoreCount = 1;
		waitInfo.pSemaphores	= &m_Semaphore;
		waitInfo.pValues		= &waitValue;

		VkResult result = m_pDevice->vkWaitSemaphores(m_pDevice->Device, &waitInfo, timeOut);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "[FenceVK]: Failed to wait for semaphore");
		}
	}

	void FenceTimelineVK::Reset(uint64 resetValue)
	{
		VALIDATE(resetValue < GetValue());

		VkSemaphoreSignalInfo signalInfo = { };
		signalInfo.sType		= VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
		signalInfo.pNext		= nullptr;
		signalInfo.value		= resetValue;
		signalInfo.semaphore	= m_Semaphore;

		VkResult result = m_pDevice->vkSignalSemaphore(m_pDevice->Device, &signalInfo);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "[FenceVK]: Failed to reset semaphore");
		}
}

	uint64 FenceTimelineVK::GetValue() const
	{
		uint64	 value	= 0;
		VkResult result = m_pDevice->vkGetSemaphoreCounterValue(m_pDevice->Device, m_Semaphore, &value);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "[FenceVK]: Failed to retrive fence-value");
			return 0xffffffffffffffff;
		}
		else
		{
			return value;
		}
	}
	
	void FenceTimelineVK::SetName(const char* pName)
	{
		if (pName)
		{
			TDeviceChild::SetName(pName);
			m_pDevice->SetVulkanObjectName(pName, (uint64)m_Semaphore, VK_OBJECT_TYPE_SEMAPHORE);

			m_Desc.pName = m_pDebugName;
		}
	}
}
