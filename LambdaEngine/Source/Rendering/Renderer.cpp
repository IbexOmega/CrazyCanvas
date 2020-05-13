#include "Rendering/Renderer.h"
#include "Rendering/Core/API/IGraphicsDevice.h"
#include "Rendering/Core/API/ICommandAllocator.h"
#include "Rendering/Core/API/ICommandQueue.h"
#include "Rendering/Core/API/ICommandList.h"
#include "Rendering/Core/API/ISwapChain.h"
#include "Rendering/Core/API/ITexture.h"
#include "Rendering/Core/API/ITextureView.h"
#include "Rendering/RenderSystem.h"
#include "Rendering/RenderGraph.h"
#include "Rendering/ImGuiRenderer.h"

#include "Application/API/PlatformApplication.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	Renderer::Renderer(const IGraphicsDevice* pGraphicsDevice) :
		m_pGraphicsDevice(pGraphicsDevice)
	{
	}

	Renderer::~Renderer()
	{
		

		if (m_pSwapChain != nullptr)
		{
			for (uint32 i = 0; i < m_BackBufferCount; i++)
			{
				SAFERELEASE(m_ppBackBuffers[i]);
				SAFERELEASE(m_ppBackBufferViews[i]);
				SAFERELEASE(m_ppImGuiCommandLists[i]);
				SAFERELEASE(m_ppImGuiCommandAllocators[i]);
			}

			SAFEDELETE_ARRAY(m_ppBackBuffers);
			SAFEDELETE_ARRAY(m_ppBackBufferViews);
			SAFEDELETE_ARRAY(m_ppImGuiCommandLists);
			SAFEDELETE_ARRAY(m_ppImGuiCommandAllocators);
		}

		SAFERELEASE(m_pSwapChain);
		SAFEDELETE(m_pImGuiRenderer);
	}

	bool Renderer::Init(const RendererDesc* pDesc)
	{
		VALIDATE(pDesc);
		VALIDATE(pDesc->pWindow);

		m_pName				= pDesc->pName;
		m_pRenderGraph		= pDesc->pRenderGraph;
		m_BackBufferCount	= pDesc->BackBufferCount;

		SwapChainDesc swapChainDesc = {};
		swapChainDesc.pName			= "Renderer Swap Chain";
		swapChainDesc.Format		= EFormat::FORMAT_B8G8R8A8_UNORM;
		swapChainDesc.Width			= 0;
		swapChainDesc.Height		= 0;
		swapChainDesc.BufferCount	= m_BackBufferCount;
		swapChainDesc.SampleCount	= 1;
		swapChainDesc.VerticalSync	= false;
		
		m_pSwapChain = m_pGraphicsDevice->CreateSwapChain(pDesc->pWindow, RenderSystem::GetGraphicsQueue(), &swapChainDesc);

		if (m_pSwapChain == nullptr)
		{
			LOG_ERROR("[Renderer]: SwapChain is nullptr after initializaiton for \"%s\"", m_pName);
			return false;
		}

		uint32 backBufferCount	= m_pSwapChain->GetDesc().BufferCount;
		m_ppBackBuffers			= DBG_NEW ITexture*[backBufferCount];
		m_ppBackBufferViews		= DBG_NEW ITextureView*[backBufferCount];

		for (uint32 v = 0; v < backBufferCount; v++)
		{
			ITexture* pBackBuffer	= m_pSwapChain->GetBuffer(v);
			m_ppBackBuffers[v]		= pBackBuffer;

			TextureViewDesc textureViewDesc = {};
			textureViewDesc.pName			= "Renderer Back Buffer Texture View";
			textureViewDesc.pTexture		= pBackBuffer;
			textureViewDesc.Flags			= FTextureViewFlags::TEXTURE_VIEW_FLAG_RENDER_TARGET;
			textureViewDesc.Format			= EFormat::FORMAT_B8G8R8A8_UNORM;
			textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_2D;
			textureViewDesc.MiplevelCount	= 1;
			textureViewDesc.ArrayCount		= 1;
			textureViewDesc.Miplevel		= 0;
			textureViewDesc.ArrayIndex		= 0;
			
			ITextureView* pBackBufferView	= m_pGraphicsDevice->CreateTextureView(&textureViewDesc);

			if (pBackBufferView == nullptr)
			{
				LOG_ERROR("[Renderer]: Could not create Back Buffer View of Back Buffer Index %u in Renderer \"%s\"", v, m_pName);
				return false;
			}
			
			m_ppBackBufferViews[v]			= pBackBufferView;
		}
		
		ResourceUpdateDesc resourceUpdateDesc = {};
		resourceUpdateDesc.pResourceName						= RENDER_GRAPH_BACK_BUFFER_ATTACHMENT;
		resourceUpdateDesc.ExternalTextureUpdate.ppTextures		= m_ppBackBuffers;
		resourceUpdateDesc.ExternalTextureUpdate.ppTextureViews	= m_ppBackBufferViews;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
		
		if (pDesc->Debug)
		{
			//ImGui Renderer
			m_pImGuiRenderer = DBG_NEW ImGuiRenderer(RenderSystem::GetDevice());

			ImGuiRendererDesc guiRendererDesc = {};
			guiRendererDesc.pWindow				= pDesc->pWindow;
			guiRendererDesc.BackBufferCount		= m_BackBufferCount;
			guiRendererDesc.VertexBufferSize	= MEGA_BYTE(8);
			guiRendererDesc.IndexBufferSize		= MEGA_BYTE(8);

			m_pImGuiRenderer->Init(&guiRendererDesc);

			m_ppImGuiCommandAllocators	= DBG_NEW ICommandAllocator*[backBufferCount];
			m_ppImGuiCommandLists		= DBG_NEW ICommandList*[backBufferCount];

			CommandListDesc commandListDesc = {};
			commandListDesc.pName			= "ImGui Copy Command List";
			commandListDesc.CommandListType = ECommandListType::COMMAND_LIST_PRIMARY;
			commandListDesc.Flags			= FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

			for (uint32 b = 0; b < backBufferCount; b++)
			{
				ICommandAllocator* pCommandAllocator	= m_pGraphicsDevice->CreateCommandAllocator("ImGui Rendering Command Allocator", ECommandQueueType::COMMAND_QUEUE_GRAPHICS);
				
				m_ppImGuiCommandAllocators[b]			= pCommandAllocator;
				m_ppImGuiCommandLists[b]				= m_pGraphicsDevice->CreateCommandList(pCommandAllocator, &commandListDesc);
			}

			
			m_pImGuiRenderer->Begin(1.0f / 60.0f, pDesc->pWindow->GetWidth(), pDesc->pWindow->GetHeight(), 1.0f, 1.0f);
		}

		return true;
	}
	
	void Renderer::Render(Timestamp delta)
	{
		m_BackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();

		m_pRenderGraph->Render(m_ModFrameIndex, m_BackBufferIndex);

		if (m_pImGuiRenderer != nullptr)
		{
			m_pImGuiRenderer->End();

			ICommandAllocator* pCommandAllocator = m_ppImGuiCommandAllocators[m_ModFrameIndex];
			ICommandList* pCommandList = m_ppImGuiCommandLists[m_ModFrameIndex];

			pCommandAllocator->Reset();
			pCommandList->Begin(nullptr);

			m_pImGuiRenderer->Render(pCommandList, m_ppBackBufferViews[m_BackBufferIndex], m_ModFrameIndex, m_BackBufferIndex);

			pCommandList->End();

			IFence* pFence;
			uint64 signalValue;

			m_pRenderGraph->GetAndIncrementFence(&pFence, &signalValue);
			RenderSystem::GetGraphicsQueue()->ExecuteCommandLists(&pCommandList, 1, FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, pFence, signalValue - 1, pFence, signalValue);

			IWindow* pWindow = PlatformApplication::Get()->GetMainWindow();
			m_pImGuiRenderer->Begin(delta, pWindow->GetWidth(), pWindow->GetHeight(), 1.0f, 1.0f);
		}

		m_pSwapChain->Present();

		m_FrameIndex++;
		m_ModFrameIndex = m_FrameIndex % m_BackBufferCount;
	}
}
