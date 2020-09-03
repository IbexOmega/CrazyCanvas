#include "Rendering/Renderer.h"
#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandQueue.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/SwapChain.h"
#include "Rendering/Core/API/Texture.h"
#include "Rendering/Core/API/TextureView.h"
#include "Rendering/RenderSystem.h"
#include "Rendering/RenderGraph.h"
#include "Rendering/ImGuiRenderer.h"

#include "Application/API/Window.h"
#include "Application/API/CommonApplication.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	Renderer::Renderer(const GraphicsDevice* pGraphicsDevice) 
		: m_Name()
		, m_pGraphicsDevice(pGraphicsDevice)
	{
	}

	Renderer::~Renderer()
	{
		if (m_SwapChain)
		{
			for (uint32 i = 0; i < m_BackBufferCount; i++)
			{
				SAFERELEASE(m_ppBackBuffers[i]);
				SAFERELEASE(m_ppBackBufferViews[i]);
			}

			SAFEDELETE_ARRAY(m_ppBackBuffers);
			SAFEDELETE_ARRAY(m_ppBackBufferViews);
		}
	}

	bool Renderer::Init(const RendererDesc* pDesc)
	{
		VALIDATE(pDesc);
		VALIDATE(pDesc->pWindow);
		VALIDATE(pDesc->pRenderGraph);

		m_Name				= pDesc->Name;
		m_pRenderGraph		= pDesc->pRenderGraph;
		m_BackBufferCount	= pDesc->BackBufferCount;

		SwapChainDesc swapChainDesc = {};
		swapChainDesc.DebugName		= "Renderer Swap Chain";
		swapChainDesc.Window		= pDesc->pWindow;
		swapChainDesc.Queue			= RenderSystem::GetGraphicsQueue();
		swapChainDesc.Format		= EFormat::FORMAT_B8G8R8A8_UNORM;
		swapChainDesc.Width			= 0;
		swapChainDesc.Height		= 0;
		swapChainDesc.BufferCount	= m_BackBufferCount;
		swapChainDesc.SampleCount	= 1;
		swapChainDesc.VerticalSync	= false;
		
		m_SwapChain = m_pGraphicsDevice->CreateSwapChain(&swapChainDesc);
		if (!m_SwapChain)
		{
			LOG_ERROR("[Renderer]: SwapChain is nullptr after initializaiton for \"%s\"", m_Name.c_str());
			return false;
		}

		uint32 backBufferCount	= m_SwapChain->GetDesc().BufferCount;
		m_ppBackBuffers			= DBG_NEW Texture*[backBufferCount];
		m_ppBackBufferViews		= DBG_NEW TextureView*[backBufferCount];

		for (uint32 v = 0; v < backBufferCount; v++)
		{
			Texture* pBackBuffer	= m_SwapChain->GetBuffer(v);
			m_ppBackBuffers[v]		= pBackBuffer;

			TextureViewDesc textureViewDesc = {};
			textureViewDesc.DebugName		= "Renderer Back Buffer Texture View";
			textureViewDesc.Texture			= pBackBuffer;
			textureViewDesc.Flags			= FTextureViewFlags::TEXTURE_VIEW_FLAG_RENDER_TARGET;
			textureViewDesc.Format			= EFormat::FORMAT_B8G8R8A8_UNORM;
			textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_TYPE_2D;
			textureViewDesc.MiplevelCount	= 1;
			textureViewDesc.ArrayCount		= 1;
			textureViewDesc.Miplevel		= 0;
			textureViewDesc.ArrayIndex		= 0;
			
			TextureView* pBackBufferView = m_pGraphicsDevice->CreateTextureView(&textureViewDesc);
			if (pBackBufferView == nullptr)
			{
				LOG_ERROR("[Renderer]: Could not create Back Buffer View of Back Buffer Index %u in Renderer \"%s\"", v, m_Name.c_str());
				return false;
			}
			
			m_ppBackBufferViews[v] = pBackBufferView;
		}
		
		ResourceUpdateDesc resourceUpdateDesc = {};
		resourceUpdateDesc.ResourceName							= RENDER_GRAPH_BACK_BUFFER_ATTACHMENT;
		resourceUpdateDesc.ExternalTextureUpdate.ppTextures		= m_ppBackBuffers;
		resourceUpdateDesc.ExternalTextureUpdate.ppTextureViews	= m_ppBackBufferViews;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);

		return InitDebugRender();
	}

	void Renderer::NewFrame(Timestamp delta)
	{
		m_pRenderGraph->NewFrame(m_ModFrameIndex, m_BackBufferIndex, delta);
	}

	void Renderer::PrepareRender(Timestamp delta)
	{
		m_pRenderGraph->PrepareRender(delta);
	}
	
	void Renderer::Render()
	{
		m_BackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

		CommandAllocator* allocator = m_CommandAllocators[m_BackBufferIndex].Get();
		CommandList* list = m_CommandLists[m_BackBufferIndex].Get();
		
		uint64 waitValue = std::max<int64>(int64(m_FrameIndex) - 2, 0);
		m_Fence->Wait(waitValue, UINT64_MAX);

		allocator->Reset();
		list->Begin(nullptr);

		BeginRenderPassDesc begin;
		begin.pRenderPass	= m_RenderPass.Get();
		begin.Flags			= 0;
		begin.Width			= m_SwapChain->GetDesc().Width;
		begin.Height		= m_SwapChain->GetDesc().Height;
		begin.Offset		= { 0, 0 };
		begin.pDepthStencil	= nullptr;

		const TextureView* ppRenderTargets[] = { m_ppBackBufferViews[m_BackBufferIndex] };
		begin.ppRenderTargets	= ppRenderTargets;
		begin.RenderTargetCount	= 1;
		
		ClearColorDesc clearColor;
		clearColor.Color[0] = 1.0f;
		clearColor.Color[1] = 1.0f;
		clearColor.Color[2] = 1.0f;
		clearColor.Color[3] = 1.0f;
		
		begin.pClearColors		= &clearColor;
		begin.ClearColorCount	= 1;
		
		list->BeginRenderPass(&begin);
		
		Viewport viewport;
		viewport.x			= 0;
		viewport.y			= 0;
		viewport.MinDepth	= 0.0f;
		viewport.MaxDepth	= 1.0f;
		viewport.Width		= begin.Width;
		viewport.Height		= begin.Height;
		
		list->SetViewports(&viewport, 0, 1);

		ScissorRect rect;
		rect.x		= 0.0f;
		rect.y		= 0.0f;
		rect.Width	= begin.Width;
		rect.Height	= begin.Height;

		list->SetScissorRects(&rect, 0, 1);

		list->BindGraphicsPipeline(m_PipelineState.Get());

		list->DrawInstanced(3, 1, 0, 0);

		list->EndRenderPass();
		list->End();

		//m_pRenderGraph->Render();

		uint64 oldIndex = m_FrameIndex++;

		CommandQueue* queue = RenderSystem::GetGraphicsQueue();
		queue->ExecuteCommandLists(&list, 1, FPipelineStageFlags::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT, m_Fence.Get(), oldIndex, m_Fence.Get(), m_FrameIndex);

		LOG_INFO("BackbufferIndex=%d", m_BackBufferIndex);

		m_SwapChain->Present();
		m_ModFrameIndex = m_FrameIndex % uint64(m_BackBufferCount);
	}

	CommandList* Renderer::AcquireGraphicsCopyCommandList()
	{
		return m_pRenderGraph->AcquireGraphicsCopyCommandList();
	}

	CommandList* Renderer::AcquireComputeCopyCommandList()
	{
		return m_pRenderGraph->AcquireComputeCopyCommandList();
	}
	
	bool Renderer::InitDebugRender()
	{
		for (uint32 i = 0; i < 3; i++)
		{
			m_CommandAllocators[i] = m_pGraphicsDevice->CreateCommandAllocator("CommandAllocator", ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);
			if (!m_CommandAllocators[i])
			{
				return false;
			}

			CommandListDesc desc;
			desc.DebugName			= "CommandList";
			desc.CommandListType	= ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
			desc.Flags				= 0;
			m_CommandLists[i] = m_pGraphicsDevice->CreateCommandList(m_CommandAllocators[i].Get(), &desc);
			if (!m_CommandLists[i])
			{
				return false;
			}
		}

		RenderPassAttachmentDesc colorAttachmentDesc = { };
		colorAttachmentDesc.Format			= EFormat::FORMAT_B8G8R8A8_UNORM;
		colorAttachmentDesc.SampleCount		= 1;
		colorAttachmentDesc.LoadOp			= ELoadOp::LOAD_OP_CLEAR;
		colorAttachmentDesc.StoreOp			= EStoreOp::STORE_OP_STORE;
		colorAttachmentDesc.StencilLoadOp	= ELoadOp::LOAD_OP_DONT_CARE;
		colorAttachmentDesc.StencilStoreOp	= EStoreOp::STORE_OP_DONT_CARE;
		colorAttachmentDesc.InitialState	= ETextureState::TEXTURE_STATE_DONT_CARE;
		colorAttachmentDesc.FinalState		= ETextureState::TEXTURE_STATE_PRESENT;

		RenderPassSubpassDesc subpassDesc = { };
		subpassDesc.RenderTargetStates			= { ETextureState::TEXTURE_STATE_RENDER_TARGET };
		subpassDesc.DepthStencilAttachmentState = ETextureState::TEXTURE_STATE_DONT_CARE;

		RenderPassSubpassDependencyDesc subpassDependencyDesc = { };
		subpassDependencyDesc.SrcSubpass	= EXTERNAL_SUBPASS;
		subpassDependencyDesc.DstSubpass	= 0;
		subpassDependencyDesc.SrcAccessMask	= 0;
		subpassDependencyDesc.DstAccessMask = FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_READ | FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
		subpassDependencyDesc.SrcStageMask	= FPipelineStageFlags::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;
		subpassDependencyDesc.DstStageMask	= FPipelineStageFlags::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;

		RenderPassDesc renderPassDesc = { };
		renderPassDesc.DebugName			= "RenderPass";
		renderPassDesc.Attachments			= { colorAttachmentDesc };
		renderPassDesc.Subpasses			= { subpassDesc };
		renderPassDesc.SubpassDependencies	= { subpassDependencyDesc };

		m_RenderPass = m_pGraphicsDevice->CreateRenderPass(&renderPassDesc);
		if (!m_RenderPass)
		{
			return false;
		}

		String vsSource = 
			R"(
				#version 450
				#extension GL_ARB_separate_shader_objects : enable

				vec2 positions[3] = vec2[]
				(
					vec2(0.0, -0.5),
					vec2(0.5, 0.5),
					vec2(-0.5, 0.5)
				);

				void main() 
				{
					gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
				}
			)";

		TSharedRef<Shader> vs = ResourceLoader::LoadShaderFromMemory(vsSource, "", FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, EShaderLang::SHADER_LANG_GLSL);
		if (!vs)
		{
			return false;
		}

		String psSource = 
			R"(
				#version 450
				#extension GL_ARB_separate_shader_objects : enable

				layout(location = 0) out vec4 outColor;

				void main() 
				{
					outColor = vec4(1.0f, 0.0f, 0.0f, 1.0);
				}
			)";

		TSharedRef<Shader> ps = ResourceLoader::LoadShaderFromMemory(psSource, "", FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER, EShaderLang::SHADER_LANG_GLSL);
		if (!vs)
		{
			return false;
		}

		PipelineLayoutDesc piplineLayoutDesc = { };
		piplineLayoutDesc.DebugName = "PipelineLayout";

		m_PiplineLayout = m_pGraphicsDevice->CreatePipelineLayout(&piplineLayoutDesc);
		if (!m_PiplineLayout)
		{
			return false;
		}

		GraphicsPipelineStateDesc pipelineStateDesc = { };
		pipelineStateDesc.DebugName = "Pipeline";

		pipelineStateDesc.InputAssembly.PrimitiveRestartEnable	= false;
		pipelineStateDesc.InputAssembly.PrimitiveTopology		= EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		
		pipelineStateDesc.DepthStencilState.MinDepthBounds			= 0.0f;
		pipelineStateDesc.DepthStencilState.MaxDepthBounds			= 1.0f;
		pipelineStateDesc.DepthStencilState.DepthBoundsTestEnable	= false;
		pipelineStateDesc.DepthStencilState.DepthTestEnable			= false;
		pipelineStateDesc.DepthStencilState.DepthWriteEnable		= false;
		pipelineStateDesc.DepthStencilState.StencilTestEnable		= false;
		
		pipelineStateDesc.BlendState.AlphaToCoverageEnable	= false;
		pipelineStateDesc.BlendState.AlphaToOneEnable		= false;
		pipelineStateDesc.BlendState.LogicOpEnable			= false;

		pipelineStateDesc.BlendState.BlendAttachmentStates.Resize(1);
		pipelineStateDesc.BlendState.BlendAttachmentStates[0].BlendEnabled = false;
		pipelineStateDesc.BlendState.BlendAttachmentStates[0].RenderTargetComponentMask = 
			COLOR_COMPONENT_FLAG_R | 
			COLOR_COMPONENT_FLAG_G | 
			COLOR_COMPONENT_FLAG_B | 
			COLOR_COMPONENT_FLAG_A;

		pipelineStateDesc.RasterizerState.CullMode					= ECullMode::CULL_MODE_NONE;
		pipelineStateDesc.RasterizerState.RasterizerDiscardEnable	= false;
		pipelineStateDesc.RasterizerState.DepthBiasClamp			= 0.0f;
		pipelineStateDesc.RasterizerState.DepthBiasConstantFactor	= 0.0f;
		pipelineStateDesc.RasterizerState.DepthBiasEnable			= false;
		pipelineStateDesc.RasterizerState.DepthClampEnable			= false;
		pipelineStateDesc.RasterizerState.FrontFaceCounterClockWise	= false;
		pipelineStateDesc.RasterizerState.LineWidth					= 1.0f;
		pipelineStateDesc.RasterizerState.MultisampleEnable			= false;
		pipelineStateDesc.RasterizerState.PolygonMode				= EPolygonMode::POLYGON_MODE_FILL;

		pipelineStateDesc.SampleCount			= 1;
		pipelineStateDesc.SampleMask			= 0;
		pipelineStateDesc.Subpass				= 0;
		pipelineStateDesc.pRenderPass			= m_RenderPass.Get();
		pipelineStateDesc.pPipelineLayout		= m_PiplineLayout.Get();
		pipelineStateDesc.VertexShader.pShader	= vs.Get();
		pipelineStateDesc.PixelShader.pShader	= ps.Get();

		m_PipelineState = m_pGraphicsDevice->CreateGraphicsPipelineState(&pipelineStateDesc);
		if (!m_PipelineState)
		{
			return false;
		}

		FenceDesc fenceDesc = { };
		fenceDesc.DebugName		= "Fence";
		fenceDesc.InitalValue	= 0;

		m_Fence = m_pGraphicsDevice->CreateFence(&fenceDesc);
		if (!m_Fence)
		{
			return false;
		}

		return true;
	}
}
