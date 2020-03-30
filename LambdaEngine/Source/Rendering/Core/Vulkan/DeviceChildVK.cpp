#include "Rendering/Core/Vulkan/DeviceChildVK.h"

namespace LambdaEngine
{
    DeviceChildVK::DeviceChildVK(const GraphicsDeviceVK* pDevice)
        : IDeviceChild(),
        m_pDevice(pDevice),
        m_StrongReferences(0)
    {
    }

    uint64 DeviceChildVK::Release()
    {
        //TODO: This needs to be synced with a mutex of some kind
        uint64 strongReferences = --m_StrongReferences;
        if (strongReferences < 1)
        {
            delete this;
        }
        
        return strongReferences;
    }

    uint64 DeviceChildVK::AddRef()
    {
        //TODO: This needs to be syncted with a mutex of some kind
        return ++m_StrongReferences;
    }
}
