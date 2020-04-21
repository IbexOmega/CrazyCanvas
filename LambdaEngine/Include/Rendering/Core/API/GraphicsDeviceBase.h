#pragma once
#include "IGraphicsDevice.h"

#include "Threading/API/SpinLock.h"

#include "Containers/TArray.h"

namespace LambdaEngine
{
    class IDeviceChild;

    class GraphicsDeviceBase : public IGraphicsDevice
    {
    public:
        DECL_UNIQUE_CLASS(GraphicsDeviceBase);
        
        GraphicsDeviceBase();
        ~GraphicsDeviceBase() = default;
        
        void DestroyObject(IDeviceChild* pObject) const;
        
        //IGraphicsDevice interface
        virtual void Release() override;
        
    private:
        mutable TArray<IDeviceChild*>   m_GarbageObjects;
        mutable SpinLock                m_GarbageLock;
    };
}
