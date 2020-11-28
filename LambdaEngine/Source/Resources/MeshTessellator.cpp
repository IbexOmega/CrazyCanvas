#include "Resources/MeshTessellator.h"

#include "Rendering/RenderAPI.h"
#include "Rendering/Core/API/Shader.h"
#include "Rendering/Core/API/DescriptorSet.h"
#include "Rendering/Core/API/DescriptorHeap.h"
#include "Rendering/Core/API/PipelineLayout.h"
#include "Rendering/Core/API/PipelineState.h"
#include "Rendering/Core/API/Fence.h"
#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/CommandQueue.h"
#include "Rendering/PipelineStateManager.h"

#include "Resources/ResourceManager.h"

namespace LambdaEngine
{
	MeshTessellator MeshTessellator::s_Instance;

	void MeshTessellator::Init()
	{
		// Create Command List
		{
			m_pCommandAllocator = RenderAPI::GetDevice()->CreateCommandAllocator("Tessellator Command Allocator", ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE);

			CommandListDesc commandListDesc = { };
			commandListDesc.DebugName = "Tessellator Compute Command List";
			commandListDesc.CommandListType = ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
			commandListDesc.Flags = FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;
			m_pCommandList = RenderAPI::GetDevice()->CreateCommandList(m_pCommandAllocator, &commandListDesc);
		}

		//Create Fence
		{
			FenceDesc fenceDesc = {};
			fenceDesc.DebugName = "Tessellator Fence";
			fenceDesc.InitalValue = 0;
			m_pFence = RenderAPI::GetDevice()->CreateFence(&fenceDesc);
		}

		// Create Descriptor Heap
		{
			DescriptorHeapInfo descriptorCountDesc = { };
			descriptorCountDesc.UnorderedAccessBufferDescriptorCount = 2;

			DescriptorHeapDesc descriptorHeapDesc = { };
			descriptorHeapDesc.DebugName = "Tessellator Descriptor Heap";
			descriptorHeapDesc.DescriptorSetCount = 2;
			descriptorHeapDesc.DescriptorCount = descriptorCountDesc;

			m_pDescriptorHeap = RenderAPI::GetDevice()->CreateDescriptorHeap(&descriptorHeapDesc);
		}

		// Create Pipeline Layout
		{
			DescriptorBindingDesc inVerticesBinding = { };
			inVerticesBinding.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
			inVerticesBinding.DescriptorCount = 1;
			inVerticesBinding.Binding = 0;
			inVerticesBinding.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

			DescriptorBindingDesc inIndicesBinding = { };
			inIndicesBinding.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
			inIndicesBinding.DescriptorCount = 1;
			inIndicesBinding.Binding = 1;
			inIndicesBinding.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

			DescriptorSetLayoutDesc inDescriptorSetLayoutDesc = { };
			inDescriptorSetLayoutDesc.DescriptorBindings =
			{
				inVerticesBinding,
				inIndicesBinding
			};

			DescriptorBindingDesc outVerticesBinding = { };
			outVerticesBinding.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
			outVerticesBinding.DescriptorCount = 1;
			outVerticesBinding.Binding = 0;
			outVerticesBinding.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

			DescriptorBindingDesc outIndicesBinding = { };
			outIndicesBinding.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
			outIndicesBinding.DescriptorCount = 1;
			outIndicesBinding.Binding = 1;
			outIndicesBinding.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

			DescriptorSetLayoutDesc outDescriptorSetLayoutDesc = { };
			outDescriptorSetLayoutDesc.DescriptorBindings =
			{
				outVerticesBinding,
				outIndicesBinding
			};

			ConstantRangeDesc constantRangeDesc = {};
			constantRangeDesc.OffsetInBytes = 0;
			constantRangeDesc.ShaderStageFlags = FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;
			constantRangeDesc.SizeInBytes = sizeof(SPushConstantData);

			PipelineLayoutDesc pPipelineLayoutDesc = { };
			pPipelineLayoutDesc.DebugName = "Tessellator Pipeline Layout";
			pPipelineLayoutDesc.DescriptorSetLayouts = { inDescriptorSetLayoutDesc, outDescriptorSetLayoutDesc };
			pPipelineLayoutDesc.ConstantRanges = { constantRangeDesc };

			m_pPipelineLayout = RenderAPI::GetDevice()->CreatePipelineLayout(&pPipelineLayoutDesc);

			m_pInDescriptorSet = RenderAPI::GetDevice()->CreateDescriptorSet("Tessellator In Descriptor Set", m_pPipelineLayout, 0, m_pDescriptorHeap);
			m_pOutDescriptorSet = RenderAPI::GetDevice()->CreateDescriptorSet("Tessellator Out Descriptor Set", m_pPipelineLayout, 1, m_pDescriptorHeap);
		}

		// Create Pipeline State
		{
			m_ShaderGUID = ResourceManager::LoadShaderFromFile("Tessellation/Tessellator.comp", FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER, EShaderLang::SHADER_LANG_GLSL, "main");

			ComputePipelineStateDesc computePipelineStateDesc = { };
			computePipelineStateDesc.DebugName = "Tessellator Pipeline State";
			computePipelineStateDesc.pPipelineLayout = m_pPipelineLayout;
			computePipelineStateDesc.Shader = { .pShader = ResourceManager::GetShader(m_ShaderGUID) };

			m_pPipelineState = RenderAPI::GetDevice()->CreateComputePipelineState(&computePipelineStateDesc);
		}
	}

