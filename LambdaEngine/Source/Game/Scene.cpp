#include "Game/Scene.h"

#include "Rendering/Core/API/IGraphicsDevice.h"
#include "Audio/API/IAudioDevice.h"
#include "Resources/ResourceManager.h"

#include "Resources/Mesh.h"
#include "Resources/Material.h"
#include "Rendering/Core/API/IBuffer.h"
#include "Rendering/Core/API/ITexture.h"
#include "Rendering/Core/API/ICommandQueue.h"
#include "Rendering/Core/API/ICommandAllocator.h"
#include "Rendering/Core/API/ICommandList.h"
#include "Rendering/Core/API/IAccelerationStructure.h"
#include "Rendering/Core/API/IFence.h"
#include "Rendering/RenderSystem.h"

#include "Rendering/Core/Vulkan/Vulkan.h"

#include "Log/Log.h"

#include "Time/API/Clock.h"

namespace LambdaEngine
{
	Scene::Scene(const IGraphicsDevice* pGraphicsDevice, const IAudioDevice* pAudioDevice) :
		m_pGraphicsDevice(pGraphicsDevice),
		m_pAudioDevice(pAudioDevice)
	{
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
		SAFERELEASE(m_pSceneInstanceCopyBuffer);
		SAFERELEASE(m_pSceneMeshIndexCopyBuffer);
		SAFERELEASE(m_pLightsBuffer);
		SAFERELEASE(m_pPerFrameBuffer);
		SAFERELEASE(m_pSceneMaterialProperties);
		SAFERELEASE(m_pSceneVertexBuffer);
		SAFERELEASE(m_pSceneIndexBuffer);
		SAFERELEASE(m_pSceneInstanceBuffer);
		SAFERELEASE(m_pSceneMeshIndexBuffer);

		SAFERELEASE(m_pDeviceAllocator);
	}

	void Scene::UpdateDirectionalLight(const glm::vec3& direction, const glm::vec3& spectralIntensity)
	{
		RenderSystem::GetGraphicsQueue()->Flush();

		m_pCopyCommandAllocator->Reset();
		m_pCopyCommandList->Begin(nullptr);

		// TODO: Remove this flush
		RenderSystem::GetGraphicsQueue()->Flush();
		RenderSystem::GetComputeQueue()->Flush();

		LightsBuffer lightsBuffer = {};
		lightsBuffer.Direction			= glm::vec4(direction, 1.0f);
		lightsBuffer.SpectralIntensity	= glm::vec4(spectralIntensity, 1.0f);

		void* pMapped = m_pLightsCopyBuffer->Map();
		memcpy(pMapped, &lightsBuffer, sizeof(LightsBuffer));
		m_pLightsCopyBuffer->Unmap();

		m_pCopyCommandList->CopyBuffer(m_pLightsCopyBuffer, 0, m_pLightsBuffer, 0, sizeof(LightsBuffer));

		m_pCopyCommandList->End();

		RenderSystem::GetGraphicsQueue()->ExecuteCommandLists(&m_pCopyCommandList, 1, FPipelineStageFlags::PIPELINE_STAGE_FLAG_UNKNOWN, nullptr, 0, nullptr, 0);
		RenderSystem::GetGraphicsQueue()->Flush();
	}

