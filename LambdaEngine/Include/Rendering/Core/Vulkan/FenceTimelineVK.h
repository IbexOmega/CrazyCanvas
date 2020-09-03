#pragma once
#include "Rendering/Core/API/Fence.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	class FenceTimelineVK : public TDeviceChildBase<GraphicsDeviceVK, Fence>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, Fence>;

	public:
		FenceTimelineVK(const GraphicsDeviceVK* pDevice);
		~FenceTimelineVK();

		bool Init(const FenceDesc* pDesc);

		FORCEINLINE VkSemaphore GetSemaphore() const
		{
			return m_Semaphore;
		}
		
	public:
		// DeviceChild interface
		virtual void SetName(const String& name) override final;
		
		// Fence interface
		virtual void Wait(uint64 waitValue, uint64 timeOut)	const	override final;
		virtual void Reset(uint64 resetValue)						override final;

		virtual uint64 GetValue() const override final;
		
	private:
		VkSemaphore m_Semaphore = VK_NULL_HANDLE;
	};
}
