#include "Game/Scene.h"

#include "Rendering/Core/API/GraphicsDevice.h"
#include "Audio/API/IAudioDevice.h"
#include "ECS/ECSCore.h"
#include "Game/ECS/Rendering/ComponentGroups.h"
#include "Resources/ResourceManager.h"

#include "Resources/Mesh.h"
#include "Resources/Material.h"
#include "Rendering/Core/API/Buffer.h"
#include "Rendering/Core/API/Texture.h"
#include "Rendering/Core/API/CommandQueue.h"
#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/AccelerationStructure.h"
#include "Rendering/Core/API/Fence.h"
#include "Rendering/RenderSystem.h"

#include "Rendering/Core/Vulkan/Vulkan.h"

#include "Log/Log.h"

#include "Time/API/Clock.h"
#include "Math/Random.h"

namespace LambdaEngine
{
	Scene::Scene(const GraphicsDevice* pGraphicsDevice, const IAudioDevice* pAudioDevice) :
		m_pGraphicsDevice(pGraphicsDevice),
		m_pAudioDevice(pAudioDevice)
	{
		CameraComponents cameraComponents;
		cameraComponents.Position.Permissions				= RW;
		cameraComponents.Rotation.Permissions				= RW;
		cameraComponents.ViewProjectionMatrices.Permissions	= RW;
		cameraComponents.CameraProperties.Permissions		= RW;

		EntitySubscriberRegistration subscriberReg =
		{
			.EntitySubscriptionRegistrations =
			{
				{{&cameraComponents}, &m_Cameras},
			}
		};
		SubscribeToEntities(subscriberReg, nullptr);
	}

	Scene::~Scene()
	{
		SAFERELEASE(m_pCopyCommandAllocator);
		SAFERELEASE(m_pCopyCommandList);

		SAFERELEASE(m_pBLASBuildCommandAllocator);
		SAFERELEASE(m_pTLASBuildCommandAllocator);
		SAFERELEASE(m_pBLASBuildCommandList);
		SAFERELEASE(m_pTLASBuildCommandList);
		SAFERELEASE(m_pASFence);

		SAFERELEASE(m_pSceneMaterialPropertiesCopyBuffer);
		SAFERELEASE(m_pSceneVertexCopyBuffer);
		SAFERELEASE(m_pSceneIndexCopyBuffer);
		SAFERELEASE(m_pScenePrimaryInstanceCopyBuffer);
		SAFERELEASE(m_pSceneSecondaryInstanceCopyBuffer);
		SAFERELEASE(m_pSceneIndirectArgsCopyBuffer);
		SAFERELEASE(m_pLightsBuffer);
		SAFERELEASE(m_pLightsCopyBuffer);
		SAFERELEASE(m_pPerFrameBuffer);
		SAFERELEASE(m_pPerFrameCopyBuffer);
		SAFERELEASE(m_pSceneMaterialProperties);
		SAFERELEASE(m_pSceneVertexBuffer);
		SAFERELEASE(m_pSceneIndexBuffer);
		SAFERELEASE(m_pScenePrimaryInstanceBuffer);
		SAFERELEASE(m_pSceneSecondaryInstanceBuffer);
		SAFERELEASE(m_pSceneIndirectArgsBuffer);

		SAFERELEASE(m_pTLAS);

		for (AccelerationStructure* pBLAS : m_BLASs)
		{
			SAFERELEASE(pBLAS);
		}
		m_BLASs.Clear();

		SAFERELEASE(m_pDeviceAllocator);
	}

	bool Scene::Init(const SceneDesc& desc)
	{
		m_Name = desc.Name;

		for (uint32 i = 0; i < NUM_RANDOM_SEEDS; i++)
		{
			m_RandomSeeds[i] = Random::Int32(INT32_MIN, INT32_MAX);
		}

		m_LightsLightSetup.AreaLightCount = 0;

		for (uint32 l = 0; l < MAX_NUM_AREA_LIGHTS; l++)
		{
			m_AreaLightIndexToInstanceIndex[l] = -1;

			AreaLight* pAreaLight = &m_LightsLightSetup.AreaLights[l];
			pAreaLight->InstanceIndex	= UINT32_MAX;
			pAreaLight->Type			= EAreaLightType::NONE;
		}

		//Device Allocator
		{
			DeviceAllocatorDesc allocatorDesc = {};
			allocatorDesc.DebugName = "Scene Allocator";
			allocatorDesc.PageSizeInBytes	= MEGA_BYTE(64);
			m_pDeviceAllocator = RenderSystem::GetDevice()->CreateDeviceAllocator(&allocatorDesc);
		}

		m_pCopyCommandAllocator		= m_pGraphicsDevice->CreateCommandAllocator("Scene Copy Command Allocator", ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);

		CommandListDesc copyCommandListDesc = {};
		copyCommandListDesc.DebugName			= "Scene Copy Command List";
		copyCommandListDesc.Flags				= FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;
		copyCommandListDesc.CommandListType		= ECommandListType::COMMAND_LIST_TYPE_PRIMARY;

		m_pCopyCommandList = m_pGraphicsDevice->CreateCommandList(m_pCopyCommandAllocator, &copyCommandListDesc);

		m_RayTracingEnabled = desc.RayTracingEnabled;

		if (m_RayTracingEnabled)
		{
			m_pBLASBuildCommandAllocator = m_pGraphicsDevice->CreateCommandAllocator("Scene BLAS Build Command Allocator", ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE);

			CommandListDesc blasBuildCommandListDesc = {};
			blasBuildCommandListDesc.DebugName			= "Scene BLAS Build Command List";
			blasBuildCommandListDesc.Flags				= FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;
			blasBuildCommandListDesc.CommandListType	= ECommandListType::COMMAND_LIST_TYPE_PRIMARY;

			m_pBLASBuildCommandList = m_pGraphicsDevice->CreateCommandList(m_pBLASBuildCommandAllocator, &blasBuildCommandListDesc);


			m_pTLASBuildCommandAllocator = m_pGraphicsDevice->CreateCommandAllocator("Scene TLAS Build Command Allocator", ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE);

			CommandListDesc tlasBuildCommandListDesc = {};
			tlasBuildCommandListDesc.DebugName			= "Scene TLAS Build Command List";
			tlasBuildCommandListDesc.Flags				= FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;
			tlasBuildCommandListDesc.CommandListType	= ECommandListType::COMMAND_LIST_TYPE_PRIMARY;

			m_pTLASBuildCommandList = m_pGraphicsDevice->CreateCommandList(m_pTLASBuildCommandAllocator, &tlasBuildCommandListDesc);


			FenceDesc asFenceDesc = {};
			asFenceDesc.DebugName		= "Scene AS Fence";
			asFenceDesc.InitalValue		= 0;

			m_pASFence = m_pGraphicsDevice->CreateFence(&asFenceDesc);
		}

		// Lights Buffer
		{
			BufferDesc lightsCopyBufferDesc = {};
			lightsCopyBufferDesc.DebugName				= "Scene Lights Copy Buffer";
			lightsCopyBufferDesc.MemoryType				= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
			lightsCopyBufferDesc.Flags					= FBufferFlags::BUFFER_FLAG_COPY_SRC;
			lightsCopyBufferDesc.SizeInBytes			= sizeof(LightSetup);

			m_pLightsCopyBuffer = m_pGraphicsDevice->CreateBuffer(&lightsCopyBufferDesc, m_pDeviceAllocator);

			BufferDesc lightsBufferDesc = {};
			lightsBufferDesc.DebugName				= "Scene Lights Buffer";
			lightsBufferDesc.MemoryType				= EMemoryType::MEMORY_TYPE_GPU;
			lightsBufferDesc.Flags					= FBufferFlags::BUFFER_FLAG_CONSTANT_BUFFER | FBufferFlags::BUFFER_FLAG_COPY_DST;
			lightsBufferDesc.SizeInBytes			= sizeof(LightSetup);

			m_pLightsBuffer = m_pGraphicsDevice->CreateBuffer(&lightsBufferDesc, m_pDeviceAllocator);
		}

		// Per Frame Buffer
		{
			BufferDesc perFrameCopyBufferDesc = {};
			perFrameCopyBufferDesc.DebugName				= "Scene Per Frame Copy Buffer";
			perFrameCopyBufferDesc.MemoryType				= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
			perFrameCopyBufferDesc.Flags					= FBufferFlags::BUFFER_FLAG_COPY_SRC;
			perFrameCopyBufferDesc.SizeInBytes				= sizeof(PerFrameBuffer);

			m_pPerFrameCopyBuffer = m_pGraphicsDevice->CreateBuffer(&perFrameCopyBufferDesc, m_pDeviceAllocator);

			BufferDesc perFrameBufferDesc = {};
			perFrameBufferDesc.DebugName			= "Scene Per Frame Buffer";
			perFrameBufferDesc.MemoryType			= EMemoryType::MEMORY_TYPE_GPU;
			perFrameBufferDesc.Flags				= FBufferFlags::BUFFER_FLAG_CONSTANT_BUFFER | FBufferFlags::BUFFER_FLAG_COPY_DST;;
			perFrameBufferDesc.SizeInBytes			= sizeof(PerFrameBuffer);

			m_pPerFrameBuffer = m_pGraphicsDevice->CreateBuffer(&perFrameBufferDesc, m_pDeviceAllocator);
		}

		return true;
	}