	void Scene::UpdateCamera(const Camera* pCamera)
	{
		RenderSystem::GetGraphicsQueue()->Flush();

		m_pCopyCommandAllocator->Reset();
		m_pCopyCommandList->Begin(nullptr);

        // TODO: Remove this flush
		RenderSystem::GetGraphicsQueue()->Flush();
		RenderSystem::GetComputeQueue()->Flush();

		PerFrameBuffer perFrameBuffer = {};
		perFrameBuffer.Camera = pCamera->GetData();

		void* pMapped = m_pPerFrameCopyBuffer->Map();
		memcpy(pMapped, &perFrameBuffer, sizeof(PerFrameBuffer));
		m_pPerFrameCopyBuffer->Unmap();

		m_pCopyCommandList->CopyBuffer(m_pPerFrameCopyBuffer, 0, m_pPerFrameBuffer, 0, sizeof(PerFrameBuffer));

		m_pCopyCommandList->End();

		RenderSystem::GetGraphicsQueue()->ExecuteCommandLists(&m_pCopyCommandList, 1, FPipelineStageFlags::PIPELINE_STAGE_FLAG_UNKNOWN, nullptr, 0, nullptr, 0);
		RenderSystem::GetGraphicsQueue()->Flush();
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
		Instance instance = {};
		instance.Transform						= glm::transpose(transform);
		instance.MeshMaterialIndex				= 0;
		instance.Mask							= 0;
		instance.SBTRecordOffset				= 0;
		instance.Flags							= 0;
		instance.AccelerationStructureAddress	= 0;

		m_Instances.push_back(instance);

		uint32 instanceIndex = uint32(m_Instances.size() - 1);
		uint32 meshIndex = 0;

		if (m_GUIDToMappedMeshes.count(gameObject.Mesh) == 0)
		{
			const Mesh* pMesh = ResourceManager::GetMesh(gameObject.Mesh);

			uint32 currentNumSceneVertices = (uint32)m_SceneVertexArray.size();
			m_SceneVertexArray.resize(currentNumSceneVertices + pMesh->VertexCount);
			memcpy(&m_SceneVertexArray[currentNumSceneVertices], pMesh->pVertexArray, pMesh->VertexCount * sizeof(Vertex));

			uint32 currentNumSceneIndices = (uint32)m_SceneIndexArray.size();
			m_SceneIndexArray.resize(currentNumSceneIndices + pMesh->IndexCount);
			memcpy(&m_SceneIndexArray[currentNumSceneIndices], pMesh->pIndexArray, pMesh->IndexCount * sizeof(uint32));

			m_Meshes.push_back(pMesh);
			meshIndex = uint32(m_Meshes.size() - 1);

			MappedMesh newMappedMesh = {};
			m_MappedMeshes.push_back(newMappedMesh);

			m_GUIDToMappedMeshes[gameObject.Mesh] = meshIndex;
		}
		else
		{
			meshIndex = m_GUIDToMappedMeshes[gameObject.Mesh];
		}

		MappedMesh& mappedMesh = m_MappedMeshes[meshIndex];
		uint32 globalMaterialIndex = 0;

		if (m_GUIDToMaterials.count(gameObject.Material) == 0)
		{
			m_Materials.push_back(ResourceManager::GetMaterial(gameObject.Material));
			globalMaterialIndex = uint32(m_Materials.size() - 1);

			m_GUIDToMaterials[gameObject.Material] = globalMaterialIndex;
		}
		else
		{
			globalMaterialIndex = m_GUIDToMaterials[gameObject.Material];
		}

		if (mappedMesh.GUIDToMappedMaterials.count(gameObject.Material) == 0)
		{
			MappedMaterial newMappedMaterial = {};
			newMappedMaterial.MaterialIndex = globalMaterialIndex;

			newMappedMaterial.InstanceIndices.push_back(instanceIndex);

			mappedMesh.MappedMaterials.push_back(newMappedMaterial);
			mappedMesh.GUIDToMappedMaterials[gameObject.Material] = GUID_Lambda(mappedMesh.MappedMaterials.size() - 1);
		}
		else
		{
			mappedMesh.MappedMaterials[mappedMesh.GUIDToMappedMaterials[gameObject.Material]].InstanceIndices.push_back(instanceIndex);
		}

		return instanceIndex;
	}

	uint32 Scene::GetIndirectArgumentOffset(uint32 materialIndex) const
	{
		auto it = m_MaterialIndexToIndirectArgOffsetMap.find(materialIndex);

		if (it != m_MaterialIndexToIndirectArgOffsetMap.end())
			return it->second;

		return m_pSceneMeshIndexBuffer->GetDesc().SizeInBytes / sizeof(IndexedIndirectMeshArgument);
	}

