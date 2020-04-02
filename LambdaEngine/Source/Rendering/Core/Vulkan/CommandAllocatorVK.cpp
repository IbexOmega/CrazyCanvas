#include "Log/Log.h"

#include "Rendering/Core/Vulkan/CommandAllocatorVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

namespace LambdaEngine
{
	CommandAllocatorVK::CommandAllocatorVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice)
	{
	}
	
	CommandAllocatorVK::~CommandAllocatorVK()
	{
		if (m_CommandPool != VK_NULL_HANDLE)
		{
			vkDestroyCommandPool(m_pDevice->Device, m_CommandPool, nullptr);
			m_CommandPool = VK_NULL_HANDLE;
		}
	}
	
	bool CommandAllocatorVK::Init(EQueueType queueType)
	{
		VkCommandPoolCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		QueueFamilyIndices queueFamilyIndices = m_pDevice->GetQueueFamilyIndices();
		if (queueType == EQueueType::QUEUE_GRAPHICS)
		{
			createInfo.queueFamilyIndex = queueFamilyIndices.GraphicsFamily;
		}
		else if (queueType == EQueueType::QUEUE_COMPUTE)
		{
			createInfo.queueFamilyIndex = queueFamilyIndices.ComputeFamily;
		}
		else if (queueType == EQueueType::QUEUE_COPY)
		{
			createInfo.queueFamilyIndex = queueFamilyIndices.TransferFamily;
		}

		VkResult result = vkCreateCommandPool(m_pDevice->Device, &createInfo, nullptr, &m_CommandPool);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR("[CommandAllocatorVK]: Failed to create commandpool", result);
			return false;
		}

		return true;
	}
	
	bool CommandAllocatorVK::Reset()
	{
		VkResult result = vkResetCommandPool(m_pDevice->Device, m_CommandPool, 0);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR("[CommandAllocatorVK]: Failed to reset commandpool", result);
			return false; 
		}

		return true;
	}

	void CommandAllocatorVK::SetName(const char* pName)
	{
		m_pDevice->SetVulkanObjectName(pName, (uint64)m_CommandPool, VK_OBJECT_TYPE_COMMAND_POOL);
	}
}