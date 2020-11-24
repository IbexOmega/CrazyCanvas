#include "RenderStages/HealthCompute.h"

#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/DescriptorHeap.h"
#include "Rendering/Core/API/DescriptorSet.h"
#include "Rendering/Core/API/PipelineState.h"
#include "Rendering/Core/API/Buffer.h"
#include "Rendering/RenderAPI.h"
#include "Rendering/RenderGraphTypes.h"
#include "Rendering/RenderGraph.h"
#include "Rendering/Core/API/Fence.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "Resources/ResourceManager.h"

HealthCompute::HealthCompute()
{

}

HealthCompute::~HealthCompute()
{
	if (m_Initilized)
	{
		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			SAFERELEASE(m_ppComputeCommandLists[b]);
			SAFERELEASE(m_ppComputeCommandAllocators[b]);
		}

		SAFEDELETE_ARRAY(m_ppComputeCommandLists);
		SAFEDELETE_ARRAY(m_ppComputeCommandAllocators);

		SAFERELEASE(m_pHealthBuffer);
		SAFERELEASE(m_pCopyBuffer);
		SAFERELEASE(m_pVertexCountBuffer);
	}
}

bool HealthCompute::Init()
{
	using namespace LambdaEngine;

	GUID_Lambda player = 0;
	TArray<GUID_Lambda> temp;
	ResourceManager::LoadMeshFromFile("Player/IdleRightUV.glb", player, temp);
	Mesh* pMesh = ResourceManager::GetMesh(player);
	m_VertexCount = pMesh->Vertices.GetSize();

	if (!CreatePipelineLayout())
	{
		LOG_ERROR("[HealthCompute]: Failed to create PipelineLayout");
		return false;
	}

	if (!CreateDescriptorSets())
	{
		LOG_ERROR("[HealthCompute]: Failed to create DescriptorSet");
		return false;
	}

	if (!CreateShaders())
	{
		LOG_ERROR("[HealthCompute]: Failed to create Shaders");
		return false;
	}

	return true;
}

bool HealthCompute::RenderGraphInit(const LambdaEngine::CustomRendererRenderGraphInitDesc* pPreInitDesc)
{
	VALIDATE(pPreInitDesc);

	m_BackBufferCount = pPreInitDesc->BackBufferCount;

	if (!m_Initilized)
	{
		if (!CreateCommandLists())
		{
			LOG_ERROR("[HealthCompute]: Failed to create render command lists");
			return false;
		}

		if (!m_PipelineContext.Init("Health Compute Pipeline Context"))
		{
			LOG_ERROR("[HealthCompute]: Failed to init Pipeline Context");
			return false;
		}

		if (!CreateResources())
		{
			LOG_ERROR("[HealthCompute]: Failed to create resources!");
			return false;
		}

		m_Initilized = true;
	}

	return true;
}

void HealthCompute::Update(
	LambdaEngine::Timestamp delta,
	uint32 modFrameIndex,
	uint32 backBufferIndex)
{
	m_PipelineContext.Update(delta, modFrameIndex, backBufferIndex);
}

void HealthCompute::UpdateBufferResource(
	const LambdaEngine::String& resourceName,
	const LambdaEngine::Buffer* const* ppBuffers,
	uint64* pOffsets,
	uint64* pSizesInBytes,
	uint32 count,
	bool backBufferBound)
{
	using namespace LambdaEngine;

	UNREFERENCED_VARIABLE(backBufferBound);

	if (resourceName == "PLAYER_HEALTH_BUFFER")
	{
		constexpr uint32 setIndex = 0U;
		constexpr uint32 setBinding = 0U;

		SDescriptorBufferUpdateDesc descriptorUpdateDesc = {};
		descriptorUpdateDesc.ppBuffers = ppBuffers;
		descriptorUpdateDesc.pOffsets = pOffsets;
		descriptorUpdateDesc.pSizes = pSizesInBytes;
		descriptorUpdateDesc.FirstBinding = setBinding;
		descriptorUpdateDesc.DescriptorCount = count;
		descriptorUpdateDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;

		m_PipelineContext.UpdateDescriptorSet("[HealthCompute] Health Compute Descriptor Set 0 Binding 0", setIndex, m_DescriptorHeap.Get(), descriptorUpdateDesc);
	}
	else if (resourceName == "VERTEX_COUNT_BUFFER")
	{
		constexpr uint32 setIndex = 0U;
		constexpr uint32 setBinding = 1U;

		SDescriptorBufferUpdateDesc descriptorUpdateDesc = {};
		descriptorUpdateDesc.ppBuffers = ppBuffers;
		descriptorUpdateDesc.pOffsets = pOffsets;
		descriptorUpdateDesc.pSizes = pSizesInBytes;
		descriptorUpdateDesc.FirstBinding = setBinding;
		descriptorUpdateDesc.DescriptorCount = count;
		descriptorUpdateDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER;

		m_PipelineContext.UpdateDescriptorSet("[HealthCompute] Health Compute Descriptor Set 0 Binding 1", setIndex, m_DescriptorHeap.Get(), descriptorUpdateDesc);
	}
}