	bool Scene::Init(const SceneDesc& desc)
	{
		m_pName = desc.pName;

		//Device Allocator
		{
			DeviceAllocatorDesc allocatorDesc = {};
			allocatorDesc.pName				= "Scene Allocator";
			allocatorDesc.PageSizeInBytes	= MEGA_BYTE(64);
			m_pDeviceAllocator = RenderSystem::GetDevice()->CreateDeviceAllocator(&allocatorDesc);
		}

		m_pCopyCommandAllocator		= m_pGraphicsDevice->CreateCommandAllocator("Scene Copy Command Allocator", ECommandQueueType::COMMAND_QUEUE_GRAPHICS);

		CommandListDesc copyCommandListDesc = {};
		copyCommandListDesc.pName				= "Scene Copy Command List";
		copyCommandListDesc.Flags				= FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;
		copyCommandListDesc.CommandListType		= ECommandListType::COMMAND_LIST_PRIMARY;

		m_pCopyCommandList = m_pGraphicsDevice->CreateCommandList(m_pCopyCommandAllocator, &copyCommandListDesc);
		
		m_RayTracingEnabled = desc.RayTracingEnabled;

		if (m_RayTracingEnabled)
		{
			m_pBLASBuildCommandAllocator = m_pGraphicsDevice->CreateCommandAllocator("Scene BLAS Build Command Allocator", ECommandQueueType::COMMAND_QUEUE_COMPUTE);

			CommandListDesc blasBuildCommandListDesc = {};
			blasBuildCommandListDesc.pName				= "Scene BLAS Build Command List";
			blasBuildCommandListDesc.Flags				= FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;
			blasBuildCommandListDesc.CommandListType	= ECommandListType::COMMAND_LIST_PRIMARY;

			m_pBLASBuildCommandList = m_pGraphicsDevice->CreateCommandList(m_pBLASBuildCommandAllocator, &blasBuildCommandListDesc);


			m_pTLASBuildCommandAllocator = m_pGraphicsDevice->CreateCommandAllocator("Scene TLAS Build Command Allocator", ECommandQueueType::COMMAND_QUEUE_COMPUTE);

			CommandListDesc tlasBuildCommandListDesc = {};
			tlasBuildCommandListDesc.pName				= "Scene TLAS Build Command List";
			tlasBuildCommandListDesc.Flags				= FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;
			tlasBuildCommandListDesc.CommandListType	= ECommandListType::COMMAND_LIST_PRIMARY;

			m_pTLASBuildCommandList = m_pGraphicsDevice->CreateCommandList(m_pTLASBuildCommandAllocator, &tlasBuildCommandListDesc);


			FenceDesc asFenceDesc = {};
			asFenceDesc.pName			= "Scene AS Fence";
			asFenceDesc.InitalValue		= 0;

			m_pASFence = m_pGraphicsDevice->CreateFence(&asFenceDesc);
		}

		// Lights Buffer
		{
			BufferDesc lightsCopyBufferDesc = {};
			lightsCopyBufferDesc.pName					= "Scene Lights Copy Buffer";
			lightsCopyBufferDesc.MemoryType				= EMemoryType::MEMORY_CPU_VISIBLE;
			lightsCopyBufferDesc.Flags					= FBufferFlags::BUFFER_FLAG_COPY_SRC;
			lightsCopyBufferDesc.SizeInBytes			= sizeof(LightsBuffer);

			m_pLightsCopyBuffer = m_pGraphicsDevice->CreateBuffer(&lightsCopyBufferDesc, m_pDeviceAllocator);

			BufferDesc lightsBufferDesc = {};
			lightsBufferDesc.pName					= "Scene Lights Buffer";
			lightsBufferDesc.MemoryType				= EMemoryType::MEMORY_GPU;
			lightsBufferDesc.Flags					= FBufferFlags::BUFFER_FLAG_CONSTANT_BUFFER | FBufferFlags::BUFFER_FLAG_COPY_DST;
			lightsBufferDesc.SizeInBytes			= sizeof(LightsBuffer);

			m_pLightsBuffer = m_pGraphicsDevice->CreateBuffer(&lightsBufferDesc, m_pDeviceAllocator);
		}

		// Per Frame Buffer
		{
			BufferDesc perFrameCopyBufferDesc = {};
			perFrameCopyBufferDesc.pName					= "Scene Per Frame Copy Buffer";
			perFrameCopyBufferDesc.MemoryType				= EMemoryType::MEMORY_CPU_VISIBLE;
			perFrameCopyBufferDesc.Flags					= FBufferFlags::BUFFER_FLAG_COPY_SRC;
			perFrameCopyBufferDesc.SizeInBytes				= sizeof(PerFrameBuffer);

			m_pPerFrameCopyBuffer = m_pGraphicsDevice->CreateBuffer(&perFrameCopyBufferDesc, m_pDeviceAllocator);

			BufferDesc perFrameBufferDesc = {};
			perFrameBufferDesc.pName				= "Scene Per Frame Buffer";
			perFrameBufferDesc.MemoryType			= EMemoryType::MEMORY_GPU;
			perFrameBufferDesc.Flags				= FBufferFlags::BUFFER_FLAG_CONSTANT_BUFFER | FBufferFlags::BUFFER_FLAG_COPY_DST;;
			perFrameBufferDesc.SizeInBytes			= sizeof(PerFrameBuffer);

			m_pPerFrameBuffer = m_pGraphicsDevice->CreateBuffer(&perFrameBufferDesc, m_pDeviceAllocator);
		}

		return true;
	}

