#include "Log/Log.h"

#include <string>

#include "Rendering/Core/Vulkan/FenceVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

#define FENCE_LOG_STATE_ENABLE  0
#if FENCE_LOG_STATE_ENABLE
	#define FENCE_LOG_STATE(...) LOG_DEBUG(__VA_ARGS__)
#else
	#define FENCE_LOG_STATE(...)
#endif

namespace LambdaEngine
{
	FenceVK::FenceVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice)
	{
	}

	FenceVK::~FenceVK()
	{
		for (VkSemaphore& semaphore : m_Semaphores)
		{
			if (semaphore != VK_NULL_HANDLE)
			{
				vkDestroySemaphore(m_pDevice->Device, semaphore, nullptr);
				semaphore = VK_NULL_HANDLE;
			}
		}

		for (VkFence& fence : m_Fences)
		{
			if (fence != VK_NULL_HANDLE)
			{
				vkDestroyFence(m_pDevice->Device, fence, nullptr);
				fence = VK_NULL_HANDLE;
			}
		}
	}

	bool FenceVK::Init(const FenceDesc* pDesc)
	{
		VkSemaphoreCreateInfo semaphoreInfo = { };
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreInfo.pNext = nullptr;
		semaphoreInfo.flags = 0;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.pNext = nullptr;
		fenceInfo.flags = 0;

		constexpr uint32 PRIMITIVE_START_COUNT = 4;
		m_Fences.Resize(PRIMITIVE_START_COUNT);
		m_Semaphores.Resize(PRIMITIVE_START_COUNT);

		for (uint32 i = 0; i < PRIMITIVE_START_COUNT; i++)
		{
			VkSemaphore semaphore = VK_NULL_HANDLE;
			VkResult result = vkCreateSemaphore(m_pDevice->Device, &semaphoreInfo, nullptr, &semaphore);
			if (result != VK_SUCCESS)
			{
				LOG_VULKAN_ERROR(result, "Failed to create semaphore[%u]", i);
				return false;
			}
			else
			{
				m_Semaphores[i] = semaphore;
			}

			VkFence fence = VK_NULL_HANDLE;
			result = vkCreateFence(m_pDevice->Device, &fenceInfo, nullptr, &fence);
			if (result != VK_SUCCESS)
			{
				LOG_VULKAN_ERROR(result, "Failed to create fence[%u]", i);
				return false;
			}
			else
			{
				m_Fences[i] = fence;
			}
		}

		LOG_VULKAN_INFO("Created Fence");

		m_Desc = *pDesc;
		SetName(m_Desc.DebugName);

		m_LastCompletedValue = m_Desc.InitalValue;

		return true;
	}

	void FenceVK::SetName(const String& debugName)
	{
		VALIDATE(!m_Fences.IsEmpty());
		VALIDATE(!m_Semaphores.IsEmpty());

		m_Desc.DebugName = debugName;

		String name;
		for (uint32 i = 0; i < m_Fences.GetSize(); i++)
		{
			name = m_Desc.DebugName + ":Fence[" + std::to_string(i) + "]";
			m_pDevice->SetVulkanObjectName(name.c_str(), reinterpret_cast<uint64>(m_Fences[i]), VK_OBJECT_TYPE_FENCE);
		}

		for (uint32 i = 0; i < m_Semaphores.GetSize(); i++)
		{
			name = m_Desc.DebugName + ":Semaphore[" + std::to_string(i) + "]";
			m_pDevice->SetVulkanObjectName(name.c_str(), reinterpret_cast<uint64>(m_Semaphores[i]), VK_OBJECT_TYPE_SEMAPHORE);
		}
	}

	VkFence FenceVK::GetSignalFence(uint64 value) const
	{
		if (!m_PendingValues.IsEmpty())
		{
			for (FenceValueVk& fenceValue : m_PendingValues)
			{
				if (fenceValue.Value == value)
				{
					return fenceValue.Fence;
				}
			}
		}

		return VK_NULL_HANDLE;
	}

	VkSemaphore FenceVK::WaitGPU(uint64 waitValue, VkQueue queue) const
	{
		FENCE_LOG_STATE("GPU wait. waitValue=%llu", waitValue);

		// Wait for semaphores that were waited on during CPU - wait, this is to enable reuse of them
		FlushWaitSemaphores(queue);

		VkSemaphore waitSemaphore   = VK_NULL_HANDLE;
		for (TArray<FenceValueVk>::Iterator it = m_PendingValues.Begin(); it != m_PendingValues.End();)
		{
			if (it->Value > waitValue)
			{
				break;
			}

			if (it->SemaphoreState == ESemaphoreState::SEMAPHORE_STATE_SIGNALED)
			{
				waitSemaphore = it->Semaphore;
				it->SemaphoreState = ESemaphoreState::SEMAPHORE_STATE_WAITING;
			}
			else
			{
				VALIDATE(it->SemaphoreState == ESemaphoreState::SEMAPHORE_STATE_WAITING);

				VkResult result = vkGetFenceStatus(m_pDevice->Device, it->Fence);
				if (result == VK_NOT_READY)
				{
					// Fence is not signaled so wait for this value to finish
					result = vkWaitForFences(m_pDevice->Device, 1, &it->Fence, true, UINT64_MAX);
					FENCE_LOG_STATE("GPU wait. Waited For fence=%p", it->Fence);
				}

				if (result == VK_SUCCESS)
				{
					// Fence should be finished here
					if (waitValue > m_LastCompletedValue)
					{
						m_LastCompletedValue = it->Value;
						FENCE_LOG_STATE("GPU wait. Fence Finished. LastCompletedValue=%llu", m_LastCompletedValue);
					}

					// Make sure we can reuse this fence and semaphore
					DisposeFence(it->Fence);
					DisposeSemaphore(it->Semaphore);

					it = m_PendingValues.Erase(m_PendingValues.Begin());
					continue;
				}
			}

			it++;
		}

		return waitSemaphore;
	}

	VkSemaphore FenceVK::SignalGPU(uint64 signalValue, VkQueue queue)
	{
		FENCE_LOG_STATE("GPU Reset. signalValue=%llu", signalValue);

		// Wait for semaphores that were waited on during CPU - wait, this is to enable reuse of them
		FlushWaitSemaphores(queue);

		// We only move forward
		if (m_LastCompletedValue < signalValue)
		{
			FenceValueVk fenceValue = { };
			fenceValue.Fence            = QueryFence();
			fenceValue.Semaphore        = QuerySemaphore();
			fenceValue.SemaphoreState   = ESemaphoreState::SEMAPHORE_STATE_SIGNALED;
			fenceValue.Value            = signalValue;

			m_PendingValues.PushBack(fenceValue);
			return fenceValue.Semaphore;
		}
		else
		{
			return VK_NULL_HANDLE;
		}
	}

	void FenceVK::Wait(uint64 waitValue, uint64 timeOut) const
	{
		FENCE_LOG_STATE("CPU Wait. waitValue=%llu, timeOut=%llu", waitValue, timeOut);

		while (!m_PendingValues.IsEmpty())
		{
			FenceValueVk& fenceValue = m_PendingValues.GetFront();
			if (fenceValue.Value > waitValue)
			{
				break;
			}

			VkResult result = vkGetFenceStatus(m_pDevice->Device, fenceValue.Fence);
			if (result == VK_NOT_READY)
			{
				// Fence is not signaled so wait for this value to finish
				result = vkWaitForFences(m_pDevice->Device, 1, &fenceValue.Fence, true, timeOut);

				FENCE_LOG_STATE("CPU wait. Waited For fence=%p", fenceValue.Fence);
			}

			if (result != VK_TIMEOUT)
			{
				// Fence should be finished here
				if (waitValue > m_LastCompletedValue)
				{
					m_LastCompletedValue = fenceValue.Value;

					FENCE_LOG_STATE("CPU wait. Fence Finished. LastCompletedValue=%llu", m_LastCompletedValue);
				}

				// Make sure we can reuse this fence
				DisposeFence(fenceValue.Fence);

				// If semaphore state is waiting it must be complete by now
				if (fenceValue.SemaphoreState == ESemaphoreState::SEMAPHORE_STATE_WAITING)
				{
					DisposeSemaphore(fenceValue.Semaphore);
				}
				else
				{
					VALIDATE(fenceValue.SemaphoreState == ESemaphoreState::SEMAPHORE_STATE_SIGNALED);
					AddWaitSemaphore(fenceValue.Semaphore, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
				}

				m_PendingValues.Erase(m_PendingValues.Begin());
			}
		}
	}

	void FenceVK::Reset(uint64 resetValue)
	{
		VALIDATE(resetValue > m_LastCompletedValue);
		m_LastCompletedValue = resetValue;
	}

	VkFence FenceVK::QueryFence() const
	{
		VkFence fence = VK_NULL_HANDLE;
		if (!m_Fences.IsEmpty())
		{
			fence = m_Fences.GetBack();
			m_Fences.PopBack();
			return fence;
		}
		else
		{
			VkFenceCreateInfo fenceInfo = {};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.pNext = nullptr;
			fenceInfo.flags = 0;

			VkResult result = vkCreateFence(m_pDevice->Device, &fenceInfo, nullptr, &fence);
			if (result != VK_SUCCESS)
			{
				LOG_VULKAN_ERROR(result, "Failed to create fence");
				return VK_NULL_HANDLE;
			}
			else
			{
				LOG_VULKAN_INFO("New fence created");
				return fence;
			}
		}
	}

	VkSemaphore FenceVK::QuerySemaphore() const
	{
		VkSemaphore semaphore = VK_NULL_HANDLE;
		if (!m_Semaphores.IsEmpty())
		{
			semaphore = m_Semaphores.GetBack();
			m_Semaphores.PopBack();
			return semaphore;
		}
		else
		{
			VkSemaphoreCreateInfo semaphoreInfo = { };
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			semaphoreInfo.pNext = nullptr;
			semaphoreInfo.flags = 0;

			VkResult result = vkCreateSemaphore(m_pDevice->Device, &semaphoreInfo, nullptr, &semaphore);
			if (result != VK_SUCCESS)
			{
				LOG_VULKAN_ERROR(result, "Failed to create semaphore");
				return VK_NULL_HANDLE;
			}
			else
			{
				LOG_VULKAN_INFO("New semaphore created");
				return semaphore;
			}
		}
	}

	void FenceVK::DisposeFence(VkFence fence) const
	{
		VALIDATE(fence != VK_NULL_HANDLE);
		VALIDATE(FenceCanReset(fence));

		FENCE_LOG_STATE("DisposeFence: fence=%p", fence);

		VkResult result = vkResetFences(m_pDevice->Device, 1, &fence);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "Failed to reset fence");
		}
		else
		{
			m_Fences.Insert(m_Fences.begin(), fence);
		}
	}

	void FenceVK::DisposeSemaphore(VkSemaphore semaphore) const
	{
		VALIDATE(semaphore != VK_NULL_HANDLE);
		m_Semaphores.Insert(m_Semaphores.begin(), semaphore);
	}

	void FenceVK::AddWaitSemaphore(VkSemaphore semaphore, VkPipelineStageFlags waitStage) const
	{
		VALIDATE(semaphore != VK_NULL_HANDLE);

		m_WaitSemaphores.PushBack(semaphore);
		m_WaitStages.PushBack(waitStage);
	}

	void FenceVK::FlushWaitSemaphores(VkQueue queue) const
	{
		VALIDATE(queue != VK_NULL_HANDLE);

		// Wait for semaphores for values that were removed during CPU-wait
		if (!m_WaitSemaphores.IsEmpty())
		{
			FENCE_LOG_STATE("Flushing waitsemaphores");

			VkSubmitInfo submitInfo = { };
			submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.pNext                = nullptr;
			submitInfo.commandBufferCount   = 0;
			submitInfo.pCommandBuffers      = nullptr;
			submitInfo.signalSemaphoreCount = 0;
			submitInfo.pSignalSemaphores    = nullptr;
			submitInfo.waitSemaphoreCount   = uint32(m_WaitSemaphores.GetSize());
			submitInfo.pWaitSemaphores      = m_WaitSemaphores.GetData();
			submitInfo.pWaitDstStageMask    = m_WaitStages.GetData();

			VkResult result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
			if (result != VK_SUCCESS)
			{
				LOG_VULKAN_ERROR(result, "Submiting waitsemaphores failed");
			}
			else
			{
				// Append semaphores to the pool
				m_Semaphores.Insert(m_Semaphores.End(), m_WaitSemaphores.Begin(), m_WaitSemaphores.End());

				// Clear
				m_WaitSemaphores.Clear();
				m_WaitStages.Clear();
			}
		}
	}

	bool FenceVK::FenceCanReset(VkFence fence) const
	{
		return (vkGetFenceStatus(m_pDevice->Device, fence) == VK_SUCCESS);
	}
}
