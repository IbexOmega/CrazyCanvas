#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandQueue.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/SwapChain.h"
#include "Rendering/Core/API/Texture.h"
#include "Rendering/Core/API/TextureView.h"
#include "Rendering/Core/API/AccelerationStructure.h"
#include "Rendering/RenderAPI.h"
#include "Rendering/RenderGraph.h"
#include "Rendering/RenderGraphSerializer.h"
#include "Rendering/ImGuiRenderer.h"

#include "Application/API/Window.h"
#include "Application/API/CommonApplication.h"

#include "ECS/ECSCore.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Rendering/MeshComponent.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Misc/MaskComponent.h"
#include "Game/ECS/Components/Misc/MeshPaintComponent.h"
#include "Game/ECS/Components/Rendering/PointLightComponent.h"
#include "Game/ECS/Components/Rendering/DirectionalLightComponent.h"

#include "Engine/EngineConfig.h"

namespace LambdaEngine
{
	RenderSystem RenderSystem::s_Instance;

	bool RenderSystem::Init()
	{
		GraphicsDeviceFeatureDesc deviceFeatures;
		RenderAPI::GetDevice()->QueryDeviceFeatures(&deviceFeatures);
		m_RayTracingEnabled = deviceFeatures.RayTracing && EngineConfig::GetBoolProperty("RayTracingEnabled");

		TransformComponents transformComponents;
		transformComponents.Position.Permissions	= R;
		transformComponents.Scale.Permissions		= R;
		transformComponents.Rotation.Permissions	= R;

		// Subscribe on Static Entities & Dynamic Entities
		{
			SystemRegistration systemReg = {};
			systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
			{
				{{{RW, MeshComponent::s_TID}},	{&transformComponents}, &m_RenderableEntities, std::bind(&RenderSystem::OnEntityAdded, this, std::placeholders::_1), std::bind(&RenderSystem::OnEntityRemoved, this, std::placeholders::_1)},
				{{{RW, DirectionalLightComponent::s_TID}, {R, RotationComponent::s_TID}}, &m_DirectionalLightEntities,			std::bind(&RenderSystem::OnDirectionalEntityAdded, this, std::placeholders::_1), std::bind(&RenderSystem::OnDirectionalEntityRemoved, this, std::placeholders::_1)},
				{{{RW, PointLightComponent::s_TID}, {R, PositionComponent::s_TID}}, &m_PointLightEntities,								std::bind(&RenderSystem::OnPointLightEntityAdded, this, std::placeholders::_1), std::bind(&RenderSystem::OnPointLightEntityRemoved, this, std::placeholders::_1) },
				{{{RW, ViewProjectionMatricesComponent::s_TID}, {R, CameraComponent::s_TID}}, {&transformComponents}, &m_CameraEntities},
			};
			systemReg.SubscriberRegistration.AdditionalDependencies = { {{R, MaskComponent::s_TID}} };
			systemReg.SubscriberRegistration.AdditionalDependencies = { {{R, MeshPaintComponent::s_TID}} };
			systemReg.Phase = g_LastPhase;

			RegisterSystem(systemReg);
		}

		//Create Swapchain
		{
			SwapChainDesc swapChainDesc = {};
			swapChainDesc.DebugName		= "Renderer Swap Chain";
			swapChainDesc.pWindow		= CommonApplication::Get()->GetActiveWindow().Get();
			swapChainDesc.pQueue		= RenderAPI::GetGraphicsQueue();
			swapChainDesc.Format		= EFormat::FORMAT_B8G8R8A8_UNORM;
			swapChainDesc.Width			= 0;
			swapChainDesc.Height		= 0;
			swapChainDesc.BufferCount	= BACK_BUFFER_COUNT;
			swapChainDesc.SampleCount	= 1;
			swapChainDesc.VerticalSync	= false;

			m_SwapChain = RenderAPI::GetDevice()->CreateSwapChain(&swapChainDesc);
			if (!m_SwapChain)
			{
				LOG_ERROR("[Renderer]: SwapChain is nullptr after initializaiton");
				return false;
			}

			m_ppBackBuffers = DBG_NEW Texture*[BACK_BUFFER_COUNT];
			m_ppBackBufferViews = DBG_NEW TextureView*[BACK_BUFFER_COUNT];

			m_FrameIndex++;
			m_ModFrameIndex = m_FrameIndex % uint64(BACK_BUFFER_COUNT);
		}

		//Create RenderGraph
		{
			RenderGraphStructureDesc renderGraphStructure = {};

			String prefix = m_RayTracingEnabled ? "RT_" : "";

			if (!RenderGraphSerializer::LoadAndParse(&renderGraphStructure, prefix + EngineConfig::GetStringProperty("RenderGraphName"), IMGUI_ENABLED))
			{
				LOG_ERROR("[RenderSystem]: Failed to Load RenderGraph, loading Default...");

				renderGraphStructure = {};
				RenderGraphSerializer::LoadAndParse(&renderGraphStructure, "", true);
			}

			RenderGraphDesc renderGraphDesc = {};
			renderGraphDesc.Name						= "Default Rendergraph";
			renderGraphDesc.pRenderGraphStructureDesc	= &renderGraphStructure;
			renderGraphDesc.BackBufferCount				= BACK_BUFFER_COUNT;

			m_pRenderGraph = DBG_NEW RenderGraph(RenderAPI::GetDevice());
			if (!m_pRenderGraph->Init(&renderGraphDesc, m_RequiredDrawArgs))
			{
				LOG_ERROR("[RenderSystem]: Failed to initialize RenderGraph");
				return false;
			}
		}

		//Update RenderGraph with Back Buffer
		{
			for (uint32 v = 0; v < BACK_BUFFER_COUNT; v++)
			{
				m_ppBackBuffers[v]		= m_SwapChain->GetBuffer(v);
				m_ppBackBufferViews[v]	= m_SwapChain->GetBufferView(v);
			}

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName							= RENDER_GRAPH_BACK_BUFFER_ATTACHMENT;
			resourceUpdateDesc.ExternalTextureUpdate.ppTextures		= m_ppBackBuffers;
			resourceUpdateDesc.ExternalTextureUpdate.ppTextureViews = m_ppBackBufferViews;

			m_pRenderGraph->UpdateResource(&resourceUpdateDesc);
		}

		// Per Frame Buffer
		{
			for (uint32 b = 0; b < BACK_BUFFER_COUNT; b++)
			{
				BufferDesc perFrameCopyBufferDesc = {};
				perFrameCopyBufferDesc.DebugName		= "Scene Per Frame Staging Buffer " + std::to_string(b);
				perFrameCopyBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
				perFrameCopyBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_SRC;
				perFrameCopyBufferDesc.SizeInBytes		= sizeof(PerFrameBuffer);

				m_ppPerFrameStagingBuffers[b] = RenderAPI::GetDevice()->CreateBuffer(&perFrameCopyBufferDesc);
			}

			BufferDesc perFrameBufferDesc = {};
			perFrameBufferDesc.DebugName			= "Scene Per Frame Buffer";
			perFrameBufferDesc.MemoryType			= EMemoryType::MEMORY_TYPE_GPU;
			perFrameBufferDesc.Flags				= FBufferFlag::BUFFER_FLAG_CONSTANT_BUFFER | FBufferFlag::BUFFER_FLAG_COPY_DST;
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
				m_pMaterialInstanceCounts[i]	= 0;
			}
		}

