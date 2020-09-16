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

#include "Engine/EngineConfig.h"

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

			s_FrameIndex++;
			s_ModFrameIndex = s_FrameIndex % uint64(BACK_BUFFER_COUNT);
		}

		//Create RenderGraph
		{
			RenderGraphStructureDesc renderGraphStructure = {};

			if (!RenderGraphSerializer::LoadAndParse(&renderGraphStructure, EngineConfig::GetStringProperty("RenderGraphName"), IMGUI_ENABLED))
			{
				return false;
			}

			//Todo: Move this
			{
				RenderGraphShaderConstants& pointLightsConstants = renderGraphStructure.ShaderConstants["POINT_LIGHT_SHADOWMAPS"];
				pointLightsConstants.Graphics.PixelShaderConstants.PushBack({ 2 });

				RenderGraphShaderConstants& shadingConstants = renderGraphStructure.ShaderConstants["DEMO"];
				shadingConstants.Graphics.PixelShaderConstants.PushBack({ 2 });
			}

			RenderGraphDesc renderGraphDesc = {};
			renderGraphDesc.Name							= "Default Rendergraph";
			renderGraphDesc.pRenderGraphStructureDesc		= &renderGraphStructure;
			renderGraphDesc.BackBufferCount					= BACK_BUFFER_COUNT;
			renderGraphDesc.MaxTexturesPerDescriptorSet		= MAX_TEXTURES_PER_DESCRIPTOR_SET;

			s_pRenderGraph = DBG_NEW RenderGraph(RenderSystem::GetDevice());
			s_pRenderGraph->Init(&renderGraphDesc);
		}

		//Update RenderGraph with Back Buffer
		{
			for (uint32 v = 0; v < BACK_BUFFER_COUNT; v++)
			{
				s_ppBackBuffers[v]		= s_SwapChain->GetBuffer(v);
				s_ppBackBufferViews[v]	= s_SwapChain->GetBufferView(v);
			}

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName							= RENDER_GRAPH_BACK_BUFFER_ATTACHMENT;
			resourceUpdateDesc.ExternalTextureUpdate.ppTextures		= s_ppBackBuffers;
			resourceUpdateDesc.ExternalTextureUpdate.ppTextureViews	= s_ppBackBufferViews;

			s_pRenderGraph->UpdateResource(&resourceUpdateDesc);
		}

		return true;
	}

	bool Renderer::Release()
	{
		SAFEDELETE(s_pRenderGraph);

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

		return true;
	}

	void Renderer::SetRenderGraph(const String& name, RenderGraphStructureDesc* pRenderGraphStructureDesc)
	{
		RenderGraphDesc renderGraphDesc = {};
		renderGraphDesc.Name							= name;
		renderGraphDesc.pRenderGraphStructureDesc		= pRenderGraphStructureDesc;
		renderGraphDesc.BackBufferCount					= BACK_BUFFER_COUNT;
		renderGraphDesc.MaxTexturesPerDescriptorSet		= MAX_TEXTURES_PER_DESCRIPTOR_SET;

		if (!s_pRenderGraph->Recreate(&renderGraphDesc))
		{
			LOG_ERROR("[Renderer]: Failed to set new RenderGraph %s", name.c_str());
		}

		if (s_pScene != nullptr)
		{
			UpdateRenderGraphFromScene();
		}
	}

	void Renderer::SetScene(Scene* pScene)
	{
		s_pScene = pScene;
		s_pRenderGraph->SetScene(s_pScene);
		UpdateRenderGraphFromScene();
	}

	void Renderer::Render()
	{
		s_BackBufferIndex = uint32(s_SwapChain->GetCurrentBackBufferIndex());

		s_pRenderGraph->Update();

		CommandList* pGraphicsCopyCommandList = s_pRenderGraph->AcquireGraphicsCopyCommandList();
		CommandList* pComputeCopyCommandList = s_pRenderGraph->AcquireComputeCopyCommandList();

		if (s_pScene != nullptr)
		{
			s_pScene->PrepareRender(pGraphicsCopyCommandList, pComputeCopyCommandList, s_FrameIndex, s_ModFrameIndex);
		}

		s_pRenderGraph->Render(s_ModFrameIndex, s_BackBufferIndex);

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

	void Renderer::UpdateRenderGraphFromScene()
	{
		//{
		//	Buffer* pBuffer = s_pScene->GetLightsBuffer();
		//	ResourceUpdateDesc resourceUpdateDesc				= {};
		//	resourceUpdateDesc.ResourceName						= SCENE_LIGHTS_BUFFER;
		//	resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		//	s_pRenderGraph->UpdateResource(&resourceUpdateDesc);
		//}

		{
			Buffer* pBuffer = s_pScene->GetPerFrameBuffer();
			ResourceUpdateDesc resourceUpdateDesc				= {};
			resourceUpdateDesc.ResourceName						= PER_FRAME_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

			s_pRenderGraph->UpdateResource(&resourceUpdateDesc);
		}

		{
			Buffer* pBuffer = s_pScene->GetMaterialProperties();
			ResourceUpdateDesc resourceUpdateDesc				= {};
			resourceUpdateDesc.ResourceName						= SCENE_MAT_PARAM_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

			s_pRenderGraph->UpdateResource(&resourceUpdateDesc);
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

			s_pRenderGraph->UpdateResource(&albedoMapsUpdateDesc);
			s_pRenderGraph->UpdateResource(&normalMapsUpdateDesc);
			s_pRenderGraph->UpdateResource(&aoMapsUpdateDesc);
			s_pRenderGraph->UpdateResource(&metallicMapsUpdateDesc);
			s_pRenderGraph->UpdateResource(&roughnessMapsUpdateDesc);
		}

		if (s_pScene->IsRayTracingEnabled())
		{
			const AccelerationStructure* pTLAS = s_pScene->GetTLAS();
			ResourceUpdateDesc resourceUpdateDesc					= {};
			resourceUpdateDesc.ResourceName							= SCENE_TLAS;
			resourceUpdateDesc.ExternalAccelerationStructure.pTLAS	= pTLAS;

			s_pRenderGraph->UpdateResource(&resourceUpdateDesc);
		}
	}
}
