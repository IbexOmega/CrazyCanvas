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

		bool Init(uint64 signalValue);

		virtual void Wait(uint64 signalValue, uint64 timeOut) const	override;
		virtual void Signal(uint64 signalValue)		override;

		virtual uint64 GetValue() const override;

		virtual void SetName(const char* pName) override;
		
	private:
		VkSemaphore m_Semaphore = VK_NULL_HANDLE;
	};
}