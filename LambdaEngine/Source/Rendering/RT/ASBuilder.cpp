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

			CreateCommandLists();
			CreateBuffers();

			m_pResourcesToRemove = DBG_NEW TArray<DeviceChild*>[m_BackBufferCount];
		}

		return true;
	}

	void ASBuilder::BuildTriBLAS(uint32& index, Buffer* pVertexBuffer, Buffer* pIndexBuffer, uint32 vertexCount, uint32 indexCount, bool allowUpdate)
	{
		std::scoped_lock<SpinLock> lock(m_Lock);

		if (index == BLAS_UNINITIALIZED_INDEX)
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

				if (!m_FreeSBTIndices.IsEmpty())
				{
					blasData.SBTRecordOffset = m_FreeSBTIndices.GetBack();
					m_FreeSBTIndices.PopBack();

					pSBTRecord = &m_SBTRecords[blasData.SBTRecordOffset];
				}
				else
				{
					blasData.SBTRecordOffset = m_SBTRecords.GetSize();
					pSBTRecord = &m_SBTRecords.PushBack({});
				}

				pSBTRecord->VertexBufferAddress	= pVertexBuffer->GetDeviceAddress();
				pSBTRecord->IndexBufferAddress	= pIndexBuffer->GetDeviceAddress();
				m_SBTRecordsDirty = true;

				if (!m_FreeBLASIndices.IsEmpty())
				{
					index = m_FreeBLASIndices.GetBack();
					m_FreeBLASIndices.PopBack();
					m_BLASes[index] = blasData;
				}
				else
				{
					index = m_BLASes.GetSize();
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
				blasBuildDesc.VertexStride				= sizeof(Vertex);
				blasBuildDesc.pIndexBuffer				= pIndexBuffer;
				blasBuildDesc.IndexBufferByteOffset		= 0;
				blasBuildDesc.TriangleCount				= indexCount / 3;
				blasBuildDesc.pTransformBuffer			= nullptr;
				blasBuildDesc.TransformByteOffset		= 0;
				blasBuildDesc.Update					= false;

				m_DirtyBLASes.PushBack(blasBuildDesc);
			}
		}
		else if (index < m_BLASes.GetSize())
		{
			BLASData& blasData = m_BLASes[index];

			//Create BLAS build Desc
			{
				//We assume that vertexCount/indexCount does not change and thus do not check if we need to recreate the BLAS
				BuildBottomLevelAccelerationStructureDesc blasBuildDesc = {};
				blasBuildDesc.pAccelerationStructure	= blasData.pBLAS;
				blasBuildDesc.Flags						= FAccelerationStructureFlag::ACCELERATION_STRUCTURE_FLAG_ALLOW_UPDATE;
				blasBuildDesc.pVertexBuffer				= pVertexBuffer;
				blasBuildDesc.FirstVertexIndex			= 0;
				blasBuildDesc.VertexStride				= sizeof(Vertex);
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

				uint64 vertexBufferDeviceAddress	= pVertexBuffer->GetDeviceAddress();
				uint64 indexBufferDeviceAddress		= pIndexBuffer->GetDeviceAddress();

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
			LOG_ERROR("[ASBuilder]: BuildTriBLAS called with index %u, but that BLAS doesn't exist...", index);
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
		std::remove_if(m_DirtyBLASes.Begin(), m_DirtyBLASes.End(), 
			[blasData](const BuildBottomLevelAccelerationStructureDesc& blasBuildDesc)
			{ 
				return blasBuildDesc.pAccelerationStructure == blasData.pBLAS;
			});

		//Push BLAS Free Index
		m_FreeBLASIndices.PushBack(index);

		//Reset BLASData
		blasData.pBLAS = nullptr;
		blasData.SBTRecordOffset = UINT32_MAX;
	}

	uint32 ASBuilder::AddInstance(uint32 blasIndex, const glm::mat4& transform, uint32 customIndex, uint8 hitMask, FAccelerationStructureFlags flags)
	{
		std::scoped_lock<SpinLock> lock(m_Lock);

		VALIDATE(blasIndex < m_BLASes.GetSize());
		
		BLASData& blasData = m_BLASes[blasIndex];

		uint32 instanceIndex = m_Instances.GetSize();

		AccelerationStructureInstance asInstance = {};
		asInstance.Transform					= glm::transpose(transform);
		asInstance.CustomIndex					= customIndex;
		asInstance.Mask							= hitMask;
		asInstance.SBTRecordOffset				= blasData.SBTRecordOffset;
		asInstance.Flags						= flags;
		asInstance.AccelerationStructureAddress	= blasData.pBLAS->GetDeviceAddress();

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

		//Push the Removed Instance Index to Free
		m_FreeInstanceIndices.PushBack(instanceIndex);
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
			//We assume the TLAS is always dirty so it's okay to always begin the Command List
			m_ppComputeCommandAllocators[m_ModFrameIndex]->Reset();
			CommandList* pCommandList = m_ppComputeCommandLists[m_ModFrameIndex];
			pCommandList->Begin(nullptr);

			if (m_SBTRecordsDirty)
			{
				m_pRenderGraph->UpdateGlobalSBT(pCommandList, m_SBTRecords, deviceResourcesToRemove);
				m_SBTRecordsDirty = false;
			}

			//Build BLASes
			{
				for (BuildBottomLevelAccelerationStructureDesc& blasBuildDesc : m_DirtyBLASes)
				{
					pCommandList->BuildBottomLevelAccelerationStructure(&blasBuildDesc);
				}

				//This is required to sync up BLAS building with TLAS building, to make sure that the BLAS is built before the TLAS
				static constexpr const PipelineMemoryBarrierDesc BLAS_MEMORY_BARRIER
				{
					.SrcMemoryAccessFlags = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_ACCELERATION_STRUCTURE_WRITE,
					.DstMemoryAccessFlags = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_ACCELERATION_STRUCTURE_READ,
				};

				pCommandList->PipelineMemoryBarriers(
					FPipelineStageFlag::PIPELINE_STAGE_FLAG_ACCELERATION_STRUCTURE_BUILD, 
					FPipelineStageFlag::PIPELINE_STAGE_FLAG_ACCELERATION_STRUCTURE_BUILD, 
					&BLAS_MEMORY_BARRIER,
					1);

				m_DirtyBLASes.Clear();
			}

			//Update TLAS, assume always dirty
			if (!m_Instances.IsEmpty())
			{
				Buffer* pInstanceBuffer = m_ppInstanceBuffers[m_ModFrameIndex];
				uint32 instanceCount = m_Instances.GetSize();
				uint64 requiredInstanceBufferSize = uint64(instanceCount) * sizeof(AccelerationStructureInstance);

				if (pInstanceBuffer == nullptr || pInstanceBuffer->GetDesc().SizeInBytes < requiredInstanceBufferSize)
				{
					if (pInstanceBuffer != nullptr) deviceResourcesToRemove.PushBack(pInstanceBuffer);

					BufferDesc instanceBufferDesc = {};
					instanceBufferDesc.DebugName		= "Instance Buffer";
					instanceBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
					instanceBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_RAY_TRACING;
					instanceBufferDesc.SizeInBytes		= requiredInstanceBufferSize;

					pInstanceBuffer = RenderAPI::GetDevice()->CreateBuffer(&instanceBufferDesc);
					m_ppInstanceBuffers[m_ModFrameIndex] = pInstanceBuffer;
				}

				void* pMapped = pInstanceBuffer->Map();
				memcpy(pMapped, m_Instances.GetData(), requiredInstanceBufferSize);
				pInstanceBuffer->Unmap();

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
				buildTLASDesc.pInstanceBuffer			= pInstanceBuffer;
				buildTLASDesc.InstanceCount				= instanceCount;

				pCommandList->BuildTopLevelAccelerationStructure(&buildTLASDesc);

				static constexpr const PipelineMemoryBarrierDesc TLAS_MEMORY_BARRIER
				{
					.SrcMemoryAccessFlags = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_ACCELERATION_STRUCTURE_WRITE,
					.DstMemoryAccessFlags = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_READ,
				};

				pCommandList->PipelineMemoryBarriers(
					FPipelineStageFlag::PIPELINE_STAGE_FLAG_ACCELERATION_STRUCTURE_BUILD,
					FPipelineStageFlag::PIPELINE_STAGE_FLAG_TOP,
					&TLAS_MEMORY_BARRIER,
					1);
			}

			pCommandList->End();
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
			SAFERELEASE(m_ppInstanceBuffers[b]);
		}

		SAFEDELETE_ARRAY(m_pResourcesToRemove);
		SAFEDELETE_ARRAY(m_ppComputeCommandAllocators);
		SAFEDELETE_ARRAY(m_ppComputeCommandLists);
		SAFEDELETE_ARRAY(m_ppInstanceBuffers);
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
		m_ppInstanceBuffers = DBG_NEW Buffer*[m_BackBufferCount];

		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			m_ppInstanceBuffers[b] = nullptr;
		}

		return true;
	}
}