	bool Scene::InitEntitySubscriber()
	{
		EntityPublisher* pEntityPublisher = ECSCore::GetInstance()->GetEntityPublisher();
		m_pCameraHandler = static_cast<CameraHandler*>(pEntityPublisher->GetComponentHandler(TID(CameraHandler)));
		m_pTransformHandler = static_cast<TransformHandler*>(pEntityPublisher->GetComponentHandler(TID(TransformHandler)));

		return m_pCameraHandler && m_pTransformHandler;
	}

	bool Scene::Finalize()
	{
		LambdaEngine::Clock clock;

		clock.Reset();
		clock.Tick();

		/*------------Ray Tracing Section Begin-------------*/
		TArray<BuildBottomLevelAccelerationStructureDesc> blasBuildDescriptions;
		blasBuildDescriptions.Reserve(m_Meshes.GetSize());

		if (m_RayTracingEnabled)
		{
			AccelerationStructureDesc tlasDesc = {};
			tlasDesc.DebugName		= "TLAS";
			tlasDesc.Type			= EAccelerationStructureType::ACCELERATION_STRUCTURE_TYPE_TOP;
			tlasDesc.Flags			= FAccelerationStructureFlags::ACCELERATION_STRUCTURE_FLAG_ALLOW_UPDATE;
			tlasDesc.InstanceCount	= m_PrimaryInstances.GetSize();

			m_pTLAS = m_pGraphicsDevice->CreateAccelerationStructure(&tlasDesc, m_pDeviceAllocator);

			m_BLASs.Reserve(m_Meshes.GetSize());
		}
		/*-------------Ray Tracing Section End--------------*/

		uint32 currentNumSceneVertices = 0;
		uint32 currentNumSceneIndices = 0;

		std::multimap<uint32, std::pair<MappedMaterial, IndexedIndirectMeshArgument>> materialIndexToMeshIndex;
		uint32 indirectArgCount = 0;

		for (uint32 meshIndex = 0; meshIndex < m_Meshes.GetSize(); meshIndex++)
		{
			MappedMesh& mappedMesh = m_MappedMeshes[meshIndex];
			const Mesh* pMesh = m_Meshes[meshIndex];

			uint32 newNumSceneVertices	= currentNumSceneVertices	+ pMesh->VertexCount;
			uint32 newNumSceneIndices	= currentNumSceneIndices	+ pMesh->IndexCount;

			/*------------Ray Tracing Section Begin-------------*/
			uint64 accelerationStructureDeviceAddress = 0;

			if (m_RayTracingEnabled)
			{
				AccelerationStructureDesc blasDesc = {};
				blasDesc.DebugName			= "BLAS";
				blasDesc.Type				= EAccelerationStructureType::ACCELERATION_STRUCTURE_TYPE_BOTTOM;
				blasDesc.Flags				= FAccelerationStructureFlags::ACCELERATION_STRUCTURE_FLAG_NONE;
				blasDesc.MaxTriangleCount	= pMesh->IndexCount / 3;
				blasDesc.MaxVertexCount		= pMesh->VertexCount;
				blasDesc.AllowsTransform	= false;

				AccelerationStructure* pBLAS = m_pGraphicsDevice->CreateAccelerationStructure(&blasDesc, m_pDeviceAllocator);
				accelerationStructureDeviceAddress = pBLAS->GetDeviceAdress();
				m_BLASs.PushBack(pBLAS);

				BuildBottomLevelAccelerationStructureDesc blasBuildDesc = {};
				blasBuildDesc.pAccelerationStructure		= pBLAS;
				blasBuildDesc.Flags							= FAccelerationStructureFlags::ACCELERATION_STRUCTURE_FLAG_NONE;
				blasBuildDesc.FirstVertexIndex				= currentNumSceneVertices;
				blasBuildDesc.VertexStride					= sizeof(Vertex);
				blasBuildDesc.IndexBufferByteOffset			= currentNumSceneIndices * sizeof(uint32);
				blasBuildDesc.TriangleCount					= pMesh->IndexCount / 3;
				blasBuildDesc.Update						= false;

				blasBuildDescriptions.PushBack(blasBuildDesc);
			}
			/*-------------Ray Tracing Section End--------------*/

			for (uint32 materialIndex = 0; materialIndex < mappedMesh.MappedMaterials.GetSize(); materialIndex++)
			{
				MappedMaterial& mappedMaterial = mappedMesh.MappedMaterials[materialIndex];

				IndexedIndirectMeshArgument indirectMeshArgument = {};
				indirectMeshArgument.IndexCount			= pMesh->IndexCount;
				indirectMeshArgument.FirstIndex			= currentNumSceneIndices;
				indirectMeshArgument.VertexOffset		= currentNumSceneVertices;

				/*------------Ray Tracing Section Begin-------------*/
				if (m_RayTracingEnabled)
				{
					indirectMeshArgument.InstanceCount		= (uint32)((accelerationStructureDeviceAddress >> 32)	& 0x00000000FFFFFFFF); // Temporarily store BLAS Device Address in Instance Count and First Instance
					indirectMeshArgument.FirstInstance		= (uint32)((accelerationStructureDeviceAddress)			& 0x00000000FFFFFFFF); // these are used in the next stage
				}
				/*-------------Ray Tracing Section End--------------*/

				indirectArgCount++;
				materialIndexToMeshIndex.insert(std::make_pair(mappedMaterial.MaterialIndex, std::make_pair(mappedMaterial, indirectMeshArgument)));
			}

			currentNumSceneVertices = newNumSceneVertices;
			currentNumSceneIndices = newNumSceneIndices;
		}

		m_IndirectArgs.Clear();
		m_IndirectArgs.Reserve(indirectArgCount);

		m_SortedPrimaryInstances.Clear();
		m_SortedPrimaryInstances.Reserve(m_PrimaryInstances.GetSize());

		m_SortedSecondaryInstances.Clear();
		m_SortedSecondaryInstances.Reserve(m_SecondaryInstances.GetSize());

		m_InstanceIndexToSortedInstanceIndex.Resize(m_PrimaryInstances.GetSize());

		// Extra Loop to sort Indirect Args by Material
		uint32 prevMaterialIndex = UINT32_MAX;
		for (auto it = materialIndexToMeshIndex.begin(); it != materialIndexToMeshIndex.end(); it++)
		{
			uint32 currentMaterialIndex = it->first;
			MappedMaterial& mappedMaterial = it->second.first;
			IndexedIndirectMeshArgument& indirectArg = it->second.second;

			uint32 instanceCount = (uint32)mappedMaterial.InstanceIndices.GetSize();
			uint32 baseInstanceIndex = (uint32)m_SortedPrimaryInstances.GetSize();

			/*------------Ray Tracing Section Begin-------------*/
			uint64 accelerationStructureDeviceAddress = ((((uint64)indirectArg.InstanceCount) << 32) & 0xFFFFFFFF00000000) | (((uint64)indirectArg.FirstInstance) & 0x00000000FFFFFFFF);
			/*-------------Ray Tracing Section End--------------*/

			for (uint32 instanceIndex = 0; instanceIndex < instanceCount; instanceIndex++)
			{
				uint32 mappedInstanceIndex = mappedMaterial.InstanceIndices[instanceIndex];

				//Check if this instance belongs to an area light
				for (uint32 l = 0; l < MAX_NUM_AREA_LIGHTS; l++)
				{
					if (m_AreaLightIndexToInstanceIndex[l] == int32(mappedInstanceIndex))
					{
						m_LightsLightSetup.AreaLights[l].InstanceIndex = m_SortedPrimaryInstances.GetSize();
					}
				}

				m_InstanceIndexToSortedInstanceIndex[mappedInstanceIndex] = m_SortedPrimaryInstances.GetSize();

				InstancePrimary primaryInstance					= m_PrimaryInstances[mappedInstanceIndex];
				InstanceSecondary secondaryInstance				= m_SecondaryInstances[mappedInstanceIndex];
				primaryInstance.IndirectArgsIndex				= (uint32)m_IndirectArgs.GetSize();

				/*------------Ray Tracing Section Begin-------------*/
				primaryInstance.AccelerationStructureAddress	= accelerationStructureDeviceAddress;
				primaryInstance.SBTRecordOffset					= 0;
				primaryInstance.Flags							= 0x00000001; //Culling Disabled
				/*-------------Ray Tracing Section End--------------*/

				m_SortedPrimaryInstances.PushBack(primaryInstance);
				m_SortedSecondaryInstances.PushBack(secondaryInstance);
			}

			if (prevMaterialIndex != currentMaterialIndex)
			{
				prevMaterialIndex = currentMaterialIndex;
				m_MaterialIndexToIndirectArgOffsetMap[currentMaterialIndex] = m_IndirectArgs.GetSize();
			}

			indirectArg.InstanceCount = instanceCount;
			indirectArg.FirstInstance = baseInstanceIndex;
			indirectArg.MaterialIndex = mappedMaterial.MaterialIndex;

			m_IndirectArgs.PushBack(indirectArg);
		}

		//// Create InstanceBuffers
		//{
		//	uint32 scenePrimaryInstanceBufferSize = uint32(m_SortedInstances.GetSize() * sizeof(InstancePrimary));
		//	uint32 sceneSecondaryInstanceBufferSize = uint32(m_SortedInstances.GetSize() * sizeof(InstanceSecondary));

		//	if (m_pScenePrimaryInstanceBuffer == nullptr || scenePrimaryInstanceBufferSize > m_pScenePrimaryInstanceBuffer->GetDesc().SizeInBytes)
		//	{
		//		SAFERELEASE(m_pScenePrimaryInstanceBuffer);

		//		BufferDesc bufferDesc = {};
		//		bufferDesc.pName		= "Scene Primary Instance Buffer";
		//		bufferDesc.MemoryType	= EMemoryType::MEMORY_GPU;
		//		bufferDesc.Flags		= FBufferFlags::BUFFER_FLAG_COPY_DST | FBufferFlags::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER;
		//		bufferDesc.SizeInBytes	= scenePrimaryInstanceBufferSize;

		//		m_pScenePrimaryInstanceBuffer = m_pGraphicsDevice->CreateBuffer(&bufferDesc, m_pDeviceAllocator);
		//	}

		//	if (m_pSceneSecondaryInstanceBuffer == nullptr || sceneSecondaryInstanceBufferSize > m_pSceneSecondaryInstanceBuffer->GetDesc().SizeInBytes)
		//	{
		//		SAFERELEASE(m_pSceneSecondaryInstanceBuffer);

		//		BufferDesc bufferDesc = {};
		//		bufferDesc.pName		= "Scene Secondary Instance Buffer";
		//		bufferDesc.MemoryType	= EMemoryType::MEMORY_GPU;
		//		bufferDesc.Flags		= FBufferFlags::BUFFER_FLAG_COPY_DST | FBufferFlags::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER;
		//		bufferDesc.SizeInBytes	= sceneSecondaryInstanceBufferSize;

		//		m_pSceneSecondaryInstanceBuffer = m_pGraphicsDevice->CreateBuffer(&bufferDesc, m_pDeviceAllocator);
		//	}
		//}

		m_SceneAlbedoMaps.Resize(MAX_UNIQUE_MATERIALS);
		m_SceneNormalMaps.Resize(MAX_UNIQUE_MATERIALS);
		m_SceneAmbientOcclusionMaps.Resize(MAX_UNIQUE_MATERIALS);
		m_SceneMetallicMaps.Resize(MAX_UNIQUE_MATERIALS);
		m_SceneRoughnessMaps.Resize(MAX_UNIQUE_MATERIALS);

		m_SceneAlbedoMapViews.Resize(MAX_UNIQUE_MATERIALS);
		m_SceneNormalMapViews.Resize(MAX_UNIQUE_MATERIALS);
		m_SceneAmbientOcclusionMapViews.Resize(MAX_UNIQUE_MATERIALS);
		m_SceneMetallicMapViews.Resize(MAX_UNIQUE_MATERIALS);
		m_SceneRoughnessMapViews.Resize(MAX_UNIQUE_MATERIALS);

		m_SceneMaterialProperties.Resize(MAX_UNIQUE_MATERIALS);

		for (uint32 i = 0; i < MAX_UNIQUE_MATERIALS; i++)
		{
			if (i < m_Materials.GetSize())
			{
				const Material* pMaterial = m_Materials[i];

				m_SceneAlbedoMaps[i]				= pMaterial->pAlbedoMap;
				m_SceneNormalMaps[i]				= pMaterial->pNormalMap;
				m_SceneAmbientOcclusionMaps[i]		= pMaterial->pAmbientOcclusionMap;
				m_SceneMetallicMaps[i]				= pMaterial->pMetallicMap;
				m_SceneRoughnessMaps[i]				= pMaterial->pRoughnessMap;

				m_SceneAlbedoMapViews[i]			= pMaterial->pAlbedoMapView;
				m_SceneNormalMapViews[i]			= pMaterial->pNormalMapView;
				m_SceneAmbientOcclusionMapViews[i]	= pMaterial->pAmbientOcclusionMapView;
				m_SceneMetallicMapViews[i]			= pMaterial->pMetallicMapView;
				m_SceneRoughnessMapViews[i]			= pMaterial->pRoughnessMapView;

				m_SceneMaterialProperties[i]		= pMaterial->Properties;
			}
			else
			{
				const Material* pMaterial = ResourceManager::GetMaterial(GUID_MATERIAL_DEFAULT);

				m_SceneAlbedoMaps[i]				= pMaterial->pAlbedoMap;
				m_SceneNormalMaps[i]				= pMaterial->pNormalMap;
				m_SceneAmbientOcclusionMaps[i]		= pMaterial->pAmbientOcclusionMap;
				m_SceneMetallicMaps[i]				= pMaterial->pMetallicMap;
				m_SceneRoughnessMaps[i]				= pMaterial->pRoughnessMap;

				m_SceneAlbedoMapViews[i]			= pMaterial->pAlbedoMapView;
				m_SceneNormalMapViews[i]			= pMaterial->pNormalMapView;
				m_SceneAmbientOcclusionMapViews[i]	= pMaterial->pAmbientOcclusionMapView;
				m_SceneMetallicMapViews[i]			= pMaterial->pMetallicMapView;
				m_SceneRoughnessMapViews[i]			= pMaterial->pRoughnessMapView;

				m_SceneMaterialProperties[i]		= pMaterial->Properties;
			}
		}

		m_pCopyCommandAllocator->Reset();
		m_pCopyCommandList->Begin(nullptr);

		UpdateMaterialPropertiesBuffers(m_pCopyCommandList);
		UpdateVertexBuffers(m_pCopyCommandList);
		UpdateIndexBuffers(m_pCopyCommandList);
		UpdateInstanceBuffers(m_pCopyCommandList);
		UpdateIndirectArgsBuffers(m_pCopyCommandList);

		m_pCopyCommandList->End();

		RenderSystem::GetGraphicsQueue()->ExecuteCommandLists(&m_pCopyCommandList, 1,		FPipelineStageFlags::PIPELINE_STAGE_FLAG_UNKNOWN, nullptr, 0, nullptr, 0);
		RenderSystem::GetGraphicsQueue()->Flush();

		/*------------Ray Tracing Section Begin-------------*/
		if (m_RayTracingEnabled)
		{
			//Build BLASs
			{
				m_pBLASBuildCommandAllocator->Reset();
				m_pBLASBuildCommandList->Begin(nullptr);

				for (uint32 i = 0; i < blasBuildDescriptions.GetSize(); i++)
				{
					BuildBottomLevelAccelerationStructureDesc* pBlasBuildDesc = &blasBuildDescriptions[i];
					pBlasBuildDesc->pVertexBuffer			= m_pSceneVertexBuffer;
					pBlasBuildDesc->pIndexBuffer			= m_pSceneIndexBuffer;
					pBlasBuildDesc->pTransformBuffer		= nullptr;
					pBlasBuildDesc->TransformByteOffset		= 0;

					m_pBLASBuildCommandList->BuildBottomLevelAccelerationStructure(pBlasBuildDesc);
				}

				m_pBLASBuildCommandList->End();
			}

			//Build TLAS
			{
				m_pTLASBuildCommandAllocator->Reset();
				m_pTLASBuildCommandList->Begin(nullptr);
				BuildTLAS(m_pTLASBuildCommandList, false);
				m_pTLASBuildCommandList->End();
			}

			RenderSystem::GetComputeQueue()->ExecuteCommandLists(&m_pBLASBuildCommandList, 1, FPipelineStageFlags::PIPELINE_STAGE_FLAG_UNKNOWN, nullptr, 0, m_pASFence, 1);
			RenderSystem::GetComputeQueue()->ExecuteCommandLists(&m_pTLASBuildCommandList, 1, FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, m_pASFence, 1, nullptr, 0);
			RenderSystem::GetComputeQueue()->Flush();

			//RayTracingTestVK::Debug(m_pGraphicsDevice, blasBuildDescriptions[0].pAccelerationStructure, m_pTLAS);
		}
		/*-------------Ray Tracing Section End--------------*/

		clock.Tick();
		LOG_INFO("Scene Build took %f milliseconds", clock.GetDeltaTime().AsMilliSeconds());

		D_LOG_MESSAGE("[Scene]: Successfully finalized \"%s\"! ", m_Name.c_str());

		m_InstanceBuffersAreDirty = false;
		return true;
	}

