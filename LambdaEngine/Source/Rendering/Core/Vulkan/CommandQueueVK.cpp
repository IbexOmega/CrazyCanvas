#include "Log/Log.h"

#include "Rendering/Core/Vulkan/CommandQueueVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

namespace LambdaEngine
{
	CommandQueueVK::CommandQueueVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice)
	{
	}

	CommandQueueVK::~CommandQueueVK()
	{
		m_Queue = VK_NULL_HANDLE;
	}

	bool CommandQueueVK::Init(uint32 queueFamilyIndex, uint32 index)
	{
		uint32 queuePropertyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(m_pDevice->PhysicalDevice, &queuePropertyCount, nullptr);

		std::vector<VkQueueFamilyProperties> properties(queuePropertyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(m_pDevice->PhysicalDevice, &queuePropertyCount, properties.data());

		//Make sure that the index is valid
		ASSERT(queueFamilyIndex < queuePropertyCount);
		if (index >= properties[queueFamilyIndex].queueCount)
		{
			return false;
		}

		vkGetDeviceQueue(m_pDevice->Device, queueFamilyIndex, index, &m_Queue);
		return true;
	}
	
	bool CommandQueueVK::ExecuteCommandList(const ICommandList* const* ppCommandList, uint32 numCommandLists, const IFence* pFence)
	{
		constexpr uint32 MAX_COMMANDBUFFERS = 8;
		VkCommandBuffer commandBuffers[MAX_COMMANDBUFFERS];

#ifndef LAMBDA_PRODUCTION
		if (numCommandLists >= MAX_COMMANDBUFFERS)
		{
			LOG_ERROR("[CommandQueueVK]: NumCommandLists(=%u) must be less that %u", numCommandLists, MAX_COMMANDBUFFERS);
			return false;
		}
#endif

		VkSubmitInfo submitInfo = { };
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		//submitInfo.pCommandBuffers = ;
		submitInfo.commandBufferCount = numCommandLists;

		VkTimelineSemaphoreSubmitInfo fenceSubmitInfo = {};
		if (pFence)
		{
			fenceSubmitInfo.sType						= VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
			fenceSubmitInfo.pNext						= nullptr;
			fenceSubmitInfo.signalSemaphoreValueCount	= 0;
			fenceSubmitInfo.pSignalSemaphoreValues		= nullptr;
			fenceSubmitInfo.waitSemaphoreValueCount		= 0;
			fenceSubmitInfo.pWaitSemaphoreValues		= nullptr;

			submitInfo.pNext				= (const void*)&fenceSubmitInfo;
			submitInfo.signalSemaphoreCount = 1;
			//submitInfo.pSignalSemaphores	= nullptr;
			submitInfo.waitSemaphoreCount	= 1;
			//submitInfo.pWaitSemaphores	= nullptr;
		}
		else
		{
			submitInfo.pNext				= nullptr;
			submitInfo.signalSemaphoreCount = 0;
			submitInfo.pSignalSemaphores	= nullptr;
			submitInfo.waitSemaphoreCount	= 0;
			submitInfo.pWaitSemaphores		= nullptr;
		}

		VkResult result = vkQueueSubmit(m_Queue, 1, &submitInfo, VK_NULL_HANDLE);
		if (result != VK_SUCCESS)
		{	
			LOG_VULKAN_ERROR("[CommandQueueVK]: Executing commandlists failed", result);
			return false;
		}

		return true;
	}
	
	void CommandQueueVK::Wait()
	{
		vkQueueWaitIdle(m_Queue);
	}
	
	void CommandQueueVK::SetName(const char* pName)
	{
		m_pDevice->SetVulkanObjectName(pName, (uint64)m_Queue, VK_OBJECT_TYPE_QUEUE);
	}
}