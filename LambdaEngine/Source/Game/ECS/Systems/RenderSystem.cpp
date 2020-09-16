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

#include "ECS/ECSCore.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Rendering/MeshComponent.h"
#include "Game/ECS/Components/Rendering/Camera.h"

#include "Engine/EngineConfig.h"

#include "Game/Scene.h"

namespace LambdaEngine 
{
	RenderSystem RenderSystem::s_Instance;

	bool RenderSystem::Init()
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
				{{{RW, MeshComponent::s_TID}, {NDA , StaticComponent::s_TID}}, {&transformComponents}, &m_StaticEntities},
				{{{RW, MeshComponent::s_TID}, {NDA , DynamicComponent::s_TID}}, {&transformComponents},& m_DynamicEntities},
				{{{RW, ViewProjectionMatrices::s_TID}}, {&transformComponents}, &m_CameraEntities},
			};
			systemReg.Phase = g_LastPhase;

			EnqueueRegistration(systemReg);
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

	bool RenderSystem::InitSystem()
	{
		
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
		ECSCore* pECSCore = ECSCore::GetInstance();

		ComponentArray< PositionComponent > * pPositionComponents = pECSCore->GetComponentArray<PositionComponent>();
		ComponentArray< RotationComponent > * pRotationComponents = pECSCore->GetComponentArray<RotationComponent>();
		ComponentArray< ScaleComponent > * pScaleComponents = pECSCore->GetComponentArray<ScaleComponent>();

		for (Entity entity : m_DynamicEntities.GetIDs())
		{
			auto& position = pPositionComponents->GetData(entity);
			auto& rotation = pRotationComponents->GetData(entity);
			auto& scale = pScaleComponents->GetData(entity);

			if (position.Dirty || rotation.Dirty || scale.Dirty)
			{
				glm::mat4 transform = glm::translate(glm::identity<glm::mat4>(), position.Position);
				transform *= glm::toMat4(rotation.Quaternion);
				transform = glm::scale(transform, scale.Scale);

				UpdateTransform(entity, transform);

				position.Dirty = rotation.Dirty = scale.Dirty = false;
			}
		}
	}

	bool RenderSystem::Render()
	{
		m_BackBufferIndex = uint32(m_SwapChain->GetCurrentBackBufferIndex());

		CommandList* pGraphicsCommandList = m_pRenderGraph->AcquireGraphicsCopyCommandList();

		// Update data on GPU
		//Release Staging Buffers from older frame
		//Todo: Better solution for this
		{
			TArray<Buffer*>& buffersToRemove = m_BuffersToRemove[m_ModFrameIndex];

			for (Buffer* pStagingBuffer : buffersToRemove)
			{
				SAFERELEASE(pStagingBuffer);
			}

			buffersToRemove.Clear();
		}

		//Update Per Frame Data
		{
			m_PerFrameData.FrameIndex = m_FrameIndex;
			m_PerFrameData.RandomSeed = uint32(Random::Int32(INT32_MIN, INT32_MAX));

			UpdatePerFrameBuffer(pGraphicsCommandList);
		}

		//Update Instance Data
		{
			UpdateInstanceBuffers(pGraphicsCommandList, m_ModFrameIndex);
		}

		//Update Empty MaterialData
		{
			UpdateMaterialPropertiesBuffer(pGraphicsCommandList, m_ModFrameIndex);
		}

		m_pRenderGraph->Update();

		m_pRenderGraph->Render(m_ModFrameIndex, m_BackBufferIndex);

		m_SwapChain->Present();

		m_FrameIndex++;
		m_ModFrameIndex = m_FrameIndex % uint64(BACK_BUFFER_COUNT);

		return true;
	}

	void RenderSystem::SetCamera(const Camera* pCamera)
	{
		m_PerFrameData.CamData = pCamera->GetData();
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
		/*{
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
		}*/
	}
	void RenderSystem::AddEntityInstance(Entity entity, CommandList* pCommandList, GUID_Lambda meshGUID, GUID_Lambda materialGUID, const glm::mat4& transform, bool isStatic, bool animated)
	{
		//auto& component = ECSCore::GetInstance().GetComponent<StaticMeshComponent>(Entity);

		if (isStatic && animated)
		{
			LOG_ERROR("[--TBD--]: A static game object cannot also be animated!");
			return;
		}

		uint32 materialSlot;
		MeshAndInstancesMap::iterator meshAndInstancesIt;

		MeshKey meshKey;
		meshKey.MeshGUID		= meshGUID;
		meshKey.IsStatic		= isStatic;
		meshKey.IsAnimated		= animated;
		meshKey.EntityID		= entity;

		//Get MeshAndInstanceIterator
		{
			meshAndInstancesIt = m_MeshAndInstancesMap.find(meshKey);

			if (meshAndInstancesIt == m_MeshAndInstancesMap.end())
			{
				const Mesh* pMesh = ResourceManager::GetMesh(meshGUID);
				VALIDATE(pMesh != nullptr);

				MeshEntry meshEntry = {};

				//Vertices
				{
					BufferDesc vertexStagingBufferDesc = {};
					vertexStagingBufferDesc.DebugName	= "Vertex Staging Buffer";
					vertexStagingBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
					vertexStagingBufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_SRC;
					vertexStagingBufferDesc.SizeInBytes = pMesh->VertexCount * sizeof(Vertex);

					Buffer* pVertexStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&vertexStagingBufferDesc);

					void* pMapped = pVertexStagingBuffer->Map();
					memcpy(pMapped, pMesh->pVertexArray, vertexStagingBufferDesc.SizeInBytes);
					pVertexStagingBuffer->Unmap();

					BufferDesc vertexBufferDesc = {};
					vertexBufferDesc.DebugName		= "Vertex Buffer";
					vertexBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_GPU;
					vertexBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER;
					vertexBufferDesc.SizeInBytes	= vertexStagingBufferDesc.SizeInBytes;

					meshEntry.pVertexBuffer = RenderAPI::GetDevice()->CreateBuffer(&vertexBufferDesc);

					pCommandList->CopyBuffer(pVertexStagingBuffer, 0, meshEntry.pVertexBuffer, 0, vertexBufferDesc.SizeInBytes);
					m_BuffersToRemove[0].PushBack(pVertexStagingBuffer);
				}

				//Indices
				{
					BufferDesc indexStagingBufferDesc = {};
					indexStagingBufferDesc.DebugName	= "Index Staging Buffer";
					indexStagingBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
					indexStagingBufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_SRC;
					indexStagingBufferDesc.SizeInBytes	= pMesh->IndexCount * sizeof(uint32);

					Buffer* pIndexStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&indexStagingBufferDesc);

					void* pMapped = pIndexStagingBuffer->Map();
					memcpy(pMapped, pMesh->pIndexArray, indexStagingBufferDesc.SizeInBytes);
					pIndexStagingBuffer->Unmap();

					BufferDesc indexBufferDesc = {};
					indexBufferDesc.DebugName		= "Index Buffer";
					indexBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_GPU;
					indexBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_INDEX_BUFFER;
					indexBufferDesc.SizeInBytes		= indexStagingBufferDesc.SizeInBytes;

					meshEntry.pIndexBuffer	= RenderAPI::GetDevice()->CreateBuffer(&indexBufferDesc);
					meshEntry.IndexCount	= pMesh->IndexCount;

					pCommandList->CopyBuffer(pIndexStagingBuffer, 0, meshEntry.pIndexBuffer, 0, indexBufferDesc.SizeInBytes);
					m_BuffersToRemove[0].PushBack(pIndexStagingBuffer);
				}

				meshAndInstancesIt = m_MeshAndInstancesMap.insert({ meshKey, meshEntry }).first;
			}
		}

		//Get Material Slot
		{
			THashTable<uint32, uint32>::iterator materialSlotIt = m_MaterialMap.find(materialGUID);

			//Push new Material if the Material is yet to be registered
			if (materialSlotIt == m_MaterialMap.end())
			{
				const Material* pMaterial = ResourceManager::GetMaterial(materialGUID);
				VALIDATE(pMaterial != nullptr && !m_FreeMaterialSlots.empty());

				materialSlot = m_FreeMaterialSlots.top();
				m_FreeMaterialSlots.pop();

				m_ppSceneAlbedoMaps[materialSlot]					= pMaterial->pAlbedoMap;
				m_ppSceneNormalMaps[materialSlot]					= pMaterial->pNormalMap;
				m_ppSceneAmbientOcclusionMaps[materialSlot]			= pMaterial->pAmbientOcclusionMap;
				m_ppSceneRoughnessMaps[materialSlot]				= pMaterial->pRoughnessMap;
				m_ppSceneMetallicMaps[materialSlot]					= pMaterial->pMetallicMap;
				m_ppSceneAlbedoMapViews[materialSlot]				= pMaterial->pAlbedoMapView;
				m_ppSceneNormalMapViews[materialSlot]				= pMaterial->pNormalMapView;
				m_ppSceneAmbientOcclusionMapViews[materialSlot]		= pMaterial->pAmbientOcclusionMapView;
				m_ppSceneRoughnessMapViews[materialSlot]			= pMaterial->pRoughnessMapView;
				m_ppSceneMetallicMapViews[materialSlot]				= pMaterial->pMetallicMapView;
				m_pSceneMaterialProperties[materialSlot]			= pMaterial->Properties;

				m_MaterialMap.insert({ materialGUID, materialSlot });
			}
			else
			{
				materialSlot = materialSlotIt->second;
			}
		}
		
		InstanceKey instanceKey = {};
		instanceKey.MeshKey			= meshKey;
		instanceKey.InstanceIndex	= meshAndInstancesIt->second.Instances.GetSize();
		m_EntityIDsToInstanceKey[entity] = instanceKey;

		Instance instance = {};
		instance.Transform			= transform;
		instance.PrevTransform		= transform;
		instance.MaterialSlot		= materialSlot;
		meshAndInstancesIt->second.Instances.PushBack(instance);

		m_DirtyInstanceBuffers.insert(&meshAndInstancesIt->second);
	}

	void RenderSystem::UpdateTransform(Entity entity, const glm::mat4& transform)
	{
		THashTable<GUID_Lambda, InstanceKey>::iterator instanceKeyIt = m_EntityIDsToInstanceKey.find(entity);

		if (instanceKeyIt == m_EntityIDsToInstanceKey.end())
		{
			LOG_ERROR("[--TBD--]: Tried to update transform of an enitity which is not registered");
			return;
		}

		MeshAndInstancesMap::iterator meshAndInstancesIt = m_MeshAndInstancesMap.find(instanceKeyIt->second.MeshKey);

		if (meshAndInstancesIt == m_MeshAndInstancesMap.end())
		{
			LOG_ERROR("[--TBD--]: Tried to update transform of an enitity which has no MeshAndInstancesMap entry");
			return;
		}

		Instance* pInstanceToUpdate = &meshAndInstancesIt->second.Instances[instanceKeyIt->second.InstanceIndex];

		pInstanceToUpdate->PrevTransform = pInstanceToUpdate->Transform;
		pInstanceToUpdate->Transform = transform;
		m_DirtyInstanceBuffers.insert(&meshAndInstancesIt->second);
	}

	void RenderSystem::UpdateCamera(Entity entity)
	{

	}

	void RenderSystem::UpdateInstanceBuffers(CommandList* pCommandList, uint64 modFrameIndex)
	{
		for (MeshEntry* pDirtyInstanceBufferEntry : m_DirtyInstanceBuffers)
		{
			uint32 requiredBufferSize = pDirtyInstanceBufferEntry->Instances.GetSize() * sizeof(Instance);

			if (pDirtyInstanceBufferEntry->pInstanceStagingBuffer == nullptr || pDirtyInstanceBufferEntry->pInstanceStagingBuffer->GetDesc().SizeInBytes < requiredBufferSize)
			{
				if (pDirtyInstanceBufferEntry->pInstanceStagingBuffer != nullptr) m_BuffersToRemove[modFrameIndex].PushBack(pDirtyInstanceBufferEntry->pInstanceStagingBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.DebugName	= "Instance Staging Buffer";
				bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
				bufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_SRC;
				bufferDesc.SizeInBytes	= requiredBufferSize;

				pDirtyInstanceBufferEntry->pInstanceStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			}

			void* pMapped = pDirtyInstanceBufferEntry->pInstanceStagingBuffer->Map();
			memcpy(pMapped, pDirtyInstanceBufferEntry->Instances.GetData(), requiredBufferSize);
			pDirtyInstanceBufferEntry->pInstanceStagingBuffer->Unmap();

			if (pDirtyInstanceBufferEntry->pInstanceBuffer == nullptr || pDirtyInstanceBufferEntry->pInstanceBuffer->GetDesc().SizeInBytes < requiredBufferSize)
			{
				if (pDirtyInstanceBufferEntry->pInstanceBuffer != nullptr) m_BuffersToRemove[modFrameIndex].PushBack(pDirtyInstanceBufferEntry->pInstanceBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.DebugName		= "Instance Buffer";
				bufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_GPU;
				bufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER;
				bufferDesc.SizeInBytes		= requiredBufferSize;

				pDirtyInstanceBufferEntry->pInstanceBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			}
			
			pCommandList->CopyBuffer(pDirtyInstanceBufferEntry->pInstanceStagingBuffer, 0, pDirtyInstanceBufferEntry->pInstanceBuffer, 0, requiredBufferSize);
		}

		m_DirtyInstanceBuffers.clear();
	}

	void RenderSystem::UpdatePerFrameBuffer(CommandList* pCommandList)
	{
		void* pMapped = m_pPerFrameStagingBuffer->Map();
		memcpy(pMapped, &m_PerFrameData, sizeof(PerFrameBuffer));
		m_pPerFrameStagingBuffer->Unmap();

		pCommandList->CopyBuffer(m_pPerFrameStagingBuffer, 0, m_pPerFrameBuffer, 0, sizeof(PerFrameBuffer));
	}

	void RenderSystem::UpdateMaterialPropertiesBuffer(CommandList* pCommandList, uint64 modFrameIndex)
	{
		uint32 requiredBufferSize = sizeof(m_pSceneMaterialProperties);

		if (m_pMaterialParametersStagingBuffer == nullptr || m_pMaterialParametersStagingBuffer->GetDesc().SizeInBytes < requiredBufferSize)
		{
			if (m_pMaterialParametersStagingBuffer != nullptr) m_BuffersToRemove[modFrameIndex].PushBack(m_pMaterialParametersStagingBuffer);

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName	= "Material Properties Staging Buffer";
			bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
			bufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_SRC;
			bufferDesc.SizeInBytes	= requiredBufferSize;

			m_pMaterialParametersStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
		}

		void* pMapped = m_pMaterialParametersStagingBuffer->Map();
		memcpy(pMapped, m_pSceneMaterialProperties, requiredBufferSize);
		m_pMaterialParametersStagingBuffer->Unmap();

		if (m_pMaterialParametersBuffer == nullptr || m_pMaterialParametersBuffer->GetDesc().SizeInBytes < requiredBufferSize)
		{
			if (m_pMaterialParametersBuffer != nullptr) m_BuffersToRemove[modFrameIndex].PushBack(m_pMaterialParametersBuffer);

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName	= "Material Properties Buffer";
			bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
			bufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_CONSTANT_BUFFER;
			bufferDesc.SizeInBytes	= requiredBufferSize;

			m_pMaterialParametersBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
		}

		pCommandList->CopyBuffer(m_pMaterialParametersStagingBuffer, 0, m_pMaterialParametersBuffer, 0, requiredBufferSize);
	}

}
