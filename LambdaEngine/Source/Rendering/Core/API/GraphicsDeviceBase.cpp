#include "Rendering/Core/API/GraphicsDeviceBase.h"
#include "Rendering/Core/API/IDeviceChild.h"

#include <mutex>

namespace LambdaEngine
{
    GraphicsDeviceBase::GraphicsDeviceBase()
        : IGraphicsDevice(),
        m_GarbageObjects()
    {
    }

    void GraphicsDeviceBase::DestroyObject(IDeviceChild* pObject) const
    {
        std::scoped_lock<SpinLock> lock(m_GarbageLock);
        m_GarbageObjects.emplace_back(pObject);
    }

    void GraphicsDeviceBase::Release()
    {
        TArray<IDeviceChild*> garbageObjects;
        {
            std::scoped_lock<SpinLock> lock(m_GarbageLock);
            
            garbageObjects = TArray<IDeviceChild*>(m_GarbageObjects);
            m_GarbageObjects.clear();
        }
        
        for (IDeviceChild* pObject : garbageObjects)
        {
            SAFEDELETE(pObject);
        }
        garbageObjects.clear();
        
        //Call release again if any new objects were added since objects can hold references to eachother
        if (!m_GarbageObjects.empty())
        {
            GraphicsDeviceBase::Release();
        }
    }
}
