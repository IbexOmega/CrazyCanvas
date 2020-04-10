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
#include "Rendering/Core/API/IRenderPass.h"
#include "Rendering/Core/API/IPipelineLayout.h"
#include "Rendering/Core/API/IFrameBuffer.h"
#include "Rendering/Core/API/IDescriptorHeap.h"

#include "Application/API/PlatformApplication.h"

namespace LambdaEngine
{
	IGraphicsDevice*	RenderSystem::s_pGraphicsDevice = nullptr;
	ICommandQueue*		RenderSystem::s_pGraphicsQueue	= nullptr;
	ICommandQueue*		RenderSystem::s_pComputeQueue	= nullptr;
	ICommandQueue*		RenderSystem::s_pCopyQueue		= nullptr;

	bool RenderSystem::Init()
	{
        return true;
        
		GraphicsDeviceDesc deviceDesc = { };
#ifndef LAMBDA_PRODUCTION
		deviceDesc.Debug = true;
#else
		deviceDesc.Debug = false;
#endif

		s_pGraphicsDevice = CreateGraphicsDevice(EGraphicsAPI::VULKAN, deviceDesc);
		if (!s_pGraphicsDevice)
		{
			return false;
		}

		s_pGraphicsQueue = s_pGraphicsDevice->CreateCommandQueue("Graphics Queue", ECommandQueueType::COMMAND_QUEUE_GRAPHICS);
		if (!s_pGraphicsQueue)
		{
			return false;
		}

		s_pComputeQueue	= s_pGraphicsDevice->CreateCommandQueue("Compute Queue", ECommandQueueType::COMMAND_QUEUE_COMPUTE);
		if (!s_pComputeQueue)
		{
			return false;
		}

		s_pCopyQueue = s_pGraphicsDevice->CreateCommandQueue("Copy Queue", ECommandQueueType::COMMAND_QUEUE_COPY);
		if (!s_pCopyQueue)
		{
			return false;
		}
		
		BufferDesc bufferDesc = { };
		bufferDesc.pName		= "VertexBuffer";
		bufferDesc.MemoryType	= EMemoryType::MEMORY_GPU;
		bufferDesc.Flags		= BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | BUFFER_FLAG_COPY_SRC | BUFFER_FLAG_COPY_DST;
		bufferDesc.SizeInBytes	= 64;

		IBuffer* pBuffer = s_pGraphicsDevice->CreateBuffer(bufferDesc);

		TextureDesc textureDesc = { };
		textureDesc.pName		= "Texture";
		textureDesc.Type		= ETextureType::TEXTURE_2D;
		textureDesc.MemoryType	= EMemoryType::MEMORY_GPU;
		textureDesc.Format		= EFormat::FORMAT_R8G8B8A8_UNORM;
		textureDesc.Flags		= TEXTURE_FLAG_COPY_SRC | TEXTURE_FLAG_COPY_DST | TEXTURE_FLAG_SHADER_RESOURCE;
		textureDesc.Width		= 256;
		textureDesc.Height		= 256;
		textureDesc.Depth		= 1;
		textureDesc.SampleCount = 1;
		textureDesc.Miplevels	= 9;
		textureDesc.ArrayCount	= 1;

		ITexture* pTexture = s_pGraphicsDevice->CreateTexture(textureDesc);

		SwapChainDesc swapChainDesc = { };
		swapChainDesc.pName			= "Main Window SwapChain";
		swapChainDesc.BufferCount	= 3;
		swapChainDesc.Format		= EFormat::FORMAT_B8G8R8A8_UNORM;
		swapChainDesc.Width			= 0;
		swapChainDesc.Height		= 0;
		swapChainDesc.SampleCount	= 1;

		ISwapChain* pSwapChain = s_pGraphicsDevice->CreateSwapChain(PlatformApplication::Get()->GetWindow(), s_pGraphicsQueue, swapChainDesc);
		swapChainDesc = pSwapChain->GetDesc();

		RenderPassAttachmentDesc attachmentDesc = { };
		attachmentDesc.Format			= swapChainDesc.Format;
		attachmentDesc.InitialState		= ETextureState::TEXTURE_STATE_DONT_CARE;
		attachmentDesc.FinalState		= ETextureState::TEXTURE_STATE_PRESENT;
		attachmentDesc.LoadOp			= ELoadOp::DONT_CARE;
		attachmentDesc.StoreOp			= EStoreOp::STORE;
		attachmentDesc.SampleCount		= 1;
		attachmentDesc.StencilLoadOp	= ELoadOp::DONT_CARE;
		attachmentDesc.StencilStoreOp	= EStoreOp::DONT_CARE;

		RenderPassSubpassDesc subpassDesc = { };
		subpassDesc.DepthStencilAttachmentState = ETextureState::TEXTURE_STATE_DONT_CARE;
		subpassDesc.InputAttachmentCount		= 0;
		subpassDesc.pInputAttachmentStates		= nullptr;
		subpassDesc.pResolveAttachmentStates	= nullptr;

		ETextureState renderTargetStates[] = { ETextureState::TEXTURE_STATE_RENDER_TARGET };
		subpassDesc.pRenderTargetStates = renderTargetStates;
		subpassDesc.RenderTargetCount	= 1;

		RenderPassSubpassDependencyDesc subpassDependencyDesc = {};
		subpassDependencyDesc.DstSubpass	= 0;
		subpassDependencyDesc.DstAccessMask	= FAccessFlags::ACCESS_FLAG_MEMORY_READ | FAccessFlags::ACCESS_FLAG_MEMORY_WRITE;
		subpassDependencyDesc.DstStageMask	= FPipelineStageFlags::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;
		subpassDependencyDesc.SrcSubpass	= EXTERNAL_SUBPASS;
		subpassDependencyDesc.SrcStageMask	= FPipelineStageFlags::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;
		subpassDependencyDesc.SrcAccessMask = 0;

		RenderPassDesc renderPassDesc = { };
		renderPassDesc.pName					= "Main RenderPass";
		renderPassDesc.AttachmentCount			= 1;
		renderPassDesc.pAttachments				= &attachmentDesc;
		renderPassDesc.SubpassCount				= 1;
		renderPassDesc.pSubpasses				= &subpassDesc;
		renderPassDesc.SubpassDependencyCount	= 1;
		renderPassDesc.pSubpassDependencies		= &subpassDependencyDesc;

		IRenderPass* pRenderPass = s_pGraphicsDevice->CreateRenderPass(renderPassDesc);

		DescriptorHeapDesc descriptorHeapDesc = { };
		descriptorHeapDesc.pName = "DescriptorHeap";
		descriptorHeapDesc.DescriptorCount.DescriptorSetCount						= 256;
		descriptorHeapDesc.DescriptorCount.AccelerationStructureDescriptorCount		= 256;
		descriptorHeapDesc.DescriptorCount.ConstantBufferDescriptorCount			= 256;
		descriptorHeapDesc.DescriptorCount.SamplerDescriptorCount					= 256;
		descriptorHeapDesc.DescriptorCount.TextureCombinedSamplerDescriptorCount	= 256;
		descriptorHeapDesc.DescriptorCount.TextureDescriptorCount					= 256;
		descriptorHeapDesc.DescriptorCount.UnorderedAccessBufferDescriptorCount		= 256;
		descriptorHeapDesc.DescriptorCount.UnorderedAccessTextureDescriptorCount	= 256;

		IDescriptorHeap* pDescriptorHeap = s_pGraphicsDevice->CreateDescriptorHeap(descriptorHeapDesc);

		DescriptorBindingDesc binding = { };
		binding.DescriptorType		= EDescriptorType::DESCRIPTOR_CONSTANT_BUFFER;
		binding.Binding				= 0;
		binding.DescriptorCount		= 1;
		binding.ppImmutableSamplers = nullptr;
		binding.ShaderStageMask		= FShaderStageFlags::SHADER_STAGE_FLAG_MESH_SHADER;

		DescriptorSetLayoutDesc descriptorSetLayout = { };
		descriptorSetLayout.DescriptorBindingCount	= 1;
		descriptorSetLayout.pDescriptorBindings		= &binding;

		ConstantRangeDesc constantRange = { };
		constantRange.ShaderStageFlags	= FShaderStageFlags::SHADER_STAGE_FLAG_MESH_SHADER;
		constantRange.OffsetInBytes		= 0;
		constantRange.SizeInBytes		= 64;

		PipelineLayoutDesc pipelineLayoutDesc = { };
		pipelineLayoutDesc.pName					= "PipelineLayout";
		pipelineLayoutDesc.pDescriptorSetLayouts	= &descriptorSetLayout;
		pipelineLayoutDesc.DescriptorSetLayoutCount = 1;
		pipelineLayoutDesc.pConstantRanges			= &constantRange;
		pipelineLayoutDesc.ConstantRangeCount		= 1;

		IPipelineLayout* pPipelineLayout = s_pGraphicsDevice->CreatePipelineLayout(pipelineLayoutDesc);

        const char* textureViewNames[] =
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
        
		const char* frameBufferNames[] =
		{
			"BackBuffer FrameBuffer [0]",
			"BackBuffer FrameBuffer [1]",
			"BackBuffer FrameBuffer [2]",
		};

		FrameBufferDesc frameBufferDesc = { };
		frameBufferDesc.pDepthStencil		= nullptr;
		frameBufferDesc.Width				= swapChainDesc.Width;
		frameBufferDesc.Height				= swapChainDesc.Height;
		frameBufferDesc.RenderTargetCount	= 1;

        ITextureView* ppTextureViews[3];
		IFrameBuffer* ppFrameBuffers[3];
        for (uint32 i = 0; i < 3; i++)
        {
			//TextureView
            textureViewDesc.pName       = textureViewNames[i];
            textureViewDesc.pTexture    = pSwapChain->GetBuffer(i);
    
            ITextureView* pTextureView = s_pGraphicsDevice->CreateTextureView(textureViewDesc);
			ppTextureViews[i] = pTextureView;
			textureViewDesc.pTexture->Release();

			//FrameBuffer
			frameBufferDesc.pName			= frameBufferNames[i];
			frameBufferDesc.ppRenderTargets = &pTextureView;
			ppFrameBuffers[i] = s_pGraphicsDevice->CreateFrameBuffer(pRenderPass, frameBufferDesc);
        }
        
        FenceDesc fenceDesc = { };
        fenceDesc.pName         = "Main Fence";
        fenceDesc.InitalValue   = 0;
        
		IFence* pFence = s_pGraphicsDevice->CreateFence(fenceDesc);

		ICommandAllocator* pCommandAllocator = s_pGraphicsDevice->CreateCommandAllocator("Graphics Command Allocator", ECommandQueueType::COMMAND_QUEUE_GRAPHICS);
        
        CommandListDesc commandListDesc = { };
        commandListDesc.pName           = "Primary CommandList";
        commandListDesc.Flags           = FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;
        commandListDesc.CommandListType = ECommandListType::COMMANDLIST_PRIMARY;
        
        ICommandList* pCommandList = s_pGraphicsDevice->CreateCommandList(pCommandAllocator, commandListDesc);
		pCommandList->Begin(nullptr);
		pCommandList->GenerateMiplevels(pTexture, ETextureState::TEXTURE_STATE_DONT_CARE, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY);
		pCommandList->End();

		uint64 waitValue	= pFence->GetValue();
		uint64 signalValue	= waitValue + 1;

		//s_pGraphicsQueue->ExecuteCommandLists(&pCommandList, 1, PIPELINE_STAGE_FLAG_TOP, pFence, waitValue, pFence, signalValue);
		//pSwapChain->Present();

		//pFence->Wait(signalValue, UINT64_MAX_);

		pCommandAllocator->Reset();

        for (uint32 i = 0; i < 3; i++)
        {
			SAFERELEASE(ppFrameBuffers[i]);
            SAFERELEASE(ppTextureViews[i]);
        }
        
		SAFERELEASE(pDescriptorHeap);
		SAFERELEASE(pRenderPass);
		SAFERELEASE(pPipelineLayout);
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