		UpdateBuffers();

		return true;
	}

	bool RenderSystem::Release()
	{
		for (MeshAndInstancesMap::iterator meshAndInstancesIt = m_MeshAndInstancesMap.begin(); meshAndInstancesIt != m_MeshAndInstancesMap.end(); meshAndInstancesIt++)
		{
			SAFERELEASE(meshAndInstancesIt->second.pBLAS);
			SAFERELEASE(meshAndInstancesIt->second.pVertexBuffer);
			SAFERELEASE(meshAndInstancesIt->second.pIndexBuffer);
			SAFERELEASE(meshAndInstancesIt->second.pRasterInstanceBuffer);
			SAFERELEASE(meshAndInstancesIt->second.pASInstanceBuffer);

			for (uint32 b = 0; b < BACK_BUFFER_COUNT; b++)
			{
				SAFERELEASE(meshAndInstancesIt->second.ppASInstanceStagingBuffers[b]);
				SAFERELEASE(meshAndInstancesIt->second.ppRasterInstanceStagingBuffers[b]);
			}
		}

		SAFERELEASE(m_pTLAS);
		SAFERELEASE(m_pCompleteInstanceBuffer);

		for (uint32 b = 0; b < BACK_BUFFER_COUNT; b++)
		{
			TArray<DeviceChild*>& resourcesToRemove = m_ResourcesToRemove[b];

			for (DeviceChild* pResource : resourcesToRemove)
			{
				SAFERELEASE(pResource);
			}

			resourcesToRemove.Clear();

			SAFERELEASE(m_ppMaterialParametersStagingBuffers[b]);
			SAFERELEASE(m_ppPerFrameStagingBuffers[b]);
			SAFERELEASE(m_ppStaticStagingInstanceBuffers[b]);
			SAFERELEASE(m_ppLightsStagingBuffer[b]);
		}

		SAFERELEASE(m_pMaterialParametersBuffer);
		SAFERELEASE(m_pPerFrameBuffer);
		SAFERELEASE(m_pLightsBuffer);

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

	void RenderSystem::Tick(Timestamp deltaTime)
	{
		UNREFERENCED_VARIABLE(deltaTime);

		ECSCore* pECSCore = ECSCore::GetInstance();

		ComponentArray<PositionComponent>*	pPositionComponents = pECSCore->GetComponentArray<PositionComponent>();
		ComponentArray<RotationComponent>*	pRotationComponents = pECSCore->GetComponentArray<RotationComponent>();
		ComponentArray<ScaleComponent>*		pScaleComponents	= pECSCore->GetComponentArray<ScaleComponent>();

		ComponentArray<PointLightComponent>* pPointLightComponents = pECSCore->GetComponentArray<PointLightComponent>();
		for (Entity entity : m_PointLightEntities.GetIDs())
		{
			auto& pointLight = pPointLightComponents->GetData(entity);
			auto& position = pPositionComponents->GetData(entity);
			if (position.Dirty)
			{
				UpdatePointLight(entity, position.Position, pointLight.ColorIntensity);
			}
		}

		ComponentArray<DirectionalLightComponent>* pDirLightComponents = pECSCore->GetComponentArray<DirectionalLightComponent>();
		for (Entity entity : m_DirectionalLightEntities.GetIDs())
		{
			auto& dirLight = pDirLightComponents->GetData(entity);
			auto& rotation = pRotationComponents->GetData(entity);
			if (rotation.Dirty)
			{
				UpdateDirectionalLight(entity, dirLight.ColorIntensity, rotation.Quaternion);
			}
		}

		ComponentArray<CameraComponent>*	pCameraComponents = pECSCore->GetComponentArray<CameraComponent>();
		for (Entity entity : m_CameraEntities.GetIDs())
		{
			auto& cameraComp = pCameraComponents->GetData(entity);
			if (cameraComp.IsActive)
			{
				UpdateCamera(entity);
			}
		}

		for (Entity entity : m_RenderableEntities)
		{
			auto& positionComp	= pPositionComponents->GetData(entity);
			auto& rotationComp	= pRotationComponents->GetData(entity);
			auto& scaleComp		= pScaleComponents->GetData(entity);

			if (positionComp.Dirty || rotationComp.Dirty || scaleComp.Dirty)
			{
				glm::mat4 transform = glm::translate(glm::identity<glm::mat4>(), positionComp.Position);
				transform *= glm::toMat4(rotationComp.Quaternion);
				transform = glm::scale(transform, scaleComp.Scale);

				//rotationComp.Quaternion = glm::rotate(rotationComp.Quaternion, glm::degrees(1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

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

		m_FrameIndex++;
		m_ModFrameIndex = m_FrameIndex % uint64(BACK_BUFFER_COUNT);

		CleanBuffers();
		UpdateBuffers();
		UpdateRenderGraph();

		m_pRenderGraph->Update();

		m_pRenderGraph->Render(m_ModFrameIndex, m_BackBufferIndex);

		m_SwapChain->Present();

		return true;
	}

	void RenderSystem::SetRenderGraph(const String& name, RenderGraphStructureDesc* pRenderGraphStructureDesc)
	{
		RenderGraphDesc renderGraphDesc = {};
		renderGraphDesc.Name						= name;
		renderGraphDesc.pRenderGraphStructureDesc	= pRenderGraphStructureDesc;
		renderGraphDesc.BackBufferCount				= BACK_BUFFER_COUNT;

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

	void RenderSystem::OnEntityAdded(Entity entity)
	{
		ECSCore* pECSCore = ECSCore::GetInstance();

		auto& positionComp	= pECSCore->GetComponent<PositionComponent>(entity);
		auto& rotationComp	= pECSCore->GetComponent<RotationComponent>(entity);
		auto& scaleComp		= pECSCore->GetComponent<ScaleComponent>(entity);
		auto& meshComp		= pECSCore->GetComponent<MeshComponent>(entity);

		glm::mat4 transform = glm::translate(glm::identity<glm::mat4>(), positionComp.Position);
		transform *= glm::toMat4(rotationComp.Quaternion);
		transform = glm::scale(transform, scaleComp.Scale);

		AddEntityInstance(entity, meshComp.MeshGUID, meshComp.MaterialGUID, transform, false);
	}

	void RenderSystem::OnEntityRemoved(Entity entity)
	{
		RemoveEntityInstance(entity);
	}

	void RenderSystem::OnDirectionalEntityAdded(Entity entity)
	{
		if (!m_DirectionalExist)
		{
			ECSCore* pECSCore = ECSCore::GetInstance();

			auto& pointLightComp = pECSCore->GetComponent<DirectionalLightComponent>(entity);
			auto& rotation = pECSCore->GetComponent<RotationComponent>(entity);

			m_LightBufferData.ColorIntensity	= pointLightComp.ColorIntensity;
			m_LightBufferData.Direction			= GetForward(rotation.Quaternion);

			m_DirectionalExist = true;
			m_LightsDirty = true;
		}
		else
		{
			LOG_WARNING("Multiple directional lights not supported!");
		}
	}

	void RenderSystem::OnPointLightEntityAdded(Entity entity)
	{
		ECSCore* pECSCore = ECSCore::GetInstance();

		auto& pointLightComp = pECSCore->GetComponent<PointLightComponent>(entity);
		auto& position = pECSCore->GetComponent<PositionComponent>(entity);
	
		uint32 pointLightIndex = m_PointLights.GetSize();
		m_EntityToPointLight[entity] = pointLightIndex;
		m_PointLightToEntity[pointLightIndex] = entity;

		m_PointLights.PushBack(PointLight{.ColorIntensity = pointLightComp.ColorIntensity, .Position = position.Position});

		m_LightsDirty = true;
	}

	void RenderSystem::OnDirectionalEntityRemoved(Entity entity)
	{
		UNREFERENCED_VARIABLE(entity);

		m_LightBufferData.ColorIntensity = glm::vec4(0.f);
		m_DirectionalExist = false;
		m_LightsDirty = true;
	}

	void RenderSystem::OnPointLightEntityRemoved(Entity entity)
	{
		uint32 lastIndex = m_PointLights.GetSize() - 1U;
		uint32 lastEntity = m_PointLightToEntity[lastIndex];
		uint32 currentIndex = m_EntityToPointLight[entity];

		m_PointLights[currentIndex] = m_PointLights[lastIndex];
		m_EntityToPointLight[lastEntity] = currentIndex;

		m_EntityToPointLight.erase(entity);
		m_PointLights.PopBack();

		m_LightsDirty = true;
	}

	void RenderSystem::AddEntityInstance(Entity entity, GUID_Lambda meshGUID, GUID_Lambda materialGUID, const glm::mat4& transform, bool animated)
	{
		//auto& component = ECSCore::GetInstance().GetComponent<StaticMeshComponent>(Entity);

		uint32 materialSlot = MAX_UNIQUE_MATERIALS;
		MeshAndInstancesMap::iterator meshAndInstancesIt;

		MeshKey meshKey;
		meshKey.MeshGUID		= meshGUID;
		meshKey.IsAnimated		= animated;
		meshKey.EntityID		= entity;

		//Get meshAndInstancesIterator
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
					vertexBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | FBufferFlag::BUFFER_FLAG_RAY_TRACING;
					vertexBufferDesc.SizeInBytes	= vertexStagingBufferDesc.SizeInBytes;

					meshEntry.pVertexBuffer = RenderAPI::GetDevice()->CreateBuffer(&vertexBufferDesc);
					meshEntry.VertexCount	= pMesh->VertexCount;

					m_PendingBufferUpdates.PushBack({ pVertexStagingBuffer, 0, meshEntry.pVertexBuffer, 0, vertexBufferDesc.SizeInBytes });
					m_ResourcesToRemove[m_ModFrameIndex].PushBack(pVertexStagingBuffer);
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
					indexBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_INDEX_BUFFER | FBufferFlag::BUFFER_FLAG_RAY_TRACING;
					indexBufferDesc.SizeInBytes		= indexStagingBufferDesc.SizeInBytes;

					meshEntry.pIndexBuffer	= RenderAPI::GetDevice()->CreateBuffer(&indexBufferDesc);
					meshEntry.IndexCount	= pMesh->IndexCount;

					m_PendingBufferUpdates.PushBack({ pIndexStagingBuffer, 0, meshEntry.pIndexBuffer, 0, indexBufferDesc.SizeInBytes });
					m_ResourcesToRemove[m_ModFrameIndex].PushBack(pIndexStagingBuffer);
				}

				meshAndInstancesIt = m_MeshAndInstancesMap.insert({ meshKey, meshEntry }).first;

				if (m_RayTracingEnabled)
				{
					meshAndInstancesIt->second.ShaderRecord.VertexBufferAddress	= meshEntry.pVertexBuffer->GetDeviceAdress();
					meshAndInstancesIt->second.ShaderRecord.IndexBufferAddress	= meshEntry.pIndexBuffer->GetDeviceAdress();
					m_DirtyBLASs.insert(&meshAndInstancesIt->second);
					m_SBTRecordsDirty = true;
				}
			}
		}

		//Get Material Slot
		{
			THashTable<uint32, uint32>::iterator materialSlotIt = m_MaterialMap.find(materialGUID);

			//Push new Material if the Material is yet to be registered
			if (materialSlotIt == m_MaterialMap.end())
			{
				const Material* pMaterial = ResourceManager::GetMaterial(materialGUID);
				VALIDATE(pMaterial != nullptr);

				if (!m_FreeMaterialSlots.empty())
				{
					materialSlot = m_FreeMaterialSlots.top();
					m_FreeMaterialSlots.pop();
				}
				else
				{
					for (uint32 m = 0; m < MAX_UNIQUE_MATERIALS; m++)
					{
						if (m_pMaterialInstanceCounts[m] == 0)
						{
							materialSlot = m;
							break;
						}
					}

					if (materialSlot == MAX_UNIQUE_MATERIALS)
					{
						LOG_WARNING("[RenderSystem]: No free Material Slots, Entity will be given a random material");
						materialSlot = 0;
					}
				}

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
				m_MaterialsPropertiesBufferDirty = true;
			}
			else
			{
				materialSlot = materialSlotIt->second;
			}

			m_pMaterialInstanceCounts[materialSlot]++;
		}

		InstanceKey instanceKey = {};
		instanceKey.MeshKey			= meshKey;
		instanceKey.InstanceIndex	= meshAndInstancesIt->second.RasterInstances.GetSize();
		m_EntityIDsToInstanceKey[entity] = instanceKey;

		if (m_RayTracingEnabled)
		{
			AccelerationStructureInstance asInstance = {};
			asInstance.Transform		= glm::transpose(transform);
			asInstance.CustomIndex		= materialSlot;
			asInstance.Mask				= 0xFF;
			asInstance.SBTRecordOffset	= 0;
			asInstance.Flags			= RAY_TRACING_INSTANCE_FLAG_FORCE_OPAQUE;

			meshAndInstancesIt->second.ASInstances.PushBack(asInstance);
			m_DirtyASInstanceBuffers.insert(&meshAndInstancesIt->second);
			m_TLASDirty = true;
		}

		Instance instance = {};
		instance.Transform			= transform;
		instance.PrevTransform		= transform;
		instance.MaterialSlot		= materialSlot;
		meshAndInstancesIt->second.RasterInstances.PushBack(instance);

		m_DirtyRasterInstanceBuffers.insert(&meshAndInstancesIt->second);

		// Fetch the draw arg mask from the entity if it has a mask component.
		uint32 drawArgHash = UINT32_MAX;
		ComponentArray<MaskComponent>* pMaskComponents = ECSCore::GetInstance()->GetComponentArray<MaskComponent>();
		if (pMaskComponents && pMaskComponents->HasComponent(entity))
			drawArgHash = pMaskComponents->GetData(entity).Mask;

		GUID_Lambda texture = GUID_NONE;
		ComponentArray<MeshPaintComponent>* pMeshPaintComponents = ECSCore::GetInstance()->GetComponentArray<MeshPaintComponent>();
		if (pMeshPaintComponents && pMeshPaintComponents->HasComponent(entity))
			texture = pMeshPaintComponents->GetData(entity).UnwrappedTexture;

		if (m_RequiredDrawArgs.count(drawArgHash))
		{
			m_DirtyDrawArgs.insert(drawArgHash);
		}
	}

	void RenderSystem::RemoveEntityInstance(Entity entity)
	{
		THashTable<GUID_Lambda, InstanceKey>::iterator instanceKeyIt = m_EntityIDsToInstanceKey.find(entity);

		if (instanceKeyIt == m_EntityIDsToInstanceKey.end())
		{
			LOG_ERROR("[RenderSystem]: Tried to remove entity which does not exist");
			return;
		}

		MeshAndInstancesMap::iterator meshAndInstancesIt = m_MeshAndInstancesMap.find(instanceKeyIt->second.MeshKey);

		if (meshAndInstancesIt == m_MeshAndInstancesMap.end())
		{
			LOG_ERROR("[RenderSystem]: Tried to remove entity which has no MeshAndInstancesMap entry");
			return;
		}

		const Instance& rasterInstance = meshAndInstancesIt->second.RasterInstances[instanceKeyIt->second.InstanceIndex];

		//Update Material Instance Counts
		{
			m_pMaterialInstanceCounts[rasterInstance.MaterialSlot]--;
		}

		if (m_RayTracingEnabled)
		{
			meshAndInstancesIt->second.ASInstances.Erase(meshAndInstancesIt->second.ASInstances.Begin() + instanceKeyIt->second.InstanceIndex);
			m_DirtyASInstanceBuffers.insert(&meshAndInstancesIt->second);
			m_TLASDirty = true;
		}

		meshAndInstancesIt->second.RasterInstances.Erase(meshAndInstancesIt->second.RasterInstances.Begin() + instanceKeyIt->second.InstanceIndex);
		m_DirtyRasterInstanceBuffers.insert(&meshAndInstancesIt->second);

		// Delete instanceKey it will not be used anymore
		m_EntityIDsToInstanceKey.erase(instanceKeyIt);

		//Unload Mesh, Todo: Should we always do this?
		if (meshAndInstancesIt->second.RasterInstances.IsEmpty())
		{
			m_ResourcesToRemove[m_ModFrameIndex].PushBack(meshAndInstancesIt->second.pBLAS);
			m_ResourcesToRemove[m_ModFrameIndex].PushBack(meshAndInstancesIt->second.pVertexBuffer);
			m_ResourcesToRemove[m_ModFrameIndex].PushBack(meshAndInstancesIt->second.pIndexBuffer);
			m_ResourcesToRemove[m_ModFrameIndex].PushBack(meshAndInstancesIt->second.pRasterInstanceBuffer);
			m_ResourcesToRemove[m_ModFrameIndex].PushBack(meshAndInstancesIt->second.pASInstanceBuffer);

			for (uint32 b = 0; b < BACK_BUFFER_COUNT; b++)
			{
				m_ResourcesToRemove[m_ModFrameIndex].PushBack(meshAndInstancesIt->second.ppASInstanceStagingBuffers[b]);
				m_ResourcesToRemove[m_ModFrameIndex].PushBack(meshAndInstancesIt->second.ppRasterInstanceStagingBuffers[b]);
			}

			m_DirtyDrawArgs = m_RequiredDrawArgs;
			m_SBTRecordsDirty = true;

			m_MeshAndInstancesMap.erase(meshAndInstancesIt);
		}
	}

	void RenderSystem::UpdateDirectionalLight(Entity entity, glm::vec4& colorIntensity, glm::quat& direction)
	{
		UNREFERENCED_VARIABLE(entity);

		m_LightBufferData.ColorIntensity	= colorIntensity;
		m_LightBufferData.Direction			= GetForward(direction);
		m_LightsDirty = true;
	}

	void RenderSystem::UpdatePointLight(Entity entity, const glm::vec3& position, glm::vec4& colorIntensity)
	{
		if (m_EntityToPointLight.find(entity) == m_EntityToPointLight.end())
		{
			LOG_ERROR("Entity non-existing in PointLight map!");
			return;
		}
		uint32 index = m_EntityToPointLight[entity];

		m_PointLights[index].ColorIntensity = colorIntensity;
		m_PointLights[index].Position = position;
		
		m_LightsDirty = true;
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

		if (m_RayTracingEnabled)
		{
			AccelerationStructureInstance* pASInstanceToUpdate = &meshAndInstancesIt->second.ASInstances[instanceKeyIt->second.InstanceIndex];
			pASInstanceToUpdate->Transform = glm::transpose(transform);
			m_DirtyASInstanceBuffers.insert(&meshAndInstancesIt->second);
			m_TLASDirty = true;
		}

		Instance* pRasterInstanceToUpdate = &meshAndInstancesIt->second.RasterInstances[instanceKeyIt->second.InstanceIndex];
		pRasterInstanceToUpdate->PrevTransform	= pRasterInstanceToUpdate->Transform;
		pRasterInstanceToUpdate->Transform		= transform;
		m_DirtyRasterInstanceBuffers.insert(&meshAndInstancesIt->second);
	}

	void RenderSystem::UpdateCamera(Entity entity)
	{
		ViewProjectionMatricesComponent& viewProjComp = ECSCore::GetInstance()->GetComponent<ViewProjectionMatricesComponent>(entity);
		PositionComponent& posComp	= ECSCore::GetInstance()->GetComponent<PositionComponent>(entity);
		RotationComponent& rotComp	= ECSCore::GetInstance()->GetComponent<RotationComponent>(entity);
		CameraComponent& camComp	= ECSCore::GetInstance()->GetComponent<CameraComponent>(entity);
		m_PerFrameData.CamData.PrevView			= m_PerFrameData.CamData.View;
		m_PerFrameData.CamData.PrevProjection	= m_PerFrameData.CamData.Projection;
		m_PerFrameData.CamData.View				= viewProjComp.View;
		m_PerFrameData.CamData.Projection		= viewProjComp.Projection;
		m_PerFrameData.CamData.ViewInv			= camComp.ViewInv;
		m_PerFrameData.CamData.ProjectionInv	= camComp.ProjectionInv;
		m_PerFrameData.CamData.Position			= glm::vec4(posComp.Position, 0.f);
		m_PerFrameData.CamData.Up				= glm::vec4(GetUp(rotComp.Quaternion), 0.f);
		m_PerFrameData.CamData.Jitter			= camComp.Jitter;
	}

	void RenderSystem::CleanBuffers()
	{
		//Todo: Better solution for this, save some Staging Buffers maybe so they don't get recreated all the time?
		TArray<DeviceChild*>& resourcesToRemove = m_ResourcesToRemove[m_ModFrameIndex];

		for (DeviceChild* pResource : resourcesToRemove)
		{
			SAFERELEASE(pResource);
		}

		resourcesToRemove.Clear();
	}

	void RenderSystem::CreateDrawArgs(TArray<DrawArg>& drawArgs, uint32 mask) const
	{
		UNREFERENCED_VARIABLE(mask);

		for (MeshAndInstancesMap::const_iterator meshAndInstancesIt = m_MeshAndInstancesMap.begin(); meshAndInstancesIt != m_MeshAndInstancesMap.end(); meshAndInstancesIt++)
		{
			//Todo: Check Key (or whatever we end up using)
			DrawArg drawArg = {};
			drawArg.pVertexBuffer		= meshAndInstancesIt->second.pVertexBuffer;
			drawArg.VertexBufferSize	= meshAndInstancesIt->second.pVertexBuffer->GetDesc().SizeInBytes;
			drawArg.pIndexBuffer		= meshAndInstancesIt->second.pIndexBuffer;
			drawArg.IndexCount			= meshAndInstancesIt->second.IndexCount;
			drawArg.pInstanceBuffer		= meshAndInstancesIt->second.pRasterInstanceBuffer;
			drawArg.InstanceBufferSize	= meshAndInstancesIt->second.pRasterInstanceBuffer->GetDesc().SizeInBytes;
			drawArg.InstanceCount		= meshAndInstancesIt->second.RasterInstances.GetSize();
			drawArgs.PushBack(drawArg);
		}
	}

	void RenderSystem::UpdateBuffers()
	{
		CommandList* pGraphicsCommandList = m_pRenderGraph->AcquireGraphicsCopyCommandList();
		CommandList* pComputeCommandList = m_pRenderGraph->AcquireComputeCopyCommandList();

		//Update Pending Buffer Updates
		{
			ExecutePendingBufferUpdates(pGraphicsCommandList);
		}

		//Update Per Frame Data
		{
			m_PerFrameData.FrameIndex = 0;
			m_PerFrameData.RandomSeed = uint32(Random::Int32(INT32_MIN, INT32_MAX));

			UpdatePerFrameBuffer(pGraphicsCommandList);
		}

		//Update Raster Instance Data
		{
			UpdateRasterInstanceBuffers(pGraphicsCommandList);
		}

		// Update Light Data
		{
			UpdateLightsBuffer(pGraphicsCommandList);
		}

		//Update Empty MaterialData
		{
			UpdateMaterialPropertiesBuffer(pGraphicsCommandList);
		}

		//Update Acceleration Structures
		if (m_RayTracingEnabled)
		{
			UpdateShaderRecords();
			BuildBLASs(pComputeCommandList);
			UpdateASInstanceBuffers(pComputeCommandList);
			BuildTLAS(pComputeCommandList);
		}
	}

	void RenderSystem::ExecutePendingBufferUpdates(CommandList* pCommandList)
	{
		if (!m_PendingBufferUpdates.IsEmpty())
		{
			for (uint32 i = 0; i < m_PendingBufferUpdates.GetSize(); i++)
			{
				const PendingBufferUpdate& pendingUpdate = m_PendingBufferUpdates[i];
				pCommandList->CopyBuffer(pendingUpdate.pSrcBuffer, pendingUpdate.SrcOffset, pendingUpdate.pDstBuffer, pendingUpdate.DstOffset, pendingUpdate.SizeInBytes);
			}

			m_PendingBufferUpdates.Clear();
		}
	}

	void RenderSystem::UpdateRasterInstanceBuffers(CommandList* pCommandList)
	{
		for (MeshEntry* pDirtyInstanceBufferEntry : m_DirtyRasterInstanceBuffers)
		{
			//Raster Instances
			{
				uint32 requiredBufferSize = pDirtyInstanceBufferEntry->RasterInstances.GetSize() * sizeof(Instance);

				Buffer* pStagingBuffer = pDirtyInstanceBufferEntry->ppRasterInstanceStagingBuffers[m_ModFrameIndex];

				if (pStagingBuffer == nullptr || pStagingBuffer->GetDesc().SizeInBytes < requiredBufferSize)
				{
					if (pStagingBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(pStagingBuffer);

					BufferDesc bufferDesc = {};
					bufferDesc.DebugName	= "Raster Instance Staging Buffer";
					bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
					bufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_SRC;
					bufferDesc.SizeInBytes	= requiredBufferSize;

					pStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
					pDirtyInstanceBufferEntry->ppRasterInstanceStagingBuffers[m_ModFrameIndex] = pStagingBuffer;
				}

				void* pMapped = pStagingBuffer->Map();
				memcpy(pMapped, pDirtyInstanceBufferEntry->RasterInstances.GetData(), requiredBufferSize);
				pStagingBuffer->Unmap();

				if (pDirtyInstanceBufferEntry->pRasterInstanceBuffer == nullptr || pDirtyInstanceBufferEntry->pRasterInstanceBuffer->GetDesc().SizeInBytes < requiredBufferSize)
				{
					if (pDirtyInstanceBufferEntry->pRasterInstanceBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(pDirtyInstanceBufferEntry->pRasterInstanceBuffer);

					BufferDesc bufferDesc = {};
					bufferDesc.DebugName		= "Raster Instance Buffer";
					bufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_GPU;
					bufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER;
					bufferDesc.SizeInBytes		= requiredBufferSize;

					pDirtyInstanceBufferEntry->pRasterInstanceBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
				}

				pCommandList->CopyBuffer(pStagingBuffer, 0, pDirtyInstanceBufferEntry->pRasterInstanceBuffer, 0, requiredBufferSize);
			}
		}

		m_DirtyRasterInstanceBuffers.clear();
	}

	void RenderSystem::UpdatePerFrameBuffer(CommandList* pCommandList)
	{
		Buffer* pPerFrameStagingBuffer = m_ppPerFrameStagingBuffers[m_ModFrameIndex];

		void* pMapped = pPerFrameStagingBuffer->Map();
		memcpy(pMapped, &m_PerFrameData, sizeof(PerFrameBuffer));
		pPerFrameStagingBuffer->Unmap();

		pCommandList->CopyBuffer(pPerFrameStagingBuffer, 0, m_pPerFrameBuffer, 0, sizeof(PerFrameBuffer));
	}

	void RenderSystem::UpdateMaterialPropertiesBuffer(CommandList* pCommandList)
	{
		if (m_MaterialsPropertiesBufferDirty)
		{
			uint32 requiredBufferSize = sizeof(m_pMaterialProperties);

			Buffer* pStagingBuffer = m_ppStaticStagingInstanceBuffers[m_ModFrameIndex];

			if (pStagingBuffer == nullptr || pStagingBuffer->GetDesc().SizeInBytes < requiredBufferSize)
			{
				if (pStagingBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(pStagingBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.DebugName	= "Material Properties Staging Buffer";
				bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
				bufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_SRC;
				bufferDesc.SizeInBytes	= requiredBufferSize;

				pStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
				m_ppStaticStagingInstanceBuffers[m_ModFrameIndex] = pStagingBuffer;
			}

			void* pMapped = pStagingBuffer->Map();
			memcpy(pMapped, m_pMaterialProperties, requiredBufferSize);
			pStagingBuffer->Unmap();

			if (m_pMaterialParametersBuffer == nullptr || m_pMaterialParametersBuffer->GetDesc().SizeInBytes < requiredBufferSize)
			{
				if (m_pMaterialParametersBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(m_pMaterialParametersBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.DebugName	= "Material Properties Buffer";
				bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
				bufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_CONSTANT_BUFFER;
				bufferDesc.SizeInBytes	= requiredBufferSize;

				m_pMaterialParametersBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			}

			pCommandList->CopyBuffer(pStagingBuffer, 0, m_pMaterialParametersBuffer, 0, requiredBufferSize);

			m_MaterialsPropertiesBufferDirty = false;
		}
	}

	void RenderSystem::UpdateShaderRecords()
	{
		if (m_SBTRecordsDirty)
		{
			m_SBTRecords.Clear();

			for (MeshAndInstancesMap::iterator meshAndInstancesIt = m_MeshAndInstancesMap.begin(); meshAndInstancesIt != m_MeshAndInstancesMap.end(); meshAndInstancesIt++)
			{
				uint32 shaderRecordOffset = m_SBTRecords.GetSize();

				for (AccelerationStructureInstance& asInstance : meshAndInstancesIt->second.ASInstances)
				{
					asInstance.SBTRecordOffset = shaderRecordOffset;
				}

				m_SBTRecords.PushBack(meshAndInstancesIt->second.ShaderRecord);
				m_DirtyASInstanceBuffers.insert(&meshAndInstancesIt->second);
			}

			m_SBTRecordsDirty = false;
			m_TLASDirty = true;
			m_RenderGraphSBTRecordsDirty = true;
		}
	}

	void RenderSystem::BuildBLASs(CommandList* pCommandList)
	{
		if (!m_DirtyBLASs.empty())
		{
			for (MeshEntry* pDirtyBLAS : m_DirtyBLASs)
			{
				//We assume that VertexCount/PrimitiveCount does not change and thus do not check if we need to recreate them

				bool update = true;

				if (pDirtyBLAS->pBLAS == nullptr)
				{
					update = false;

					AccelerationStructureDesc blasCreateDesc = {};
					blasCreateDesc.DebugName		= "BLAS";
					blasCreateDesc.Type				= EAccelerationStructureType::ACCELERATION_STRUCTURE_TYPE_BOTTOM;
					blasCreateDesc.Flags			= FAccelerationStructureFlag::ACCELERATION_STRUCTURE_FLAG_ALLOW_UPDATE;
					blasCreateDesc.MaxTriangleCount = pDirtyBLAS->IndexCount / 3;
					blasCreateDesc.MaxVertexCount	= pDirtyBLAS->VertexCount;
					blasCreateDesc.AllowsTransform	= false;

					pDirtyBLAS->pBLAS = RenderAPI::GetDevice()->CreateAccelerationStructure(&blasCreateDesc);
				}

				BuildBottomLevelAccelerationStructureDesc blasBuildDesc = {};
				blasBuildDesc.pAccelerationStructure	= pDirtyBLAS->pBLAS;
				blasBuildDesc.Flags						= FAccelerationStructureFlag::ACCELERATION_STRUCTURE_FLAG_ALLOW_UPDATE;
				blasBuildDesc.pVertexBuffer				= pDirtyBLAS->pVertexBuffer;
				blasBuildDesc.FirstVertexIndex			= 0;
				blasBuildDesc.VertexStride				= sizeof(Vertex);
				blasBuildDesc.pIndexBuffer				= pDirtyBLAS->pIndexBuffer;
				blasBuildDesc.IndexBufferByteOffset		= 0;
				blasBuildDesc.TriangleCount				= pDirtyBLAS->IndexCount / 3;
				blasBuildDesc.pTransformBuffer			= nullptr;
				blasBuildDesc.TransformByteOffset		= 0;
				blasBuildDesc.Update					= update;

				pCommandList->BuildBottomLevelAccelerationStructure(&blasBuildDesc);

				uint64 blasAddress = pDirtyBLAS->pBLAS->GetDeviceAdress();

				for (AccelerationStructureInstance& asInstance : pDirtyBLAS->ASInstances)
				{
					asInstance.AccelerationStructureAddress = blasAddress;
				}

				m_DirtyASInstanceBuffers.insert(pDirtyBLAS);
			}

			//This is required to sync up BLAS building with TLAS building, to make sure that the BLAS is built before the TLAS
			PipelineMemoryBarrierDesc memoryBarrier = {};
			memoryBarrier.SrcMemoryAccessFlags = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
			memoryBarrier.DstMemoryAccessFlags = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_READ;
			pCommandList->PipelineMemoryBarriers(FPipelineStageFlag::PIPELINE_STAGE_FLAG_TOP, FPipelineStageFlag::PIPELINE_STAGE_FLAG_COPY, &memoryBarrier, 1);

			m_DirtyBLASs.clear();
		}
	}

	void RenderSystem::UpdateASInstanceBuffers(CommandList* pCommandList)
	{
		if (!m_DirtyASInstanceBuffers.empty())
		{
			//AS Instances
			for (MeshEntry* pDirtyInstanceBufferEntry : m_DirtyASInstanceBuffers)
			{
				uint32 requiredBufferSize = pDirtyInstanceBufferEntry->ASInstances.GetSize() * sizeof(AccelerationStructureInstance);

				Buffer* pStagingBuffer = pDirtyInstanceBufferEntry->ppASInstanceStagingBuffers[m_ModFrameIndex];

				if (pStagingBuffer == nullptr || pStagingBuffer->GetDesc().SizeInBytes < requiredBufferSize)
				{
					if (pStagingBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(pStagingBuffer);

					BufferDesc bufferDesc = {};
					bufferDesc.DebugName = "AS Instance Staging Buffer";
					bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
					bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_SRC;
					bufferDesc.SizeInBytes = requiredBufferSize;

					pStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
					pDirtyInstanceBufferEntry->ppASInstanceStagingBuffers[m_ModFrameIndex] = pStagingBuffer;
				}

				void* pMapped = pStagingBuffer->Map();
				memcpy(pMapped, pDirtyInstanceBufferEntry->ASInstances.GetData(), requiredBufferSize);
				pStagingBuffer->Unmap();

				if (pDirtyInstanceBufferEntry->pASInstanceBuffer == nullptr || pDirtyInstanceBufferEntry->pASInstanceBuffer->GetDesc().SizeInBytes < requiredBufferSize)
				{
					if (pDirtyInstanceBufferEntry->pASInstanceBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(pDirtyInstanceBufferEntry->pASInstanceBuffer);

					BufferDesc bufferDesc = {};
					bufferDesc.DebugName = "AS Instance Buffer";
					bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_GPU;
					bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_COPY_SRC;
					bufferDesc.SizeInBytes = requiredBufferSize;

					pDirtyInstanceBufferEntry->pASInstanceBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
				}

				pCommandList->CopyBuffer(pStagingBuffer, 0, pDirtyInstanceBufferEntry->pASInstanceBuffer, 0, requiredBufferSize);
			}

			PipelineMemoryBarrierDesc memoryBarrier = {};
			memoryBarrier.SrcMemoryAccessFlags = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
			memoryBarrier.DstMemoryAccessFlags = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_READ;
			pCommandList->PipelineMemoryBarriers(FPipelineStageFlag::PIPELINE_STAGE_FLAG_COPY, FPipelineStageFlag::PIPELINE_STAGE_FLAG_ACCELERATION_STRUCTURE_BUILD, &memoryBarrier, 1);

			m_DirtyASInstanceBuffers.clear();
		}
	}

	void RenderSystem::BuildTLAS(CommandList* pCommandList)
	{
		if (m_TLASDirty)
		{
			m_TLASDirty = false;
			m_CompleteInstanceBufferPendingCopies.Clear();

			uint32 newInstanceCount = 0;

			for (MeshAndInstancesMap::const_iterator meshAndInstancesIt = m_MeshAndInstancesMap.begin(); meshAndInstancesIt != m_MeshAndInstancesMap.end(); meshAndInstancesIt++)
			{
				uint32 instanceCount = meshAndInstancesIt->second.ASInstances.GetSize();

				PendingBufferUpdate copyToCompleteInstanceBuffer = {};
				copyToCompleteInstanceBuffer.pSrcBuffer		= meshAndInstancesIt->second.pASInstanceBuffer;
				copyToCompleteInstanceBuffer.SrcOffset		= 0;
				copyToCompleteInstanceBuffer.DstOffset		= newInstanceCount * sizeof(AccelerationStructureInstance);
				copyToCompleteInstanceBuffer.SizeInBytes	= instanceCount * sizeof(AccelerationStructureInstance);
				m_CompleteInstanceBufferPendingCopies.PushBack(copyToCompleteInstanceBuffer);

				newInstanceCount += instanceCount;
			}

			if (newInstanceCount == 0)
				return;

			uint32 requiredCompleteInstancesBufferSize = newInstanceCount * sizeof(AccelerationStructureInstance);

			if (m_pCompleteInstanceBuffer == nullptr || m_pCompleteInstanceBuffer->GetDesc().SizeInBytes < requiredCompleteInstancesBufferSize)
			{
				if (m_pCompleteInstanceBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(m_pCompleteInstanceBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.DebugName	= "Complete Instance Buffer";
				bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
				bufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_RAY_TRACING;
				bufferDesc.SizeInBytes	= requiredCompleteInstancesBufferSize;

				m_pCompleteInstanceBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			}

			for (const PendingBufferUpdate& pendingUpdate : m_CompleteInstanceBufferPendingCopies)
			{
				pCommandList->CopyBuffer(pendingUpdate.pSrcBuffer, pendingUpdate.SrcOffset, m_pCompleteInstanceBuffer, pendingUpdate.DstOffset, pendingUpdate.SizeInBytes);
			}

			if (m_MeshAndInstancesMap.empty())
				return;

			bool update = true;

			//Recreate TLAS completely if oldInstanceCount != newInstanceCount
			if (m_MaxInstances < newInstanceCount)
			{
				if (m_pTLAS != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(m_pTLAS);

				m_MaxInstances = newInstanceCount;

				AccelerationStructureDesc createTLASDesc = {};
				createTLASDesc.DebugName		= "TLAS";
				createTLASDesc.Type				= EAccelerationStructureType::ACCELERATION_STRUCTURE_TYPE_TOP;
				createTLASDesc.Flags			= FAccelerationStructureFlag::ACCELERATION_STRUCTURE_FLAG_ALLOW_UPDATE;
				createTLASDesc.InstanceCount	= m_MaxInstances;

				m_pTLAS = RenderAPI::GetDevice()->CreateAccelerationStructure(&createTLASDesc);

				update = false;

				m_TLASResourceDirty = true;
			}

			if (m_pTLAS != nullptr)
			{
				BuildTopLevelAccelerationStructureDesc buildTLASDesc = {};
				buildTLASDesc.pAccelerationStructure	= m_pTLAS;
				buildTLASDesc.Flags						= FAccelerationStructureFlag::ACCELERATION_STRUCTURE_FLAG_ALLOW_UPDATE;
				buildTLASDesc.Update					= update;
				buildTLASDesc.pInstanceBuffer			= m_pCompleteInstanceBuffer;
				buildTLASDesc.InstanceCount				= newInstanceCount;

				pCommandList->BuildTopLevelAccelerationStructure(&buildTLASDesc);
			}
		}
	}

	void RenderSystem::UpdateLightsBuffer(CommandList* pCommandList)
	{
		// Light Buffer Initilization
		if (m_LightsDirty)
		{
			size_t pointLightCount			= m_PointLights.GetSize();
			size_t dirLightBufferSize		= sizeof(LightBuffer);
			size_t pointLightsBufferSize	= sizeof(PointLight) * pointLightCount;
			size_t lightBufferSize			= dirLightBufferSize + pointLightsBufferSize;

			// Set point light count
			m_LightBufferData.PointLightCount = uint32(pointLightCount);

			Buffer* pCurrentStagingBuffer = m_ppLightsStagingBuffer[m_ModFrameIndex];

			if (pCurrentStagingBuffer == nullptr || pCurrentStagingBuffer->GetDesc().SizeInBytes < lightBufferSize)
			{
				if (pCurrentStagingBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(pCurrentStagingBuffer);

				BufferDesc lightCopyBufferDesc = {};
				lightCopyBufferDesc.DebugName		= "Lights Copy Buffer";
				lightCopyBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
				lightCopyBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_SRC;
				lightCopyBufferDesc.SizeInBytes		= lightBufferSize;

				pCurrentStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&lightCopyBufferDesc);
				m_ppLightsStagingBuffer[m_ModFrameIndex] = pCurrentStagingBuffer;
			}

			void* pMapped = pCurrentStagingBuffer->Map();
			memcpy(pMapped, &m_LightBufferData, dirLightBufferSize);
			if (pointLightsBufferSize > 0) memcpy((uint8*)pMapped + dirLightBufferSize, m_PointLights.GetData(), pointLightsBufferSize);
			pCurrentStagingBuffer->Unmap();

			if (m_pLightsBuffer == nullptr || m_pLightsBuffer->GetDesc().SizeInBytes < lightBufferSize)
			{
				if (m_pLightsBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(m_pLightsBuffer);

				BufferDesc lightBufferDesc = {};
				lightBufferDesc.DebugName		= "Lights Buffer";
				lightBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_GPU;
				lightBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | FBufferFlag::BUFFER_FLAG_COPY_DST;
				lightBufferDesc.SizeInBytes		= lightBufferSize;

				m_pLightsBuffer = RenderAPI::GetDevice()->CreateBuffer(&lightBufferDesc);

				m_LightsResourceDirty = true;
			}

			pCommandList->CopyBuffer(pCurrentStagingBuffer, 0, m_pLightsBuffer, 0, lightBufferSize);
			m_LightsDirty = false;
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

		if (m_RenderGraphSBTRecordsDirty)
		{
			if (!m_SBTRecords.IsEmpty())
			{
				m_pRenderGraph->UpdateGlobalSBT(m_SBTRecords);
			}

			m_RenderGraphSBTRecordsDirty = false;
		}

		if (m_PerFrameResourceDirty)
		{
			ResourceUpdateDesc resourceUpdateDesc				= {};
			resourceUpdateDesc.ResourceName						= PER_FRAME_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &m_pPerFrameBuffer;

			m_pRenderGraph->UpdateResource(&resourceUpdateDesc);

			m_PerFrameResourceDirty = false;
		}

		if (m_LightsResourceDirty)
		{
			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName						= SCENE_LIGHTS_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &m_pLightsBuffer;
			m_pRenderGraph->UpdateResource(&resourceUpdateDesc);

			m_LightsResourceDirty = false;
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

		if (m_RayTracingEnabled)
		{
			if (m_TLASResourceDirty)
			{
				//Create Resource Update for RenderGraph
				ResourceUpdateDesc resourceUpdateDesc					= {};
				resourceUpdateDesc.ResourceName							= SCENE_TLAS;
				resourceUpdateDesc.ExternalAccelerationStructure.pTLAS	= m_pTLAS;

				m_pRenderGraph->UpdateResource(&resourceUpdateDesc);

				m_TLASResourceDirty = false;
			}
		}
	}
}
