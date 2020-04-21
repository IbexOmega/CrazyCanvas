#include "Rendering/Core/API/IGraphicsDevice.h"

#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"

namespace LambdaEngine
{
    IGraphicsDevice* CreateGraphicsDevice(EGraphicsAPI api, const GraphicsDeviceDesc* pDesc)
    {
        VALIDATE(pDesc != nullptr);
        
        if (api == EGraphicsAPI::VULKAN)
        {
            GraphicsDeviceVK* pDevice = DBG_NEW GraphicsDeviceVK();
            if (pDevice->Init(pDesc))
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