	bool Scene::Finalize()
	{
		LambdaEngine::Clock clock;

		clock.Reset();
		clock.Tick();

		/*------------Ray Tracing Section Begin-------------*/
		std::vector<BuildBottomLevelAccelerationStructureDesc> blasBuildDescriptions;
		blasBuildDescriptions.reserve(m_Meshes.size());

		if (m_RayTracingEnabled)
		{
			AccelerationStructureDesc tlasDesc = {};
			tlasDesc.pName			= "TLAS";
			tlasDesc.Type			= EAccelerationStructureType::ACCELERATION_STRUCTURE_TOP;
			tlasDesc.Flags			= FAccelerationStructureFlags::ACCELERATION_STRUCTURE_FLAG_NONE;// FAccelerationStructureFlags::ACCELERATION_STRUCTURE_FLAG_ALLOW_UPDATE;
			tlasDesc.InstanceCount	= m_Instances.size();

			m_pTLAS = m_pGraphicsDevice->CreateAccelerationStructure(&tlasDesc, m_pDeviceAllocator);

			m_BLASs.reserve(m_Meshes.size());
		}
		/*-------------Ray Tracing Section End--------------*/

		uint32 currentNumSceneVertices = 0;
		uint32 currentNumSceneIndices = 0;

		std::multimap<uint32, std::pair<MappedMaterial, IndexedIndirectMeshArgument>> materialIndexToMeshIndex;
		uint32 indirectArgCount = 0;

		for (uint32 meshIndex = 0; meshIndex < m_Meshes.size(); meshIndex++)
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
				blasDesc.pName				= "BLAS";
				blasDesc.Type				= EAccelerationStructureType::ACCELERATION_STRUCTURE_BOTTOM;
				blasDesc.Flags				= FAccelerationStructureFlags::ACCELERATION_STRUCTURE_FLAG_NONE;
				blasDesc.MaxTriangleCount	= pMesh->IndexCount / 3;
				blasDesc.MaxVertexCount		= pMesh->VertexCount;
				blasDesc.AllowsTransform	= false;

				IAccelerationStructure* pBLAS = m_pGraphicsDevice->CreateAccelerationStructure(&blasDesc, m_pDeviceAllocator);
				accelerationStructureDeviceAddress = pBLAS->GetDeviceAdress();
				m_BLASs.push_back(pBLAS);

				BuildBottomLevelAccelerationStructureDesc blasBuildDesc = {};
				blasBuildDesc.pAccelerationStructure		= pBLAS;
				blasBuildDesc.Flags							= FAccelerationStructureFlags::ACCELERATION_STRUCTURE_FLAG_NONE;
				blasBuildDesc.FirstVertexIndex				= currentNumSceneVertices;
				blasBuildDesc.VertexStride					= sizeof(Vertex);
				blasBuildDesc.IndexBufferByteOffset			= currentNumSceneIndices * sizeof(uint32);
				blasBuildDesc.TriangleCount					= pMesh->IndexCount / 3;
				blasBuildDesc.Update						= false;

				blasBuildDescriptions.push_back(blasBuildDesc);
			}
			/*-------------Ray Tracing Section End--------------*/

			for (uint32 materialIndex = 0; materialIndex < mappedMesh.MappedMaterials.size(); materialIndex++)
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

		m_IndirectArgs.clear();
		m_IndirectArgs.reserve(indirectArgCount);

		m_SortedInstances.clear();
		m_SortedInstances.reserve(m_Instances.size());