	void MeshTessellator::Release()
	{
		m_pPipelineState->Release();
		m_pInDescriptorSet->Release();
		m_pOutDescriptorSet->Release();
		m_pDescriptorHeap->Release();
		m_pPipelineLayout->Release();
		m_pCommandList->Release();
		m_pCommandAllocator->Release();
	}

	void MeshTessellator::Tessellate(Mesh* pMesh)
	{
		LOG_WARNING("Tesselate...");

		/* TODO:
		*	1. [x] Make input buffers for the original vertices and indices. 
		*	1. [ ] Make result buffers for the new vertices and indices, have them be cleard and set to MAX_UINT32 before calling dispatch.
		*	2. [x] Write to the descriptor sets!
		*	3. [x] Call dispatch
		*	4. [ ] Compact buffers (Remove all elements which did not have any change and merge vertices by distance)
		*	5. [ ] Copy data from the new compacted buffers to pMesh
		**/

		SPushConstantData pushConstantData = {};
		pushConstantData.triangleCount = pMesh->Indices.GetSize() / 3;
		pushConstantData.maxTesselationLevels = 2;
		pushConstantData.outerBorder = 3; // Not counting the corners! One outer edge on the triangle will be, o-x-x-x-o, if the outerBorder is 3.

		// TODO: Change this to be the correct size, if assuming all triangle will be fully tessellated!
		uint64 newVerticesMaxSize = pMesh->Vertices.GetSize() * sizeof(Vertex);
		uint64 newIndicesMaxSize = pMesh->Indices.GetSize() * sizeof(MeshIndexType);

		LOG_WARNING("Create buffers...");
		static uint64 signalValue = 0;
		{
			m_pCommandAllocator->Reset();
			m_pCommandList->Begin(nullptr);

			// ------- Create buffers if needed and copy data to them -------

			// In Buffers
			void* data = pMesh->Vertices.GetData();
			uint64 size = pMesh->Vertices.GetSize() * sizeof(Vertex);
			CreateAndCopyInBuffer(m_pCommandList, &m_pInVertexBuffer, &m_pInVertexStagingBuffer, pMesh->Vertices.GetData(), size, "Tessellator In Vertex Buffer");

			data = pMesh->Indices.GetData();
			size = pMesh->Indices.GetSize() * sizeof(MeshIndexType);
			CreateAndCopyInBuffer(m_pCommandList, &m_pInIndicesBuffer, &m_pInIndicesStagingBuffer, pMesh->Indices.GetData(), size, "Tessellator In Index Buffer");

			// Out Buffers
			CreateAndClearOutBuffer(m_pCommandList, &m_pOutVertexBuffer, &m_pOutVertexFirstStagingBuffer, &m_pOutVertexSecondStagingBuffer, newVerticesMaxSize, UINT32_MAX, "Tessellator Out Vertex Buffer");
			CreateAndClearOutBuffer(m_pCommandList, &m_pOutIndicesBuffer, &m_pOutIndicesFirstStagingBuffer, &m_pOutIndicesSecondStagingBuffer, newIndicesMaxSize, UINT32_MAX, "Tessellator Out Index Buffer");

			m_pCommandList->End();

			signalValue++;
			RenderAPI::GetComputeQueue()->ExecuteCommandLists(
				&m_pCommandList, 1,
				FPipelineStageFlag::PIPELINE_STAGE_FLAG_UNKNOWN,
				nullptr, 0,
				m_pFence, signalValue);
		}

		LOG_WARNING("Done");

		LOG_WARNING("Write descriptors...");
		// Write to descriptor sets
		{
			// In descriptor set
			{
				Buffer* buffers[2] = { m_pInVertexBuffer, m_pInIndicesBuffer };
				uint64 offsets[2] = { 0, 0 };
				uint64 sizes[2] = { m_pInVertexBuffer->GetDesc().SizeInBytes, m_pInIndicesBuffer->GetDesc().SizeInBytes };
				m_pInDescriptorSet->WriteBufferDescriptors(buffers, offsets, sizes, 0, 2, EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER);
			}
			
			// Out descriptor set
			{
				Buffer* buffers[2] = { m_pOutVertexBuffer, m_pOutIndicesBuffer };
				uint64 offsets[2] = { 0, 0 };
				uint64 sizes[2] = { m_pOutVertexBuffer->GetDesc().SizeInBytes, m_pOutIndicesBuffer->GetDesc().SizeInBytes };
				m_pOutDescriptorSet->WriteBufferDescriptors(buffers, offsets, sizes, 0, 2, EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER);
			}
		}
		LOG_WARNING("Done");

		LOG_WARNING("Dispatch...");
		{
			m_pCommandAllocator->Reset();
			m_pCommandList->Begin(nullptr);

			// ------- Compute pipeline -------
			m_pCommandList->BindDescriptorSetCompute(m_pInDescriptorSet, m_pPipelineLayout, 0);
			m_pCommandList->BindDescriptorSetCompute(m_pOutDescriptorSet, m_pPipelineLayout, 1);

			m_pCommandList->BindComputePipeline(m_pPipelineState);
			m_pCommandList->SetConstantRange(m_pPipelineLayout, FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER, (void*)&pushConstantData, sizeof(pushConstantData), 0);

			// Dispatch
			constexpr uint32 WORK_GROUP_INVOCATIONS = 32;
			uint32 workGroupX = uint32(std::ceilf(float(pushConstantData.triangleCount) / float(WORK_GROUP_INVOCATIONS)));
			m_pCommandList->Dispatch(workGroupX, 1U, 1U);

			m_pCommandList->End();

			signalValue++;
			RenderAPI::GetComputeQueue()->ExecuteCommandLists(
				&m_pCommandList, 1,
				FPipelineStageFlag::PIPELINE_STAGE_FLAG_TOP,
				m_pFence, signalValue-1,
				m_pFence, signalValue);

			m_pFence->Wait(signalValue, UINT64_MAX);
		}
		LOG_WARNING("Done");

		// Copy data to CPU

		LOG_WARNING("Copy over new data...");
		// TODO: Compact this! Check for elements which are UINT32_MAX, they should be removed!
		void* pMapped = m_pOutIndicesSecondStagingBuffer->Map();
		pMesh->Indices.Resize(newIndicesMaxSize);
		memcpy(pMapped, pMesh->Indices.GetData(), newIndicesMaxSize);
		m_pOutIndicesSecondStagingBuffer->Unmap();

		// TODO: Compact this and merge by distance! If merging, change the index too!
		pMapped = m_pOutVertexSecondStagingBuffer->Map();
		pMesh->Vertices.Resize(newVerticesMaxSize);
		memcpy(pMapped, pMesh->Vertices.GetData(), newVerticesMaxSize);
		m_pOutVertexSecondStagingBuffer->Unmap();
		LOG_WARNING("Done");
		LOG_WARNING("Tessellation Complete");
	}

