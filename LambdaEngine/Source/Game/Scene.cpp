#include "Game/Scene.h"

#include "Rendering/Core/API/IGraphicsDevice.h"
#include "Audio/API/IAudioDevice.h"
#include "Resources/ResourceManager.h"

#include "Resources/Mesh.h"
#include "Resources/Material.h"
#include "Rendering/Core/API/IBuffer.h"
#include "Rendering/Core/API/ITexture.h"
#include "Rendering/Core/API/ICommandQueue.h"
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

	bool Scene::Finalize(const SceneDesc& desc)
	{
		LambdaEngine::Clock clock;

		clock.Reset();
		clock.Tick();

		m_SortedInstances.clear();

		uint32 currentNumSceneVertices = 0;
		uint32 currentNumSceneIndices = 0;

		m_SortedInstances.reserve(m_Instances.size());

		std::vector<IndexedIndirectMeshArgument> meshIndexBuffer;
		meshIndexBuffer.reserve(200);

		for (uint32 meshIndex = 0; meshIndex < m_Meshes.size(); meshIndex++)
		{
			MappedMesh& mappedMesh = m_MappedMeshes[meshIndex];
			const Mesh* pMesh = m_Meshes[meshIndex];

			uint32 newNumSceneVertices = currentNumSceneVertices + pMesh->VertexCount;
			uint32 newNumSceneIndices = currentNumSceneIndices + pMesh->IndexCount;

			for (uint32 materialIndex = 0; materialIndex < mappedMesh.MappedMaterials.size(); materialIndex++)
			{
				MappedMaterial& mappedMaterial = mappedMesh.MappedMaterials[materialIndex];

				uint32 instanceCount = (uint32)mappedMaterial.InstanceIndices.size();
				uint32 baseInstanceIndex = (uint32)m_SortedInstances.size();

				for (uint32 instanceIndex = 0; instanceIndex < instanceCount; instanceIndex++)
				{
					Instance instance = m_Instances[mappedMaterial.InstanceIndices[instanceIndex]];
					instance.MeshMaterialIndex = uint32(meshIndexBuffer.size() - 1);

					m_SortedInstances.push_back(instance);
				}

				IndexedIndirectMeshArgument indirectMeshArgument = {};
				indirectMeshArgument.IndexCount			= pMesh->IndexCount;
				indirectMeshArgument.InstanceCount		= instanceCount;
				indirectMeshArgument.FirstIndex			= currentNumSceneIndices;
				indirectMeshArgument.VertexOffset		= currentNumSceneVertices;
				indirectMeshArgument.FirstInstance		= baseInstanceIndex;
				indirectMeshArgument.MaterialIndex		= mappedMaterial.MaterialIndex;
				indirectMeshArgument.Padding0			= 0;
				indirectMeshArgument.Padding1			= 0;

				meshIndexBuffer.push_back(indirectMeshArgument);
			}

			currentNumSceneVertices = newNumSceneVertices;
			currentNumSceneIndices = newNumSceneIndices;
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

		{
			uint32 sceneMaterialPropertiesSize = uint32(sceneMaterialProperties.size() * sizeof(MaterialProperties));

			if (m_pSceneMaterialProperties == nullptr || sceneMaterialPropertiesSize > m_pSceneMaterialProperties->GetDesc().SizeInBytes)
			{
				SAFERELEASE(m_pSceneMaterialProperties);

				BufferDesc sceneMaterialPropertiesBufferDesc = {};
				sceneMaterialPropertiesBufferDesc.pName			= "Scene Material Properties";
				sceneMaterialPropertiesBufferDesc.MemoryType	= EMemoryType::MEMORY_CPU_VISIBLE;
				sceneMaterialPropertiesBufferDesc.Flags			= FBufferFlags::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER;
				sceneMaterialPropertiesBufferDesc.SizeInBytes	= sceneMaterialPropertiesSize;

				m_pSceneMaterialProperties = m_pGraphicsDevice->CreateBuffer(sceneMaterialPropertiesBufferDesc);
			}

			void* pMapped = m_pSceneMaterialProperties->Map();
			memcpy(pMapped, sceneMaterialProperties.data(), sceneMaterialPropertiesSize);
			m_pSceneMaterialProperties->Unmap();
		}

		{
			uint32 sceneVertexBufferSize = uint32(m_SceneVertexArray.size() * sizeof(Vertex));

			if (m_pSceneVertexBuffer == nullptr || sceneVertexBufferSize > m_pSceneVertexBuffer->GetDesc().SizeInBytes)
			{
				SAFERELEASE(m_pSceneVertexBuffer);

				BufferDesc sceneVertexBufferDesc = {};
				sceneVertexBufferDesc.pName						= "Scene Vertex Buffer";
				sceneVertexBufferDesc.MemoryType				= EMemoryType::MEMORY_CPU_VISIBLE;
				sceneVertexBufferDesc.Flags						= FBufferFlags::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | FBufferFlags::BUFFER_FLAG_VERTEX_BUFFER;
				sceneVertexBufferDesc.SizeInBytes				= sceneVertexBufferSize;

				m_pSceneVertexBuffer = m_pGraphicsDevice->CreateBuffer(sceneVertexBufferDesc);
			}

			void* pMapped = m_pSceneVertexBuffer->Map();
			memcpy(pMapped, m_SceneVertexArray.data(), sceneVertexBufferSize);
			m_pSceneVertexBuffer->Unmap();
		}
		
		{
			uint32 sceneIndexBufferSize = uint32(m_SceneIndexArray.size() * sizeof(uint32));

			if (m_pSceneIndexBuffer == nullptr || sceneIndexBufferSize > m_pSceneIndexBuffer->GetDesc().SizeInBytes)
			{
				SAFERELEASE(m_pSceneIndexBuffer);

				BufferDesc sceneIndexBufferDesc = {};
				sceneIndexBufferDesc.pName						= "Scene Index Buffer";
				sceneIndexBufferDesc.MemoryType					= EMemoryType::MEMORY_CPU_VISIBLE;
				sceneIndexBufferDesc.Flags						= FBufferFlags::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | FBufferFlags::BUFFER_FLAG_INDEX_BUFFER;
				sceneIndexBufferDesc.SizeInBytes				= sceneIndexBufferSize;

				m_pSceneIndexBuffer = m_pGraphicsDevice->CreateBuffer(sceneIndexBufferDesc);
			}

			void* pMapped = m_pSceneIndexBuffer->Map();
			memcpy(pMapped, m_SceneIndexArray.data(), sceneIndexBufferSize);
			m_pSceneIndexBuffer->Unmap();
		}

		{
			uint32 sceneIndexBufferSize = uint32(m_SortedInstances.size() * sizeof(Instance));

			if (m_pSceneInstanceBuffer == nullptr || sceneIndexBufferSize > m_pSceneInstanceBuffer->GetDesc().SizeInBytes)
			{
				SAFERELEASE(m_pSceneInstanceBuffer);

				BufferDesc sceneInstanceBufferDesc = {};
				sceneInstanceBufferDesc.pName					= "Scene Instance Buffer";
				sceneInstanceBufferDesc.MemoryType				= EMemoryType::MEMORY_CPU_VISIBLE;
				sceneInstanceBufferDesc.Flags					= FBufferFlags::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER;
				sceneInstanceBufferDesc.SizeInBytes				= sceneIndexBufferSize;

				m_pSceneInstanceBuffer = m_pGraphicsDevice->CreateBuffer(sceneInstanceBufferDesc);
			}

			void* pMapped = m_pSceneInstanceBuffer->Map();
			memcpy(pMapped, m_SortedInstances.data(), sceneIndexBufferSize);
			m_pSceneInstanceBuffer->Unmap();
		}

		
		{
			uint32 sceneMeshIndexBufferSize = uint32(meshIndexBuffer.size() * sizeof(IndexedIndirectMeshArgument));

			if (m_pSceneMeshIndexBuffer == nullptr || sceneMeshIndexBufferSize > m_pSceneMeshIndexBuffer->GetDesc().SizeInBytes)
			{
				SAFERELEASE(m_pSceneMeshIndexBuffer);

				BufferDesc sceneMeshIndexBufferDesc = {};
				sceneMeshIndexBufferDesc.pName					= "Scene Mesh Index Buffer";
				sceneMeshIndexBufferDesc.MemoryType				= EMemoryType::MEMORY_CPU_VISIBLE;
				sceneMeshIndexBufferDesc.Flags					= FBufferFlags::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | FBufferFlags::BUFFER_FLAG_INDIRECT_BUFFER;
				sceneMeshIndexBufferDesc.SizeInBytes			= sceneMeshIndexBufferSize;

				m_pSceneMeshIndexBuffer = m_pGraphicsDevice->CreateBuffer(sceneMeshIndexBufferDesc);
			}

			void* pMapped = m_pSceneMeshIndexBuffer->Map();
			memcpy(pMapped, meshIndexBuffer.data(), sceneMeshIndexBufferSize);
			m_pSceneMeshIndexBuffer->Unmap();
		}


		{
			BufferDesc sceneMeshIndexBufferDesc = {};
			sceneMeshIndexBufferDesc.pName					= "Scene Per Frame Buffer";
			sceneMeshIndexBufferDesc.MemoryType				= EMemoryType::MEMORY_CPU_VISIBLE;
			sceneMeshIndexBufferDesc.Flags					= FBufferFlags::BUFFER_FLAG_CONSTANT_BUFFER;
			sceneMeshIndexBufferDesc.SizeInBytes			= sizeof(PerFrameBuffer);

			m_pPerFrameBuffer = m_pGraphicsDevice->CreateBuffer(sceneMeshIndexBufferDesc);
		}

		m_pName = desc.pName;

		D_LOG_MESSAGE("[Scene]: Successfully finalized \"%s\"! ", m_pName);

		return true;
	}
}
