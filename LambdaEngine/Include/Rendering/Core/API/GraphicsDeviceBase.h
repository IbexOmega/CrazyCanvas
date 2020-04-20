#pragma once
#include "IGraphicsDevice.h"

#include "Containers/TArray.h"

namespace LambdaEngine
{
    class IDeviceChild;

    class GraphicsDeviceBase : public IGraphicsDevice
    {
    public:
        DECL_UNIQUE_CLASS(GraphicsDeviceBase);
        
        GraphicsDeviceBase();
        ~GraphicsDeviceBase();
        
        void DestroyObject(IDeviceChild* pObject);
        
    private:
        TArray<IDeviceChild*> m_GarbageObjects;
    };
}
