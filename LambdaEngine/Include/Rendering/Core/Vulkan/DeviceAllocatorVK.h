#pragma once
#include "Rendering/Core/API/IDeviceAllocator.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
    class GraphicsDeviceVK;

    struct DeviceMemoryBlockVK;

    struct AllocationVK
    {
        DeviceMemoryBlockVK*    pBlock  = nullptr;
        VkDeviceMemory          Memory  = VK_NULL_HANDLE;
        uint64                  Offset  = 0;
    };

    class DeviceAllocatorVK : public TDeviceChildBase<GraphicsDeviceVK, IDeviceAllocator>
    {
        using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, IDeviceAllocator>;
        
    public:
        DeviceAllocatorVK(const GraphicsDeviceVK* pDevice);
        ~DeviceAllocatorVK();
        
        bool Init(const DeviceAllocatorDesc* pDesc);
        
        bool Allocate(AllocationVK* pAllocation);
        bool Free(AllocationVK* pAllocation);
        
        // IDeviceChild Interface
        virtual void SetName(const char* pName) override final;
        
        // IDeviceAllocator Interface
        FORCEINLINE virtual DeviceAllocatorStatistics GetStatistics() const override final
        {
            return m_Statistics;
        }
        
        FORCEINLINE virtual DeviceAllocatorDesc GetDesc() const override final
        {
            return m_Desc;
        }
        
    private:
        DeviceAllocatorStatistics   m_Statistics;
        DeviceAllocatorDesc         m_Desc;
    };
}
