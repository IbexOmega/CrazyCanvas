#include "Game/Scene.h"

#include "Rendering/Core/API/GraphicsDevice.h"
#include "Audio/API/IAudioDevice.h"
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
#include "Rendering/Renderer.h"
#include "Rendering/RenderGraph.h"

#include "Rendering/Core/Vulkan/Vulkan.h"

#include "Log/Log.h"

#include "Time/API/Clock.h"
#include "Math/Random.h"

namespace LambdaEngine
{
	Scene::Scene()
	{
	}

	Scene::~Scene()
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

		SAFERELEASE(m_pTLAS);

		for (AccelerationStructure* pBLAS : m_BLASs)
		{
			SAFERELEASE(pBLAS);
		}
		m_BLASs.Clear();
	}

	bool Scene::Init(const SceneDesc& desc)
	{
		m_RayTracingEnabled = desc.RayTracingEnabled;

		for (uint32 i = 0; i < MAX_UNIQUE_MATERIALS; i++)
		{
			m_FreeMaterialSlots.push(i);
		}

		// Per Frame Buffer
		{
			BufferDesc perFrameCopyBufferDesc = {};
			perFrameCopyBufferDesc.DebugName		= "Scene Per Frame Copy Buffer";
			perFrameCopyBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
			perFrameCopyBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_SRC;
			perFrameCopyBufferDesc.SizeInBytes		= sizeof(PerFrameBuffer);

			m_pPerFrameStagingBuffer = RenderSystem::GetDevice()->CreateBuffer(&perFrameCopyBufferDesc);

			BufferDesc perFrameBufferDesc = {};
			perFrameBufferDesc.DebugName			= "Scene Per Frame Buffer";
			perFrameBufferDesc.MemoryType			= EMemoryType::MEMORY_TYPE_GPU;
			perFrameBufferDesc.Flags				= FBufferFlag::BUFFER_FLAG_CONSTANT_BUFFER | FBufferFlag::BUFFER_FLAG_COPY_DST;;
			perFrameBufferDesc.SizeInBytes			= sizeof(PerFrameBuffer);

			m_pPerFrameBuffer = RenderSystem::GetDevice()->CreateBuffer(&perFrameBufferDesc);
		}

		Texture*		pDefaultColorMap		= ResourceManager::GetTexture(GUID_TEXTURE_DEFAULT_COLOR_MAP);
		TextureView*	pDefaultColorMapView	= ResourceManager::GetTextureView(GUID_TEXTURE_DEFAULT_COLOR_MAP);
		Texture*		pDefaultNormalMap		= ResourceManager::GetTexture(GUID_TEXTURE_DEFAULT_NORMAL_MAP);
		TextureView*	pDefaultNormalMapView	= ResourceManager::GetTextureView(GUID_TEXTURE_DEFAULT_NORMAL_MAP);

		for (uint32 i = 0; i < MAX_UNIQUE_MATERIALS; i++)
		{
			m_ppSceneAlbedoMaps[i]					= pDefaultColorMap;
			m_ppSceneNormalMaps[i]					= pDefaultNormalMap;
			m_ppSceneAmbientOcclusionMaps[i]		= pDefaultColorMap;
			m_ppSceneRoughnessMaps[i]				= pDefaultColorMap;
			m_ppSceneMetallicMaps[i]				= pDefaultColorMap;
			m_ppSceneAlbedoMapViews[i]				= pDefaultColorMapView;
			m_ppSceneNormalMapViews[i]				= pDefaultNormalMapView;
			m_ppSceneAmbientOcclusionMapViews[i]	= pDefaultColorMapView;
			m_ppSceneRoughnessMapViews[i]			= pDefaultColorMapView;
			m_ppSceneMetallicMapViews[i]			= pDefaultColorMapView;
		}

		return true;
	}

	bool Scene::Finalize()
	{
		CommandList* pGraphicsCommandList = Renderer::GetRenderGraph()->AcquireGraphicsCopyCommandList();
		CommandList* pComputeCommandList = Renderer::GetRenderGraph()->AcquireGraphicsCopyCommandList();

		//Update Per Frame Data
		{
			m_PerFrameData.FrameIndex = 0;
			m_PerFrameData.RandomSeed = uint32(Random::Int32(INT32_MIN, INT32_MAX));

			UpdatePerFrameBuffer(pGraphicsCommandList);
		}
		
		//Update Instance Data
		{
			UpdateInstanceBuffers(pGraphicsCommandList, 0);

			if (m_RayTracingEnabled)
			{
				BuildTLAS(pComputeCommandList, true);
			}
		}

		//Update Empty MaterialData
		{
			UpdateMaterialPropertiesBuffer(pGraphicsCommandList, 0);
		}

		return true;
	}

	void Scene::PrepareRender(CommandList* pGraphicsCommandList, CommandList* pComputeCommandList, uint64 frameIndex, uint64 modFrameIndex)
	{
		//Release Staging Buffers from older frame
		//Todo: Better solution for this
		{
			TArray<Buffer*>& buffersToRemove = m_BuffersToRemove[modFrameIndex];

			for (Buffer* pStagingBuffer : buffersToRemove)
			{
				SAFERELEASE(pStagingBuffer);
			}

			buffersToRemove.Clear();
		}

		//Update Per Frame Data
		{
			m_PerFrameData.FrameIndex = frameIndex;
			m_PerFrameData.RandomSeed = uint32(Random::Int32(INT32_MIN, INT32_MAX));

			UpdatePerFrameBuffer(pGraphicsCommandList);
		}
		
		//Update Instance Data
		{
			UpdateInstanceBuffers(pGraphicsCommandList, modFrameIndex);

			if (m_RayTracingEnabled)
			{
				BuildTLAS(pComputeCommandList, true);
			}
		}

		//Update Empty MaterialData
		{
			UpdateMaterialPropertiesBuffer(pGraphicsCommandList, modFrameIndex);
		}
	}

	void Scene::AddGameObject(uint32 entityID, const GameObject& gameObject, const glm::mat4& transform, bool isStatic, bool animated)
	{
		if (isStatic && animated)
		{
			LOG_ERROR("[--TBD--]: A static game object cannot also be animated!");
			return;
		}

		uint32 materialSlot;
		MeshAndInstancesMap::iterator meshAndInstancesIt;

		MeshKey meshKey;
		meshKey.MeshGUID		= gameObject.Mesh;
		meshKey.IsStatic		= isStatic;
		meshKey.IsAnimated		= animated;
		meshKey.EntityID		= entityID;

		//Get MeshAndInstanceIterator
		{
			meshAndInstancesIt = m_MeshAndInstancesMap.find(meshKey);

			if (meshAndInstancesIt == m_MeshAndInstancesMap.end())
			{
				const Mesh* pMesh = ResourceManager::GetMesh(gameObject.Mesh);
				VALIDATE(pMesh != nullptr);

				MeshEntry meshEntry = {};

				CommandList* pCommandList = Renderer::GetRenderGraph()->AcquireGraphicsCopyCommandList();

				//Vertices
				{
					BufferDesc vertexStagingBufferDesc = {};
					vertexStagingBufferDesc.DebugName	= "Vertex Staging Buffer";
					vertexStagingBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
					vertexStagingBufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_SRC;
					vertexStagingBufferDesc.SizeInBytes = pMesh->VertexCount * sizeof(Vertex);

					Buffer* pVertexStagingBuffer = RenderSystem::GetDevice()->CreateBuffer(&vertexStagingBufferDesc);

					void* pMapped = pVertexStagingBuffer->Map();
					memcpy(pMapped, pMesh->pVertexArray, vertexStagingBufferDesc.SizeInBytes);
					pVertexStagingBuffer->Unmap();

					BufferDesc vertexBufferDesc = {};
					vertexBufferDesc.DebugName		= "Vertex Buffer";
					vertexBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_GPU;
					vertexBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER;
					vertexBufferDesc.SizeInBytes	= vertexStagingBufferDesc.SizeInBytes;

					meshEntry.pVertexBuffer = RenderSystem::GetDevice()->CreateBuffer(&vertexBufferDesc);

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

					Buffer* pIndexStagingBuffer = RenderSystem::GetDevice()->CreateBuffer(&indexStagingBufferDesc);

					void* pMapped = pIndexStagingBuffer->Map();
					memcpy(pMapped, pMesh->pIndexArray, indexStagingBufferDesc.SizeInBytes);
					pIndexStagingBuffer->Unmap();

					BufferDesc indexBufferDesc = {};
					indexBufferDesc.DebugName		= "Index Buffer";
					indexBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_GPU;
					indexBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_INDEX_BUFFER;
					indexBufferDesc.SizeInBytes		= indexStagingBufferDesc.SizeInBytes;

					meshEntry.pIndexBuffer	= RenderSystem::GetDevice()->CreateBuffer(&indexBufferDesc);
					meshEntry.IndexCount	= pMesh->IndexCount;

					pCommandList->CopyBuffer(pIndexStagingBuffer, 0, meshEntry.pIndexBuffer, 0, indexBufferDesc.SizeInBytes);
					m_BuffersToRemove[0].PushBack(pIndexStagingBuffer);
				}

				meshAndInstancesIt = m_MeshAndInstancesMap.insert({ meshKey, meshEntry }).first;
			}
		}

		//Get Material Slot
		{
			THashTable<uint32, uint32>::iterator materialSlotIt = m_MaterialMap.find(gameObject.Material);

			//Push new Material if the Material is yet to be registered
			if (materialSlotIt == m_MaterialMap.end())
			{
				const Material* pMaterial = ResourceManager::GetMaterial(gameObject.Material);
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

				m_MaterialMap.insert({ gameObject.Material, materialSlot });
			}
			else
			{
				materialSlot = materialSlotIt->second;
			}
		}
		
		InstanceKey instanceKey = {};
		instanceKey.MeshKey			= meshKey;
		instanceKey.InstanceIndex	= meshAndInstancesIt->second.Instances.GetSize();
		m_EntityIDsToInstanceKey[entityID] = instanceKey;

		Instance instance = {};
		instance.Transform			= transform;
		instance.PrevTransform		= transform;
		instance.MaterialSlot		= materialSlot;
		meshAndInstancesIt->second.Instances.PushBack(instance);

		m_DirtyInstanceBuffers.insert(&meshAndInstancesIt->second);
	}

	void Scene::UpdateTransform(uint32 entityID, const glm::mat4& transform)
	{
		THashTable<GUID_Lambda, InstanceKey>::iterator instanceKeyIt = m_EntityIDsToInstanceKey.find(entityID);

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

		pInstanceToUpdate->PrevTransform	= pInstanceToUpdate->Transform;
		pInstanceToUpdate->Transform		= transform;
		m_DirtyInstanceBuffers.insert(&meshAndInstancesIt->second);
	}

	void Scene::UpdateCamera(const Camera* pCamera)
	{
		m_PerFrameData.Camera = pCamera->GetData();
	}

	void Scene::GetDrawArgs(TArray<DrawArg>& drawArgs, uint32 key) const
	{
		//Todo: Cache these

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

	void Scene::UpdateInstanceBuffers(CommandList* pCommandList, uint64 modFrameIndex)
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

				pDirtyInstanceBufferEntry->pInstanceStagingBuffer = RenderSystem::GetDevice()->CreateBuffer(&bufferDesc);
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

				pDirtyInstanceBufferEntry->pInstanceBuffer = RenderSystem::GetDevice()->CreateBuffer(&bufferDesc);
			}
			
			pCommandList->CopyBuffer(pDirtyInstanceBufferEntry->pInstanceStagingBuffer, 0, pDirtyInstanceBufferEntry->pInstanceBuffer, 0, requiredBufferSize);
		}

		m_DirtyInstanceBuffers.clear();
	}

	void Scene::UpdatePerFrameBuffer(CommandList* pCommandList)
	{
		void* pMapped = m_pPerFrameStagingBuffer->Map();
		memcpy(pMapped, &m_PerFrameData, sizeof(PerFrameBuffer));
		m_pPerFrameStagingBuffer->Unmap();

		pCommandList->CopyBuffer(m_pPerFrameStagingBuffer, 0, m_pPerFrameBuffer, 0, sizeof(PerFrameBuffer));
	}

	void Scene::UpdateMaterialPropertiesBuffer(CommandList* pCommandList, uint64 modFrameIndex)
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

			m_pMaterialParametersStagingBuffer = RenderSystem::GetDevice()->CreateBuffer(&bufferDesc);
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

			m_pMaterialParametersBuffer = RenderSystem::GetDevice()->CreateBuffer(&bufferDesc);
		}

		pCommandList->CopyBuffer(m_pMaterialParametersStagingBuffer, 0, m_pMaterialParametersBuffer, 0, requiredBufferSize);
	}

	void Scene::BuildTLAS(CommandList* pBuildCommandList, bool update)
	{
		//BuildTopLevelAccelerationStructureDesc tlasBuildDesc = {};
		//tlasBuildDesc.pAccelerationStructure	= m_pTLAS;
		//tlasBuildDesc.Flags						= FAccelerationStructureFlag::ACCELERATION_STRUCTURE_FLAG_ALLOW_UPDATE;
		//tlasBuildDesc.pInstanceBuffer			= m_pScenePrimaryInstanceBuffer;
		//tlasBuildDesc.InstanceCount				= m_PrimaryInstances.GetSize();
		//tlasBuildDesc.Update					= update;

		//pBuildCommandList->BuildTopLevelAccelerationStructure(&tlasBuildDesc);
	}
}
