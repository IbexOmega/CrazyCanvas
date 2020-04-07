#pragma once
#include "Rendering/Core/API/IFence.h"
#include "Rendering/Core/API/DeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

    /*
    * Fences used for systems that does not support Timeline Semaphores
    */
	class FenceLegacyVK : public DeviceChildBase<GraphicsDeviceVK, IFence>
	{
		using TDeviceChild = DeviceChildBase<GraphicsDeviceVK, IFence>;

	public:
        FenceLegacyVK(const GraphicsDeviceVK* pDevice);
		~FenceLegacyVK();

        bool Init(const FenceDesc& desc);

        FORCEINLINE VkSemaphore GetSemaphore() const
        {
            return m_Semaphore;
        }

        //IDeviceChild interface
        virtual void SetName(const char* pName) override;

        //IFence interface
        virtual void Wait(uint64 signalValue, uint64 timeOut) const override;
        virtual void Signal(uint64 signalValue)                     override;

        virtual uint64 GetValue() const override;

        FORCEINLINE virtual FenceDesc GetDesc() const override
        {
            return m_Desc;
        }

    private:
        VkSemaphore m_Semaphore = VK_NULL_HANDLE;
        FenceDesc   m_Desc;
	};
}