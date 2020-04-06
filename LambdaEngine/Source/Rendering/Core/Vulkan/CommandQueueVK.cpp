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
		m_CommandBuffers()
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
	
	bool CommandQueueVK::ExecuteCommandLists(const ICommandList* const* ppCommandLists, uint32 numCommandLists, FPipelineStageFlags waitStage, const IFence* pWaitFence, uint64 waitValue, const IFence* pSignalFence, uint64 signalValue)
	{
		UNREFERENCED_VARIABLE(ppCommandLists);

		for (uint32 i = 0; i < numCommandLists; i++)
		{
			const CommandListVK* pCommandListVk = reinterpret_cast<const CommandListVK*>(ppCommandLists[i]);
			m_CommandBuffers[i] = pCommandListVk->GetCommandBuffer();
		}

#ifndef LAMBDA_PRODUCTION
		if (numCommandLists >= MAX_COMMANDBUFFERS)
		{
			LOG_ERROR("[CommandQueueVK]: NumCommandLists(=%u) must be less that %u", numCommandLists, MAX_COMMANDBUFFERS);
			return false;
		}
#endif

		VkSubmitInfo submitInfo = { };
		submitInfo.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pCommandBuffers		= m_CommandBuffers;
		submitInfo.commandBufferCount	= numCommandLists;

		//TODO: Add ability to query this functionallty from the device
		VkTimelineSemaphoreSubmitInfo fenceSubmitInfo = {};
		if (pWaitFence && m_pDevice->vkGetSemaphoreCounterValue)
		{
			const FenceVK* pWaitFenceVk		= reinterpret_cast<const FenceVK*>(pWaitFence);
			const FenceVK* pSignalFenceVk	= reinterpret_cast<const FenceVK*>(pSignalFence);

			fenceSubmitInfo.sType						= VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
			fenceSubmitInfo.pNext						= nullptr;
			
			uint64 signalValues[] = { signalValue };
			fenceSubmitInfo.signalSemaphoreValueCount	= 1;
			fenceSubmitInfo.pSignalSemaphoreValues		= signalValues;

			uint64 waitValues[] = { waitValue };
			fenceSubmitInfo.waitSemaphoreValueCount		= 1;
			fenceSubmitInfo.pWaitSemaphoreValues		= waitValues;

			VkSemaphore	signalSemaphores[] = { pSignalFenceVk->GetSemaphore() };
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores	= signalSemaphores;

			VkSemaphore				waitSemaphores[]	= { pWaitFenceVk->GetSemaphore() };
			VkPipelineStageFlags	waitStagesVk[]		= { ConvertPipelineStageMask(waitStage) };
			submitInfo.waitSemaphoreCount	= 1;
			submitInfo.pWaitSemaphores		= waitSemaphores;
			submitInfo.pWaitDstStageMask	= waitStagesVk;
			submitInfo.pNext				= (const void*)&fenceSubmitInfo;
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
	
	void CommandQueueVK::WaitForCompletion()
	{
		vkQueueWaitIdle(m_Queue);
	}
	
	void CommandQueueVK::SetName(const char* pName)
	{
		m_pDevice->SetVulkanObjectName(pName, (uint64)m_Queue, VK_OBJECT_TYPE_QUEUE);
	}
}
