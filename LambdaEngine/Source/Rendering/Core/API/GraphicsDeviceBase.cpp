#include "Rendering/Core/API/GraphicsDeviceBase.h"

namespace LambdaEngine
{
    GraphicsDeviceBase::GraphicsDeviceBase()
        : IGraphicsDevice()
    {
    }

    GraphicsDeviceBase::~GraphicsDeviceBase()
    {
    }

    void GraphicsDeviceBase::DestroyObject(IDeviceChild* pObject)
    {
        m_GarbageObjects.emplace_back(pObject);
    }
}
