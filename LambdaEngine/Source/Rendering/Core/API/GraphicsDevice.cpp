#include "Rendering/Core/API/GraphicsDevice.h"

#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"

#include <unordered_map>

namespace LambdaEngine
{
	GraphicsDevice* CreateGraphicsDevice(EGraphicsAPI api, const GraphicsDeviceDesc* pDesc)
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