		// Extra Loop to sort Indirect Args by Material
		uint32 prevMaterialIndex = UINT32_MAX;
		for (auto it = materialIndexToMeshIndex.begin(); it != materialIndexToMeshIndex.end(); it++)
		{
			uint32 currentMaterialIndex = it->first;
			MappedMaterial& mappedMaterial = it->second.first;
			IndexedIndirectMeshArgument& indirectArg = it->second.second;

			uint32 instanceCount = (uint32)mappedMaterial.InstanceIndices.size();
			uint32 baseInstanceIndex = (uint32)m_SortedInstances.size();

			/*------------Ray Tracing Section Begin-------------*/
			uint64 accelerationStructureDeviceAddress = ((((uint64)indirectArg.InstanceCount) << 32) & 0xFFFFFFFF00000000) | (((uint64)indirectArg.FirstInstance) & 0x00000000FFFFFFFF);
			/*-------------Ray Tracing Section End--------------*/

			for (uint32 instanceIndex = 0; instanceIndex < instanceCount; instanceIndex++)
			{
				Instance instance = m_Instances[mappedMaterial.InstanceIndices[instanceIndex]];
				instance.MeshMaterialIndex				= (uint32)m_IndirectArgs.size();

				/*------------Ray Tracing Section Begin-------------*/
				instance.AccelerationStructureAddress	= accelerationStructureDeviceAddress;
				instance.SBTRecordOffset				= 0;
				instance.Flags							= 0x00000001; //Culling Disabled
				instance.Mask							= 0xFF;
				/*-------------Ray Tracing Section End--------------*/

				m_SortedInstances.push_back(instance);
			}

			if (prevMaterialIndex != currentMaterialIndex)
			{
				prevMaterialIndex = currentMaterialIndex;
				m_MaterialIndexToIndirectArgOffsetMap[currentMaterialIndex] = m_IndirectArgs.size();
			}

			indirectArg.InstanceCount = instanceCount;
			indirectArg.FirstInstance = baseInstanceIndex;
			indirectArg.MaterialIndex = mappedMaterial.MaterialIndex;

			m_IndirectArgs.push_back(indirectArg);
		}

		// Create InstanceBuffer
		{
			uint32 sceneInstanceBufferSize = uint32(m_SortedInstances.size() * sizeof(Instance));

			if (m_pSceneInstanceBuffer == nullptr || sceneInstanceBufferSize > m_pSceneInstanceBuffer->GetDesc().SizeInBytes)
			{
				SAFERELEASE(m_pSceneInstanceBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.pName		= "Scene Instance Buffer";
				bufferDesc.MemoryType	= EMemoryType::MEMORY_GPU;
				bufferDesc.Flags		= FBufferFlags::BUFFER_FLAG_COPY_DST | FBufferFlags::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER;
				bufferDesc.SizeInBytes	= sceneInstanceBufferSize;

				m_pSceneInstanceBuffer = m_pGraphicsDevice->CreateBuffer(&bufferDesc, m_pDeviceAllocator);
			}
		}

		m_SceneAlbedoMaps.resize(MAX_UNIQUE_MATERIALS);
		m_SceneNormalMaps.resize(MAX_UNIQUE_MATERIALS);
		m_SceneAmbientOcclusionMaps.resize(MAX_UNIQUE_MATERIALS);
		m_SceneMetallicMaps.resize(MAX_UNIQUE_MATERIALS);
		m_SceneRoughnessMaps.resize(MAX_UNIQUE_MATERIALS);

		m_SceneAlbedoMapViews.resize(MAX_UNIQUE_MATERIALS);
		m_SceneNormalMapViews.resize(MAX_UNIQUE_MATERIALS);
		m_SceneAmbientOcclusionMapViews.resize(MAX_UNIQUE_MATERIALS);
		m_SceneMetallicMapViews.resize(MAX_UNIQUE_MATERIALS);
		m_SceneRoughnessMapViews.resize(MAX_UNIQUE_MATERIALS);

		std::vector<MaterialProperties> sceneMaterialProperties;
		sceneMaterialProperties.resize(MAX_UNIQUE_MATERIALS);

		for (uint32 i = 0; i < MAX_UNIQUE_MATERIALS; i++)
		{
			if (i < m_Materials.size())
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
			
				sceneMaterialProperties[i]			= pMaterial->Properties;
			}
			else
			{
				const Material* pMaterial = ResourceManager::GetMaterial(DEFAULT_MATERIAL);

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
			
				sceneMaterialProperties[i]			= pMaterial->Properties;
			}
		}

