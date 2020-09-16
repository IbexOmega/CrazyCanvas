#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandQueue.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/SwapChain.h"
#include "Rendering/Core/API/Texture.h"
#include "Rendering/Core/API/TextureView.h"
#include "Rendering/RenderAPI.h"
#include "Rendering/RenderGraph.h"
#include "Rendering/RenderGraphSerializer.h"
#include "Rendering/ImGuiRenderer.h"

#include "Application/API/Window.h"
#include "Application/API/CommonApplication.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Rendering/MeshComponent.h"

#include "Game/Scene.h"

namespace LambdaEngine 
{
	bool RenderSystem::InitSystem()
	{
		{
			TransformComponents transformComponents;
			transformComponents.Position.Permissions = R;
			transformComponents.Scale.Permissions = R;
			transformComponents.Rotation.Permissions = R;

			// Subscribe on Static Entities & Dynamic Entities
			{
				SystemRegistration systemReg = {};
				systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
				{
					{{{R, g_TIDStaticMeshComponent}}, {&transformComponents}, &m_StaticEntities},
					{{{R, g_TIDDynamicMeshComponent}}, {&transformComponents}, &m_DynamicEntities},
				};
				systemReg.Phase = g_LastPhase - 1U;
			}
		}

		//Create Swapchain
		{
			SwapChainDesc swapChainDesc = {};
			swapChainDesc.DebugName = "Renderer Swap Chain";
			swapChainDesc.pWindow = CommonApplication::Get()->GetActiveWindow().Get();
			swapChainDesc.pQueue = RenderAPI::GetGraphicsQueue();
			swapChainDesc.Format = EFormat::FORMAT_B8G8R8A8_UNORM;
			swapChainDesc.Width = 0;
			swapChainDesc.Height = 0;
			swapChainDesc.BufferCount = BACK_BUFFER_COUNT;
			swapChainDesc.SampleCount = 1;
			swapChainDesc.VerticalSync = false;

			m_SwapChain = RenderAPI::GetDevice()->CreateSwapChain(&swapChainDesc);
			if (!m_SwapChain)
			{
				LOG_ERROR("[Renderer]: SwapChain is nullptr after initializaiton");
				return false;
			}

			m_ppBackBuffers = DBG_NEW Texture * [BACK_BUFFER_COUNT];
			m_ppBackBufferViews = DBG_NEW TextureView * [BACK_BUFFER_COUNT];

			m_FrameIndex++;
			m_ModFrameIndex = m_FrameIndex % uint64(BACK_BUFFER_COUNT);
		}

		//Create RenderGraph
		{
			RenderGraphStructureDesc renderGraphStructure = {};

			if (!RenderGraphSerializer::LoadAndParse(&renderGraphStructure, "DEMO_SKYBOX.lrg", IMGUI_ENABLED))
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
			renderGraphDesc.Name = "Default Rendergraph";
			renderGraphDesc.pRenderGraphStructureDesc = &renderGraphStructure;
			renderGraphDesc.BackBufferCount = BACK_BUFFER_COUNT;
			renderGraphDesc.MaxTexturesPerDescriptorSet = MAX_TEXTURES_PER_DESCRIPTOR_SET;

			m_pRenderGraph = DBG_NEW RenderGraph(RenderAPI::GetDevice());
			m_pRenderGraph->Init(&renderGraphDesc);
		}

		//Update RenderGraph with Back Buffer
		{
			for (uint32 v = 0; v < BACK_BUFFER_COUNT; v++)
			{
				m_ppBackBuffers[v] = m_SwapChain->GetBuffer(v);
				m_ppBackBufferViews[v] = m_SwapChain->GetBufferView(v);
			}

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName = RENDER_GRAPH_BACK_BUFFER_ATTACHMENT;
			resourceUpdateDesc.ExternalTextureUpdate.ppTextures = m_ppBackBuffers;
			resourceUpdateDesc.ExternalTextureUpdate.ppTextureViews = m_ppBackBufferViews;

			m_pRenderGraph->UpdateResource(&resourceUpdateDesc);
		}

		return true;
	}