	void Scene::PrepareRender(CommandList* pGraphicsCommandList, CommandList* pComputeCommandList, uint64 frameIndex)
	{
		for (uint32 instanceIndex : m_DirtySecondaryInstances)
		{
			uint32 sortedInstanceIndex = m_InstanceIndexToSortedInstanceIndex[instanceIndex];

			//Todo: Implement Array of Pointers for instances instead so that we don't have two copies of every instance
			//Update Unsorted Copies
			{
				InstancePrimary* pPrimaryInstance = &m_PrimaryInstances[instanceIndex];

				m_SecondaryInstances[instanceIndex].PrevTransform	= pPrimaryInstance->Transform;
			}

			//Update Sorted Copies
			{
				InstancePrimary* pPrimaryInstance = &m_SortedPrimaryInstances[sortedInstanceIndex];

				m_SortedSecondaryInstances[sortedInstanceIndex].PrevTransform	= pPrimaryInstance->Transform;
			}

			m_InstanceBuffersAreDirty = true;
		}
		m_DirtySecondaryInstances.clear();

		//Update Per Frame Data
		{
			m_PerFrameData.FrameIndex = uint32(frameIndex % UINT32_MAX);
			//perFrameBuffer.RandomSeed	= m_RandomSeeds[frameIndex % NUM_RANDOM_SEEDS];
			m_PerFrameData.RandomSeed = Random::Int32(INT32_MIN, INT32_MAX);

			UpdatePerFrameBuffer(pGraphicsCommandList);
		}

		if (m_LightSetupIsDirty)
		{
			UpdateLightsBuffer(pGraphicsCommandList);

			m_LightSetupIsDirty = false;
		}

		if (m_InstanceBuffersAreDirty)
		{
			UpdateInstanceBuffers(pGraphicsCommandList);

			if (m_RayTracingEnabled)
			{
				BuildTLAS(pComputeCommandList, true);
			}

			m_InstanceBuffersAreDirty = false;
		}

		if (m_MaterialPropertiesBuffersAreDirty)
		{
			UpdateMaterialPropertiesBuffers(pGraphicsCommandList);

			m_MaterialPropertiesBuffersAreDirty = false;
		}
	}

