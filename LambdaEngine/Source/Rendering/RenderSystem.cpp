#include "Rendering/RenderSystem.h"

#include "Rendering/Core/API/IGraphicsDevice.h"

namespace LambdaEngine
{
	IGraphicsDevice*	RenderSystem::s_pGraphicsDevice = nullptr;
	IQueue*				RenderSystem::s_pGraphicsQueue = nullptr;

	bool RenderSystem::Init()
	{
		GraphicsDeviceDesc deviceDesc = { };
#ifndef LAMBDA_PRODUCTION
		deviceDesc.Debug = true;
#else
		deviceDesc.Debug = false;
#endif

		s_pGraphicsDevice = CreateGraphicsDevice(deviceDesc, EGraphicsAPI::VULKAN);
		if (!s_pGraphicsDevice)
		{
			return false;
		}

		s_pGraphicsQueue = s_pGraphicsDevice->CreateQueue(EQueueType::QUEUE_GRAPHICS);
		return (s_pGraphicsQueue != nullptr);
	}

	bool RenderSystem::Release()
	{
		SAFERELEASE(s_pGraphicsDevice);
		return true;
	}
	
	void RenderSystem::Tick()
	{
	}
}