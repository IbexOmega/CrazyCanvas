#pragma once
#include "Rendering/Core/API/Fence.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Containers/TArray.h"
#include "Containers/THashTable.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	enum class ESemaphoreState : uint8
	{
		SEMAPHORE_STATE_NEW			= 1,
		SEMAPHORE_STATE_WAITING		= 2,
		SEMAPHORE_STATE_SIGNALED	= 3,
	};

	struct FenceValueVk
	{
		uint64 Value = 0;
		VkFence Fence = VK_NULL_HANDLE;
		VkSemaphore Semaphore = VK_NULL_HANDLE;
		ESemaphoreState SemaphoreState = ESemaphoreState::SEMAPHORE_STATE_NEW;
	};

	/*
	* Fences used for systems that does not support Timeline Semaphores
	*/
	class FenceVK : public TDeviceChildBase<GraphicsDeviceVK, Fence>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, Fence>;

	public:
		FenceVK(const GraphicsDeviceVK* pDevice);
		~FenceVK();

		bool Init(const FenceDesc* pDesc);

		VkFence		GetSignalFence(uint64 value)				const;
		VkSemaphore WaitGPU(uint64 waitValue, VkQueue queue)	const;
		VkSemaphore SignalGPU(uint64 signalValue, VkQueue queue);

	public:
		// DeviceChild interface
		virtual void SetName(const String& name) override final;

		// Fence interface
		virtual void Wait(uint64 waitValue, uint64 timeOut) const	override final;
		virtual void Reset(uint64 resetValue)						override final;

		FORCEINLINE virtual uint64 GetValue() const override final
		{
			return m_LastCompletedValue;
		}

	private:
		VkFence		QueryFence()		const;
		VkSemaphore QuerySemaphore()	const;
		
		void DisposeFence(VkFence fence)				const;
		void DisposeSemaphore(VkSemaphore semaphore)	const;
		
		void AddWaitSemaphore(VkSemaphore semaphore, VkPipelineStageFlags waitStage) const;
		
		void FlushWaitSemaphores(VkQueue queue) const;
		
		/*
		 * Debug tools
		 */
		bool FenceCanReset(VkFence fence) const;
		
	private:
		mutable TArray<FenceValueVk>	m_PendingValues;
		mutable TArray<VkFence>			m_Fences;
		mutable TArray<VkSemaphore>		m_Semaphores;
		
		mutable TArray<VkSemaphore>				m_WaitSemaphores;
		mutable TArray<VkPipelineStageFlags>	m_WaitStages;
		
		mutable uint64 m_LastCompletedValue;
	};
}
