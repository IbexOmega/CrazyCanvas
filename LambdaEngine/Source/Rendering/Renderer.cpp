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

		return true;
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

		m_pRenderGraph->Render();

		m_SwapChain->Present();
		
		m_FrameIndex++;
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
}
