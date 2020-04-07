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

		bool Init(const FenceDesc& desc);

        FORCEINLINE VkSemaphore GetSemaphore() const
        {
            return m_Semaphore;
        }
        
        //IDeviceChild interface
        virtual void SetName(const char* pName) override final;
        
        //IFence interface
        virtual void Wait(uint64 signalValue, uint64 timeOut) const override final;
        virtual void Signal(uint64 signalValue)                     override final;

        virtual uint64 GetValue() const override final;
        
        FORCEINLINE virtual FenceDesc GetDesc() const override final
        {
            return m_Desc;
        }
		
	private:
		VkSemaphore m_Semaphore = VK_NULL_HANDLE;
        FenceDesc   m_Desc;
	};
}