	uint32 Scene::AddStaticGameObject(const GameObject& gameObject, const glm::mat4& transform)
	{
		UNREFERENCED_VARIABLE(gameObject);
		UNREFERENCED_VARIABLE(transform);

		LOG_WARNING("[Scene]: Call to unimplemented function AddStaticGameObject!");
		return 0;
	}

	uint32 Scene::AddDynamicGameObject(const GameObject& gameObject, const glm::mat4& transform)
	{
		return InternalAddDynamicObject(gameObject.Mesh, gameObject.Material, transform, HIT_MASK_GAME_OBJECT);
	}

	void Scene::UpdateTransform(uint32 instanceIndex, const glm::mat4& transform)
	{
		uint32 sortedInstanceIndex = m_InstanceIndexToSortedInstanceIndex[instanceIndex];

		glm::mat4 transposedTransform = glm::transpose(transform);

		//Todo: Implement Array of Pointers for instances instead so that we don't have two copies of every instance
		//Update Unsorted Copies
		{
			InstancePrimary* pPrimaryInstance = &m_PrimaryInstances[instanceIndex];

			m_SecondaryInstances[instanceIndex].PrevTransform	= pPrimaryInstance->Transform;
			pPrimaryInstance->Transform							= transposedTransform;
		}

		//Update Sorted Copies
		{
			InstancePrimary* pPrimaryInstance = &m_SortedPrimaryInstances[sortedInstanceIndex];

			m_SortedSecondaryInstances[sortedInstanceIndex].PrevTransform	= pPrimaryInstance->Transform;
			pPrimaryInstance->Transform										= transposedTransform;
		}

		m_DirtySecondaryInstances.insert(instanceIndex);
		m_InstanceBuffersAreDirty = true;
	}

