#include "Log/Log.h"

#include "Rendering/Core/Vulkan/CommandQueueVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/CommandListVK.h"
#include "Rendering/Core/Vulkan/FenceVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

namespace LambdaEngine
{
	CommandQueueVK::CommandQueueVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice),
		m_SubmitCommandBuffers()
	{
	}

	CommandQueueVK::~CommandQueueVK()
	{
		m_Queue = VK_NULL_HANDLE;
	}

	bool CommandQueueVK::Init(const char* pName, uint32 queueFamilyIndex, uint32 index)
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
		if (pName)
		{
			SetName(pName);
		}

		return true;
	}

	void CommandQueueVK::AddWaitSemaphore(VkSemaphore semaphore, VkPipelineStageFlagBits waitStage)
	{
		m_WaitSemaphores.push_back(semaphore);
		m_WaitStages.push_back(waitStage);
	}

	void CommandQueueVK::AddSignalSemaphore(VkSemaphore semaphore)
	{
		m_SignalSemaphores.push_back(semaphore);
	}
	
	bool CommandQueueVK::ExecuteCommandLists(const ICommandList* const* ppCommandLists, uint32 numCommandLists, FPipelineStageFlags waitStage, const IFence* pWaitFence, uint64 waitValue, const IFence* pSignalFence, uint64 signalValue)
	{
		for (uint32 i = 0; i < numCommandLists; i++)
		{
			const CommandListVK* pCommandListVk = reinterpret_cast<const CommandListVK*>(ppCommandLists[i]);
			m_SubmitCommandBuffers[i] = pCommandListVk->GetCommandBuffer();
		}

#ifndef LAMBDA_PRODUCTION
		if (numCommandLists >= MAX_COMMANDBUFFERS)
		{
			LOG_ERROR("[CommandQueueVK]: NumCommandLists(=%u) must be less that %u", numCommandLists, MAX_COMMANDBUFFERS);
			return false;
		}
#endif
		// Perform empty submit on queue for wait on the semaphore
		if (!m_WaitSemaphores.empty())
		{
			VkSubmitInfo submitInfo = { };
			submitInfo.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.pNext				= nullptr;
			submitInfo.commandBufferCount	= 0;
			submitInfo.pCommandBuffers		= nullptr;
			submitInfo.signalSemaphoreCount = 0;
			submitInfo.pSignalSemaphores	= nullptr;
			submitInfo.waitSemaphoreCount	= uint32(m_WaitSemaphores.size());
			submitInfo.pWaitSemaphores		= m_WaitSemaphores.data();
			submitInfo.pWaitDstStageMask	= m_WaitStages.data();

			VkResult result = vkQueueSubmit(m_Queue, 1, &submitInfo, VK_NULL_HANDLE);
			if (result != VK_SUCCESS)
			{
				LOG_VULKAN_ERROR(result, "[CommandQueueVK]: Submit failed");
				return false;
			}
		}

		VkSubmitInfo submitInfo = { };
		submitInfo.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pCommandBuffers		= m_SubmitCommandBuffers;
		submitInfo.commandBufferCount	= numCommandLists;

		const FenceVK* pWaitFenceVk		= reinterpret_cast<const FenceVK*>(pWaitFence);
		const FenceVK* pSignalFenceVk	= reinterpret_cast<const FenceVK*>(pSignalFence);

		//Add Timeline Semaphores
		uint64 signalValues[]	= { signalValue };
		uint32 SignalValueCount = 0;
		
		VkSemaphore				waitSemaphoreVk = VK_NULL_HANDLE;
		VkPipelineStageFlags	waitStageVk = 0;
		uint64 waitValues[]		= { waitValue };
		uint32 waitValueCount	= 0;

		if (pWaitFenceVk && m_pDevice->vkWaitSemaphores)
		{
			waitSemaphoreVk = pWaitFenceVk->GetSemaphore();
			waitStageVk		= ConvertPipelineStage(waitStage);

			waitValueCount = 1;
		}

		if (pSignalFenceVk && m_pDevice->vkWaitSemaphores)
		{
			m_SignalSemaphores.insert(m_SignalSemaphores.begin(), pSignalFenceVk->GetSemaphore());
			SignalValueCount = 1;
		}
		
		//TODO: Add ability to query this functionallty from the device
		VkTimelineSemaphoreSubmitInfo fenceSubmitInfo = {};
		if (m_pDevice->vkGetSemaphoreCounterValue)
		{
			fenceSubmitInfo.sType						= VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
			fenceSubmitInfo.pNext						= nullptr;	
			fenceSubmitInfo.signalSemaphoreValueCount	= SignalValueCount;
			fenceSubmitInfo.pSignalSemaphoreValues		= signalValues;
			fenceSubmitInfo.waitSemaphoreValueCount		= waitValueCount;
			fenceSubmitInfo.pWaitSemaphoreValues		= waitValues;

			submitInfo.pNext = (const void*)&fenceSubmitInfo;
		}
		else
		{
			submitInfo.pNext = nullptr;
		}

		submitInfo.signalSemaphoreCount	= uint32(m_SignalSemaphores.size());
		submitInfo.pSignalSemaphores	= m_SignalSemaphores.data();
		submitInfo.waitSemaphoreCount	= waitValueCount;
		submitInfo.pWaitSemaphores		= &waitSemaphoreVk;
		submitInfo.pWaitDstStageMask	= &waitStageVk;

		VkResult result = vkQueueSubmit(m_Queue, 1, &submitInfo, VK_NULL_HANDLE);
		if (result != VK_SUCCESS)
		{	
			LOG_VULKAN_ERROR(result, "[CommandQueueVK]: Executing commandlists failed");
			return false;
		}
		else
		{
			m_WaitSemaphores.clear();
			m_WaitStages.clear();

			m_SignalSemaphores.clear();
			return true;
		}
	}
	
	void CommandQueueVK::Flush()
	{
		vkQueueWaitIdle(m_Queue);
	}
	
	void CommandQueueVK::SetName(const char* pName)
	{
		if (pName)
		{
			TDeviceChild::SetName(pName);
			m_pDevice->SetVulkanObjectName(pName, (uint64)m_Queue, VK_OBJECT_TYPE_QUEUE);
		}
	}
}