	bool RenderSystem::Release()
	{
		SAFEDELETE(m_pRenderGraph);

		if (m_SwapChain)
		{
			for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
			{
				SAFERELEASE(m_ppBackBuffers[i]);
				SAFERELEASE(m_ppBackBufferViews[i]);
			}

			SAFEDELETE_ARRAY(m_ppBackBuffers);
			SAFEDELETE_ARRAY(m_ppBackBufferViews);
			m_SwapChain.Reset();
		}

		return true;
	}

	void RenderSystem::Tick(float dt)
	{
		for (Entity entity : m_StaticEntities.GetIDs())
		{

		}

		for (Entity entity : m_DynamicEntities.GetIDs())
		{

		}
	}

	bool RenderSystem::Render()
	{
		m_BackBufferIndex = uint32(m_SwapChain->GetCurrentBackBufferIndex());

		m_pRenderGraph->Update();

		// Update data on GPU

		m_pRenderGraph->Render(m_ModFrameIndex, m_BackBufferIndex);

		m_SwapChain->Present();

		m_FrameIndex++;
		m_ModFrameIndex = m_FrameIndex % uint64(BACK_BUFFER_COUNT);

		return true;
	}

	CommandList* RenderSystem::AcquireGraphicsCopyCommandList()
	{
		return m_pRenderGraph->AcquireGraphicsCopyCommandList();
	}

	CommandList* RenderSystem::AcquireComputeCopyCommandList()
	{
		return m_pRenderGraph->AcquireGraphicsCopyCommandList();;
	}

