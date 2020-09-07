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
	
	bool CommandAllocatorVK::Init(const String& debugname, ECommandQueueType queueType)
	{
		VkCommandPoolCreateInfo createInfo = {};
		createInfo.sType			= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		createInfo.pNext			= nullptr;
		createInfo.flags			= 0;
		createInfo.queueFamilyIndex = m_pDevice->GetQueueFamilyIndexFromQueueType(queueType);

		VkResult result = vkCreateCommandPool(m_pDevice->Device, &createInfo, nullptr, &m_CommandPool);
		if (result != VK_SUCCESS)
		{
			if (!debugname.empty())
			{
				LOG_VULKAN_ERROR(result, "[CommandAllocatorVK]: Failed to create commandpool \"%s\"", debugname.c_str());
			}
			else
			{
				LOG_VULKAN_ERROR(result, "[CommandAllocatorVK]: Failed to create commandpool");
			}

			return false;
		}
		else
		{
			m_Type = queueType;
			if (!debugname.empty())
			{
				LOG_MESSAGE("[CommandAllocatorVK]: Created commandpool \"%s\"", debugname.c_str());
				SetName(debugname);
			}
			else
			{
				LOG_MESSAGE("[CommandAllocatorVK]: Created commandpool");
			}

			return true;
		}
	}

	VkCommandBuffer CommandAllocatorVK::AllocateCommandBuffer(VkCommandBufferLevel level)
	{
		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
		
		VkCommandBufferAllocateInfo allocateInfo = { };
		allocateInfo.sType					= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.pNext					= nullptr;
		allocateInfo.level					= level;
		allocateInfo.commandPool			= m_CommandPool;
		allocateInfo.commandBufferCount		= 1;

		VkResult result = vkAllocateCommandBuffers(m_pDevice->Device, &allocateInfo, &commandBuffer);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "[CommandAllocatorVK]: Failed to allocate commandbuffer");
			return VK_NULL_HANDLE;
		}
		else
		{
			return commandBuffer;
		}
	}

	void CommandAllocatorVK::FreeCommandBuffer(VkCommandBuffer commandBuffer)
	{
		vkFreeCommandBuffers(m_pDevice->Device, m_CommandPool, 1, &commandBuffer);
	}
	
	bool CommandAllocatorVK::Reset()
	{
		VkResult result = vkResetCommandPool(m_pDevice->Device, m_CommandPool, 0);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "[CommandAllocatorVK]: Failed to reset commandpool");
			return false; 
		}

		return true;
	}

	void CommandAllocatorVK::SetName(const String& debugname)
	{
		m_pDevice->SetVulkanObjectName(debugname, reinterpret_cast<uint64>(m_CommandPool), VK_OBJECT_TYPE_COMMAND_POOL);
		m_DebugName = debugname;
	}
}