	void MeshTessellator::CreateAndCopyInBuffer(CommandList* pCommandList, Buffer** inBuffer, Buffer** inStagingBuffer, void* data, uint64 size, const String& name)
	{
		if ((*inStagingBuffer) == nullptr || (*inStagingBuffer)->GetDesc().SizeInBytes < size)
		{
			if ((*inStagingBuffer) != nullptr)
				SAFERELEASE((*inStagingBuffer));

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName = name + " Staging Buffer";
			bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
			bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_SRC;
			bufferDesc.SizeInBytes = size;

			(*inStagingBuffer) = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
		}

		void* pMapped = (*inStagingBuffer)->Map();
		memcpy(pMapped, data, size);
		(*inStagingBuffer)->Unmap();

		if ((*inBuffer) == nullptr || (*inBuffer)->GetDesc().SizeInBytes < size)
		{
			if ((*inBuffer) != nullptr)
				SAFERELEASE((*inBuffer));

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName = name;
			bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_GPU;
			bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_DST;
			bufferDesc.SizeInBytes = size;

			(*inBuffer) = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
		}

		pCommandList->CopyBuffer((*inStagingBuffer), 0, (*inBuffer), 0, size);
	}

	void MeshTessellator::CreateAndClearOutBuffer(CommandList* pCommandList, Buffer** outBuffer, Buffer** outFirstStagingBuffer, Buffer** outSecondStagingBuffer, uint64 size, uint32 clearData, const String& name)
	{
		if ((*outFirstStagingBuffer) == nullptr || (*outFirstStagingBuffer)->GetDesc().SizeInBytes < size)
		{
			if ((*outFirstStagingBuffer) != nullptr)
				SAFERELEASE((*outFirstStagingBuffer));

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName = name + " First Staging Buffer";
			bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
			bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_SRC;
			bufferDesc.SizeInBytes = size;

			(*outFirstStagingBuffer) = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
		}

		void* pMapped = (*outFirstStagingBuffer)->Map();
		memset(pMapped, clearData, size);
		(*outFirstStagingBuffer)->Unmap();

		if ((*outBuffer) == nullptr || (*outBuffer)->GetDesc().SizeInBytes < size)
		{
			if ((*outBuffer) != nullptr)
				SAFERELEASE((*outBuffer));

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName = name;
			bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_GPU;
			bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_SRC | FBufferFlag::BUFFER_FLAG_COPY_DST;
			bufferDesc.SizeInBytes = size;

			(*outBuffer) = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
		}

		pCommandList->CopyBuffer((*outFirstStagingBuffer), 0, (*outBuffer), 0, size);

		if ((*outSecondStagingBuffer) == nullptr || (*outSecondStagingBuffer)->GetDesc().SizeInBytes < size)
		{
			if ((*outSecondStagingBuffer) != nullptr)
				SAFERELEASE((*outSecondStagingBuffer));

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName = name + " Second Staging Buffer";
			bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
			bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_DST;
			bufferDesc.SizeInBytes = size;

			(*outSecondStagingBuffer) = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
		}
	}
}
