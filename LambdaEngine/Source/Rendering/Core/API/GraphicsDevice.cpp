#include "Rendering/Core/API/IGraphicsDevice.h"

#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"

namespace LambdaEngine
{
    IGraphicsDevice* CreateGraphicsDevice(EGraphicsAPI api, const GraphicsDeviceDesc& desc)
    {
        if (api == EGraphicsAPI::VULKAN)
        {
            GraphicsDeviceVK* pDevice = DBG_NEW GraphicsDeviceVK();
            if (pDevice->Init(desc))
            {
                return pDevice;
            }
            else
            {
                return nullptr;
            }
        }
        else
        {
            return nullptr;
        }
    }
}