void HealthCompute::UpdateDrawArgsResource(
	const LambdaEngine::String& resourceName,
	const LambdaEngine::DrawArg* pDrawArgs,
	uint32 count)
{
	using namespace LambdaEngine;

	if (resourceName == SCENE_DRAW_ARGS)
	{
		if (count > 0U && pDrawArgs != nullptr)
		{
			constexpr uint32 setIndex = 1U;
			constexpr uint32 setBinding = 0U;

			const uint64 size = pDrawArgs->pVertexBuffer->GetDesc().SizeInBytes;
			SDescriptorBufferUpdateDesc descriptorUpdateDesc = {};
			descriptorUpdateDesc.ppBuffers = &pDrawArgs->pVertexBuffer;
			descriptorUpdateDesc.pOffsets = 0;
			descriptorUpdateDesc.pSizes = &size;
			descriptorUpdateDesc.FirstBinding = setBinding;
			descriptorUpdateDesc.DescriptorCount = count;
			descriptorUpdateDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;

			m_PipelineContext.UpdateDescriptorSet("[HealthCompute] Health Compute Descriptor Set 1 Binding 0", setIndex, m_DescriptorHeap.Get(), descriptorUpdateDesc);
		}
	}
}

void HealthCompute::Render(
	uint32 modFrameIndex,
	uint32 backBufferIndex,
	LambdaEngine::CommandList** ppFirstExecutionStage,
	LambdaEngine::CommandList** ppSecondaryExecutionStage,
	bool sleeping)
{
	using namespace LambdaEngine;

	if (sleeping)
		return;

	CommandList* pCommandList = m_ppComputeCommandLists[modFrameIndex];
	m_ppComputeCommandAllocators[modFrameIndex]->Reset();
	pCommandList->Begin(nullptr);

	m_PipelineContext.Bind(pCommandList);
	// m_PipelineContext.BindConstantRange();

	constexpr uint32 WORK_GROUP_INVOCATIONS = 32;
	uint32 workGroup = uint32(std::ceilf(float(m_VertexCount) / float(WORK_GROUP_INVOCATIONS)));
	pCommandList->Dispatch(workGroup, 1U, 1U);

	pCommandList->End();
	(*ppFirstExecutionStage) = pCommandList;
}

bool HealthCompute::CreatePipelineLayout()
{
	using namespace LambdaEngine;

	// Set 0 (BUFFER_SET_INDEX)
	DescriptorBindingDesc healthBindingDesc	= {};
	healthBindingDesc.DescriptorType		= EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
	healthBindingDesc.DescriptorCount		= 1;
	healthBindingDesc.Binding				= 0;
	healthBindingDesc.ShaderStageMask		= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

	DescriptorBindingDesc vertexCountDesc	= {};
	vertexCountDesc.DescriptorType			= EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER;
	vertexCountDesc.DescriptorCount			= 1;
	vertexCountDesc.Binding					= 1;
	vertexCountDesc.ShaderStageMask			= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

	// Set 1 (DRAW_SET_INDEX)
	DescriptorBindingDesc verticiesBindingDesc = {};
	verticiesBindingDesc.DescriptorType		= EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
	verticiesBindingDesc.DescriptorCount	= 1;
	verticiesBindingDesc.Binding			= 0;
	verticiesBindingDesc.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

	TArray<DescriptorBindingDesc> set0 = {
		healthBindingDesc,
		vertexCountDesc
	};

	TArray<DescriptorBindingDesc> set1 = {
		verticiesBindingDesc
	};

	m_PipelineContext.CreateDescriptorSetLayout(set0);
	m_PipelineContext.CreateDescriptorSetLayout(set1);

	// ConstantRangeDesc constantRange	= {};
	// constantRange.ShaderStageFlags	= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;
	// constantRange.SizeInBytes		= sizeof(float) * 2;
	// constantRange.OffsetInBytes		= 0;

	// m_PipelineContext.CreateConstantRange(constantRange);

	return true;
}

