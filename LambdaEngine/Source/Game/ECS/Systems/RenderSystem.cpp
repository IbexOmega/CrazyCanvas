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
				{{{RW, MeshComponent::s_TID}, {NDA, StaticComponent::s_TID}},	{&transformComponents}, &m_StaticEntities,	std::bind(&RenderSystem::OnStaticEntityAdded, this, std::placeholders::_1),	 std::bind(&RenderSystem::OnStaticEntityRemoved, this, std::placeholders::_1)},
				{{{RW, MeshComponent::s_TID}, {NDA, DynamicComponent::s_TID}},	{&transformComponents}, &m_DynamicEntities,	std::bind(&RenderSystem::OnDynamicEntityAdded, this, std::placeholders::_1), std::bind(&RenderSystem::OnDynamicEntityRemoved, this, std::placeholders::_1)},
				{{{RW, ViewProjectionMatrices::s_TID}}, {&transformComponents}, &m_CameraEntities},
			};
			systemReg.Phase = g_LastPhase;

			RegisterSystem(systemReg);
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
			m_pRenderGraph->Init(&renderGraphDesc, m_RequiredDrawArgs);
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

		// Per Frame Buffer
		{
			BufferDesc perFrameCopyBufferDesc = {};
			perFrameCopyBufferDesc.DebugName		= "Scene Per Frame Copy Buffer";
			perFrameCopyBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
			perFrameCopyBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_SRC;
			perFrameCopyBufferDesc.SizeInBytes		= sizeof(PerFrameBuffer);

			m_pPerFrameStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&perFrameCopyBufferDesc);

			BufferDesc perFrameBufferDesc = {};
			perFrameBufferDesc.DebugName			= "Scene Per Frame Buffer";
			perFrameBufferDesc.MemoryType			= EMemoryType::MEMORY_TYPE_GPU;
			perFrameBufferDesc.Flags				= FBufferFlag::BUFFER_FLAG_CONSTANT_BUFFER | FBufferFlag::BUFFER_FLAG_COPY_DST;;
			perFrameBufferDesc.SizeInBytes			= sizeof(PerFrameBuffer);

			m_pPerFrameBuffer = RenderAPI::GetDevice()->CreateBuffer(&perFrameBufferDesc);
		}

		//Material Defaults
		{
			for (uint32 i = 0; i < MAX_UNIQUE_MATERIALS; i++)
			{
				m_FreeMaterialSlots.push(i);
			}

			Texture*		pDefaultColorMap		= ResourceManager::GetTexture(GUID_TEXTURE_DEFAULT_COLOR_MAP);
			TextureView*	pDefaultColorMapView	= ResourceManager::GetTextureView(GUID_TEXTURE_DEFAULT_COLOR_MAP);
			Texture*		pDefaultNormalMap		= ResourceManager::GetTexture(GUID_TEXTURE_DEFAULT_NORMAL_MAP);
			TextureView*	pDefaultNormalMapView	= ResourceManager::GetTextureView(GUID_TEXTURE_DEFAULT_NORMAL_MAP);

			for (uint32 i = 0; i < MAX_UNIQUE_MATERIALS; i++)
			{
				m_ppAlbedoMaps[i]				= pDefaultColorMap;
				m_ppNormalMaps[i]				= pDefaultNormalMap;
				m_ppAmbientOcclusionMaps[i]		= pDefaultColorMap;
				m_ppRoughnessMaps[i]			= pDefaultColorMap;
				m_ppMetallicMaps[i]				= pDefaultColorMap;
				m_ppAlbedoMapViews[i]			= pDefaultColorMapView;
				m_ppNormalMapViews[i]			= pDefaultNormalMapView;
				m_ppAmbientOcclusionMapViews[i]	= pDefaultColorMapView;
				m_ppRoughnessMapViews[i]		= pDefaultColorMapView;
				m_ppMetallicMapViews[i]			= pDefaultColorMapView;
			}
		}

		UpdateBuffers();

		return true;
	}

	bool RenderSystem::Release()
	{
		for (MeshAndInstancesMap::iterator meshAndInstanceIt = m_MeshAndInstancesMap.begin(); meshAndInstanceIt != m_MeshAndInstancesMap.end(); meshAndInstanceIt++)
		{
			SAFERELEASE(meshAndInstanceIt->second.pVertexBuffer);
			SAFERELEASE(meshAndInstanceIt->second.pIndexBuffer);
			SAFERELEASE(meshAndInstanceIt->second.pInstanceStagingBuffer);
			SAFERELEASE(meshAndInstanceIt->second.pInstanceBuffer);
		}

		for (uint32 b = 0; b < BACK_BUFFER_COUNT; b++)
		{
			TArray<Buffer*>& buffersToRemove = m_BuffersToRemove[b];

			for (Buffer* pStagingBuffer : buffersToRemove)
			{
				SAFERELEASE(pStagingBuffer);
			}

			buffersToRemove.Clear();
		}

		SAFERELEASE(m_pMaterialParametersStagingBuffer);
		SAFERELEASE(m_pMaterialParametersBuffer);
		SAFERELEASE(m_pPerFrameStagingBuffer);
		SAFERELEASE(m_pPerFrameBuffer);

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

		ComponentArray<PositionComponent>*	pPositionComponents = pECSCore->GetComponentArray<PositionComponent>();
		ComponentArray<RotationComponent>*	pRotationComponents = pECSCore->GetComponentArray<RotationComponent>();
		ComponentArray<ScaleComponent>*		pScaleComponents	= pECSCore->GetComponentArray<ScaleComponent>();

		for (Entity entity : m_DynamicEntities.GetIDs())
		{
			auto& positionComp	= pPositionComponents->GetData(entity);
			auto& rotationComp	= pRotationComponents->GetData(entity);
			auto& scaleComp		= pScaleComponents->GetData(entity);

			if (positionComp.Dirty || rotationComp.Dirty || scaleComp.Dirty)
			{
				glm::mat4 transform = glm::translate(glm::identity<glm::mat4>(), positionComp.Position);
				transform *= glm::toMat4(rotationComp.Quaternion);
				transform = glm::scale(transform, scaleComp.Scale);

				UpdateTransform(entity, transform);

				positionComp.Dirty	= false;
				rotationComp.Dirty	= false;
				scaleComp.Dirty		= false;
			}
		}
	}

	bool RenderSystem::Render()
	{
		m_BackBufferIndex = uint32(m_SwapChain->GetCurrentBackBufferIndex());

		CleanBuffers();
		UpdateBuffers();
		UpdateRenderGraph();

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

		m_RequiredDrawArgs.clear();
		if (!m_pRenderGraph->Recreate(&renderGraphDesc, m_RequiredDrawArgs))
		{
			LOG_ERROR("[Renderer]: Failed to set new RenderGraph %s", name.c_str());
		}

		m_DirtyDrawArgs				= m_RequiredDrawArgs;
		m_PerFrameResourceDirty		= true;
		m_MaterialsResourceDirty	= true;
		UpdateRenderGraph();
	}

	void RenderSystem::OnStaticEntityAdded(Entity entity)
	{
		ECSCore* pECSCore = ECSCore::GetInstance();

		auto& positionComp	= pECSCore->GetComponent<PositionComponent>(entity);
		auto& rotationComp	= pECSCore->GetComponent<RotationComponent>(entity);
		auto& scaleComp		= pECSCore->GetComponent<ScaleComponent>(entity);
		auto& meshComp		= pECSCore->GetComponent<MeshComponent>(entity);

		glm::mat4 transform = glm::translate(glm::identity<glm::mat4>(), positionComp.Position);
		transform *= glm::toMat4(rotationComp.Quaternion);
		transform = glm::scale(transform, scaleComp.Scale);

		AddEntityInstance(entity, meshComp.MeshGUID, meshComp.MaterialGUID, transform, true, false);
	}

	void RenderSystem::OnStaticEntityRemoved(Entity entity)
	{
	}

	void RenderSystem::OnDynamicEntityAdded(Entity entity)
	{
		ECSCore* pECSCore = ECSCore::GetInstance();

		auto& positionComp	= pECSCore->GetComponent<PositionComponent>(entity);
		auto& rotationComp	= pECSCore->GetComponent<RotationComponent>(entity);
		auto& scaleComp		= pECSCore->GetComponent<ScaleComponent>(entity);
		auto& meshComp		= pECSCore->GetComponent<MeshComponent>(entity);

		glm::mat4 transform = glm::translate(glm::identity<glm::mat4>(), positionComp.Position);
		transform *= glm::toMat4(rotationComp.Quaternion);
		transform = glm::scale(transform, scaleComp.Scale);

		AddEntityInstance(entity, meshComp.MeshGUID, meshComp.MaterialGUID, transform, false, false);
	}

	void RenderSystem::OnDynamicEntityRemoved(Entity entity)
	{
	}

	void RenderSystem::AddEntityInstance(Entity entity, GUID_Lambda meshGUID, GUID_Lambda materialGUID, const glm::mat4& transform, bool isStatic, bool animated)
	{
		//auto& component = ECSCore::GetInstance().GetComponent<StaticMeshComponent>(Entity);

		if (isStatic && animated)
		{
			LOG_ERROR("[RenderSystem]: A static game object cannot also be animated!");
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

					m_PendingBufferUpdates.PushBack({ pVertexStagingBuffer, meshEntry.pVertexBuffer, vertexBufferDesc.SizeInBytes });
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

					m_PendingBufferUpdates.PushBack({ pIndexStagingBuffer, meshEntry.pIndexBuffer, indexBufferDesc.SizeInBytes });
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

				m_ppAlbedoMaps[materialSlot]				= pMaterial->pAlbedoMap;
				m_ppNormalMaps[materialSlot]				= pMaterial->pNormalMap;
				m_ppAmbientOcclusionMaps[materialSlot]		= pMaterial->pAmbientOcclusionMap;
				m_ppRoughnessMaps[materialSlot]				= pMaterial->pRoughnessMap;
				m_ppMetallicMaps[materialSlot]				= pMaterial->pMetallicMap;
				m_ppAlbedoMapViews[materialSlot]			= pMaterial->pAlbedoMapView;
				m_ppNormalMapViews[materialSlot]			= pMaterial->pNormalMapView;
				m_ppAmbientOcclusionMapViews[materialSlot]	= pMaterial->pAmbientOcclusionMapView;
				m_ppRoughnessMapViews[materialSlot]			= pMaterial->pRoughnessMapView;
				m_ppMetallicMapViews[materialSlot]			= pMaterial->pMetallicMapView;
				m_pMaterialProperties[materialSlot]			= pMaterial->Properties;

				m_MaterialMap.insert({ materialGUID, materialSlot });
				m_MaterialsResourceDirty = true;
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

		//This needs to come from the Entity in some way
		uint32 drawArgHash = 0xFFFFFFFF;
		if (m_RequiredDrawArgs.count(drawArgHash))
		{
			m_DirtyDrawArgs.insert(drawArgHash);
		}
	}

	void RenderSystem::UpdateTransform(Entity entity, const glm::mat4& transform)
	{
		THashTable<GUID_Lambda, InstanceKey>::iterator instanceKeyIt = m_EntityIDsToInstanceKey.find(entity);

		if (instanceKeyIt == m_EntityIDsToInstanceKey.end())
		{
			LOG_ERROR("[RenderSystem]: Tried to update transform of an entity which is not registered");
			return;
		}

		MeshAndInstancesMap::iterator meshAndInstancesIt = m_MeshAndInstancesMap.find(instanceKeyIt->second.MeshKey);

		if (meshAndInstancesIt == m_MeshAndInstancesMap.end())
		{
			LOG_ERROR("[RenderSystem]: Tried to update transform of an entity which has no MeshAndInstancesMap entry");
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

	void RenderSystem::CleanBuffers()
	{
		// Update data on GPU
		//Release Staging Buffers from older frame
		//Todo: Better solution for this
		TArray<Buffer*>& buffersToRemove = m_BuffersToRemove[m_ModFrameIndex];

		for (Buffer* pBuffer : buffersToRemove)
		{
			SAFERELEASE(pBuffer);
		}

		buffersToRemove.Clear();
	}

	void RenderSystem::UpdateBuffers()
	{
		CommandList* pGraphicsCommandList = m_pRenderGraph->AcquireGraphicsCopyCommandList();
		//CommandList* pComputeCommandList = m_pRenderGraph->AcquireComputeCopyCommandList();

		//Update Pending Buffer Updates
		{
			ExecutePendingBufferUpdates(pGraphicsCommandList);
		}

		//Update Instance Data
		{
			UpdateInstanceBuffers(pGraphicsCommandList);
		}

		//Update Per Frame Data
		{
			m_PerFrameData.FrameIndex = 0;
			m_PerFrameData.RandomSeed = uint32(Random::Int32(INT32_MIN, INT32_MAX));

			UpdatePerFrameBuffer(pGraphicsCommandList);
		}

		//Update Empty MaterialData
		{
			UpdateMaterialPropertiesBuffer(pGraphicsCommandList);
		}
	}

	void RenderSystem::UpdateRenderGraph()
	{
		//Should we check for Draw Args to be removed here?

		if (!m_DirtyDrawArgs.empty())
		{
			for (uint32 drawArgMask : m_DirtyDrawArgs)
			{
				TArray<DrawArg> drawArgs;
				CreateDrawArgs(drawArgs, drawArgMask);

				//Create Resource Update for RenderGraph
				ResourceUpdateDesc resourceUpdateDesc					= {};
				resourceUpdateDesc.ResourceName							= SCENE_DRAW_ARGS;
				resourceUpdateDesc.ExternalDrawArgsUpdate.DrawArgsMask	= drawArgMask;
				resourceUpdateDesc.ExternalDrawArgsUpdate.pDrawArgs		= drawArgs.GetData();
				resourceUpdateDesc.ExternalDrawArgsUpdate.DrawArgsCount	= drawArgs.GetSize();

				m_pRenderGraph->UpdateResource(&resourceUpdateDesc);
			}

			m_DirtyDrawArgs.clear();
		}

		if (m_PerFrameResourceDirty)
		{
			ResourceUpdateDesc resourceUpdateDesc				= {};
			resourceUpdateDesc.ResourceName						= PER_FRAME_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &m_pPerFrameBuffer;

			m_pRenderGraph->UpdateResource(&resourceUpdateDesc);

			m_PerFrameResourceDirty = false;
		}

		if (m_MaterialsResourceDirty)
		{
			ResourceUpdateDesc resourceUpdateDesc				= {};
			resourceUpdateDesc.ResourceName						= SCENE_MAT_PARAM_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &m_pMaterialParametersBuffer;

			m_pRenderGraph->UpdateResource(&resourceUpdateDesc);

			std::vector<Sampler*> nearestSamplers(MAX_UNIQUE_MATERIALS, Sampler::GetNearestSampler());

			ResourceUpdateDesc albedoMapsUpdateDesc = {};
			albedoMapsUpdateDesc.ResourceName								= SCENE_ALBEDO_MAPS;
			albedoMapsUpdateDesc.ExternalTextureUpdate.ppTextures			= m_ppAlbedoMaps;
			albedoMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews		= m_ppAlbedoMapViews;
			albedoMapsUpdateDesc.ExternalTextureUpdate.ppSamplers			= nearestSamplers.data();

			ResourceUpdateDesc normalMapsUpdateDesc = {};
			normalMapsUpdateDesc.ResourceName								= SCENE_NORMAL_MAPS;
			normalMapsUpdateDesc.ExternalTextureUpdate.ppTextures			= m_ppNormalMaps;
			normalMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews		= m_ppNormalMapViews;
			normalMapsUpdateDesc.ExternalTextureUpdate.ppSamplers			= nearestSamplers.data();

			ResourceUpdateDesc aoMapsUpdateDesc = {};
			aoMapsUpdateDesc.ResourceName									= SCENE_AO_MAPS;
			aoMapsUpdateDesc.ExternalTextureUpdate.ppTextures				= m_ppAmbientOcclusionMaps;
			aoMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews			= m_ppAmbientOcclusionMapViews;
			aoMapsUpdateDesc.ExternalTextureUpdate.ppSamplers				= nearestSamplers.data();

			ResourceUpdateDesc metallicMapsUpdateDesc = {};
			metallicMapsUpdateDesc.ResourceName								= SCENE_METALLIC_MAPS;
			metallicMapsUpdateDesc.ExternalTextureUpdate.ppTextures			= m_ppMetallicMaps;
			metallicMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews		= m_ppMetallicMapViews;
			metallicMapsUpdateDesc.ExternalTextureUpdate.ppSamplers			= nearestSamplers.data();

			ResourceUpdateDesc roughnessMapsUpdateDesc = {};
			roughnessMapsUpdateDesc.ResourceName							= SCENE_ROUGHNESS_MAPS;
			roughnessMapsUpdateDesc.ExternalTextureUpdate.ppTextures		= m_ppRoughnessMaps;
			roughnessMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews	= m_ppRoughnessMapViews;
			roughnessMapsUpdateDesc.ExternalTextureUpdate.ppSamplers		= nearestSamplers.data();

			m_pRenderGraph->UpdateResource(&albedoMapsUpdateDesc);
			m_pRenderGraph->UpdateResource(&normalMapsUpdateDesc);
			m_pRenderGraph->UpdateResource(&aoMapsUpdateDesc);
			m_pRenderGraph->UpdateResource(&metallicMapsUpdateDesc);
			m_pRenderGraph->UpdateResource(&roughnessMapsUpdateDesc);

			m_MaterialsResourceDirty = false;
		}
	}

	void RenderSystem::CreateDrawArgs(TArray<DrawArg>& drawArgs, uint32 mask) const
	{
		for (MeshAndInstancesMap::const_iterator meshAndInstancesIt = m_MeshAndInstancesMap.begin(); meshAndInstancesIt != m_MeshAndInstancesMap.end(); meshAndInstancesIt++)
		{
			//Todo: Check Key (or whatever we end up using)
			DrawArg drawArg = {};
			drawArg.pVertexBuffer		= meshAndInstancesIt->second.pVertexBuffer;
			drawArg.VertexBufferSize	= meshAndInstancesIt->second.pVertexBuffer->GetDesc().SizeInBytes;
			drawArg.pIndexBuffer		= meshAndInstancesIt->second.pIndexBuffer;
			drawArg.IndexCount			= meshAndInstancesIt->second.IndexCount;
			drawArg.pInstanceBuffer		= meshAndInstancesIt->second.pInstanceBuffer;
			drawArg.InstanceBufferSize	= meshAndInstancesIt->second.pInstanceBuffer->GetDesc().SizeInBytes;
			drawArg.InstanceCount		= meshAndInstancesIt->second.Instances.GetSize();
			drawArgs.PushBack(drawArg);
		}
	}

	void RenderSystem::ExecutePendingBufferUpdates(CommandList* pCommandList)
	{
		if (!m_PendingBufferUpdates.IsEmpty())
		{
			for (PendingBufferUpdate& pendingUpdate : m_PendingBufferUpdates)
			{
				pCommandList->CopyBuffer(pendingUpdate.pSrcBuffer, 0, pendingUpdate.pDstBuffer, 0, pendingUpdate.SizeInBytes);
			}

			m_PendingBufferUpdates.Clear();
		}
	}

	void RenderSystem::UpdateInstanceBuffers(CommandList* pCommandList)
	{
		for (MeshEntry* pDirtyInstanceBufferEntry : m_DirtyInstanceBuffers)
		{
			uint32 requiredBufferSize = pDirtyInstanceBufferEntry->Instances.GetSize() * sizeof(Instance);

			if (pDirtyInstanceBufferEntry->pInstanceStagingBuffer == nullptr || pDirtyInstanceBufferEntry->pInstanceStagingBuffer->GetDesc().SizeInBytes < requiredBufferSize)
			{
				if (pDirtyInstanceBufferEntry->pInstanceStagingBuffer != nullptr) m_BuffersToRemove[m_ModFrameIndex].PushBack(pDirtyInstanceBufferEntry->pInstanceStagingBuffer);

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
				if (pDirtyInstanceBufferEntry->pInstanceBuffer != nullptr) m_BuffersToRemove[m_ModFrameIndex].PushBack(pDirtyInstanceBufferEntry->pInstanceBuffer);

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

	void RenderSystem::UpdateMaterialPropertiesBuffer(CommandList* pCommandList)
	{
		uint32 requiredBufferSize = sizeof(m_pMaterialProperties);

		if (m_pMaterialParametersStagingBuffer == nullptr || m_pMaterialParametersStagingBuffer->GetDesc().SizeInBytes < requiredBufferSize)
		{
			if (m_pMaterialParametersStagingBuffer != nullptr) m_BuffersToRemove[m_ModFrameIndex].PushBack(m_pMaterialParametersStagingBuffer);

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName	= "Material Properties Staging Buffer";
			bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
			bufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_SRC;
			bufferDesc.SizeInBytes	= requiredBufferSize;

			m_pMaterialParametersStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
		}

		void* pMapped = m_pMaterialParametersStagingBuffer->Map();
		memcpy(pMapped, m_pMaterialProperties, requiredBufferSize);
		m_pMaterialParametersStagingBuffer->Unmap();

		if (m_pMaterialParametersBuffer == nullptr || m_pMaterialParametersBuffer->GetDesc().SizeInBytes < requiredBufferSize)
		{
			if (m_pMaterialParametersBuffer != nullptr) m_BuffersToRemove[m_ModFrameIndex].PushBack(m_pMaterialParametersBuffer);

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
