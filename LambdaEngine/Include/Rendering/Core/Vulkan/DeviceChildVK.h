#pragma once
#include "IDeviceChild.h"

namespace LambdaEngine
{
    class GraphicsDeviceVK;

    class DeviceChildVK : public virtual IDeviceChild
    {
    public:
        DeviceChildVK(const GraphicsDeviceVK* pDevice);
        virtual ~DeviceChildVK() = default;
        
        virtual uint64 Release()    override;
        virtual uint64 AddRef()     override;
        
    protected:
        const GraphicsDeviceVK* const m_pDevice;
        
    private:
        uint64 m_StrongReferences;
    };
}