	void RenderSystem::SetRenderGraph(const String& name, RenderGraphStructureDesc* pRenderGraphStructureDesc)
	{
		RenderGraphDesc renderGraphDesc = {};
		renderGraphDesc.Name = name;
		renderGraphDesc.pRenderGraphStructureDesc = pRenderGraphStructureDesc;
		renderGraphDesc.BackBufferCount = BACK_BUFFER_COUNT;
		renderGraphDesc.MaxTexturesPerDescriptorSet = MAX_TEXTURES_PER_DESCRIPTOR_SET;

		if (!m_pRenderGraph->Recreate(&renderGraphDesc))
		{
			LOG_ERROR("[Renderer]: Failed to set new RenderGraph %s", name.c_str());
		}

		if (m_pScene != nullptr)
		{
			UpdateRenderGraphFromScene();
		}
	}
	void RenderSystem::UpdateRenderGraphFromScene()
	{
		{
			Buffer* pBuffer = m_pScene->GetLightsBuffer();
			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName = SCENE_LIGHTS_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer = &pBuffer;

			m_pRenderGraph->UpdateResource(&resourceUpdateDesc);
		}

		{
			Buffer* pBuffer = m_pScene->GetPerFrameBuffer();
			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName = PER_FRAME_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer = &pBuffer;

			m_pRenderGraph->UpdateResource(&resourceUpdateDesc);
		}

		{
			Buffer* pBuffer = m_pScene->GetMaterialProperties();
			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName = SCENE_MAT_PARAM_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer = &pBuffer;

			m_pRenderGraph->UpdateResource(&resourceUpdateDesc);
		}

		{
			Buffer* pBuffer = m_pScene->GetVertexBuffer();
			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName = SCENE_VERTEX_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer = &pBuffer;

			m_pRenderGraph->UpdateResource(&resourceUpdateDesc);
		}

		{
			Buffer* pBuffer = m_pScene->GetIndexBuffer();
			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName = SCENE_INDEX_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer = &pBuffer;

			m_pRenderGraph->UpdateResource(&resourceUpdateDesc);
		}

		{
			Buffer* pBuffer = m_pScene->GetPrimaryInstanceBuffer();
			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName = SCENE_PRIMARY_INSTANCE_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer = &pBuffer;

			m_pRenderGraph->UpdateResource(&resourceUpdateDesc);
		}

		{
			Buffer* pBuffer = m_pScene->GetSecondaryInstanceBuffer();
			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName = SCENE_SECONDARY_INSTANCE_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer = &pBuffer;

			m_pRenderGraph->UpdateResource(&resourceUpdateDesc);
		}

		{
			Buffer* pBuffer = m_pScene->GetIndirectArgsBuffer();
			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName = SCENE_INDIRECT_ARGS_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer = &pBuffer;

			m_pRenderGraph->UpdateResource(&resourceUpdateDesc);
		}

		{
			Texture** ppAlbedoMaps = m_pScene->GetAlbedoMaps();
			Texture** ppNormalMaps = m_pScene->GetNormalMaps();
			Texture** ppAmbientOcclusionMaps = m_pScene->GetAmbientOcclusionMaps();
			Texture** ppMetallicMaps = m_pScene->GetMetallicMaps();
			Texture** ppRoughnessMaps = m_pScene->GetRoughnessMaps();

			TextureView** ppAlbedoMapViews = m_pScene->GetAlbedoMapViews();
			TextureView** ppNormalMapViews = m_pScene->GetNormalMapViews();
			TextureView** ppAmbientOcclusionMapViews = m_pScene->GetAmbientOcclusionMapViews();
			TextureView** ppMetallicMapViews = m_pScene->GetMetallicMapViews();
			TextureView** ppRoughnessMapViews = m_pScene->GetRoughnessMapViews();

			std::vector<Sampler*> nearestSamplers(MAX_UNIQUE_MATERIALS, Sampler::GetNearestSampler());

			ResourceUpdateDesc albedoMapsUpdateDesc = {};
			albedoMapsUpdateDesc.ResourceName = SCENE_ALBEDO_MAPS;
			albedoMapsUpdateDesc.ExternalTextureUpdate.ppTextures = ppAlbedoMaps;
			albedoMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews = ppAlbedoMapViews;
			albedoMapsUpdateDesc.ExternalTextureUpdate.ppSamplers = nearestSamplers.data();

			ResourceUpdateDesc normalMapsUpdateDesc = {};
			normalMapsUpdateDesc.ResourceName = SCENE_NORMAL_MAPS;
			normalMapsUpdateDesc.ExternalTextureUpdate.ppTextures = ppNormalMaps;
			normalMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews = ppNormalMapViews;
			normalMapsUpdateDesc.ExternalTextureUpdate.ppSamplers = nearestSamplers.data();

			ResourceUpdateDesc aoMapsUpdateDesc = {};
			aoMapsUpdateDesc.ResourceName = SCENE_AO_MAPS;
			aoMapsUpdateDesc.ExternalTextureUpdate.ppTextures = ppAmbientOcclusionMaps;
			aoMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews = ppAmbientOcclusionMapViews;
			aoMapsUpdateDesc.ExternalTextureUpdate.ppSamplers = nearestSamplers.data();

			ResourceUpdateDesc metallicMapsUpdateDesc = {};
			metallicMapsUpdateDesc.ResourceName = SCENE_METALLIC_MAPS;
			metallicMapsUpdateDesc.ExternalTextureUpdate.ppTextures = ppMetallicMaps;
			metallicMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews = ppMetallicMapViews;
			metallicMapsUpdateDesc.ExternalTextureUpdate.ppSamplers = nearestSamplers.data();

			ResourceUpdateDesc roughnessMapsUpdateDesc = {};
			roughnessMapsUpdateDesc.ResourceName = SCENE_ROUGHNESS_MAPS;
			roughnessMapsUpdateDesc.ExternalTextureUpdate.ppTextures = ppRoughnessMaps;
			roughnessMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews = ppRoughnessMapViews;
			roughnessMapsUpdateDesc.ExternalTextureUpdate.ppSamplers = nearestSamplers.data();

			m_pRenderGraph->UpdateResource(&albedoMapsUpdateDesc);
			m_pRenderGraph->UpdateResource(&normalMapsUpdateDesc);
			m_pRenderGraph->UpdateResource(&aoMapsUpdateDesc);
			m_pRenderGraph->UpdateResource(&metallicMapsUpdateDesc);
			m_pRenderGraph->UpdateResource(&roughnessMapsUpdateDesc);
		}

		if (m_pScene->IsRayTracingEnabled())
		{
			const AccelerationStructure* pTLAS = m_pScene->GetTLAS();
			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName = SCENE_TLAS;
			resourceUpdateDesc.ExternalAccelerationStructure.pTLAS = pTLAS;

			m_pRenderGraph->UpdateResource(&resourceUpdateDesc);
		}
	}
}
