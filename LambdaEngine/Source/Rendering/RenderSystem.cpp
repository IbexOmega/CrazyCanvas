#include "Log/Log.h"

#include "Rendering/RenderSystem.h"

#include "Rendering/Core/API/ICommandQueue.h"
#include "Rendering/Core/API/IGraphicsDevice.h"
#include "Rendering/Core/API/IBuffer.h"
#include "Rendering/Core/API/ITexture.h"
#include "Rendering/Core/API/IFence.h"
#include "Rendering/Core/API/ISwapChain.h"
#include "Rendering/Core/API/ICommandAllocator.h"
#include "Rendering/Core/API/ICommandList.h"
#include "Rendering/Core/API/ITextureView.h"

#include "Application/API/PlatformApplication.h"

namespace LambdaEngine
{
	IGraphicsDevice*	RenderSystem::s_pGraphicsDevice = nullptr;
	ICommandQueue*		RenderSystem::s_pGraphicsQueue	= nullptr;
	ICommandQueue*		RenderSystem::s_pComputeQueue	= nullptr;
	ICommandQueue*		RenderSystem::s_pCopyQueue		= nullptr;

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

		s_pGraphicsQueue = s_pGraphicsDevice->CreateCommandQueue(ECommandQueueType::COMMAND_QUEUE_GRAPHICS);
		if (!s_pGraphicsQueue)
		{
			return false;
		}

		s_pComputeQueue	= s_pGraphicsDevice->CreateCommandQueue(ECommandQueueType::COMMAND_QUEUE_COMPUTE);
		if (!s_pComputeQueue)
		{
			return false;
		}

		s_pCopyQueue = s_pGraphicsDevice->CreateCommandQueue(ECommandQueueType::COMMAND_QUEUE_COPY);
		if (!s_pCopyQueue)
		{
			return false;
		}
		
		BufferDesc bufferDesc = { };
		bufferDesc.pName		= "VertexBuffer";
		bufferDesc.MemoryType	= EMemoryType::GPU_MEMORY;
		bufferDesc.Flags		= BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | BUFFER_FLAG_COPY_DST;
		bufferDesc.SizeInBytes	= 64;

		IBuffer* pBuffer		= s_pGraphicsDevice->CreateBuffer(bufferDesc);
		uint64 bufferAddress	= pBuffer->GetDeviceAdress();

		UNREFERENCED_VARIABLE(bufferAddress);

		TextureDesc textureDesc = { };
		textureDesc.pName		= "Texture";
		textureDesc.Type		= ETextureType::TEXTURE_2D;
		textureDesc.MemoryType	= EMemoryType::GPU_MEMORY;
		textureDesc.Format		= EFormat::R8G8B8A8_UNORM;
		textureDesc.Flags		= TEXTURE_FLAG_COPY_DST | TEXTURE_FLAG_SHADER_RESOURCE;
		textureDesc.Width		= 256;
		textureDesc.Height		= 256;
		textureDesc.Depth		= 1;
		textureDesc.SampleCount = 1;
		textureDesc.Miplevels	= 1;
		textureDesc.ArrayCount	= 1;

		ITexture* pTexture = s_pGraphicsDevice->CreateTexture(textureDesc);

		SwapChainDesc swapChainDesc = { };
		swapChainDesc.pName			= "Main Window SwapChain";
		swapChainDesc.BufferCount	= 3;
		swapChainDesc.Format		= EFormat::B8G8R8A8_UNORM;
		swapChainDesc.Width			= 0;
		swapChainDesc.Height		= 0;
		swapChainDesc.SampleCount	= 1;

		ISwapChain* pSwapChain = s_pGraphicsDevice->CreateSwapChain(PlatformApplication::Get()->GetWindow(), swapChainDesc);

        const char* names[] =
        {
            "BackBuffer View [0]",
            "BackBuffer View [1]",
            "BackBuffer View [2]",
        };
        
        TextureViewDesc textureViewDesc = { };
        textureViewDesc.Flags           = FTextureViewFlags::TEXTURE_VIEW_FLAG_RENDER_TARGET;
        textureViewDesc.Type            = ETextureViewType::TEXTURE_VIEW_2D;
        textureViewDesc.Miplevel        = 0;
        textureViewDesc.MiplevelCount   = 1;
        textureViewDesc.ArrayIndex      = 0;
        textureViewDesc.ArrayCount      = 1;
        textureViewDesc.Format          = swapChainDesc.Format;
        
        ITextureView* pTextureViews[3];
        for (uint32 i = 0; i < 3; i++)
        {
            textureViewDesc.pName       = names[i];
            textureViewDesc.pTexture    = pSwapChain->GetBuffer(i);
    
            pTextureViews[i] = s_pGraphicsDevice->CreateTextureView(textureViewDesc);
            textureViewDesc.pTexture->Release();
        }
        
        FenceDesc fenceDesc = { };
        fenceDesc.pName         = "Main Fence";
        fenceDesc.InitalValue   = 0;
        
		IFence* pFence = s_pGraphicsDevice->CreateFence(fenceDesc);
        pFence->Signal(1);
            
		ICommandAllocator* pCommandAllocator    = s_pGraphicsDevice->CreateCommandAllocator(ECommandQueueType::COMMAND_QUEUE_GRAPHICS);
        pCommandAllocator->SetName("Graphics Command Allocator");
        
        CommandListDesc commandListDesc = { };
        commandListDesc.pName           = "Primary CommandList";
        commandListDesc.Flags           = FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;
        commandListDesc.CommandListType = ECommandListType::COMMANDLIST_PRIMARY;
        
        ICommandList* pCommandList = s_pGraphicsDevice->CreateCommandList(pCommandAllocator, commandListDesc);
        
        for (uint32 i = 0; i < 3; i++)
        {
            SAFERELEASE(pTextureViews[i]);
        }
        
        SAFERELEASE(pCommandList);
		SAFERELEASE(pCommandAllocator);
		SAFERELEASE(pFence);
		SAFERELEASE(pSwapChain);
		SAFERELEASE(pTexture);
		SAFERELEASE(pBuffer);

		return true;
	}

	bool RenderSystem::Release()
	{
		SAFERELEASE(s_pGraphicsQueue);
		SAFERELEASE(s_pComputeQueue);
		SAFERELEASE(s_pCopyQueue);
		SAFERELEASE(s_pGraphicsDevice);
		return true;
	}
	
	void RenderSystem::Tick()
	{
	}
}