bool HealthCompute::CreateDescriptorSets()
{
	using namespace LambdaEngine;

	// TODO: UPDATE THIS TO MATCH THE DESIRED AMOUNT
	DescriptorHeapInfo descriptorCountDesc = { };
	descriptorCountDesc.SamplerDescriptorCount = 0;
	descriptorCountDesc.TextureDescriptorCount = 0;
	descriptorCountDesc.TextureCombinedSamplerDescriptorCount = 0;
	descriptorCountDesc.ConstantBufferDescriptorCount = 1;
	descriptorCountDesc.UnorderedAccessBufferDescriptorCount = 2;
	descriptorCountDesc.UnorderedAccessTextureDescriptorCount = 0;
	descriptorCountDesc.AccelerationStructureDescriptorCount = 0;

	DescriptorHeapDesc descriptorHeapDesc = { };
	descriptorHeapDesc.DebugName = "Health Compute Descriptor Heap";
	descriptorHeapDesc.DescriptorSetCount = 10;
	descriptorHeapDesc.DescriptorCount = descriptorCountDesc;

	m_DescriptorHeap = RenderAPI::GetDevice()->CreateDescriptorHeap(&descriptorHeapDesc);
	if (!m_DescriptorHeap)
	{
		return false;
	}

	return true;
}

bool HealthCompute::CreateShaders()
{
	using namespace LambdaEngine;

	bool success = true;

	String computeShaderFileName = "ComputeHealth.comp";
	GUID_Lambda computeShaderGUID = ResourceManager::LoadShaderFromFile(computeShaderFileName, FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER, EShaderLang::SHADER_LANG_GLSL);
	success &= computeShaderGUID != GUID_NONE;

	m_PipelineContext.SetComputeShader(computeShaderGUID);

	return success;
}

bool HealthCompute::CreateCommandLists()
{
	using namespace LambdaEngine;

	m_ppComputeCommandAllocators = DBG_NEW CommandAllocator * [m_BackBufferCount];
	m_ppComputeCommandLists = DBG_NEW CommandList * [m_BackBufferCount];

	for (uint32 b = 0; b < m_BackBufferCount; b++)
	{
		m_ppComputeCommandAllocators[b] = RenderAPI::GetDevice()->CreateCommandAllocator("Health Compute Command Allocator " + std::to_string(b), ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE);

		if (!m_ppComputeCommandAllocators[b])
		{
			return false;
		}

		CommandListDesc commandListDesc = {};
		commandListDesc.DebugName = "Health Compute Command List " + std::to_string(b);
		commandListDesc.CommandListType = ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
		commandListDesc.Flags = FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

		m_ppComputeCommandLists[b] = RenderAPI::GetDevice()->CreateCommandList(m_ppComputeCommandAllocators[b], &commandListDesc);

		if (!m_ppComputeCommandLists[b])
		{
			return false;
		}
	}

	return true;
}

