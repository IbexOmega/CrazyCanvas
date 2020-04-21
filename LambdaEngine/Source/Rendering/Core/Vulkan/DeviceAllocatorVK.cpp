#include "Rendering/Core/Vulkan/DeviceAllocatorVK.h"

namespace LambdaEngine
{
    DeviceAllocatorVK::DeviceAllocatorVK(const GraphicsDeviceVK* pDevice)
        : TDeviceChild(pDevice),
        m_Statistics(),
        m_Desc()
    {
    }

    DeviceAllocatorVK::~DeviceAllocatorVK()
    {
    }

    bool DeviceAllocatorVK::Init(const DeviceAllocatorDesc* pDesc)
    {
        VALIDATE(pDesc != nullptr);
        return true;
    }

    bool DeviceAllocatorVK::Allocate(AllocationVK* pAllocation)
    {
        VALIDATE(pAllocation != nullptr);
        return true;
    }

    bool DeviceAllocatorVK::Free(AllocationVK* pAllocation)
    {
        VALIDATE(pAllocation != nullptr);
        return true;
    }

    void DeviceAllocatorVK::SetName(const char* pName)
    {
    }
}