	void Scene::SetDirectionalLight(const DirectionalLight& directionalLight)
	{
		m_LightsLightSetup.DirectionalLight = directionalLight;
		m_LightSetupIsDirty = true;
	}

	uint32 Scene::AddAreaLight(const AreaLightObject& lightObject, const glm::mat4& transform)
	{
		if (m_LightsLightSetup.AreaLightCount < MAX_NUM_AREA_LIGHTS)
		{
			GUID_Lambda mesh = GUID_NONE;

			switch (lightObject.Type)
			{
			case EAreaLightType::QUAD:	mesh = GUID_MESH_QUAD;
			default:					mesh = GUID_MESH_QUAD;
			}

			uint32 instanceIndex = InternalAddDynamicObject(mesh, lightObject.Material, transform, HIT_MASK_LIGHT);
			m_AreaLightIndexToInstanceIndex[m_LightsLightSetup.AreaLightCount]		= instanceIndex;
			m_LightsLightSetup.AreaLights[m_LightsLightSetup.AreaLightCount].Type	= lightObject.Type;
			m_LightsLightSetup.AreaLightCount++;

			m_LightSetupIsDirty = true;

			return instanceIndex;
		}

		//Todo: Change return type or add failure defines
		return UINT32_MAX;
	}

	void Scene::UpdateCamera()
	{
		if (m_Cameras.Empty())
			return;

		Entity camera = m_Cameras[0];
		const glm::quat& rotation = m_pTransformHandler->GetRotation(camera);

		const ViewProjectionMatrices& viewProjectMatrices = m_pCameraHandler->GetViewProjectionMatrices(camera);

		m_PerFrameData.Camera =
		{
			.Projection = viewProjectMatrices.Projection,
			.View = viewProjectMatrices.View,
			.PrevProjection = viewProjectMatrices.PrevProjection,
			.PrevView = viewProjectMatrices.PrevView,
			.ViewInv = glm::inverse(viewProjectMatrices.View),
			.ProjectionInv = glm::inverse(viewProjectMatrices.Projection),
			.Position = glm::vec4(m_pTransformHandler->GetPosition(camera), 0.0f),
			.Right = glm::vec4(TransformHandler::GetRight(rotation), 0.0f),
			.Up = glm::vec4(TransformHandler::GetUp(rotation), 0.0f),
			.Jitter = m_pCameraHandler->GetCameraProperties(camera).Jitter
		};
	}

