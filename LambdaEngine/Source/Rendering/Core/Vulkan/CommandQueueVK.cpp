#include "Log/Log.h"

#include <mutex>

#include "Rendering/Core/Vulkan/CommandQueueVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/CommandListVK.h"
#include "Rendering/Core/Vulkan/FenceVK.h"
#include "Rendering/Core/Vulkan/FenceTimelineVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

namespace LambdaEngine
{
	CommandQueueVK::CommandQueueVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice),
		m_SubmitCommandBuffers()
	{
        memset(m_SubmitCommandBuffers, 0, sizeof(m_SubmitCommandBuffers));
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
		
        uint32 queueCount = properties[queueFamilyIndex].queueCount;
        if (index >= properties[queueFamilyIndex].queueCount)
		{
            LOG_ERROR("[CommandQueueVK]: index=%u exceeds the queueCount=%u for queueFamily=%u", index, queueCount, queueFamilyIndex);
			return false;
		}
        else
        {
            vkGetDeviceQueue(m_pDevice->Device, queueFamilyIndex, index, &m_Queue);
            if (pName)
            {
                SetName(pName);
            }
            
            D_LOG_MESSAGE("[CommandQueueVK]: Created commandqueue from queuefamily=%u with index=%u", queueFamilyIndex, index);

            m_Type = m_pDevice->GetCommandQueueTypeFromQueueIndex(queueFamilyIndex);
            return true;
        }
	}

	void CommandQueueVK::AddWaitSemaphore(VkSemaphore semaphore, VkPipelineStageFlags waitStage)
	{
        std::scoped_lock<SpinLock> lock(m_SpinLock);
        InternalAddWaitSemaphore(semaphore, waitStage);
        
	}

	void CommandQueueVK::AddSignalSemaphore(VkSemaphore semaphore)
	{
        std::scoped_lock<SpinLock> lock(m_SpinLock);
		InternalAddSignalSemaphore(semaphore);
	}

    void CommandQueueVK::InternalAddWaitSemaphore(VkSemaphore semaphore, VkPipelineStageFlags waitStage)
    {
        VALIDATE(semaphore != VK_NULL_HANDLE);
        
        m_WaitSemaphores.push_back(semaphore);
        m_WaitStages.push_back(waitStage);
    }

    void CommandQueueVK::InternalAddSignalSemaphore(VkSemaphore semaphore)
    {
        VALIDATE(semaphore != VK_NULL_HANDLE);
        m_SignalSemaphores.push_back(semaphore);
    }

	void CommandQueueVK::FlushBarriers()
	{
        //Lock and flush barriers
        std::scoped_lock<SpinLock> lock(m_SpinLock);
        InternalFlushBarriers();
	}
	
	bool CommandQueueVK::ExecuteCommandLists(const ICommandList* const* ppCommandLists, uint32 numCommandLists, FPipelineStageFlags waitStage, const IFence* pWaitFence, uint64 waitValue, IFence* pSignalFence, uint64 signalValue)
	{
#ifndef LAMBDA_PRODUCTION
        if (numCommandLists >= MAX_COMMANDBUFFERS)
        {
            LOG_ERROR("[CommandQueueVK]: NumCommandLists(=%u) must be less that %u", numCommandLists, MAX_COMMANDBUFFERS);
            return false;
        }
#endif
        
        {
            //If we function passes the "assertion" lock and execute commandbuffers
            std::scoped_lock<SpinLock> lock(m_SpinLock);
            for (uint32 i = 0; i < numCommandLists; i++)
            {
                const CommandListVK* pCommandListVk = reinterpret_cast<const CommandListVK*>(ppCommandLists[i]);
                m_SubmitCommandBuffers[i] = pCommandListVk->GetCommandBuffer();
            }

            //Setup common submit info
            VkSubmitInfo submitInfo = { };
            submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.pCommandBuffers      = m_SubmitCommandBuffers;
            submitInfo.commandBufferCount   = numCommandLists;
            
            //Check if we are using FenceTimelineVK
            VkFence submitFence = VK_NULL_HANDLE;
            if (m_pDevice->UseTimelineFences())
            {
                // Perform empty submit on queue for wait on the semaphore
                InternalFlushBarriers();
                
                const FenceTimelineVK*  pWaitFenceVk      = reinterpret_cast<const FenceTimelineVK*>(pWaitFence);
                FenceTimelineVK*        pSignalFenceVk    = reinterpret_cast<FenceTimelineVK*>(pSignalFence);

                //Add Timeline Semaphores
                uint64 signalValues[]   = { signalValue };
                uint32 SignalValueCount = 0;

                VkSemaphore          waitSemaphoreVk = VK_NULL_HANDLE;
                VkPipelineStageFlags waitStageVk     = 0;
                uint64 waitValues[]   = { waitValue };
                uint32 waitValueCount = 0;

                if (pWaitFenceVk)
                {
                   waitSemaphoreVk = pWaitFenceVk->GetSemaphore();
                   waitStageVk     = ConvertPipelineStage(waitStage);

                   waitValueCount = 1;
                }

                if (pSignalFenceVk)
                {
                   m_SignalSemaphores.insert(m_SignalSemaphores.begin(), pSignalFenceVk->GetSemaphore());
                   SignalValueCount = 1;
                }

                //TODO: Add ability to query this functionallty from the device
                VkTimelineSemaphoreSubmitInfo fenceSubmitInfo = {};
                fenceSubmitInfo.sType                        = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
                fenceSubmitInfo.pNext                        = nullptr;
                fenceSubmitInfo.signalSemaphoreValueCount    = SignalValueCount;
                fenceSubmitInfo.pSignalSemaphoreValues       = signalValues;
                fenceSubmitInfo.waitSemaphoreValueCount      = waitValueCount;
                fenceSubmitInfo.pWaitSemaphoreValues         = waitValues;
                
                submitInfo.pNext                = (const void*)&fenceSubmitInfo;
                submitInfo.waitSemaphoreCount   = waitValueCount;
                submitInfo.pWaitSemaphores      = &waitSemaphoreVk;
                submitInfo.pWaitDstStageMask    = &waitStageVk;
            }
            else
            {
                const FenceVK*  pWaitFenceVk   = reinterpret_cast<const FenceVK*>(pWaitFence);
                FenceVK*        pSignalFenceVk = reinterpret_cast<FenceVK*>(pSignalFence);
                
                if (pSignalFenceVk)
                {
                    VkSemaphore signalSemaphore = pSignalFenceVk->SignalGPU(signalValue, m_Queue);
                    if (signalSemaphore != VK_NULL_HANDLE)
                    {
                        InternalAddSignalSemaphore(signalSemaphore);
                        submitFence = pSignalFenceVk->GetSignalFence(signalValue);
                    }
                }
                
                if (pWaitFenceVk)
                {
                    VkSemaphore             waitSemaphore   = pWaitFenceVk->WaitGPU(waitValue, m_Queue);
                    VkPipelineStageFlagBits waitStageVk     = ConvertPipelineStage(waitStage);
                    
                    if (waitSemaphore != VK_NULL_HANDLE)
                    {
                        InternalAddWaitSemaphore(waitSemaphore, waitStageVk);
                    }
                }
                
                VALIDATE(m_WaitSemaphores.size() == m_WaitStages.size());
                
                submitInfo.pNext                = nullptr;
                submitInfo.waitSemaphoreCount   = uint32(m_WaitSemaphores.size());
                submitInfo.pWaitSemaphores      = m_WaitSemaphores.data();
                submitInfo.pWaitDstStageMask    = m_WaitStages.data();
                
                m_WaitSemaphores.clear();
                m_WaitStages.clear();
            }
            
            //Setup common signal semaphores
            submitInfo.signalSemaphoreCount = uint32(m_SignalSemaphores.size());
            submitInfo.pSignalSemaphores    = m_SignalSemaphores.data();
            
            VkResult result = vkQueueSubmit(m_Queue, 1, &submitInfo, submitFence);
            if (result != VK_SUCCESS)
            {
                LOG_VULKAN_ERROR(result, "[CommandQueueVK]: Executing commandlists failed");
                return false;
            }
            else
            {
                m_SignalSemaphores.clear();
                return true;
            }
        }
    }
    
    void CommandQueueVK::Flush()
    {
        std::scoped_lock<SpinLock> lock(m_SpinLock);
        
        //Flush all pending barriers (Semaphores) and then wait for queue to become idle
        InternalFlushBarriers();
		vkQueueWaitIdle(m_Queue);
	}

    VkResult CommandQueueVK::InternalFlushBarriers()
    {
        if (!m_WaitSemaphores.empty())
        {
            //If we need to flushbarriers, then add a lock so that vkQueueSubmit is protected

            VALIDATE(m_WaitSemaphores.size() == m_WaitStages.size());
            
            VkSubmitInfo submitInfo = { };
            submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.pNext                = nullptr;
            submitInfo.commandBufferCount   = 0;
            submitInfo.pCommandBuffers      = nullptr;
            submitInfo.signalSemaphoreCount = 0;
            submitInfo.pSignalSemaphores    = nullptr;
            submitInfo.waitSemaphoreCount   = uint32(m_WaitSemaphores.size());
            submitInfo.pWaitSemaphores      = m_WaitSemaphores.data();
            submitInfo.pWaitDstStageMask    = m_WaitStages.data();

            VkResult result = vkQueueSubmit(m_Queue, 1, &submitInfo, VK_NULL_HANDLE);
            if (result != VK_SUCCESS)
            {
                LOG_VULKAN_ERROR(result, "[CommandQueueVK]: Submit failed");
            }
            else
            {
                m_WaitSemaphores.clear();
                m_WaitStages.clear();

                m_SignalSemaphores.clear();
            }
            
            return result;
        }
        else
        {
            return VK_SUCCESS;
        }
    }
	
	void CommandQueueVK::SetName(const char* pName)
	{
		if (pName)
		{
            std::scoped_lock<SpinLock> lock(m_SpinLock);
            
			TDeviceChild::SetName(pName);
			m_pDevice->SetVulkanObjectName(pName, (uint64)m_Queue, VK_OBJECT_TYPE_QUEUE);
		}
	}
}
