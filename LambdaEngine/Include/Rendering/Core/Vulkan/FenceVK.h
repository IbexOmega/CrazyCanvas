#pragma once
#include "Rendering/Core/API/IFence.h"
#include "Rendering/Core/API/DeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	class FenceVK : public DeviceChildBase<GraphicsDeviceVK, IFence>
	{
		using TDeviceChild = DeviceChildBase<GraphicsDeviceVK, IFence>;

	public:
		FenceVK(const GraphicsDeviceVK* pDevice);
		~FenceVK();

		bool Init();

		virtual void Wait() const	override;
		virtual void Signal()		override;

		FORCEINLINE virtual uint64 GetValue() const override
		{
			return m_FenceValue;
		}
		
	private:
		VkSemaphore m_Semaphore		= VK_NULL_HANDLE;
		uint64		m_FenceValue	= 0;
	};
}