	void Scene::UpdateMaterialProperties(GUID_Lambda materialGUID)
	{
		m_SceneMaterialProperties[m_GUIDToMaterials[materialGUID]] = ResourceManager::GetMaterial(materialGUID)->Properties;
		m_MaterialPropertiesBuffersAreDirty = true;
	}

	uint32 Scene::GetIndirectArgumentOffset(uint32 materialIndex) const
	{
		auto it = m_MaterialIndexToIndirectArgOffsetMap.find(materialIndex);

		if (it != m_MaterialIndexToIndirectArgOffsetMap.end())
			return it->second;

		return uint32(m_pSceneIndirectArgsBuffer->GetDesc().SizeInBytes / sizeof(IndexedIndirectMeshArgument));
	}

	uint32 Scene::InternalAddDynamicObject(GUID_Lambda meshGUID, GUID_Lambda materialGUID, const glm::mat4& transform, HitMask hitMask)
	{
		//Todo: Am I retarded, what is the reason we have to do this?
		glm::mat4 tranposedTransform = glm::transpose(transform);

		InstancePrimary primaryInstance = {};
		primaryInstance.Transform						= tranposedTransform;
		primaryInstance.IndirectArgsIndex				= 0;
		primaryInstance.Mask							= hitMask;
		primaryInstance.SBTRecordOffset					= 0;
		primaryInstance.Flags							= 0;
		primaryInstance.AccelerationStructureAddress	= 0;

		InstanceSecondary secondaryInstance = {};
		secondaryInstance.PrevTransform					= tranposedTransform;

		m_PrimaryInstances.PushBack(primaryInstance);
		m_SecondaryInstances.PushBack(secondaryInstance);

		uint32 instanceIndex = uint32(m_PrimaryInstances.GetSize() - 1);
		uint32 meshIndex = 0;

		if (m_GUIDToMappedMeshes.count(meshGUID) == 0)
		{
			const Mesh* pMesh = ResourceManager::GetMesh(meshGUID);

			uint32 currentNumSceneVertices = (uint32)m_SceneVertexArray.GetSize();
			m_SceneVertexArray.Resize(uint64(currentNumSceneVertices + pMesh->VertexCount));
			memcpy(&m_SceneVertexArray[currentNumSceneVertices], pMesh->pVertexArray, pMesh->VertexCount * sizeof(Vertex));

			uint32 currentNumSceneIndices = (uint32)m_SceneIndexArray.GetSize();
			m_SceneIndexArray.Resize(uint64(currentNumSceneIndices + pMesh->IndexCount));
			memcpy(&m_SceneIndexArray[currentNumSceneIndices], pMesh->pIndexArray, pMesh->IndexCount * sizeof(uint32));

			m_Meshes.PushBack(pMesh);
			meshIndex = uint32(m_Meshes.GetSize() - 1);

			MappedMesh newMappedMesh = {};
			m_MappedMeshes.PushBack(newMappedMesh);

			m_GUIDToMappedMeshes[meshGUID] = meshIndex;
		}
		else
		{
			meshIndex = m_GUIDToMappedMeshes[meshGUID];
		}

		MappedMesh& mappedMesh = m_MappedMeshes[meshIndex];
		uint32 globalMaterialIndex = 0;

		if (m_GUIDToMaterials.count(materialGUID) == 0)
		{
			m_Materials.PushBack(ResourceManager::GetMaterial(materialGUID));
			globalMaterialIndex = uint32(m_Materials.GetSize() - 1);

			m_GUIDToMaterials[materialGUID] = globalMaterialIndex;
		}
		else
		{
			globalMaterialIndex = m_GUIDToMaterials[materialGUID];
		}

		if (mappedMesh.GUIDToMappedMaterials.count(materialGUID) == 0)
		{
			MappedMaterial newMappedMaterial = {};
			newMappedMaterial.MaterialIndex = globalMaterialIndex;

			newMappedMaterial.InstanceIndices.PushBack(instanceIndex);

			mappedMesh.MappedMaterials.PushBack(newMappedMaterial);
			mappedMesh.GUIDToMappedMaterials[materialGUID] = GUID_Lambda(mappedMesh.MappedMaterials.GetSize() - 1);
		}
		else
		{
			mappedMesh.MappedMaterials[mappedMesh.GUIDToMappedMaterials[materialGUID]].InstanceIndices.PushBack(instanceIndex);
		}

		return instanceIndex;
	}

	void Scene::UpdateLightsBuffer(CommandList* pCommandList)
	{
		void* pMapped = m_pLightsCopyBuffer->Map();
		memcpy(pMapped, &m_LightsLightSetup, sizeof(LightSetup));
		m_pLightsCopyBuffer->Unmap();

		pCommandList->CopyBuffer(m_pLightsCopyBuffer, 0, m_pLightsBuffer, 0, sizeof(LightSetup));
	}

	void Scene::UpdatePerFrameBuffer(CommandList* pCommandList)
	{
		void* pMapped = m_pPerFrameCopyBuffer->Map();
		memcpy(pMapped, &m_PerFrameData, sizeof(PerFrameBuffer));
		m_pPerFrameCopyBuffer->Unmap();

		pCommandList->CopyBuffer(m_pPerFrameCopyBuffer, 0, m_pPerFrameBuffer, 0, sizeof(PerFrameBuffer));
	}

