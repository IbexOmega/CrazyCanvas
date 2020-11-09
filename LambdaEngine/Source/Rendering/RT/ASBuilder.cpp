#include "Rendering/RT/ASBuilder.h"

#include "Rendering/RenderGraphTypes.h"

#include "Rendering/Core/API/AccelerationStructure.h"
#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/CommandAllocator.h"

#include "Rendering/RenderAPI.h"
#include "Rendering/RenderGraph.h"

namespace LambdaEngine
{
	ASBuilder::ASBuilder()
	{
	}

	ASBuilder::~ASBuilder()
	{
		SAFERELEASE(m_pTLAS);

		for (BLASData& blasData : m_BLASes)
		{
			SAFERELEASE(blasData.pBLAS);
		}

		m_BLASes.Clear();

		ReleaseBackBufferBound();
	}

	bool ASBuilder::Init()
	{
		return true;
	}

	bool ASBuilder::RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc)
	{
		m_pRenderGraph = pPreInitDesc->pRenderGraph;

		if (m_BackBufferCount != pPreInitDesc->BackBufferCount)
		{
			ReleaseBackBufferBound();

			m_BackBufferCount = pPreInitDesc->BackBufferCount;

			if (!CreateCommandLists())
			{
				LOG_ERROR("[ASBuilder]: Failed to create Command Lists");
				return false;
			}

			if (!CreateBuffers())
			{
				LOG_ERROR("[ASBuilder]: Failed to create Buffers");
				return false;
			}

			m_pResourcesToRemove = DBG_NEW TArray<DeviceChild*>[m_BackBufferCount];
		}

		return true;
	}

	bool ASBuilder::RenderGraphPostInit()
	{
		if (!CreateDummyBuffers())
		{
			LOG_ERROR("[ASBuilder]: Failed to create Dummy Buffers");
			return false;
		}

		return true;
	}

	void ASBuilder::BuildTriBLAS(uint32& blasIndex, uint32 hitGroupIndex, Buffer* pVertexBuffer, Buffer* pIndexBuffer, uint32 vertexCount, uint32 vertexSize, uint32 indexCount, bool allowUpdate)
	{
		std::scoped_lock<SpinLock> lock(m_Lock);

		if (blasIndex == BLAS_UNINITIALIZED_INDEX)
		{
			BLASData blasData = {};

			//Create BLAS and SBT Record
			{
				AccelerationStructureDesc blasCreateDesc = {};
				blasCreateDesc.DebugName		= "BLAS";
				blasCreateDesc.Type				= EAccelerationStructureType::ACCELERATION_STRUCTURE_TYPE_BOTTOM;
				blasCreateDesc.Flags			= allowUpdate ? FAccelerationStructureFlag::ACCELERATION_STRUCTURE_FLAG_ALLOW_UPDATE : 0;
				blasCreateDesc.MaxTriangleCount = indexCount / 3;
				blasCreateDesc.MaxVertexCount	= vertexCount;
				blasCreateDesc.AllowsTransform	= false;

				blasData.pBLAS = RenderAPI::GetDevice()->CreateAccelerationStructure(&blasCreateDesc);

				SBTRecord* pSBTRecord;
				uint32* pHitGroupIndex;

				if (!m_FreeSBTIndices.IsEmpty())
				{
					blasData.SBTRecordOffset = m_FreeSBTIndices.GetBack();
					m_FreeSBTIndices.PopBack();

					pSBTRecord = &m_SBTRecords[blasData.SBTRecordOffset];
					pHitGroupIndex = &m_HitGroupIndices[blasData.SBTRecordOffset];
				}
				else
				{
					blasData.SBTRecordOffset = m_SBTRecords.GetSize();
					pSBTRecord = &m_SBTRecords.PushBack({});
					pHitGroupIndex = &m_HitGroupIndices.PushBack(0);
				}

				pSBTRecord->VertexBufferAddress	= pVertexBuffer->GetDeviceAddress();
				pSBTRecord->IndexBufferAddress	= pIndexBuffer->GetDeviceAddress();
				*pHitGroupIndex = hitGroupIndex;

				m_SBTRecordsDirty = true;

				if (!m_FreeBLASIndices.IsEmpty())
				{
					blasIndex = m_FreeBLASIndices.GetBack();
					m_FreeBLASIndices.PopBack();
					m_BLASes[blasIndex] = blasData;
				}
				else
				{
					blasIndex = m_BLASes.GetSize();
					m_BLASes.PushBack(blasData);
				}
			}

			//Create Build Desc
			{
				BuildBottomLevelAccelerationStructureDesc blasBuildDesc = {};
				blasBuildDesc.pAccelerationStructure	= blasData.pBLAS;
				blasBuildDesc.Flags						= allowUpdate ? FAccelerationStructureFlag::ACCELERATION_STRUCTURE_FLAG_ALLOW_UPDATE : 0;
				blasBuildDesc.pVertexBuffer				= pVertexBuffer;
				blasBuildDesc.FirstVertexIndex			= 0;
				blasBuildDesc.VertexStride				= vertexSize;
				blasBuildDesc.pIndexBuffer				= pIndexBuffer;
				blasBuildDesc.IndexBufferByteOffset		= 0;
				blasBuildDesc.TriangleCount				= indexCount / 3;
				blasBuildDesc.pTransformBuffer			= nullptr;
				blasBuildDesc.TransformByteOffset		= 0;
				blasBuildDesc.Update					= false;

				m_DirtyBLASes.PushBack(blasBuildDesc);
			}
		}
		else if (blasIndex < m_BLASes.GetSize())
		{
			BLASData& blasData = m_BLASes[blasIndex];

			//Create BLAS build Desc
			{
				//We assume that vertexCount/indexCount does not change and thus do not check if we need to recreate the BLAS
				BuildBottomLevelAccelerationStructureDesc blasBuildDesc = {};
				blasBuildDesc.pAccelerationStructure	= blasData.pBLAS;
				blasBuildDesc.Flags						= FAccelerationStructureFlag::ACCELERATION_STRUCTURE_FLAG_ALLOW_UPDATE;
				blasBuildDesc.pVertexBuffer				= pVertexBuffer;
				blasBuildDesc.FirstVertexIndex			= 0;
				blasBuildDesc.VertexStride				= vertexSize;
				blasBuildDesc.pIndexBuffer				= pIndexBuffer;
				blasBuildDesc.IndexBufferByteOffset		= 0;
				blasBuildDesc.TriangleCount				= indexCount / 3;
				blasBuildDesc.pTransformBuffer			= nullptr;
				blasBuildDesc.TransformByteOffset		= 0;
				blasBuildDesc.Update					= true;

				m_DirtyBLASes.PushBack(blasBuildDesc);
			}

			//Update SBT Record (if needed)
			{
				SBTRecord* pSBTRecord = &m_SBTRecords[blasData.SBTRecordOffset];
				uint32* pHitGroupIndex = &m_HitGroupIndices[blasData.SBTRecordOffset];

				uint64 vertexBufferDeviceAddress	= pVertexBuffer->GetDeviceAddress();
				uint64 indexBufferDeviceAddress		= pIndexBuffer->GetDeviceAddress();

				if (*pHitGroupIndex != hitGroupIndex)
				{
					*pHitGroupIndex = hitGroupIndex;
					m_SBTRecordsDirty = true;
				}

				if (pSBTRecord->VertexBufferAddress != vertexBufferDeviceAddress)
				{
					pSBTRecord->VertexBufferAddress = vertexBufferDeviceAddress;
					m_SBTRecordsDirty = true;
				}

				if (pSBTRecord->IndexBufferAddress != indexBufferDeviceAddress)
				{
					pSBTRecord->IndexBufferAddress = indexBufferDeviceAddress;
					m_SBTRecordsDirty = true;
				}
			}
		}
		else
		{
			LOG_ERROR("[ASBuilder]: BuildTriBLAS called with index %u, but that BLAS doesn't exist...", blasIndex);
		}
	}

	void ASBuilder::ReleaseBLAS(uint32 index)
	{
		std::scoped_lock<SpinLock> lock(m_Lock);

		VALIDATE(index < m_BLASes.GetSize());

		BLASData& blasData = m_BLASes[index];

		// Push SBT Offset to Free
		m_FreeSBTIndices.PushBack(blasData.SBTRecordOffset);

		// Release BLAS
		m_pResourcesToRemove[m_ModFrameIndex].PushBack(blasData.pBLAS);

		//Dequeue from updating if enqueued
		for (auto dirtyBLASIt = m_DirtyBLASes.Begin(); dirtyBLASIt != m_DirtyBLASes.End();)
		{
			if (dirtyBLASIt->pAccelerationStructure == blasData.pBLAS)
			{
				dirtyBLASIt = m_DirtyBLASes.Erase(dirtyBLASIt);
			}
			else
			{
				dirtyBLASIt++;
			}
		}

		//Push BLAS Free Index
		m_FreeBLASIndices.PushBack(index);

		//Reset BLASData
		blasData.pBLAS = nullptr;
		blasData.SBTRecordOffset = UINT32_MAX;
	}

	uint32 ASBuilder::AddInstance(const ASInstanceDesc& asInstanceDesc)
	{
		std::scoped_lock<SpinLock> lock(m_Lock);

		VALIDATE(asInstanceDesc.BlasIndex < m_BLASes.GetSize());
		
		BLASData& blasData = m_BLASes[asInstanceDesc.BlasIndex];

		uint32 instanceIndex = m_Instances.GetSize();

		AccelerationStructureInstance asInstance = {};
		asInstance.Transform					= glm::transpose(asInstanceDesc.Transform);
		asInstance.CustomIndex					= asInstanceDesc.CustomIndex;
		asInstance.Mask							= asInstanceDesc.HitMask;
		asInstance.SBTRecordOffset				= blasData.SBTRecordOffset;
		asInstance.Flags						= asInstanceDesc.Flags;
		asInstance.AccelerationStructureAddress	= blasData.pBLAS->GetDeviceAddress();

		m_InstanceIndicesChanged = true;
		m_Instances.PushBack(asInstance);
			
		uint32 externalIndex;

		if (!m_FreeInstanceIndices.IsEmpty())
		{
			externalIndex = m_FreeInstanceIndices.GetBack();
			m_FreeInstanceIndices.PopBack();
			m_InstanceIndices[externalIndex] = instanceIndex;
		}
		else
		{
			externalIndex = m_InstanceIndices.GetSize();
			m_InstanceIndices.PushBack(instanceIndex);
		}

		return externalIndex;
	}

	void ASBuilder::AddInstances(const TArray<ASInstanceDesc>& asInstanceDescriptions, TArray<uint32>& asInstanceIDs)
	{
		asInstanceIDs.Reserve(asInstanceDescriptions.GetSize());

		for (const ASInstanceDesc& asInstanceDesc : asInstanceDescriptions)
		{
			asInstanceIDs.PushBack(AddInstance(asInstanceDesc));
		}
	}

	void ASBuilder::RemoveInstance(uint32 instanceIndex)
	{
		std::scoped_lock<SpinLock> lock(m_Lock);

		VALIDATE(instanceIndex < m_InstanceIndices.GetSize());

		//Get True (Indirect) Instance Index
		uint32 trueInstanceIndex = m_InstanceIndices[instanceIndex];

		VALIDATE(trueInstanceIndex < m_Instances.GetSize());

		//Find the True (Indirect) Instance Index Entry of the Last Instance
		uint32 backIndex = m_Instances.GetSize() - 1;
		auto lastInstanceIndexIt = std::find(m_InstanceIndices.Begin(), m_InstanceIndices.End(), backIndex);
		VALIDATE(lastInstanceIndexIt != m_InstanceIndices.End());

		//Move the Last Instance to the Removed Instance Index
		m_Instances[trueInstanceIndex] = m_Instances.GetBack();
		m_Instances.PopBack();

		//Remap the previous Last Instance True (Indirect) Instance Index to point to the removed Index
		(*lastInstanceIndexIt) = trueInstanceIndex;

		//Invalidate Removed Instance Index
		m_InstanceIndices[instanceIndex] = UINT32_MAX;

		//Push the Removed Instance Index to Free
		m_FreeInstanceIndices.PushBack(instanceIndex);

		//Set Instance Indices Changed to true
		m_InstanceIndicesChanged = true;
	}

	void ASBuilder::UpdateInstanceTransform(uint32 instanceIndex, const glm::mat4& transform)
	{
		std::scoped_lock<SpinLock> lock(m_Lock);

		VALIDATE(instanceIndex < m_InstanceIndices.GetSize());

		//Get True (Indirect) Instance Index
		uint32 trueInstanceIndex = m_InstanceIndices[instanceIndex];

		VALIDATE(trueInstanceIndex < m_Instances.GetSize());

		AccelerationStructureInstance& asInstance = m_Instances[trueInstanceIndex];
		asInstance.Transform = glm::transpose(transform);
	}

	void ASBuilder::UpdateInstances(std::function<void(AccelerationStructureInstance&)> updateFunc)
	{
		std::scoped_lock<SpinLock> lock(m_Lock);

		for (AccelerationStructureInstance& asInstance : m_Instances)
		{
			updateFunc(asInstance);
		}
	}

	void ASBuilder::Update(
		Timestamp delta, 
		uint32 modFrameIndex, 
		uint32 backBufferIndex)
	{
		UNREFERENCED_VARIABLE(delta);
		UNREFERENCED_VARIABLE(backBufferIndex);

		m_ModFrameIndex = modFrameIndex;

		TArray<DeviceChild*>& deviceResourcesToRemove = m_pResourcesToRemove[m_ModFrameIndex];

		for (DeviceChild* pResource : deviceResourcesToRemove)
		{
			SAFERELEASE(pResource);
		}

		deviceResourcesToRemove.Clear();

		//Record Command List, we do this here instead of in Render as we would like to be able to do RenderGraph::UpdateResource if we recreate the TLAS
		{
			//Copy commands must be executed early so we use Render Graph Acquire for this
			CommandList* pCopyCommandList = m_pRenderGraph->AcquireComputeCopyCommandList();

			//We assume the TLAS is always dirty so it's okay to always begin the Command List
			m_ppComputeCommandAllocators[m_ModFrameIndex]->Reset();
			CommandList* pMainCommandList = m_ppComputeCommandLists[m_ModFrameIndex];
			pMainCommandList->Begin(nullptr);

			if (m_SBTRecordsDirty)
			{
				m_pRenderGraph->UpdateGlobalSBT(pMainCommandList, m_SBTRecords, m_HitGroupIndices, deviceResourcesToRemove);
				m_SBTRecordsDirty = false;
			}

			//Build BLASes
			{
				for (BuildBottomLevelAccelerationStructureDesc& blasBuildDesc : m_DirtyBLASes)
				{
					pMainCommandList->BuildBottomLevelAccelerationStructure(&blasBuildDesc);
				}

				//This is required to sync up BLAS building with TLAS building, to make sure that the BLAS is built before the TLAS
				static constexpr const PipelineMemoryBarrierDesc BLAS_MEMORY_BARRIER
				{
					.SrcMemoryAccessFlags = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_ACCELERATION_STRUCTURE_WRITE,
					.DstMemoryAccessFlags = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_ACCELERATION_STRUCTURE_READ,
				};

				pMainCommandList->PipelineMemoryBarriers(
					FPipelineStageFlag::PIPELINE_STAGE_FLAG_ACCELERATION_STRUCTURE_BUILD, 
					FPipelineStageFlag::PIPELINE_STAGE_FLAG_ACCELERATION_STRUCTURE_BUILD, 
					&BLAS_MEMORY_BARRIER,
					1);

				m_DirtyBLASes.Clear();
			}

			//Update TLAS, assume always dirty
			if (!m_Instances.IsEmpty())
			{
				Buffer* pInstanceStagingBuffer = m_ppInstanceStagingBuffers[m_ModFrameIndex];
				Buffer* pInstanceIndicesStagingBuffer = m_ppInstanceIndicesStagingBuffers[m_ModFrameIndex];
				uint32 instanceCount = m_Instances.GetSize();

				//Update Instance Indices Buffer
				if (m_InstanceIndicesChanged)
				{
					m_InstanceIndicesChanged = false;
					uint64 requiredInstanceIndicesBufferSize = uint64(m_InstanceIndices.GetSize()) * sizeof(uint32);

					if (pInstanceIndicesStagingBuffer == nullptr || pInstanceIndicesStagingBuffer->GetDesc().SizeInBytes < requiredInstanceIndicesBufferSize)
					{
						if (pInstanceIndicesStagingBuffer != nullptr) deviceResourcesToRemove.PushBack(pInstanceIndicesStagingBuffer);

						BufferDesc instanceIndicesStagingBufferDesc = {};
						instanceIndicesStagingBufferDesc.DebugName		= "Instance Indices Staging Buffer";
						instanceIndicesStagingBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
						instanceIndicesStagingBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_SRC;
						instanceIndicesStagingBufferDesc.SizeInBytes	= requiredInstanceIndicesBufferSize;

						pInstanceIndicesStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&instanceIndicesStagingBufferDesc);
						m_ppInstanceIndicesStagingBuffers[m_ModFrameIndex] = pInstanceIndicesStagingBuffer;
					}

					void* pMapped = pInstanceIndicesStagingBuffer->Map();
					memcpy(pMapped, m_InstanceIndices.GetData(), requiredInstanceIndicesBufferSize);
					pInstanceIndicesStagingBuffer->Unmap();

					if (m_pInstanceIndicesBuffer == nullptr || m_pInstanceIndicesBuffer->GetDesc().SizeInBytes < requiredInstanceIndicesBufferSize)
					{
						if (m_pInstanceIndicesBuffer != nullptr) deviceResourcesToRemove.PushBack(m_pInstanceIndicesBuffer);

						BufferDesc instanceIndicesBufferDesc = {};
						instanceIndicesBufferDesc.DebugName		= "Instance Indices Buffer";
						instanceIndicesBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
						instanceIndicesBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER;
						instanceIndicesBufferDesc.SizeInBytes	= requiredInstanceIndicesBufferSize;

						m_pInstanceIndicesBuffer = RenderAPI::GetDevice()->CreateBuffer(&instanceIndicesBufferDesc);

						ResourceUpdateDesc resourceUpdateDesc = {};
						resourceUpdateDesc.ResourceName						= AS_INSTANCE_INDICES_BUFFER;
						resourceUpdateDesc.ExternalBufferUpdate.Count		= 1;
						resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &m_pInstanceIndicesBuffer;

						m_pRenderGraph->UpdateResource(&resourceUpdateDesc);
					}

					pCopyCommandList->CopyBuffer(pInstanceIndicesStagingBuffer, 0, m_pInstanceIndicesBuffer, 0, requiredInstanceIndicesBufferSize);
				}

				//Update Instance Buffer
				{
					uint64 requiredInstanceBufferSize = uint64(instanceCount) * sizeof(AccelerationStructureInstance);

					if (pInstanceStagingBuffer == nullptr || pInstanceStagingBuffer->GetDesc().SizeInBytes < requiredInstanceBufferSize)
					{
						if (pInstanceStagingBuffer != nullptr) deviceResourcesToRemove.PushBack(pInstanceStagingBuffer);

						BufferDesc instanceStagingBufferDesc = {};
						instanceStagingBufferDesc.DebugName		= "Instance Staging Buffer";
						instanceStagingBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
						instanceStagingBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_SRC;
						instanceStagingBufferDesc.SizeInBytes	= requiredInstanceBufferSize;

						pInstanceStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&instanceStagingBufferDesc);
						m_ppInstanceStagingBuffers[m_ModFrameIndex] = pInstanceStagingBuffer;
					}

					void* pMapped = pInstanceStagingBuffer->Map();
					memcpy(pMapped, m_Instances.GetData(), requiredInstanceBufferSize);
					pInstanceStagingBuffer->Unmap();

					if (m_pInstanceBuffer == nullptr || m_pInstanceBuffer->GetDesc().SizeInBytes < requiredInstanceBufferSize)
					{
						if (m_pInstanceBuffer != nullptr) deviceResourcesToRemove.PushBack(m_pInstanceBuffer);

						BufferDesc instanceBufferDesc = {};
						instanceBufferDesc.DebugName		= "Instance Buffer";
						instanceBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_GPU;
						instanceBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | FBufferFlag::BUFFER_FLAG_RAY_TRACING;
						instanceBufferDesc.SizeInBytes		= requiredInstanceBufferSize;

						m_pInstanceBuffer = RenderAPI::GetDevice()->CreateBuffer(&instanceBufferDesc);

						ResourceUpdateDesc resourceUpdateDesc = {};
						resourceUpdateDesc.ResourceName						= AS_INSTANCES_BUFFER;
						resourceUpdateDesc.ExternalBufferUpdate.Count		= 1;
						resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &m_pInstanceBuffer;

						m_pRenderGraph->UpdateResource(&resourceUpdateDesc);
					}

					pCopyCommandList->CopyBuffer(pInstanceStagingBuffer, 0, m_pInstanceBuffer, 0, requiredInstanceBufferSize);
				}

				static constexpr const PipelineMemoryBarrierDesc INSTANCE_BUFFER_MEMORY_BARRIER
				{
					.SrcMemoryAccessFlags = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE,
					.DstMemoryAccessFlags = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_READ,
				};

				pCopyCommandList->PipelineMemoryBarriers(
					FPipelineStageFlag::PIPELINE_STAGE_FLAG_COPY,
					FPipelineStageFlag::PIPELINE_STAGE_FLAG_ACCELERATION_STRUCTURE_BUILD,
					&INSTANCE_BUFFER_MEMORY_BARRIER,
					1);

				bool update = true;

				//Recreate TLAS completely if m_MaxSupportedTLASInstances < newInstanceCount
				if (m_MaxSupportedTLASInstances < instanceCount)
				{
					if (m_pTLAS != nullptr) deviceResourcesToRemove.PushBack(m_pTLAS);

					m_MaxSupportedTLASInstances = instanceCount;
					m_BuiltTLASInstanceCount = instanceCount;

					AccelerationStructureDesc createTLASDesc = {};
					createTLASDesc.DebugName		= "TLAS";
					createTLASDesc.Type				= EAccelerationStructureType::ACCELERATION_STRUCTURE_TYPE_TOP;
					createTLASDesc.Flags			= FAccelerationStructureFlag::ACCELERATION_STRUCTURE_FLAG_ALLOW_UPDATE;
					createTLASDesc.InstanceCount	= m_MaxSupportedTLASInstances;

					m_pTLAS = RenderAPI::GetDevice()->CreateAccelerationStructure(&createTLASDesc);

					update = false;

					ResourceUpdateDesc resourceUpdateDesc = {};
					resourceUpdateDesc.ResourceName							= SCENE_TLAS;
					resourceUpdateDesc.ExternalAccelerationStructure.pTLAS	= m_pTLAS;

					m_pRenderGraph->UpdateResource(&resourceUpdateDesc);
				}
				else if (m_BuiltTLASInstanceCount != instanceCount)
				{
					m_BuiltTLASInstanceCount = instanceCount;
					update = false;
				}

				BuildTopLevelAccelerationStructureDesc buildTLASDesc = {};
				buildTLASDesc.pAccelerationStructure	= m_pTLAS;
				buildTLASDesc.Flags						= FAccelerationStructureFlag::ACCELERATION_STRUCTURE_FLAG_ALLOW_UPDATE;
				buildTLASDesc.Update					= update;
				buildTLASDesc.pInstanceBuffer			= m_pInstanceBuffer;
				buildTLASDesc.InstanceCount				= instanceCount;

				pMainCommandList->BuildTopLevelAccelerationStructure(&buildTLASDesc);

				static constexpr const PipelineMemoryBarrierDesc TLAS_MEMORY_BARRIER
				{
					.SrcMemoryAccessFlags = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_ACCELERATION_STRUCTURE_WRITE,
					.DstMemoryAccessFlags = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_READ,
				};

				pMainCommandList->PipelineMemoryBarriers(
					FPipelineStageFlag::PIPELINE_STAGE_FLAG_ACCELERATION_STRUCTURE_BUILD,
					FPipelineStageFlag::PIPELINE_STAGE_FLAG_TOP,
					&TLAS_MEMORY_BARRIER,
					1);
			}

			pMainCommandList->End();
		}
	}

	void ASBuilder::Render(
		uint32 modFrameIndex, 
		uint32 backBufferIndex, 
		CommandList** ppFirstExecutionStage, 
		CommandList** ppSecondaryExecutionStage, 
		bool sleeping)
	{
		UNREFERENCED_VARIABLE(modFrameIndex);
		UNREFERENCED_VARIABLE(backBufferIndex);
		UNREFERENCED_VARIABLE(ppSecondaryExecutionStage);

		if (!sleeping)
		{
			CommandList* pCommandList = m_ppComputeCommandLists[m_ModFrameIndex];
			(*ppFirstExecutionStage) = pCommandList;
		}
	}

	AccelerationStructureInstance& ASBuilder::GetInstance(uint32 instanceIndex)
	{
		std::scoped_lock<SpinLock> lock(m_Lock);

		VALIDATE(instanceIndex < m_InstanceIndices.GetSize());

		//Get True (Indirect) Instance Index
		uint32 trueInstanceIndex = m_InstanceIndices[instanceIndex];

		VALIDATE(trueInstanceIndex < m_Instances.GetSize());

		return m_Instances[trueInstanceIndex];
	}

	void ASBuilder::ReleaseBackBufferBound()
	{
		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			TArray<DeviceChild*>& resourcesToRemove = m_pResourcesToRemove[b];

			for (DeviceChild* pResource : resourcesToRemove)
			{
				SAFERELEASE(pResource);
			}

			SAFERELEASE(m_ppComputeCommandAllocators[b]);
			SAFERELEASE(m_ppComputeCommandLists[b]);
			SAFERELEASE(m_ppInstanceIndicesStagingBuffers[b]);
			SAFERELEASE(m_ppInstanceStagingBuffers[b]);
		}

		SAFERELEASE(m_pInstanceIndicesBuffer);
		SAFERELEASE(m_pInstanceBuffer);

		SAFEDELETE_ARRAY(m_pResourcesToRemove);
		SAFEDELETE_ARRAY(m_ppComputeCommandAllocators);
		SAFEDELETE_ARRAY(m_ppComputeCommandLists);
		SAFEDELETE_ARRAY(m_ppInstanceIndicesStagingBuffers);
		SAFEDELETE_ARRAY(m_ppInstanceStagingBuffers);
	}

	bool ASBuilder::CreateCommandLists()
	{
		m_ppComputeCommandAllocators = DBG_NEW CommandAllocator*[m_BackBufferCount];
		m_ppComputeCommandLists = DBG_NEW CommandList*[m_BackBufferCount];

		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			m_ppComputeCommandAllocators[b] = RenderAPI::GetDevice()->CreateCommandAllocator("AS Builder Compute Command Allocator " + std::to_string(b), ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE);

			if (!m_ppComputeCommandAllocators[b])
			{
				return false;
			}

			CommandListDesc commandListDesc = {};
			commandListDesc.DebugName			= "AS Builder Compute Command List " + std::to_string(b);
			commandListDesc.CommandListType		= ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
			commandListDesc.Flags				= FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

			m_ppComputeCommandLists[b] = RenderAPI::GetDevice()->CreateCommandList(m_ppComputeCommandAllocators[b], &commandListDesc);

			if (!m_ppComputeCommandLists[b])
			{
				return false;
			}
		}

		return true;
	}

	bool ASBuilder::CreateBuffers()
	{
		m_ppInstanceIndicesStagingBuffers = DBG_NEW Buffer*[m_BackBufferCount];
		m_ppInstanceStagingBuffers = DBG_NEW Buffer*[m_BackBufferCount];

		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			m_ppInstanceIndicesStagingBuffers[b] = nullptr;
			m_ppInstanceStagingBuffers[b] = nullptr;
		}

		return true;
	}

	bool ASBuilder::CreateDummyBuffers()
	{
		SAFERELEASE(m_pInstanceIndicesBuffer);
		SAFERELEASE(m_pInstanceBuffer);

		// Create Instance Index Dummy Buffer
		{
			BufferDesc instanceBufferDesc = {};
			instanceBufferDesc.DebugName		= "Instance Indices Dummy Buffer";
			instanceBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_GPU;
			instanceBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER;
			instanceBufferDesc.SizeInBytes		= 1;

			m_pInstanceIndicesBuffer = RenderAPI::GetDevice()->CreateBuffer(&instanceBufferDesc);

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName						= AS_INSTANCE_INDICES_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.Count		= 1;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &m_pInstanceIndicesBuffer;

			m_pRenderGraph->UpdateResource(&resourceUpdateDesc);
		}

		// Create Instance Dummy Buffer
		{
			BufferDesc instanceIndicesBufferDesc = {};
			instanceIndicesBufferDesc.DebugName		= "Instance Dummy Buffer";
			instanceIndicesBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
			instanceIndicesBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | FBufferFlag::BUFFER_FLAG_RAY_TRACING;
			instanceIndicesBufferDesc.SizeInBytes	= 1;

			m_pInstanceBuffer = RenderAPI::GetDevice()->CreateBuffer(&instanceIndicesBufferDesc);

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName						= AS_INSTANCES_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.Count		= 1;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &m_pInstanceBuffer;

			m_pRenderGraph->UpdateResource(&resourceUpdateDesc);
		}

		return true;
	}
}