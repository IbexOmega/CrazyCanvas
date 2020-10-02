#include "Log/Log.h"

#include "Rendering/RenderAPI.h"
#include "Rendering/PipelineStateManager.h"

#include "Rendering/Core/API/CommandQueue.h"
#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/Buffer.h"
#include "Rendering/Core/API/Texture.h"
#include "Rendering/Core/API/Fence.h"
#include "Rendering/Core/API/SwapChain.h"
#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/TextureView.h"
#include "Rendering/Core/API/RenderPass.h"
#include "Rendering/Core/API/PipelineLayout.h"
#include "Rendering/Core/API/DescriptorHeap.h"
#include "Rendering/Core/API/DescriptorSet.h"
#include "Rendering/Core/API/AccelerationStructure.h"

#include "Application/API/CommonApplication.h"

namespace LambdaEngine
{
	GraphicsDevice* RenderAPI::s_pGraphicsDevice = nullptr;
	
	TSharedRef<CommandQueue> RenderAPI::s_GraphicsQueue	= nullptr;
	TSharedRef<CommandQueue> RenderAPI::s_ComputeQueue	= nullptr;
	TSharedRef<CommandQueue> RenderAPI::s_CopyQueue		= nullptr;

	/*
	* RenderAPI
	*/

	bool RenderAPI::Init()
	{
		GraphicsDeviceDesc deviceDesc = { };
#ifdef LAMBDA_DEVELOPMENT
		deviceDesc.Debug = true;
#else
		deviceDesc.Debug = false;
#endif

		s_pGraphicsDevice = CreateGraphicsDevice(EGraphicsAPI::VULKAN, &deviceDesc);
		if (!s_pGraphicsDevice)
		{
			return false;
		}

		s_GraphicsQueue = s_pGraphicsDevice->CreateCommandQueue("Graphics Queue", ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);
		if (!s_GraphicsQueue)
		{
			return false;
		}

		s_ComputeQueue	= s_pGraphicsDevice->CreateCommandQueue("Compute Queue", ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE);
		if (!s_ComputeQueue)
		{
			return false;
		}

		s_CopyQueue = s_pGraphicsDevice->CreateCommandQueue("Copy Queue", ECommandQueueType::COMMAND_QUEUE_TYPE_COPY);
		if (!s_CopyQueue)
		{
			return false;
		}

		if (!Sampler::InitDefaults())
		{
			return false;
		}

		if (!PipelineStateManager::Init())
		{
			return false;
		}

		return true;
	}

	bool RenderAPI::Release()
	{
		s_GraphicsQueue->Flush();
		s_ComputeQueue->Flush();
		s_CopyQueue->Flush();

		Sampler::ReleaseDefaults();
		PipelineStateManager::Release();

		SAFERELEASE(s_pGraphicsDevice);
		return true;
	}

	void RenderAPI::Tick()
	{
	}
}