	void Scene::UpdateMaterialPropertiesBuffers(CommandList* pCopyCommandList)
	{
		uint32 sceneMaterialPropertiesSize = uint32(m_SceneMaterialProperties.GetSize() * sizeof(MaterialProperties));

		if (m_pSceneMaterialPropertiesCopyBuffer == nullptr || sceneMaterialPropertiesSize > m_pSceneMaterialPropertiesCopyBuffer->GetDesc().SizeInBytes)
		{
			SAFERELEASE(m_pSceneMaterialPropertiesCopyBuffer);

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName	= "Scene Material Properties Copy Buffer";
			bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
			bufferDesc.Flags		= FBufferFlags::BUFFER_FLAG_COPY_SRC;
			bufferDesc.SizeInBytes	= sceneMaterialPropertiesSize;

			m_pSceneMaterialPropertiesCopyBuffer = m_pGraphicsDevice->CreateBuffer(&bufferDesc, m_pDeviceAllocator);
		}

		void* pMapped = m_pSceneMaterialPropertiesCopyBuffer->Map();
		memcpy(pMapped, m_SceneMaterialProperties.GetData(), sceneMaterialPropertiesSize);
		m_pSceneMaterialPropertiesCopyBuffer->Unmap();

		if (m_pSceneMaterialProperties == nullptr || sceneMaterialPropertiesSize > m_pSceneMaterialProperties->GetDesc().SizeInBytes)
		{
			SAFERELEASE(m_pSceneMaterialProperties);

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName	= "Scene Material Properties";
			bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
			bufferDesc.Flags		= FBufferFlags::BUFFER_FLAG_COPY_DST | FBufferFlags::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER;
			bufferDesc.SizeInBytes	= sceneMaterialPropertiesSize;

			m_pSceneMaterialProperties = m_pGraphicsDevice->CreateBuffer(&bufferDesc, m_pDeviceAllocator);
		}

		pCopyCommandList->CopyBuffer(m_pSceneMaterialPropertiesCopyBuffer, 0, m_pSceneMaterialProperties, 0, sceneMaterialPropertiesSize);
	}

	void Scene::UpdateVertexBuffers(CommandList* pCopyCommandList)
	{
		uint32 sceneVertexBufferSize = uint32(m_SceneVertexArray.GetSize() * sizeof(Vertex));

		if (m_pSceneVertexCopyBuffer == nullptr || sceneVertexBufferSize > m_pSceneVertexCopyBuffer->GetDesc().SizeInBytes)
		{
			SAFERELEASE(m_pSceneVertexCopyBuffer);

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName	= "Scene Vertex Copy Buffer";
			bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
			bufferDesc.Flags		= FBufferFlags::BUFFER_FLAG_COPY_SRC;
			bufferDesc.SizeInBytes	= sceneVertexBufferSize;

			m_pSceneVertexCopyBuffer = m_pGraphicsDevice->CreateBuffer(&bufferDesc, m_pDeviceAllocator);
		}

		void* pMapped = m_pSceneVertexCopyBuffer->Map();
		memcpy(pMapped, m_SceneVertexArray.GetData(), sceneVertexBufferSize);
		m_pSceneVertexCopyBuffer->Unmap();

		if (m_pSceneVertexBuffer == nullptr || sceneVertexBufferSize > m_pSceneVertexBuffer->GetDesc().SizeInBytes)
		{
			SAFERELEASE(m_pSceneVertexBuffer);

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName	= "Scene Vertex Buffer";
			bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
			bufferDesc.Flags		= FBufferFlags::BUFFER_FLAG_COPY_DST | FBufferFlags::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | FBufferFlags::BUFFER_FLAG_VERTEX_BUFFER;
			bufferDesc.SizeInBytes	= sceneVertexBufferSize;

			m_pSceneVertexBuffer = m_pGraphicsDevice->CreateBuffer(&bufferDesc, m_pDeviceAllocator);
		}

		pCopyCommandList->CopyBuffer(m_pSceneVertexCopyBuffer, 0, m_pSceneVertexBuffer, 0, sceneVertexBufferSize);
	}

	void Scene::UpdateIndexBuffers(CommandList* pCopyCommandList)
	{
		uint32 sceneIndexBufferSize = uint32(m_SceneIndexArray.GetSize() * sizeof(uint32));

		if (m_pSceneIndexCopyBuffer == nullptr || sceneIndexBufferSize > m_pSceneIndexCopyBuffer->GetDesc().SizeInBytes)
		{
			SAFERELEASE(m_pSceneIndexCopyBuffer);

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName	= "Scene Index Copy Buffer";
			bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
			bufferDesc.Flags		= FBufferFlags::BUFFER_FLAG_COPY_SRC;
			bufferDesc.SizeInBytes	= sceneIndexBufferSize;

			m_pSceneIndexCopyBuffer = m_pGraphicsDevice->CreateBuffer(&bufferDesc, m_pDeviceAllocator);
		}

		void* pMapped = m_pSceneIndexCopyBuffer->Map();
		memcpy(pMapped, m_SceneIndexArray.GetData(), sceneIndexBufferSize);
		m_pSceneIndexCopyBuffer->Unmap();

		if (m_pSceneIndexBuffer == nullptr || sceneIndexBufferSize > m_pSceneIndexBuffer->GetDesc().SizeInBytes)
		{
			SAFERELEASE(m_pSceneIndexBuffer);

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName	= "Scene Index Buffer";
			bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
			bufferDesc.Flags		= FBufferFlags::BUFFER_FLAG_COPY_DST | FBufferFlags::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | FBufferFlags::BUFFER_FLAG_INDEX_BUFFER;
			bufferDesc.SizeInBytes	= sceneIndexBufferSize;

			m_pSceneIndexBuffer = m_pGraphicsDevice->CreateBuffer(&bufferDesc, m_pDeviceAllocator);
		}

		pCopyCommandList->CopyBuffer(m_pSceneIndexCopyBuffer, 0, m_pSceneIndexBuffer, 0, sceneIndexBufferSize);
	}

