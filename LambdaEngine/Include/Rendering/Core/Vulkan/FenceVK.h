#pragma once
#include "Rendering/Core/API/IFence.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Containers/TArray.h"
#include "Containers/THashTable.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

    enum class ESemaphoreState : uint8
    {
        SEMAPHORE_STATE_NEW         = 1,
        SEMAPHORE_STATE_WAITING     = 2,
        SEMAPHORE_STATE_SIGNALED    = 3,
    };

    struct FenceValueVk
    {
        VkSemaphore     Semaphore      = VK_NULL_HANDLE;
        ESemaphoreState SemaphoreState = ESemaphoreState::SEMAPHORE_STATE_NEW;
        
        VkFence Fence           = VK_NULL_HANDLE;
        bool    IsFenceSignaled = false;
        
        uint64 Value = 0;
    };

    /*
    * Fences used for systems that does not support Timeline Semaphores
    */
	class FenceVK : public TDeviceChildBase<GraphicsDeviceVK, IFence>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, IFence>;

	public:
        FenceVK(const GraphicsDeviceVK* pDevice);
		~FenceVK();

        bool Init(const FenceDesc* pDesc);

        VkFence     GetSignalFence(uint64 value)                const;
        VkSemaphore WaitGPU(uint64 waitValue, VkQueue queue)    const;
        VkSemaphore SignalGPU(uint64 signalValue, VkQueue queue);

        // IDeviceChild interface
        virtual void SetName(const char* pName) override final;

        // IFence interface
        virtual void Wait(uint64 waitValue, uint64 timeOut) const override final;
        virtual void Signal(uint64 signalValue)                   override final;

        FORCEINLINE virtual uint64 GetValue() const override final
        {
            return m_LastCompletedValue;
        }

        FORCEINLINE virtual FenceDesc GetDesc() const override final
        {
            return m_Desc;
        }

    private:
        VkFence     QueryFence()     const;
        VkSemaphore QuerySemaphore() const;
        
        void DisposeFence(VkFence fence)             const;
        void DisposeSemaphore(VkSemaphore semaphore) const;
        
        void AddWaitSemaphore(VkSemaphore semaphore, VkPipelineStageFlags waitStage) const;
        
        void FlushWaitSemaphores(VkQueue queue) const;
        
        /*
         * Debug tools
         */
        bool FenceCanReset(VkFence fence) const;
        
    private:
        mutable TArray<FenceValueVk>    m_PendingValues;
        mutable TArray<VkFence>         m_Fences;
        mutable TArray<VkSemaphore>     m_Semaphores;
        
        mutable TArray<VkSemaphore>          m_WaitSemaphores;
        mutable TArray<VkPipelineStageFlags> m_WaitStages;
        
        mutable uint64  m_LastCompletedValue;
        FenceDesc       m_Desc;
	};
}
