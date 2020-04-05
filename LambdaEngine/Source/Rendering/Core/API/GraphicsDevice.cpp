#include "Rendering/Core/API/IGraphicsDevice.h"

#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"

namespace LambdaEngine
{
    IGraphicsDevice* CreateGraphicsDevice(const GraphicsDeviceDesc& desc, EGraphicsAPI api)
    {
        IGraphicsDevice* pDevice = nullptr;
        if (api == EGraphicsAPI::VULKAN)
        {
            pDevice = DBG_NEW GraphicsDeviceVK();
        }

        if (pDevice)
        {
            if (pDevice->Init(desc))
            {
                return pDevice;
            }
        }
        
        return nullptr;
    }
}