	void Scene::UpdateInstanceBuffers(CommandList* pCopyCommandList)
	{
		uint32 scenePrimaryInstanceBufferSize = uint32(m_SortedPrimaryInstances.GetSize() * sizeof(InstancePrimary));
		uint32 sceneSecondaryInstanceBufferSize = uint32(m_SortedSecondaryInstances.GetSize() * sizeof(InstanceSecondary));

		if (m_pScenePrimaryInstanceCopyBuffer == nullptr || scenePrimaryInstanceBufferSize > m_pScenePrimaryInstanceCopyBuffer->GetDesc().SizeInBytes)
		{
			SAFERELEASE(m_pScenePrimaryInstanceCopyBuffer);

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName	= "Scene Primary Instance Copy Buffer";
			bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
			bufferDesc.Flags		= FBufferFlags::BUFFER_FLAG_COPY_SRC;
			bufferDesc.SizeInBytes	= scenePrimaryInstanceBufferSize;

			m_pScenePrimaryInstanceCopyBuffer = m_pGraphicsDevice->CreateBuffer(&bufferDesc, m_pDeviceAllocator);
		}

		if (m_pSceneSecondaryInstanceCopyBuffer == nullptr || sceneSecondaryInstanceBufferSize > m_pSceneSecondaryInstanceCopyBuffer->GetDesc().SizeInBytes)
		{
			SAFERELEASE(m_pSceneSecondaryInstanceCopyBuffer);

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName	= "Scene Secondary Instance Copy Buffer";
			bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
			bufferDesc.Flags		= FBufferFlags::BUFFER_FLAG_COPY_SRC;
			bufferDesc.SizeInBytes	= sceneSecondaryInstanceBufferSize;

			m_pSceneSecondaryInstanceCopyBuffer = m_pGraphicsDevice->CreateBuffer(&bufferDesc, m_pDeviceAllocator);
		}

		void* pPrimaryMapped = m_pScenePrimaryInstanceCopyBuffer->Map();
		memcpy(pPrimaryMapped, m_SortedPrimaryInstances.GetData(), scenePrimaryInstanceBufferSize);
		m_pScenePrimaryInstanceCopyBuffer->Unmap();

		void* pSecondaryMapped = m_pSceneSecondaryInstanceCopyBuffer->Map();
		memcpy(pSecondaryMapped, m_SortedSecondaryInstances.GetData(), sceneSecondaryInstanceBufferSize);
		m_pSceneSecondaryInstanceCopyBuffer->Unmap();

		if (m_pScenePrimaryInstanceBuffer == nullptr || scenePrimaryInstanceBufferSize > m_pScenePrimaryInstanceBuffer->GetDesc().SizeInBytes)
		{
			SAFERELEASE(m_pScenePrimaryInstanceBuffer);

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName	= "Scene Primary Instance Buffer";
			bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
			bufferDesc.Flags		= FBufferFlags::BUFFER_FLAG_COPY_DST | FBufferFlags::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER;
			bufferDesc.SizeInBytes	= scenePrimaryInstanceBufferSize;

			m_pScenePrimaryInstanceBuffer = m_pGraphicsDevice->CreateBuffer(&bufferDesc, m_pDeviceAllocator);
		}

		if (m_pSceneSecondaryInstanceBuffer == nullptr || scenePrimaryInstanceBufferSize > m_pSceneSecondaryInstanceBuffer->GetDesc().SizeInBytes)
		{
			SAFERELEASE(m_pSceneSecondaryInstanceBuffer);

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName	= "Scene Secondary Instance Buffer";
			bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
			bufferDesc.Flags		= FBufferFlags::BUFFER_FLAG_COPY_DST | FBufferFlags::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER;
			bufferDesc.SizeInBytes	= sceneSecondaryInstanceBufferSize;

			m_pSceneSecondaryInstanceBuffer = m_pGraphicsDevice->CreateBuffer(&bufferDesc, m_pDeviceAllocator);
		}

		pCopyCommandList->CopyBuffer(m_pScenePrimaryInstanceCopyBuffer, 0, m_pScenePrimaryInstanceBuffer, 0, scenePrimaryInstanceBufferSize);
		pCopyCommandList->CopyBuffer(m_pSceneSecondaryInstanceCopyBuffer, 0, m_pSceneSecondaryInstanceBuffer, 0, scenePrimaryInstanceBufferSize);
	}

	void Scene::UpdateIndirectArgsBuffers(CommandList* pCopyCommandList)
	{
		uint32 sceneMeshIndexBufferSize = uint32(m_IndirectArgs.GetSize() * sizeof(IndexedIndirectMeshArgument));

		if (m_pSceneIndirectArgsCopyBuffer == nullptr || sceneMeshIndexBufferSize > m_pSceneIndirectArgsCopyBuffer->GetDesc().SizeInBytes)
		{
			SAFERELEASE(m_pSceneIndirectArgsCopyBuffer);

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName	= "Scene Mesh Index Copy Buffer";
			bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
			bufferDesc.Flags		= FBufferFlags::BUFFER_FLAG_COPY_SRC;
			bufferDesc.SizeInBytes	= sceneMeshIndexBufferSize;

			m_pSceneIndirectArgsCopyBuffer = m_pGraphicsDevice->CreateBuffer(&bufferDesc, m_pDeviceAllocator);
		}

		void* pMapped = m_pSceneIndirectArgsCopyBuffer->Map();
		memcpy(pMapped, m_IndirectArgs.GetData(), sceneMeshIndexBufferSize);
		m_pSceneIndirectArgsCopyBuffer->Unmap();

		if (m_pSceneIndirectArgsBuffer == nullptr || sceneMeshIndexBufferSize > m_pSceneIndirectArgsBuffer->GetDesc().SizeInBytes)
		{
			SAFERELEASE(m_pSceneIndirectArgsBuffer);

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName	= "Scene Mesh Index Buffer";
			bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
			bufferDesc.Flags		= FBufferFlags::BUFFER_FLAG_COPY_DST | FBufferFlags::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | FBufferFlags::BUFFER_FLAG_INDIRECT_BUFFER;
			bufferDesc.SizeInBytes	= sceneMeshIndexBufferSize;

			m_pSceneIndirectArgsBuffer = m_pGraphicsDevice->CreateBuffer(&bufferDesc, m_pDeviceAllocator);
		}

		pCopyCommandList->CopyBuffer(m_pSceneIndirectArgsCopyBuffer, 0, m_pSceneIndirectArgsBuffer, 0, sceneMeshIndexBufferSize);
	}

	void Scene::BuildTLAS(CommandList* pBuildCommandList, bool update)
	{
		BuildTopLevelAccelerationStructureDesc tlasBuildDesc = {};
		tlasBuildDesc.pAccelerationStructure	= m_pTLAS;
		tlasBuildDesc.Flags						= FAccelerationStructureFlags::ACCELERATION_STRUCTURE_FLAG_ALLOW_UPDATE;
		tlasBuildDesc.pInstanceBuffer			= m_pScenePrimaryInstanceBuffer;
		tlasBuildDesc.InstanceCount				= m_PrimaryInstances.GetSize();
		tlasBuildDesc.Update					= update;

		pBuildCommandList->BuildTopLevelAccelerationStructure(&tlasBuildDesc);
	}
}
