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
#include "Rendering/RenderSystem.h"

#include "Log/Log.h"

#include "Time/API/Clock.h"

namespace LambdaEngine
{
	Scene::Scene(const IGraphicsDevice* pGraphicsDevice, const IAudioDevice* pAudioDevice, const ResourceManager* pResourceManager) :
		m_pGraphicsDevice(pGraphicsDevice),
		m_pAudioDevice(pAudioDevice),
		m_pResourceManager(pResourceManager)
	{
	}

	Scene::~Scene()
	{
		SAFERELEASE(m_pCopyCommandAllocator);
		SAFERELEASE(m_pCopyCommandList);
		SAFERELEASE(m_pSceneMaterialPropertiesCopyBuffer);
		SAFERELEASE(m_pSceneVertexCopyBuffer);
		SAFERELEASE(m_pSceneIndexCopyBuffer);
		SAFERELEASE(m_pSceneInstanceCopyBuffer);
		SAFERELEASE(m_pSceneMeshIndexCopyBuffer);
		SAFERELEASE(m_pPerFrameBuffer);
		SAFERELEASE(m_pSceneMaterialProperties);
		SAFERELEASE(m_pSceneVertexBuffer);
		SAFERELEASE(m_pSceneIndexBuffer);
		SAFERELEASE(m_pSceneInstanceBuffer);
		SAFERELEASE(m_pSceneMeshIndexBuffer);
	}

	void Scene::UpdateCamera(const Camera* pCamera)
	{
		PerFrameBuffer perFrameBuffer = {};
		perFrameBuffer.Camera = pCamera->GetData();

		void* pMapped = m_pPerFrameBuffer->Map();
		memcpy(pMapped, &perFrameBuffer, sizeof(PerFrameBuffer));
		m_pPerFrameBuffer->Unmap();
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
		instance.Transform = transform;
		instance.MeshMaterialIndex = 0;
		instance.Mask = 0;
		instance.SBTRecordOffset = 0;
		instance.Flags = 0;
		instance.AccelerationStructureHandle = 0;

		m_Instances.push_back(instance);

		uint32 instanceIndex = uint32(m_Instances.size() - 1);
		uint32 meshIndex = 0;

		if (m_GUIDToMappedMeshes.count(gameObject.Mesh) == 0)
		{
			const Mesh* pMesh = m_pResourceManager->GetMesh(gameObject.Mesh);

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
			m_Materials.push_back(m_pResourceManager->GetMaterial(gameObject.Material));
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

		m_pCopyCommandAllocator = m_pGraphicsDevice->CreateCommandAllocator("Scene Command Allocator", ECommandQueueType::COMMAND_QUEUE_GRAPHICS);

		CommandListDesc commandListDesc = {};
		commandListDesc.pName			= "Scene Command List";
		commandListDesc.Flags			= FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;
		commandListDesc.CommandListType = ECommandListType::COMMAND_LIST_PRIMARY;

		m_pCopyCommandList		= m_pGraphicsDevice->CreateCommandList(m_pCopyCommandAllocator, commandListDesc);

		return true;
	}

	bool Scene::Finalize()
	{
		LambdaEngine::Clock clock;

		clock.Reset();
		clock.Tick();

		uint32 currentNumSceneVertices = 0;
		uint32 currentNumSceneIndices = 0;

		std::multimap<uint32, std::pair<MappedMaterial, IndexedIndirectMeshArgument>> materialIndexToMeshIndex;
		uint32 indirectArgCount = 0;

		for (uint32 meshIndex = 0; meshIndex < m_Meshes.size(); meshIndex++)
		{
			MappedMesh& mappedMesh = m_MappedMeshes[meshIndex];
			const Mesh* pMesh = m_Meshes[meshIndex];

			uint32 newNumSceneVertices = currentNumSceneVertices + pMesh->VertexCount;
			uint32 newNumSceneIndices = currentNumSceneIndices + pMesh->IndexCount;

			for (uint32 materialIndex = 0; materialIndex < mappedMesh.MappedMaterials.size(); materialIndex++)
			{
				MappedMaterial& mappedMaterial = mappedMesh.MappedMaterials[materialIndex];

				IndexedIndirectMeshArgument indirectMeshArgument = {};
				indirectMeshArgument.IndexCount			= pMesh->IndexCount;
				//indirectMeshArgument.InstanceCount		= instanceCount;
				indirectMeshArgument.FirstIndex			= currentNumSceneIndices;
				indirectMeshArgument.VertexOffset		= currentNumSceneVertices;
				//indirectMeshArgument.FirstInstance		= baseInstanceIndex;
				//indirectMeshArgument.MaterialIndex		= mappedMaterial.MaterialIndex;
				indirectMeshArgument.Padding0			= 0;
				indirectMeshArgument.Padding1			= 0;

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

		//Extra Loop to sort Indirect Args by Material
		uint32 prevMaterialIndex = UINT32_MAX;
		for (auto it = materialIndexToMeshIndex.begin(); it != materialIndexToMeshIndex.end(); it++)
		{
			uint32 currentMaterialIndex = it->first;
			MappedMaterial& mappedMaterial = it->second.first;
			IndexedIndirectMeshArgument& indirectArg = it->second.second;

			uint32 instanceCount = (uint32)mappedMaterial.InstanceIndices.size();
			uint32 baseInstanceIndex = (uint32)m_SortedInstances.size();

			for (uint32 instanceIndex = 0; instanceIndex < instanceCount; instanceIndex++)
			{
				Instance instance = m_Instances[mappedMaterial.InstanceIndices[instanceIndex]];
				instance.MeshMaterialIndex = (uint32)m_IndirectArgs.size();

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
				const Material* pMaterial = m_pResourceManager->GetMaterial(DEFAULT_MATERIAL);

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
		m_pCopyCommandList->Reset();

		m_pCopyCommandList->Begin(nullptr);

		//Material Properties
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

				m_pSceneMaterialPropertiesCopyBuffer = m_pGraphicsDevice->CreateBuffer(bufferDesc);
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

				m_pSceneMaterialProperties = m_pGraphicsDevice->CreateBuffer(bufferDesc);
			}

			m_pCopyCommandList->CopyBuffer(m_pSceneMaterialPropertiesCopyBuffer, 0, m_pSceneMaterialProperties, 0, sceneMaterialPropertiesSize);
		}

		//Vertices
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

				m_pSceneVertexCopyBuffer = m_pGraphicsDevice->CreateBuffer(bufferDesc);
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

				m_pSceneVertexBuffer = m_pGraphicsDevice->CreateBuffer(bufferDesc);
			}

			m_pCopyCommandList->CopyBuffer(m_pSceneVertexCopyBuffer, 0, m_pSceneVertexBuffer, 0, sceneVertexBufferSize);
		}
		
		//Indices
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

				m_pSceneIndexCopyBuffer = m_pGraphicsDevice->CreateBuffer(bufferDesc);
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

				m_pSceneIndexBuffer = m_pGraphicsDevice->CreateBuffer(bufferDesc);
			}

			m_pCopyCommandList->CopyBuffer(m_pSceneIndexCopyBuffer, 0, m_pSceneIndexBuffer, 0, sceneIndexBufferSize);
		}

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

