#pragma once
#include "Containers/TArray.h"

#include "Rendering/Core/API/ICommandQueue.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Threading/API/SpinLock.h"

#include "Vulkan.h"

#define MAX_COMMANDBUFFERS 32

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	class CommandQueueVK : public TDeviceChildBase<GraphicsDeviceVK, ICommandQueue>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, ICommandQueue>;

	public:
		CommandQueueVK(const GraphicsDeviceVK* pDevice);
		~CommandQueueVK();

		bool Init(const char* pName, uint32 queueFamilyIndex, uint32 index);

		void AddWaitSemaphore(VkSemaphore semaphore, VkPipelineStageFlags waitStage);
		void AddSignalSemaphore(VkSemaphore semaphore);

		void FlushBarriers();

		FORCEINLINE VkQueue GetQueue() const
		{
			return m_Queue;
		}

		// IDeviceChild interface
		virtual void SetName(const char* pName) override final;
		
		// ICommandQueue interface
		virtual bool ExecuteCommandLists(const ICommandList* const* ppCommandLists, uint32 numCommandLists, FPipelineStageFlags waitStage, const IFence* pWaitFence, uint64 waitValue, IFence* pSignalFence, uint64 signalValue) override final;
		virtual void Flush() override final;

		FORCEINLINE virtual uint64 GetHandle() const override final
		{
			return (uint64)m_Queue;
		}
        
    private:
        VkResult InternalFlushBarriers();
        
        void InternalAddWaitSemaphore(VkSemaphore semaphore, VkPipelineStageFlags waitStage);
        void InternalAddSignalSemaphore(VkSemaphore semaphore);

	private:
		VkQueue	        m_Queue = VK_NULL_HANDLE;
		VkCommandBuffer m_SubmitCommandBuffers[MAX_COMMANDBUFFERS];

		TArray<VkSemaphore>             m_SignalSemaphores;
		TArray<VkSemaphore>             m_WaitSemaphores;
		TArray<VkPipelineStageFlags>    m_WaitStages;
        
        SpinLock m_SpinLock;
	};
}
