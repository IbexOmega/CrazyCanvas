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

		m_ParticleCount = 0;
		m_PushConstant.particleCount = 0;
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
		DescriptorBindingDesc instanceBindingDesc = {};
		instanceBindingDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		instanceBindingDesc.DescriptorCount = 1;
		instanceBindingDesc.Binding = 0;
		instanceBindingDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

		DescriptorSetLayoutDesc descriptorSetLayoutDesc0 = {};
		descriptorSetLayoutDesc0.DescriptorBindings = { instanceBindingDesc };

		ConstantRangeDesc constantRange = {};
		constantRange.ShaderStageFlags	= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;
		constantRange.SizeInBytes		= sizeof(float) * 2.0f;
		constantRange.OffsetInBytes		= 0;

		PipelineLayoutDesc pipelineLayoutDesc = { };
		pipelineLayoutDesc.DebugName			= "Particle Updater Pipeline Layout";
		pipelineLayoutDesc.DescriptorSetLayouts = { descriptorSetLayoutDesc0 };
		pipelineLayoutDesc.ConstantRanges		= { constantRange };
		
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
		descriptorCountDesc.UnorderedAccessBufferDescriptorCount = 1;
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

		return true;
	}

	bool LambdaEngine::ParticleUpdater::CreateShaders()
	{
		bool success = true;

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
		m_ParticleCount = 0;

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
		UNREFERENCED_VARIABLE(backBufferIndex);

		m_PushConstant.delta = delta.AsSeconds();

		m_DescriptorCache.HandleUnavailableDescriptors(modFrameIndex);
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

		if (resourceName == SCENE_PARTICLE_INSTANCE_BUFFER)
		{
			constexpr uint32 setIndex = 0U;

			m_InstanceDescriptorSet = m_DescriptorCache.GetDescriptorSet("Particle Instance Buffer Descriptor Set 0", m_PipelineLayout.Get(), setIndex, m_DescriptorHeap.Get());
			if (m_InstanceDescriptorSet != nullptr)
			{
				m_InstanceDescriptorSet->WriteBufferDescriptors(
					ppBuffers,
					pOffsets,
					pSizesInBytes,
					0,
					count,
					EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER
				);
			}
			else
			{
				LOG_ERROR("[ParticleUpdater]: Failed to update DescriptorSet[%d]", 0);
			}
		}
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
		UNREFERENCED_VARIABLE(ppSecondaryExecutionStage);
		UNREFERENCED_VARIABLE(Sleeping);

		CommandList* pCommandList = m_ppComputeCommandLists[modFrameIndex];
		m_ppComputeCommandAllocators[modFrameIndex]->Reset();
		pCommandList->Begin(nullptr);
		pCommandList->BindComputePipeline(PipelineStateManager::GetPipelineState(m_PipelineStateID));

		pCommandList->SetConstantRange(m_PipelineLayout.Get(), FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER, &m_PushConstant, sizeof(PushConstantData), 0U);
		pCommandList->BindDescriptorSetCompute(m_InstanceDescriptorSet.Get(), m_PipelineLayout.Get(), 0);	// Particle Instance Buffer
		
		constexpr uint32 WORK_GROUP_INVOCATIONS = 32;
		uint32 workGroupX = std::ceilf(float(m_ParticleCount) / float(WORK_GROUP_INVOCATIONS));
		pCommandList->Dispatch(workGroupX, 1U, 1U);

		pCommandList->End();
		(*ppFirstExecutionStage) = pCommandList;
	}
}