bool HealthCompute::CreateResources()
{
	using namespace LambdaEngine;

	BufferDesc healthBufferDesc		= {};
	healthBufferDesc.DebugName		= "Health System Server Health Buffer";
	healthBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_GPU;
	healthBufferDesc.SizeInBytes	= sizeof(uint32) * 10;
	healthBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | FBufferFlag::BUFFER_FLAG_COPY_SRC;
	m_pHealthBuffer					= RenderAPI::GetDevice()->CreateBuffer(&healthBufferDesc);

	BufferDesc copyBufferDesc		= {};
	copyBufferDesc.DebugName		= "Health System Server Health Buffer";
	copyBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
	copyBufferDesc.SizeInBytes		= sizeof(uint32) * 10;
	copyBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | FBufferFlag::BUFFER_FLAG_COPY_DST;
	m_pCopyBuffer					= RenderAPI::GetDevice()->CreateBuffer(&copyBufferDesc);

	BufferDesc countBufferDesc		= {};
	countBufferDesc.DebugName		= "Health System Server Vertices Count Buffer";
	countBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_GPU;
	countBufferDesc.SizeInBytes		= sizeof(uint32);
	countBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_CONSTANT_BUFFER | FBufferFlag::BUFFER_FLAG_COPY_DST;
	m_pVertexCountBuffer			= RenderAPI::GetDevice()->CreateBuffer(&countBufferDesc);

	ResourceUpdateDesc updateHealthDesc = {};
	updateHealthDesc.ResourceName					= "PLAYER_HEALTH_BUFFER";
	updateHealthDesc.ExternalBufferUpdate.ppBuffer	= &m_pHealthBuffer;
	updateHealthDesc.ExternalBufferUpdate.Count		= 1;
	RenderSystem::GetInstance().GetRenderGraph()->UpdateResource(&updateHealthDesc);
	uint64 offset = 0;
	uint64 sizeInBytes = m_pHealthBuffer->GetDesc().SizeInBytes;
	UpdateBufferResource(updateHealthDesc.ResourceName, updateHealthDesc.ExternalBufferUpdate.ppBuffer, &offset, &sizeInBytes, 1, false);

	ResourceUpdateDesc updateCountDesc = {};
	updateCountDesc.ResourceName					= "VERTEX_COUNT_BUFFER";
	updateCountDesc.ExternalBufferUpdate.ppBuffer	= &m_pVertexCountBuffer;
	updateCountDesc.ExternalBufferUpdate.Count		= 1;
	RenderSystem::GetInstance().GetRenderGraph()->UpdateResource(&updateCountDesc);
	sizeInBytes = m_pVertexCountBuffer->GetDesc().SizeInBytes;
	UpdateBufferResource(updateCountDesc.ResourceName, updateCountDesc.ExternalBufferUpdate.ppBuffer, &offset, &sizeInBytes, 1, false);


	FenceDesc fenceDesc		= {};
	fenceDesc.DebugName		= "Health System Fence";
	fenceDesc.InitalValue	= 1;
	Fence* pCopyFence		= RenderAPI::GetDevice()->CreateFence(&fenceDesc);

	// Transfer vertex count to buffer
	{
		BufferDesc stagingBufferDesc	= {};
		stagingBufferDesc.DebugName		= "Health System Server Vertices Count Staging Buffer";
		stagingBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
		stagingBufferDesc.SizeInBytes	= sizeof(uint32);
		stagingBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_CONSTANT_BUFFER | FBufferFlag::BUFFER_FLAG_COPY_SRC;
		Buffer* stagingBuffer			= RenderAPI::GetDevice()->CreateBuffer(&stagingBufferDesc);

		// Get mesh to know how many vertices there are
		// NOTE: The mesh should already be loaded and should therefore
		//		 not create it again.
		GUID_Lambda player = 0;
		TArray<GUID_Lambda> temp;
		ResourceManager::LoadMeshFromFile("Player/IdleRightUV.glb", player, temp);
		Mesh* pMesh = ResourceManager::GetMesh(player);
		m_VertexCount = pMesh->Vertices.GetSize();

		uint32* data = reinterpret_cast<uint32*>(stagingBuffer->Map());
		memcpy(data, &m_VertexCount, sizeof(uint32));
		stagingBuffer->Unmap();

		m_ppComputeCommandAllocators[0]->Reset();
		m_ppComputeCommandLists[0]->Begin(nullptr);
		m_ppComputeCommandLists[0]->CopyBuffer(stagingBuffer, 0, m_pVertexCountBuffer, 0, sizeof(uint32));
		m_ppComputeCommandLists[0]->End();
		RenderAPI::GetComputeQueue()->ExecuteCommandLists(
			&m_ppComputeCommandLists[0],
			1,
			FPipelineStageFlag::PIPELINE_STAGE_FLAG_UNKNOWN,
			nullptr,
			0,
			pCopyFence,
			2
		);

		pCopyFence->Wait(2, UINT64_MAX);

		SAFERELEASE(stagingBuffer);
	}

	SAFERELEASE(pCopyFence);

	return true;
}