#include "Rendering/ParticleUpdater.h"

#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/DescriptorHeap.h"
#include "Rendering/Core/API/DescriptorSet.h"
#include "Rendering/Core/API/PipelineState.h"
#include "Rendering/Core/API/TextureView.h"

#include "Rendering/RenderAPI.h"
namespace LambdaEngine
{
	ParticleUpdater* ParticleUpdater::s_pInstance = nullptr;

	ParticleUpdater::ParticleUpdater()
	{
		VALIDATE(s_pInstance == nullptr);
		s_pInstance = this;
	}

	ParticleUpdater::~ParticleUpdater()
	{
		VALIDATE(s_pInstance != nullptr);
		s_pInstance = nullptr;

		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			SAFERELEASE(m_ppComputeCommandLists[b]);
			SAFERELEASE(m_ppComputeCommandAllocators[b]);
		}

		SAFEDELETE_ARRAY(m_ppComputeCommandLists);
		SAFEDELETE_ARRAY(m_ppComputeCommandAllocators);
	}

	bool LambdaEngine::ParticleUpdater::CreatePipelineLayout()
	{
		DescriptorBindingDesc verticesBindingDesc = {};
		verticesBindingDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		verticesBindingDesc.DescriptorCount = 1;
		verticesBindingDesc.Binding = 0;
		verticesBindingDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER;

		DescriptorBindingDesc instanceBindingDesc = {};
		instanceBindingDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		instanceBindingDesc.DescriptorCount = 1;
		instanceBindingDesc.Binding = 1;
		instanceBindingDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER;

		DescriptorSetLayoutDesc descriptorSetLayoutDesc0 = {};
		descriptorSetLayoutDesc0.DescriptorBindings = { verticesBindingDesc, instanceBindingDesc };

		PipelineLayoutDesc pipelineLayoutDesc = { };
		pipelineLayoutDesc.DebugName = "Particle Updater Pipeline Layout";
		pipelineLayoutDesc.DescriptorSetLayouts = { descriptorSetLayoutDesc0 };

		m_PipelineLayout = RenderAPI::GetDevice()->CreatePipelineLayout(&pipelineLayoutDesc);

		return m_PipelineLayout != nullptr;
	}

	bool LambdaEngine::ParticleUpdater::CreateDescriptorSets()
	{
		DescriptorHeapInfo descriptorCountDesc = { };
		descriptorCountDesc.SamplerDescriptorCount = 0;
		descriptorCountDesc.TextureDescriptorCount = 0;
		descriptorCountDesc.TextureCombinedSamplerDescriptorCount = 0;
		descriptorCountDesc.ConstantBufferDescriptorCount = 0;
		descriptorCountDesc.UnorderedAccessBufferDescriptorCount = 2;
		descriptorCountDesc.UnorderedAccessTextureDescriptorCount = 0;
		descriptorCountDesc.AccelerationStructureDescriptorCount = 0;

		DescriptorHeapDesc descriptorHeapDesc = { };
		descriptorHeapDesc.DebugName = "Particle Updater Descriptor Heap";
		descriptorHeapDesc.DescriptorSetCount = 64;
		descriptorHeapDesc.DescriptorCount = descriptorCountDesc;

		m_DescriptorHeap = RenderAPI::GetDevice()->CreateDescriptorHeap(&descriptorHeapDesc);
		if (!m_DescriptorHeap)
		{
			return false;
		}

		m_PerFrameBufferDescriptorSet = RenderAPI::GetDevice()->CreateDescriptorSet("Particle Updater Buffer Descriptor Set 0", m_PipelineLayout.Get(), 0, m_DescriptorHeap.Get());
		if (m_PerFrameBufferDescriptorSet == nullptr)
		{
			LOG_ERROR("[ParticleUpdater]: Failed to create PerFrameBuffer Descriptor Set 0");
			return false;
		}

		return true;
	}

	bool LambdaEngine::ParticleUpdater::CreateShaders()
	{
		bool success = false;

		m_ComputeShaderGUID = ResourceManager::LoadShaderFromFile("/Particles/Particle.comp", FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER, EShaderLang::SHADER_LANG_GLSL);
		success &= m_ComputeShaderGUID != GUID_NONE;

		return success;
	}

	bool LambdaEngine::ParticleUpdater::CreateCommandLists()
	{
		m_ppComputeCommandAllocators = DBG_NEW CommandAllocator * [m_BackBufferCount];
		m_ppComputeCommandLists = DBG_NEW CommandList * [m_BackBufferCount];

		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			m_ppComputeCommandAllocators[b] = RenderAPI::GetDevice()->CreateCommandAllocator("Particle Updater Compute Command Allocator " + std::to_string(b), ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE);

			if (!m_ppComputeCommandAllocators[b])
			{
				return false;
			}

			CommandListDesc commandListDesc = {};
			commandListDesc.DebugName = "Particle Updater Compute Command List " + std::to_string(b);
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

	bool LambdaEngine::ParticleUpdater::CreatePipelineState()
	{
		ManagedComputePipelineStateDesc pipelineDesc = { };
		pipelineDesc.Shader.ShaderGUID = m_ComputeShaderGUID;
		pipelineDesc.DebugName = "Particle Update Compute Sipeline State";
		pipelineDesc.PipelineLayout = m_PipelineLayout;

		m_PipelineStateID = PipelineStateManager::CreateComputePipelineState(&pipelineDesc);
		return true;
	}

	bool LambdaEngine::ParticleUpdater::Init()
	{
		m_BackBufferCount = BACK_BUFFER_COUNT;

		if (!CreatePipelineLayout())
		{
			LOG_ERROR("[ParticleUpdater]: Failed to create PipelineLayout");
			return false;
		}

		if (!CreateDescriptorSets())
		{
			LOG_ERROR("[ParticleUpdater]: Failed to create DescriptorSet");
			return false;
		}

		if (!CreateShaders())
		{
			LOG_ERROR("[ParticleUpdater]: Failed to create Shaders");
			return false;
		}

		return true;
	}

	bool LambdaEngine::ParticleUpdater::RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc)
	{
		VALIDATE(pPreInitDesc);
		VALIDATE(pPreInitDesc->pColorAttachmentDesc != nullptr);
		VALIDATE(pPreInitDesc->pDepthStencilAttachmentDesc != nullptr);

		if (!m_Initilized)
		{
			if (!CreateCommandLists())
			{
				LOG_ERROR("[ParticleUpdater]: Failed to create render command lists");
				return false;
			}

			if (!CreatePipelineState())
			{
				LOG_ERROR("[ParticleUpdater]: Failed to create PipelineState");
				return false;
			}

			m_Initilized = true;
		}

		return true;
	}
	void ParticleUpdater::PreBuffersDescriptorSetWrite()
	{
	}
	void ParticleUpdater::PreTexturesDescriptorSetWrite()
	{
	}
	void ParticleUpdater::Update(Timestamp delta, uint32 modFrameIndex, uint32 backBufferIndex)
	{
		UNREFERENCED_VARIABLE(delta);
		UNREFERENCED_VARIABLE(modFrameIndex);
		UNREFERENCED_VARIABLE(backBufferIndex);
	}
	void ParticleUpdater::UpdateTextureResource(const String& resourceName, const TextureView* const* ppPerImageTextureViews, const TextureView* const* ppPerSubImageTextureViews, uint32 imageCount, uint32 subImageCount, bool backBufferBound)
	{
		UNREFERENCED_VARIABLE(resourceName);
		UNREFERENCED_VARIABLE(ppPerImageTextureViews);
		UNREFERENCED_VARIABLE(ppPerSubImageTextureViews);
		UNREFERENCED_VARIABLE(imageCount);
		UNREFERENCED_VARIABLE(subImageCount);
		UNREFERENCED_VARIABLE(backBufferBound);
	}
	void ParticleUpdater::UpdateBufferResource(const String& resourceName, const Buffer* const* ppBuffers, uint64* pOffsets, uint64* pSizesInBytes, uint32 count, bool backBufferBound)
	{
		UNREFERENCED_VARIABLE(resourceName);
		UNREFERENCED_VARIABLE(ppBuffers);
		UNREFERENCED_VARIABLE(pOffsets);
		UNREFERENCED_VARIABLE(pSizesInBytes);
		UNREFERENCED_VARIABLE(count);
		UNREFERENCED_VARIABLE(backBufferBound);
	}

	void ParticleUpdater::UpdateAccelerationStructureResource(const String& resourceName, const AccelerationStructure* pAccelerationStructure)
	{
		UNREFERENCED_VARIABLE(resourceName);
		UNREFERENCED_VARIABLE(pAccelerationStructure);
	}
	void ParticleUpdater::UpdateDrawArgsResource(const String& resourceName, const DrawArg* pDrawArgs, uint32 count)
	{
		UNREFERENCED_VARIABLE(resourceName);
		UNREFERENCED_VARIABLE(pDrawArgs);
		UNREFERENCED_VARIABLE(count);
	}
	void ParticleUpdater::Render(uint32 modFrameIndex, uint32 backBufferIndex, CommandList** ppFirstExecutionStage, CommandList** ppSecondaryExecutionStage, bool Sleeping)
	{
		UNREFERENCED_VARIABLE(modFrameIndex);
		UNREFERENCED_VARIABLE(backBufferIndex);
		UNREFERENCED_VARIABLE(ppFirstExecutionStage);
		UNREFERENCED_VARIABLE(ppSecondaryExecutionStage);
		UNREFERENCED_VARIABLE(Sleeping);

		// TODO: Might need to divide this into two custom renderers, one for compute and one for graphics!!

		/*
		if (pRenderStage->FrameCounter == pRenderStage->FrameOffset && !pRenderStage->Sleeping)
		{
			pComputeCommandAllocator->Reset();
			pComputeCommandList->Begin(nullptr);

			pComputeCommandList->BindComputePipeline(pRenderStage->pPipelineState);

			if (pRenderStage->ppBufferDescriptorSets != nullptr)
				pComputeCommandList->BindDescriptorSetCompute(pRenderStage->ppBufferDescriptorSets[m_BackBufferIndex], pRenderStage->pPipelineLayout, pRenderStage->BufferSetIndex);

			if (pRenderStage->ppTextureDescriptorSets != nullptr)
				pComputeCommandList->BindDescriptorSetCompute(pRenderStage->ppTextureDescriptorSets[m_BackBufferIndex], pRenderStage->pPipelineLayout, pRenderStage->TextureSetIndex);

			pComputeCommandList->Dispatch(pRenderStage->Dimensions.x, pRenderStage->Dimensions.y, pRenderStage->Dimensions.z);

			Profiler::GetGPUProfiler()->EndTimestamp(pComputeCommandList);
			pComputeCommandList->End();

			(*ppExecutionStage) = pComputeCommandList;
		}
	}*/
	}