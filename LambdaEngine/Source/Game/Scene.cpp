#include "Game/Scene.h"

#include "Rendering/Core/API/IGraphicsDevice.h"
#include "Audio/AudioDevice.h"
#include "Resources/ResourceManager.h"

#include "Resources/Mesh.h"
#include "Resources/Material.h"
#include "Rendering/Core/API/IBuffer.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	Scene::Scene(const IGraphicsDevice* pGraphicsDevice, const AudioDevice* pAudioDevice, const ResourceManager* pResourceManager) :
		m_pGraphicsDevice(pGraphicsDevice),
		m_pAudioDevice(pAudioDevice),
		m_pResourceManager(pResourceManager)
	{
	}

	Scene::~Scene()
	{
	}

	uint32 Scene::AddStaticGameObject(const GameObject& gameObject, glm::mat4& transform)
	{
		Instance instance = {};
		instance.Transform						= transform;
		instance.CustomIndex					= 0;
		instance.Mask							= 0;
		instance.SBTRecordOffset				= 0;
		instance.Flags							= 0;
		instance.AccelerationStructureHandle	= 0;

		m_Instances.push_back(instance);

		uint32 instanceIndex = m_Instances.size() - 1;
		uint32 meshIndex = 0;

		if (m_GUIDToMappedMeshes.count(gameObject.Mesh) == 0)
		{
			m_Meshes.push_back(m_pResourceManager->GetMesh(gameObject.Mesh));
			meshIndex = m_Meshes.size() - 1;

			MappedMesh newMappedMesh = {};
			m_MappedMeshes.push_back(newMappedMesh);

			m_GUIDToMappedMeshes[gameObject.Mesh] = meshIndex;
		}
		else
		{
			meshIndex = m_GUIDToMappedMeshes[gameObject.Mesh];
		}

		MappedMesh& mappedMesh = m_MappedMeshes[meshIndex];
		uint32 meshLocalMaterialIndex = 0;
		
		if (mappedMesh.GUIDToMappedMaterials.count(gameObject.Material) == 0)
		{
			m_Materials.push_back(m_pResourceManager->GetMaterial(gameObject.Material));
			uint32 globalMaterialIndex = m_Materials.size() - 1;

			MappedMaterial newMappedMaterial = {};
			newMappedMaterial.MaterialIndex = globalMaterialIndex;

			
			newMappedMaterial.InstanceIndices.push_back(instanceIndex);

			mappedMesh.MappedMaterials.push_back(newMappedMaterial);
			mappedMesh.GUIDToMappedMaterials[gameObject.Material] = mappedMesh.MappedMaterials.size() - 1;
		}

		return instanceIndex;
	}

	uint32 Scene::AddDynamicGameObject(const GameObject& gameObject, glm::mat4& transform)
	{
		LOG_WARNING("[Scene]: Call to unimplemented function AddDynamicGameObject!");
	}

	bool Scene::Finalize(const SceneDesc& desc)
	{
		std::vector<Vertex> sceneVertices;
		uint32 numSceneVertices = 0;

		std::vector<uint32> sceneIndices;
		uint32 numSceneIndices = 0;

		m_SortedInstances.clear();
		m_SortedInstances.reserve(m_Instances.size());

		std::vector<IndirectMeshArgument>	meshIndexBuffer;

		for (uint32 meshIndex = 0; meshIndex < m_Meshes.size(); meshIndex++)
		{
			MappedMesh& mappedMesh = m_MappedMeshes[meshIndex];
			const Mesh* pMesh = m_Meshes[meshIndex];

			uint32 newNumSceneVertices = numSceneVertices + pMesh->VertexCount;
			uint32 newNumSceneIndices = numSceneIndices + pMesh->IndexCount;

			sceneVertices.reserve(newNumSceneVertices);
			sceneIndices.reserve(newNumSceneIndices);

			memcpy(&sceneVertices[numSceneVertices], pMesh->pVertexArray, pMesh->VertexCount * sizeof(Vertex));
			memcpy(&sceneIndices[numSceneIndices], pMesh->pIndexArray, pMesh->IndexCount * sizeof(uint32));

			for (uint32 materialIndex = 0; materialIndex < mappedMesh.MappedMaterials.size(); materialIndex++)
			{
				MappedMaterial& mappedMaterial = mappedMesh.MappedMaterials[materialIndex];

				uint32 instanceCount = mappedMaterial.InstanceIndices.size();
				
				for (uint32 instanceIndex = 0; instanceIndex < instanceCount; instanceIndex)
				{
					m_SortedInstances.push_back(m_Instances[mappedMaterial.InstanceIndices[instanceIndex]]);
				}

				IndirectMeshArgument indirectMeshArgument = {};
				indirectMeshArgument.VertexCount		= pMesh->IndexCount;
				indirectMeshArgument.InstanceCount		= instanceCount;
				indirectMeshArgument.FirstIndex			= numSceneIndices;
				indirectMeshArgument.FirstInstance		= 0;
				indirectMeshArgument.BaseVertexIndex	= numSceneVertices;
				indirectMeshArgument.MaterialIndex		= mappedMaterial.MaterialIndex;

				meshIndexBuffer.push_back(indirectMeshArgument);
			}

			numSceneVertices = newNumSceneVertices;
			numSceneIndices = newNumSceneIndices;

		}

		std::vector<ITexture*> sceneAlbedoMaps;
		std::vector<ITexture*> sceneNormalMaps;
		std::vector<ITexture*> sceneAmbientOcclusionMaps;
		std::vector<ITexture*> sceneMetallicMaps;
		std::vector<ITexture*> sceneRoughnessMaps;
		sceneAlbedoMaps.reserve(m_Materials.size());
		sceneNormalMaps.reserve(m_Materials.size());
		sceneAmbientOcclusionMaps.reserve(m_Materials.size());
		sceneMetallicMaps.reserve(m_Materials.size());
		sceneRoughnessMaps.reserve(m_Materials.size());

		std::vector<MaterialProperties> sceneMaterialProperties;
		sceneMaterialProperties.reserve(m_Materials.size());

		for (const Material* pMaterial : m_Materials)
		{
			sceneAlbedoMaps.push_back(pMaterial->pAlbedoMap);
			sceneNormalMaps.push_back(pMaterial->pNormalMap);
			sceneAmbientOcclusionMaps.push_back(pMaterial->pAmbientOcclusionMap);
			sceneMetallicMaps.push_back(pMaterial->pMetallicMap);
			sceneRoughnessMaps.push_back(pMaterial->pRoughnessMap);
			
			sceneMaterialProperties.push_back(pMaterial->Properties);
		}

		{
			BufferDesc sceneMaterialPropertiesBufferDesc = {};
			sceneMaterialPropertiesBufferDesc.pName = "Scene Material Properties";
			sceneMaterialPropertiesBufferDesc.MemoryType = EMemoryType::CPU_MEMORY;
			sceneMaterialPropertiesBufferDesc.Flags = EBufferFlags::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER;
			sceneMaterialPropertiesBufferDesc.SizeInBytes = sceneMaterialProperties.size() * sizeof(MaterialProperties);

			m_pSceneMaterialProperties = m_pGraphicsDevice->CreateBuffer(sceneMaterialPropertiesBufferDesc);

			void* pMapped = m_pSceneMaterialProperties->Map();
			memcpy(pMapped, sceneMaterialProperties.data(), sceneMaterialPropertiesBufferDesc.SizeInBytes);
			m_pSceneMaterialProperties->Unmap();
		}

		{
			BufferDesc sceneVertexBufferDesc = {};
			sceneVertexBufferDesc.pName						= "Scene Vertex Buffer";
			sceneVertexBufferDesc.MemoryType				= EMemoryType::CPU_MEMORY;
			sceneVertexBufferDesc.Flags						= EBufferFlags::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER;
			sceneVertexBufferDesc.SizeInBytes				= sceneVertices.size() * sizeof(Vertex);

			m_pSceneVertexBuffer = m_pGraphicsDevice->CreateBuffer(sceneVertexBufferDesc);

			void* pMapped = m_pSceneVertexBuffer->Map();
			memcpy(pMapped, sceneVertices.data(), sceneVertexBufferDesc.SizeInBytes);
			m_pSceneVertexBuffer->Unmap();
		}
		
		{
			BufferDesc sceneIndexBufferDesc = {};
			sceneIndexBufferDesc.pName						= "Scene Index Buffer";
			sceneIndexBufferDesc.MemoryType					= EMemoryType::CPU_MEMORY;
			sceneIndexBufferDesc.Flags						= EBufferFlags::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER;
			sceneIndexBufferDesc.SizeInBytes				= sceneIndices.size() * sizeof(uint32);

			m_pSceneIndexBuffer = m_pGraphicsDevice->CreateBuffer(sceneIndexBufferDesc);

			void* pMapped = m_pSceneIndexBuffer->Map();
			memcpy(pMapped, sceneIndices.data(), sceneIndexBufferDesc.SizeInBytes);
			m_pSceneIndexBuffer->Unmap();
		}

		{
			BufferDesc sceneInstanceBufferDesc = {};
			sceneInstanceBufferDesc.pName					= "Scene Instance Buffer";
			sceneInstanceBufferDesc.MemoryType				= EMemoryType::CPU_MEMORY;
			sceneInstanceBufferDesc.Flags					= EBufferFlags::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER;
			sceneInstanceBufferDesc.SizeInBytes				= m_SortedInstances.size() * sizeof(Instance);

			m_pSceneInstanceBuffer = m_pGraphicsDevice->CreateBuffer(sceneInstanceBufferDesc);

			void* pMapped = m_pSceneInstanceBuffer->Map();
			memcpy(pMapped, m_SortedInstances.data(), sceneInstanceBufferDesc.SizeInBytes);
			m_pSceneInstanceBuffer->Unmap();
		}

		
		{
			BufferDesc sceneMeshIndexBufferDesc = {};
			sceneMeshIndexBufferDesc.pName					= "Scene Mesh Index Buffer";
			sceneMeshIndexBufferDesc.MemoryType				= EMemoryType::CPU_MEMORY;
			sceneMeshIndexBufferDesc.Flags					= EBufferFlags::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER;
			sceneMeshIndexBufferDesc.SizeInBytes			= meshIndexBuffer.size() * sizeof(IndirectMeshArgument);

			m_pSceneMeshIndexBuffer = m_pGraphicsDevice->CreateBuffer(sceneMeshIndexBufferDesc);

			void* pMapped = m_pSceneMeshIndexBuffer->Map();
			memcpy(pMapped, meshIndexBuffer.data(), sceneMeshIndexBufferDesc.SizeInBytes);
			m_pSceneMeshIndexBuffer->Unmap();
		}

		m_pName = desc.pName;

		D_LOG_MESSAGE("[Scene]: Successfully intialized \"%s\"! ", m_pName);

		return true;
	}
}