		clock.Tick();
		LOG_INFO("Scene Build took %f milliseconds", clock.GetDeltaTime().AsMilliSeconds());

		m_pCopyCommandAllocator->Reset();
		m_pCopyCommandList->Begin(nullptr);

		// Material Properties
		{
			uint32 sceneMaterialPropertiesSize = uint32(sceneMaterialProperties.size() * sizeof(MaterialProperties));

			if (m_pSceneMaterialPropertiesCopyBuffer == nullptr || sceneMaterialPropertiesSize > m_pSceneMaterialPropertiesCopyBuffer->GetDesc().SizeInBytes)
			{
				SAFERELEASE(m_pSceneMaterialPropertiesCopyBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.pName		= "Scene Material Properties Copy Buffer";
				bufferDesc.MemoryType	= EMemoryType::MEMORY_CPU_VISIBLE;
				bufferDesc.Flags		= FBufferFlags::BUFFER_FLAG_COPY_SRC;
				bufferDesc.SizeInBytes	= sceneMaterialPropertiesSize;

				m_pSceneMaterialPropertiesCopyBuffer = m_pGraphicsDevice->CreateBuffer(&bufferDesc, m_pDeviceAllocator);
			}

			void* pMapped = m_pSceneMaterialPropertiesCopyBuffer->Map();
			memcpy(pMapped, sceneMaterialProperties.data(), sceneMaterialPropertiesSize);
			m_pSceneMaterialPropertiesCopyBuffer->Unmap();

			if (m_pSceneMaterialProperties == nullptr || sceneMaterialPropertiesSize > m_pSceneMaterialProperties->GetDesc().SizeInBytes)
			{
				SAFERELEASE(m_pSceneMaterialProperties);

				BufferDesc bufferDesc = {};
				bufferDesc.pName		= "Scene Material Properties";
				bufferDesc.MemoryType	= EMemoryType::MEMORY_GPU;
				bufferDesc.Flags		= FBufferFlags::BUFFER_FLAG_COPY_DST | FBufferFlags::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER;
				bufferDesc.SizeInBytes	= sceneMaterialPropertiesSize;

				m_pSceneMaterialProperties = m_pGraphicsDevice->CreateBuffer(&bufferDesc, m_pDeviceAllocator);
			}

			m_pCopyCommandList->CopyBuffer(m_pSceneMaterialPropertiesCopyBuffer, 0, m_pSceneMaterialProperties, 0, sceneMaterialPropertiesSize);
		}

		// Vertices
		{
			uint32 sceneVertexBufferSize = uint32(m_SceneVertexArray.size() * sizeof(Vertex));

			if (m_pSceneVertexCopyBuffer == nullptr || sceneVertexBufferSize > m_pSceneVertexCopyBuffer->GetDesc().SizeInBytes)
			{
				SAFERELEASE(m_pSceneVertexCopyBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.pName		= "Scene Vertex Copy Buffer";
				bufferDesc.MemoryType	= EMemoryType::MEMORY_CPU_VISIBLE;
				bufferDesc.Flags		= FBufferFlags::BUFFER_FLAG_COPY_SRC;
				bufferDesc.SizeInBytes	= sceneVertexBufferSize;

				m_pSceneVertexCopyBuffer = m_pGraphicsDevice->CreateBuffer(&bufferDesc, m_pDeviceAllocator);
			}

			void* pMapped = m_pSceneVertexCopyBuffer->Map();
			memcpy(pMapped, m_SceneVertexArray.data(), sceneVertexBufferSize);
			m_pSceneVertexCopyBuffer->Unmap();

			if (m_pSceneVertexBuffer == nullptr || sceneVertexBufferSize > m_pSceneVertexBuffer->GetDesc().SizeInBytes)
			{
				SAFERELEASE(m_pSceneVertexBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.pName		= "Scene Vertex Buffer";
				bufferDesc.MemoryType	= EMemoryType::MEMORY_GPU;
				bufferDesc.Flags		= FBufferFlags::BUFFER_FLAG_COPY_DST | FBufferFlags::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | FBufferFlags::BUFFER_FLAG_VERTEX_BUFFER;
				bufferDesc.SizeInBytes	= sceneVertexBufferSize;

				m_pSceneVertexBuffer = m_pGraphicsDevice->CreateBuffer(&bufferDesc, m_pDeviceAllocator);
			}

			m_pCopyCommandList->CopyBuffer(m_pSceneVertexCopyBuffer, 0, m_pSceneVertexBuffer, 0, sceneVertexBufferSize);
		}
		
		// Indices
		{
			uint32 sceneIndexBufferSize = uint32(m_SceneIndexArray.size() * sizeof(uint32));

			if (m_pSceneIndexCopyBuffer == nullptr || sceneIndexBufferSize > m_pSceneIndexCopyBuffer->GetDesc().SizeInBytes)
			{
				SAFERELEASE(m_pSceneIndexCopyBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.pName		= "Scene Index Copy Buffer";
				bufferDesc.MemoryType	= EMemoryType::MEMORY_CPU_VISIBLE;
				bufferDesc.Flags		= FBufferFlags::BUFFER_FLAG_COPY_SRC;
				bufferDesc.SizeInBytes	= sceneIndexBufferSize;

				m_pSceneIndexCopyBuffer = m_pGraphicsDevice->CreateBuffer(&bufferDesc, m_pDeviceAllocator);
			}

			void* pMapped = m_pSceneIndexCopyBuffer->Map();
			memcpy(pMapped, m_SceneIndexArray.data(), sceneIndexBufferSize);
			m_pSceneIndexCopyBuffer->Unmap();

			if (m_pSceneIndexBuffer == nullptr || sceneIndexBufferSize > m_pSceneIndexBuffer->GetDesc().SizeInBytes)
			{
				SAFERELEASE(m_pSceneIndexBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.pName		= "Scene Index Buffer";
				bufferDesc.MemoryType	= EMemoryType::MEMORY_GPU;
				bufferDesc.Flags		= FBufferFlags::BUFFER_FLAG_COPY_DST | FBufferFlags::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | FBufferFlags::BUFFER_FLAG_INDEX_BUFFER;
				bufferDesc.SizeInBytes	= sceneIndexBufferSize;

				m_pSceneIndexBuffer = m_pGraphicsDevice->CreateBuffer(&bufferDesc, m_pDeviceAllocator);
			}

			m_pCopyCommandList->CopyBuffer(m_pSceneIndexCopyBuffer, 0, m_pSceneIndexBuffer, 0, sceneIndexBufferSize);
		}

		// Instances
		{
			uint32 sceneInstanceBufferSize = uint32(m_SortedInstances.size() * sizeof(Instance));

			if (m_pSceneInstanceCopyBuffer == nullptr || sceneInstanceBufferSize > m_pSceneInstanceCopyBuffer->GetDesc().SizeInBytes)
			{
				SAFERELEASE(m_pSceneInstanceCopyBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.pName		= "Scene Instance Copy Buffer";
				bufferDesc.MemoryType	= EMemoryType::MEMORY_CPU_VISIBLE;
				bufferDesc.Flags		= FBufferFlags::BUFFER_FLAG_COPY_SRC;
				bufferDesc.SizeInBytes	= sceneInstanceBufferSize;

				m_pSceneInstanceCopyBuffer = m_pGraphicsDevice->CreateBuffer(&bufferDesc, m_pDeviceAllocator);
			}

			void* pMapped = m_pSceneInstanceCopyBuffer->Map();
			memcpy(pMapped, m_SortedInstances.data(), sceneInstanceBufferSize);
			m_pSceneInstanceCopyBuffer->Unmap();

			if (m_pSceneInstanceBuffer == nullptr || sceneInstanceBufferSize > m_pSceneInstanceBuffer->GetDesc().SizeInBytes)
			{
				SAFERELEASE(m_pSceneInstanceBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.pName		= "Scene Instance Buffer";
				bufferDesc.MemoryType	= EMemoryType::MEMORY_GPU;
				bufferDesc.Flags		= FBufferFlags::BUFFER_FLAG_COPY_DST | FBufferFlags::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER;
				bufferDesc.SizeInBytes	= sceneInstanceBufferSize;

				m_pSceneInstanceBuffer = m_pGraphicsDevice->CreateBuffer(&bufferDesc, m_pDeviceAllocator);
			}

			m_pCopyCommandList->CopyBuffer(m_pSceneInstanceCopyBuffer, 0, m_pSceneInstanceBuffer, 0, sceneInstanceBufferSize);
		}

		// Indirect Args
		{
			uint32 sceneMeshIndexBufferSize = uint32(m_IndirectArgs.size() * sizeof(IndexedIndirectMeshArgument));

			if (m_pSceneMeshIndexCopyBuffer == nullptr || sceneMeshIndexBufferSize > m_pSceneMeshIndexCopyBuffer->GetDesc().SizeInBytes)
			{
				SAFERELEASE(m_pSceneMeshIndexCopyBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.pName		= "Scene Mesh Index Copy Buffer";
				bufferDesc.MemoryType	= EMemoryType::MEMORY_CPU_VISIBLE;
				bufferDesc.Flags		= FBufferFlags::BUFFER_FLAG_COPY_SRC;
				bufferDesc.SizeInBytes	= sceneMeshIndexBufferSize;

				m_pSceneMeshIndexCopyBuffer = m_pGraphicsDevice->CreateBuffer(&bufferDesc, m_pDeviceAllocator);
			}

			void* pMapped = m_pSceneMeshIndexCopyBuffer->Map();
			memcpy(pMapped, m_IndirectArgs.data(), sceneMeshIndexBufferSize);
			m_pSceneMeshIndexCopyBuffer->Unmap();

			if (m_pSceneMeshIndexBuffer == nullptr || sceneMeshIndexBufferSize > m_pSceneMeshIndexBuffer->GetDesc().SizeInBytes)
			{
				SAFERELEASE(m_pSceneMeshIndexBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.pName		= "Scene Mesh Index Buffer";
				bufferDesc.MemoryType	= EMemoryType::MEMORY_GPU;
				bufferDesc.Flags		= FBufferFlags::BUFFER_FLAG_COPY_DST | FBufferFlags::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | FBufferFlags::BUFFER_FLAG_INDIRECT_BUFFER;
				bufferDesc.SizeInBytes	= sceneMeshIndexBufferSize;

				m_pSceneMeshIndexBuffer = m_pGraphicsDevice->CreateBuffer(&bufferDesc, m_pDeviceAllocator);
			}

			m_pCopyCommandList->CopyBuffer(m_pSceneMeshIndexCopyBuffer, 0, m_pSceneMeshIndexBuffer, 0, sceneMeshIndexBufferSize);
		}

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

				for (uint32 i = 0; i < blasBuildDescriptions.size(); i++)
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

				BuildTopLevelAccelerationStructureDesc tlasBuildDesc = {};
				tlasBuildDesc.pAccelerationStructure	= m_pTLAS;
				tlasBuildDesc.Flags						= FAccelerationStructureFlags::ACCELERATION_STRUCTURE_FLAG_NONE;
				tlasBuildDesc.pInstanceBuffer			= m_pSceneInstanceBuffer;
				tlasBuildDesc.InstanceCount				= m_Instances.size();
				tlasBuildDesc.Update					= false;

				//m_pTLAS = RayTracingTestVK::CreateTLAS(m_pGraphicsDevice, &tlasDesc, &tlasBuildDesc, m_pASBuildCommandList);

				m_pTLASBuildCommandList->BuildTopLevelAccelerationStructure(&tlasBuildDesc);

				m_pTLASBuildCommandList->End();
			}

			RenderSystem::GetComputeQueue()->ExecuteCommandLists(&m_pBLASBuildCommandList, 1, FPipelineStageFlags::PIPELINE_STAGE_FLAG_UNKNOWN, nullptr, 0, m_pASFence, 1);
			RenderSystem::GetComputeQueue()->ExecuteCommandLists(&m_pTLASBuildCommandList, 1, FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, m_pASFence, 1, nullptr, 0);
			RenderSystem::GetComputeQueue()->Flush();

			//RayTracingTestVK::Debug(m_pGraphicsDevice, blasBuildDescriptions[0].pAccelerationStructure, m_pTLAS);
		}
		/*-------------Ray Tracing Section End--------------*/

		D_LOG_MESSAGE("[Scene]: Successfully finalized \"%s\"! ", m_pName);

		return true;
	}
}
