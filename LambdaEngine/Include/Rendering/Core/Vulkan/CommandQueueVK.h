#pragma once
#include "Containers/TArray.h"

#include "Rendering/Core/API/CommandQueue.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Threading/API/SpinLock.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	class CommandQueueVK : public TDeviceChildBase<GraphicsDeviceVK, CommandQueue>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, CommandQueue>;

	public:
		CommandQueueVK(const GraphicsDeviceVK* pDevice);
		~CommandQueueVK();

		bool Init(const String& debugName, uint32 queueFamilyIndex, uint32 index);

		void AddWaitSemaphore(VkSemaphore semaphore, VkPipelineStageFlags waitStage);
		void AddSignalSemaphore(VkSemaphore semaphore);

		void FlushBarriers();

		FORCEINLINE VkQueue GetQueue() const
		{
			return m_Queue;
		}

	public:
		// DeviceChild interface
		virtual void SetName(const String& debugName) override final;
		
		// CommandQueue interface
		virtual bool ExecuteCommandLists(const CommandList* const* ppCommandLists, uint32 numCommandLists, FPipelineStageFlags waitStage, const Fence* pWaitFence, uint64 waitValue, Fence* pSignalFence, uint64 signalValue)	override final;
		virtual void Flush()																																																	override final;

		FORCEINLINE virtual uint64 GetHandle() const override final
		{
			return reinterpret_cast<uint64>(m_Queue);
		}
		
	private:
		VkResult InternalFlushBarriers();
		
		void InternalAddWaitSemaphore(VkSemaphore semaphore, VkPipelineStageFlags waitStage);
		void InternalAddSignalSemaphore(VkSemaphore semaphore);

	private:
		VkQueue		m_Queue = VK_NULL_HANDLE;
		SpinLock	m_SpinLock;
		
		TArray<VkCommandBuffer>			m_CommandBuffersToSubmit;
		TArray<VkSemaphore>				m_SignalSemaphores;
		TArray<VkSemaphore>				m_WaitSemaphores;
		TArray<VkPipelineStageFlags>	m_WaitStages;
	};
}