				m_pSceneInstanceCopyBuffer = m_pGraphicsDevice->CreateBuffer(bufferDesc);
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

				m_pSceneInstanceBuffer = m_pGraphicsDevice->CreateBuffer(bufferDesc);
			}

			m_pCopyCommandList->CopyBuffer(m_pSceneInstanceCopyBuffer, 0, m_pSceneInstanceBuffer, 0, sceneInstanceBufferSize);
		}

		
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

				m_pSceneMeshIndexCopyBuffer = m_pGraphicsDevice->CreateBuffer(bufferDesc);
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

				m_pSceneMeshIndexBuffer = m_pGraphicsDevice->CreateBuffer(bufferDesc);
			}

			m_pCopyCommandList->CopyBuffer(m_pSceneMeshIndexCopyBuffer, 0, m_pSceneMeshIndexBuffer, 0, sceneMeshIndexBufferSize);
		}

		{
			BufferDesc sceneMeshIndexBufferDesc = {};
			sceneMeshIndexBufferDesc.pName					= "Scene Per Frame Buffer";
			sceneMeshIndexBufferDesc.MemoryType				= EMemoryType::MEMORY_CPU_VISIBLE;
			sceneMeshIndexBufferDesc.Flags					= FBufferFlags::BUFFER_FLAG_CONSTANT_BUFFER;
			sceneMeshIndexBufferDesc.SizeInBytes			= sizeof(PerFrameBuffer);

			m_pPerFrameBuffer = m_pGraphicsDevice->CreateBuffer(sceneMeshIndexBufferDesc);
		}

		m_pCopyCommandList->End();

		RenderSystem::GetGraphicsQueue()->ExecuteCommandLists(&m_pCopyCommandList, 1, FPipelineStageFlags::PIPELINE_STAGE_FLAG_UNKNOWN, nullptr, 0, nullptr, 0);
		RenderSystem::GetGraphicsQueue()->Flush();

		D_LOG_MESSAGE("[Scene]: Successfully finalized \"%s\"! ", m_pName);

		return true;
	}
}
