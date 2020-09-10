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
#include "Rendering/RenderGraphSerializer.h"
#include "Rendering/ImGuiRenderer.h"
#include "Game/Scene.h"

#include "Application/API/Window.h"
#include "Application/API/CommonApplication.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	TSharedRef<SwapChain>	Renderer::s_SwapChain			= nullptr;
	Texture**				Renderer::s_ppBackBuffers		= nullptr;
	TextureView**			Renderer::s_ppBackBufferViews	= nullptr;
	RenderGraph*			Renderer::s_pRenderGraph		= nullptr;
	Scene*					Renderer::s_pScene				= nullptr;
	uint64					Renderer::s_FrameIndex			= 0;
	uint64					Renderer::s_ModFrameIndex		= 0;
	uint32					Renderer::s_BackBufferIndex		= 0;

	bool Renderer::Init()
	{
		//Create Swapchain
		{
			SwapChainDesc swapChainDesc = {};
			swapChainDesc.DebugName		= "Renderer Swap Chain";
			swapChainDesc.pWindow		= CommonApplication::Get()->GetActiveWindow().Get();
			swapChainDesc.pQueue		= RenderSystem::GetGraphicsQueue();
			swapChainDesc.Format		= EFormat::FORMAT_B8G8R8A8_UNORM;
			swapChainDesc.Width			= 0;
			swapChainDesc.Height		= 0;
			swapChainDesc.BufferCount	= BACK_BUFFER_COUNT;
			swapChainDesc.SampleCount	= 1;
			swapChainDesc.VerticalSync	= false;
		
			s_SwapChain = RenderSystem::GetDevice()->CreateSwapChain(&swapChainDesc);
			if (!s_SwapChain)
			{
				LOG_ERROR("[Renderer]: SwapChain is nullptr after initializaiton");
				return false;
			}

			s_ppBackBuffers			= DBG_NEW Texture*[BACK_BUFFER_COUNT];
			s_ppBackBufferViews		= DBG_NEW TextureView*[BACK_BUFFER_COUNT];
		}

		//Create RenderGraph
		{
			RenderGraphStructureDesc renderGraphStructure = {};

			if (!RenderGraphSerializer::LoadAndParse(&renderGraphStructure, "CUBEMAPTEST.lrg", IMGUI_ENABLED))
			{
				return false;
			}

			RenderGraphDesc renderGraphDesc = {};
			renderGraphDesc.pRenderGraphStructureDesc		= &renderGraphStructure;
			renderGraphDesc.BackBufferCount					= BACK_BUFFER_COUNT;
			renderGraphDesc.MaxTexturesPerDescriptorSet		= MAX_TEXTURES_PER_DESCRIPTOR_SET;

			s_pRenderGraph = DBG_NEW RenderGraph(RenderSystem::GetDevice());
			s_pRenderGraph->Init(&renderGraphDesc);
		}

		//Update RenderGraph wit 
		{
			for (uint32 v = 0; v < BACK_BUFFER_COUNT; v++)
			{
				Texture* pBackBuffer	= s_SwapChain->GetBuffer(v);
				s_ppBackBuffers[v]		= pBackBuffer;

				TextureViewDesc textureViewDesc = {};
				textureViewDesc.DebugName		= "Renderer Back Buffer Texture View";
				textureViewDesc.pTexture		= pBackBuffer;
				textureViewDesc.Flags			= FTextureViewFlags::TEXTURE_VIEW_FLAG_RENDER_TARGET;
				textureViewDesc.Format			= EFormat::FORMAT_B8G8R8A8_UNORM;
				textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_TYPE_2D;
				textureViewDesc.MiplevelCount	= 1;
				textureViewDesc.ArrayCount		= 1;
				textureViewDesc.Miplevel		= 0;
				textureViewDesc.ArrayIndex		= 0;
			
				TextureView* pBackBufferView = RenderSystem::GetDevice()->CreateTextureView(&textureViewDesc);
				if (pBackBufferView == nullptr)
				{
					LOG_ERROR("[Renderer]: Could not create Back Buffer View of Back Buffer Index %u in Renderer", v);
					return false;
				}
			
				s_ppBackBufferViews[v] = pBackBufferView;
			}
		
			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName							= RENDER_GRAPH_BACK_BUFFER_ATTACHMENT;
			resourceUpdateDesc.ExternalTextureUpdate.ppTextures		= s_ppBackBuffers;
			resourceUpdateDesc.ExternalTextureUpdate.ppTextureViews	= s_ppBackBufferViews;

			s_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}

		return true;
	}

	bool Renderer::Release()
	{
		if (s_SwapChain)
		{
			for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
			{
				SAFERELEASE(s_ppBackBuffers[i]);
				SAFERELEASE(s_ppBackBufferViews[i]);
			}

			SAFEDELETE_ARRAY(s_ppBackBuffers);
			SAFEDELETE_ARRAY(s_ppBackBufferViews);
			s_SwapChain.Reset();
		}

		SAFEDELETE(s_pRenderGraph);


		return true;
	}

	void Renderer::SetScene(Scene* pScene)
	{
		s_pScene = pScene;

		s_pRenderGraph->SetScene(s_pScene);

		{
			Buffer* pBuffer = s_pScene->GetLightsBuffer();
			ResourceUpdateDesc resourceUpdateDesc				= {};
			resourceUpdateDesc.ResourceName						= SCENE_LIGHTS_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

			s_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}

		{
			Buffer* pBuffer = s_pScene->GetPerFrameBuffer();
			ResourceUpdateDesc resourceUpdateDesc				= {};
			resourceUpdateDesc.ResourceName						= PER_FRAME_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

			s_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}

		{
			Buffer* pBuffer = s_pScene->GetMaterialProperties();
			ResourceUpdateDesc resourceUpdateDesc				= {};
			resourceUpdateDesc.ResourceName						= SCENE_MAT_PARAM_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

			s_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}

		{
			Buffer* pBuffer = s_pScene->GetVertexBuffer();
			ResourceUpdateDesc resourceUpdateDesc				= {};
			resourceUpdateDesc.ResourceName						= SCENE_VERTEX_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

			s_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}

		{
			Buffer* pBuffer = s_pScene->GetIndexBuffer();
			ResourceUpdateDesc resourceUpdateDesc				= {};
			resourceUpdateDesc.ResourceName						= SCENE_INDEX_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

			s_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}

		{
			Buffer* pBuffer = s_pScene->GetPrimaryInstanceBuffer();
			ResourceUpdateDesc resourceUpdateDesc				= {};
			resourceUpdateDesc.ResourceName						= SCENE_PRIMARY_INSTANCE_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

			s_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}

		{
			Buffer* pBuffer = s_pScene->GetSecondaryInstanceBuffer();
			ResourceUpdateDesc resourceUpdateDesc				= {};
			resourceUpdateDesc.ResourceName						= SCENE_SECONDARY_INSTANCE_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

			s_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}

		{
			Buffer* pBuffer = s_pScene->GetIndirectArgsBuffer();
			ResourceUpdateDesc resourceUpdateDesc				= {};
			resourceUpdateDesc.ResourceName						= SCENE_INDIRECT_ARGS_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

			s_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}

		{
			Texture** ppAlbedoMaps						= s_pScene->GetAlbedoMaps();
			Texture** ppNormalMaps						= s_pScene->GetNormalMaps();
			Texture** ppAmbientOcclusionMaps			= s_pScene->GetAmbientOcclusionMaps();
			Texture** ppMetallicMaps					= s_pScene->GetMetallicMaps();
			Texture** ppRoughnessMaps					= s_pScene->GetRoughnessMaps();

			TextureView** ppAlbedoMapViews				= s_pScene->GetAlbedoMapViews();
			TextureView** ppNormalMapViews				= s_pScene->GetNormalMapViews();
			TextureView** ppAmbientOcclusionMapViews	= s_pScene->GetAmbientOcclusionMapViews();
			TextureView** ppMetallicMapViews			= s_pScene->GetMetallicMapViews();
			TextureView** ppRoughnessMapViews			= s_pScene->GetRoughnessMapViews();

			std::vector<Sampler*> nearestSamplers(MAX_UNIQUE_MATERIALS, Sampler::GetNearestSampler());

			ResourceUpdateDesc albedoMapsUpdateDesc = {};
			albedoMapsUpdateDesc.ResourceName								= SCENE_ALBEDO_MAPS;
			albedoMapsUpdateDesc.ExternalTextureUpdate.ppTextures			= ppAlbedoMaps;
			albedoMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews		= ppAlbedoMapViews;
			albedoMapsUpdateDesc.ExternalTextureUpdate.ppSamplers			= nearestSamplers.data();

			ResourceUpdateDesc normalMapsUpdateDesc = {};
			normalMapsUpdateDesc.ResourceName								= SCENE_NORMAL_MAPS;
			normalMapsUpdateDesc.ExternalTextureUpdate.ppTextures			= ppNormalMaps;
			normalMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews		= ppNormalMapViews;
			normalMapsUpdateDesc.ExternalTextureUpdate.ppSamplers			= nearestSamplers.data();

			ResourceUpdateDesc aoMapsUpdateDesc = {};
			aoMapsUpdateDesc.ResourceName									= SCENE_AO_MAPS;
			aoMapsUpdateDesc.ExternalTextureUpdate.ppTextures				= ppAmbientOcclusionMaps;
			aoMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews			= ppAmbientOcclusionMapViews;
			aoMapsUpdateDesc.ExternalTextureUpdate.ppSamplers				= nearestSamplers.data();

			ResourceUpdateDesc metallicMapsUpdateDesc = {};
			metallicMapsUpdateDesc.ResourceName								= SCENE_METALLIC_MAPS;
			metallicMapsUpdateDesc.ExternalTextureUpdate.ppTextures			= ppMetallicMaps;
			metallicMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews		= ppMetallicMapViews;
			metallicMapsUpdateDesc.ExternalTextureUpdate.ppSamplers			= nearestSamplers.data();

			ResourceUpdateDesc roughnessMapsUpdateDesc = {};
			roughnessMapsUpdateDesc.ResourceName							= SCENE_ROUGHNESS_MAPS;
			roughnessMapsUpdateDesc.ExternalTextureUpdate.ppTextures		= ppRoughnessMaps;
			roughnessMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews	= ppRoughnessMapViews;
			roughnessMapsUpdateDesc.ExternalTextureUpdate.ppSamplers		= nearestSamplers.data();

			s_pRenderGraph->UpdateResource(albedoMapsUpdateDesc);
			s_pRenderGraph->UpdateResource(normalMapsUpdateDesc);
			s_pRenderGraph->UpdateResource(aoMapsUpdateDesc);
			s_pRenderGraph->UpdateResource(metallicMapsUpdateDesc);
			s_pRenderGraph->UpdateResource(roughnessMapsUpdateDesc);
		}

		if (s_pScene->IsRayTracingEnabled())
		{
			const AccelerationStructure* pTLAS = s_pScene->GetTLAS();
			ResourceUpdateDesc resourceUpdateDesc					= {};
			resourceUpdateDesc.ResourceName							= SCENE_TLAS;
			resourceUpdateDesc.ExternalAccelerationStructure.pTLAS	= pTLAS;

			s_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}
	}

	void Renderer::NewFrame(Timestamp delta)
	{
		s_pRenderGraph->Update();
		s_pRenderGraph->NewFrame(s_ModFrameIndex, s_BackBufferIndex, delta);
	}

	void Renderer::PrepareRender(Timestamp delta)
	{
		CommandList* pGraphicsCopyCommandList	= s_pRenderGraph->AcquireGraphicsCopyCommandList();
		CommandList* pComputeCopyCommandList	= s_pRenderGraph->AcquireGraphicsCopyCommandList();

		s_pScene->PrepareRender(pGraphicsCopyCommandList, pComputeCopyCommandList, s_FrameIndex, delta);

		s_pRenderGraph->PrepareRender(delta);
	}
	
	void Renderer::Render()
	{
		s_BackBufferIndex = uint32(s_SwapChain->GetCurrentBackBufferIndex());

		s_pRenderGraph->Render();

		s_SwapChain->Present();
		
		s_FrameIndex++;
		s_ModFrameIndex = s_FrameIndex % uint64(BACK_BUFFER_COUNT);
	}

	CommandList* Renderer::AcquireGraphicsCopyCommandList()
	{
		return s_pRenderGraph->AcquireGraphicsCopyCommandList();
	}

	CommandList* Renderer::AcquireComputeCopyCommandList()
	{
		return s_pRenderGraph->AcquireComputeCopyCommandList();
	}
}
