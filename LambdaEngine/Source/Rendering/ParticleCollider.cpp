#include "Rendering/ParticleCollider.h"

#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/DescriptorHeap.h"
#include "Rendering/Core/API/DescriptorSet.h"
#include "Rendering/Core/API/PipelineState.h"
#include "Rendering/Core/API/TextureView.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "Rendering/RenderAPI.h"

namespace LambdaEngine
{
	ParticleCollider* ParticleCollider::s_pInstance = nullptr;

	ParticleCollider::ParticleCollider()
	{
		VALIDATE(s_pInstance == nullptr);
		s_pInstance = this;

		m_ParticleCount = 0;
		m_PushConstant.particleCount = 0;
	}

	ParticleCollider::~ParticleCollider()
	{
		VALIDATE(s_pInstance != nullptr);
		s_pInstance = nullptr;
		if (m_Initilized)
		{
			for (uint32 b = 0; b < m_BackBufferCount; b++)
			{
				SAFERELEASE(m_ppComputeCommandLists[b]);
				SAFERELEASE(m_ppComputeCommandAllocators[b]);
			}

			SAFEDELETE_ARRAY(m_ppComputeCommandLists);
			SAFEDELETE_ARRAY(m_ppComputeCommandAllocators);

			if (m_Sampler)
				SAFERELEASE(m_Sampler);
		}
	}

	bool LambdaEngine::ParticleCollider::CreatePipelineLayout()
	{
		DescriptorBindingDesc instanceBindingDesc0 = {};
		instanceBindingDesc0.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER;
		instanceBindingDesc0.DescriptorCount = 1;
		instanceBindingDesc0.Binding = 0;
		instanceBindingDesc0.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

		DescriptorBindingDesc instanceBindingDesc1 = {};
		instanceBindingDesc1.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		instanceBindingDesc1.DescriptorCount = 1;
		instanceBindingDesc1.Binding = 1;
		instanceBindingDesc1.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

		DescriptorBindingDesc instanceBindingDesc2 = {};
		instanceBindingDesc2.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		instanceBindingDesc2.DescriptorCount = 1;
		instanceBindingDesc2.Binding = 2;
		instanceBindingDesc2.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

		DescriptorBindingDesc instanceBindingDesc3 = {};
		instanceBindingDesc3.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		instanceBindingDesc3.DescriptorCount = 1;
		instanceBindingDesc3.Binding = 3;
		instanceBindingDesc3.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

		DescriptorBindingDesc instanceBindingDesc4 = {};
		instanceBindingDesc4.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		instanceBindingDesc4.DescriptorCount = 1;
		instanceBindingDesc4.Binding = 4;
		instanceBindingDesc4.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

		DescriptorBindingDesc instanceBindingDesc5 = {};
		instanceBindingDesc5.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		instanceBindingDesc5.DescriptorCount = 1;
		instanceBindingDesc5.Binding = 5;
		instanceBindingDesc5.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

		DescriptorBindingDesc instanceBindingDesc6 = {};
		instanceBindingDesc6.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE;
		instanceBindingDesc6.DescriptorCount = 1;
		instanceBindingDesc6.Binding = 6;
		instanceBindingDesc6.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

		TArray<DescriptorBindingDesc> descriptorBindings = {
			instanceBindingDesc0,
			instanceBindingDesc1,
			instanceBindingDesc2,
			instanceBindingDesc3,
			instanceBindingDesc4,
			instanceBindingDesc5,
			instanceBindingDesc6
		};

		// Set 0
		m_UpdatePipeline.CreateDescriptorSetLayout(descriptorBindings);

		ConstantRangeDesc constantRange = {};
		constantRange.ShaderStageFlags = FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;
		constantRange.SizeInBytes = sizeof(float) * 2;
		constantRange.OffsetInBytes = 0;

		m_UpdatePipeline.CreateConstantRange(constantRange);

		return true;
	}

	bool LambdaEngine::ParticleCollider::CreateDescriptorSets()
	{
		DescriptorHeapInfo descriptorCountDesc = { };
		descriptorCountDesc.SamplerDescriptorCount = 0;
		descriptorCountDesc.TextureDescriptorCount = 0;
		descriptorCountDesc.TextureCombinedSamplerDescriptorCount = 0;
		descriptorCountDesc.ConstantBufferDescriptorCount = 1;
		descriptorCountDesc.UnorderedAccessBufferDescriptorCount = 5;
		descriptorCountDesc.UnorderedAccessTextureDescriptorCount = 0;
		descriptorCountDesc.AccelerationStructureDescriptorCount = 1;

		DescriptorHeapDesc descriptorHeapDesc = { };
		descriptorHeapDesc.DebugName = "Particle Collider Descriptor Heap";
		descriptorHeapDesc.DescriptorSetCount = 128;
		descriptorHeapDesc.DescriptorCount = descriptorCountDesc;

		m_DescriptorHeap = RenderAPI::GetDevice()->CreateDescriptorHeap(&descriptorHeapDesc);
		if (!m_DescriptorHeap)
		{
			return false;
		}

		return true;
	}

	bool LambdaEngine::ParticleCollider::CreateShaders()
	{
		bool success = true;

		String computeShaderFileName = "Particles/ParticleCollider.comp";

		GUID_Lambda computeShaderGUID = ResourceManager::LoadShaderFromFile(computeShaderFileName, FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER, EShaderLang::SHADER_LANG_GLSL);
		success &= computeShaderGUID != GUID_NONE;

		m_UpdatePipeline.SetComputeShader(computeShaderGUID);

		return success;
	}

	bool LambdaEngine::ParticleCollider::CreateCommandLists()
	{
		m_ppComputeCommandAllocators = DBG_NEW CommandAllocator * [m_BackBufferCount];
		m_ppComputeCommandLists = DBG_NEW CommandList * [m_BackBufferCount];

		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			m_ppComputeCommandAllocators[b] = RenderAPI::GetDevice()->CreateCommandAllocator("Particle Collider Compute Command Allocator " + std::to_string(b), ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE);

			if (!m_ppComputeCommandAllocators[b])
			{
				return false;
			}

			CommandListDesc commandListDesc = {};
			commandListDesc.DebugName = "Particle Collider Compute Command List " + std::to_string(b);
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

	bool LambdaEngine::ParticleCollider::Init()
	{
		m_BackBufferCount = BACK_BUFFER_COUNT;
		m_ParticleCount = 0;

		m_InlineRayTracingEnabled = RenderSystem::GetInstance().IsInlineRayTracingEnabled();
		if (!m_InlineRayTracingEnabled)
		{
			D_LOG_INFO("[ParticleCollider] Ray query is disabled, disable particle collisions.");
		}

		if (m_InlineRayTracingEnabled)
		{
			if (!CreatePipelineLayout())
			{
				LOG_ERROR("[ParticleCollider]: Failed to create PipelineLayout");
				return false;
			}

			if (!CreateDescriptorSets())
			{
				LOG_ERROR("[ParticleCollider]: Failed to create DescriptorSet");
				return false;
			}

			if (!CreateShaders())
			{
				LOG_ERROR("[ParticleCollider]: Failed to create Shaders");
				return false;
			}
		}

		return true;
	}

	bool LambdaEngine::ParticleCollider::RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc)
	{
		VALIDATE(pPreInitDesc);

		if (!m_Initilized && m_InlineRayTracingEnabled)
		{
			if (!CreateCommandLists())
			{
				LOG_ERROR("[ParticleCollider]: Failed to create render command lists");
				return false;
			}

			if (!m_UpdatePipeline.Init("Updater Compute"))
			{
				LOG_ERROR("[ParticleCollider]: Failed to init Updater Pipeline Context");
				return false;
			}

			m_Initilized = true;
		}

		return true;
	}

	void ParticleCollider::Update(Timestamp delta, uint32 modFrameIndex, uint32 backBufferIndex)
	{
		UNREFERENCED_VARIABLE(delta);

		if (m_InlineRayTracingEnabled)
		{
			m_PushConstant.delta = float(delta.AsSeconds());

			m_UpdatePipeline.Update(delta, modFrameIndex, backBufferIndex);
		}
	}

	void ParticleCollider::UpdateAccelerationStructureResource(const String& resourceName, const AccelerationStructure* const* pAccelerationStructure)
	{
		if (m_InlineRayTracingEnabled)
		{
			if (resourceName == SCENE_TLAS)
			{
				constexpr uint32 setIndex = 0U;
				constexpr uint32 setBinding = 6U;

				SDescriptorTLASUpdateDesc descriptorUpdateDesc = {};
				descriptorUpdateDesc.ppTLAS = pAccelerationStructure;
				descriptorUpdateDesc.FirstBinding = setBinding;
				descriptorUpdateDesc.DescriptorCount = 1;

				m_UpdatePipeline.UpdateDescriptorSet("[ParticleCollider] SCENE_TLAS Descriptor Set 0 Binding 6", setIndex, m_DescriptorHeap.Get(), descriptorUpdateDesc);
			}
		}
	}

	void ParticleCollider::UpdateTextureResource(const String& resourceName, const TextureView* const* ppPerImageTextureViews, const TextureView* const* ppPerSubImageTextureViews, const Sampler* const* ppPerImageSamplers, uint32 imageCount, uint32 subImageCount, bool backBufferBound)
	{
		UNREFERENCED_VARIABLE(resourceName);
		UNREFERENCED_VARIABLE(ppPerImageTextureViews);
		UNREFERENCED_VARIABLE(ppPerSubImageTextureViews);
		UNREFERENCED_VARIABLE(ppPerImageSamplers);
		UNREFERENCED_VARIABLE(imageCount);
		UNREFERENCED_VARIABLE(subImageCount);
		UNREFERENCED_VARIABLE(backBufferBound);
	}

	void ParticleCollider::UpdateBufferResource(const String& resourceName, const Buffer* const* ppBuffers, uint64* pOffsets, uint64* pSizesInBytes, uint32 count, bool backBufferBound)
	{
		UNREFERENCED_VARIABLE(backBufferBound);

		if (m_InlineRayTracingEnabled)
		{
			if (resourceName == PER_FRAME_BUFFER)
			{
				constexpr uint32 setIndex = 0U;
				constexpr uint32 setBinding = 0U;

				SDescriptorBufferUpdateDesc descriptorUpdateDesc = {};
				descriptorUpdateDesc.ppBuffers = ppBuffers;
				descriptorUpdateDesc.pOffsets = pOffsets;
				descriptorUpdateDesc.pSizes = pSizesInBytes;
				descriptorUpdateDesc.FirstBinding = setBinding;
				descriptorUpdateDesc.DescriptorCount = count;
				descriptorUpdateDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER;

				m_UpdatePipeline.UpdateDescriptorSet("[ParticleCollider] Particle Instance Buffer Descriptor Set 0 Binding 0", setIndex, m_DescriptorHeap.Get(), descriptorUpdateDesc);
			}
			else if (resourceName == SCENE_PARTICLE_INSTANCE_BUFFER)
			{
				constexpr uint32 setIndex = 0U;
				constexpr uint32 setBinding = 1U;

				SDescriptorBufferUpdateDesc descriptorUpdateDesc = {};
				descriptorUpdateDesc.ppBuffers = ppBuffers;
				descriptorUpdateDesc.pOffsets = pOffsets;
				descriptorUpdateDesc.pSizes = pSizesInBytes;
				descriptorUpdateDesc.FirstBinding = setBinding;
				descriptorUpdateDesc.DescriptorCount = count;
				descriptorUpdateDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;

				m_UpdatePipeline.UpdateDescriptorSet("[ParticleCollider] Particle Instance Buffer Descriptor Set 0 Binding 1", setIndex, m_DescriptorHeap.Get(), descriptorUpdateDesc);
			}
			else if (resourceName == SCENE_EMITTER_INSTANCE_BUFFER)
			{
				constexpr uint32 setIndex = 0U;
				constexpr uint32 setBinding = 2U;

				SDescriptorBufferUpdateDesc descriptorUpdateDesc = {};
				descriptorUpdateDesc.ppBuffers = ppBuffers;
				descriptorUpdateDesc.pOffsets = pOffsets;
				descriptorUpdateDesc.pSizes = pSizesInBytes;
				descriptorUpdateDesc.FirstBinding = setBinding;
				descriptorUpdateDesc.DescriptorCount = count;
				descriptorUpdateDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;

				m_UpdatePipeline.UpdateDescriptorSet("[ParticleCollider] Emitter Instance Buffer Descriptor Set 0 Binding 2", setIndex, m_DescriptorHeap.Get(), descriptorUpdateDesc);
			}
			else if (resourceName == SCENE_EMITTER_TRANSFORM_BUFFER)
			{
				constexpr uint32 setIndex = 0U;
				constexpr uint32 setBinding = 3U;

				SDescriptorBufferUpdateDesc descriptorUpdateDesc = {};
				descriptorUpdateDesc.ppBuffers = ppBuffers;
				descriptorUpdateDesc.pOffsets = pOffsets;
				descriptorUpdateDesc.pSizes = pSizesInBytes;
				descriptorUpdateDesc.FirstBinding = setBinding;
				descriptorUpdateDesc.DescriptorCount = count;
				descriptorUpdateDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;

				m_UpdatePipeline.UpdateDescriptorSet("[ParticleCollider] Emitter Transform Buffer Descriptor Set 0 Binding 3", setIndex, m_DescriptorHeap.Get(), descriptorUpdateDesc);
			}
			else if (resourceName == SCENE_EMITTER_INDEX_BUFFER)
			{
				constexpr uint32 setIndex = 0U;
				constexpr uint32 setBinding = 4U;

				SDescriptorBufferUpdateDesc descriptorUpdateDesc = {};
				descriptorUpdateDesc.ppBuffers = ppBuffers;
				descriptorUpdateDesc.pOffsets = pOffsets;
				descriptorUpdateDesc.pSizes = pSizesInBytes;
				descriptorUpdateDesc.FirstBinding = setBinding;
				descriptorUpdateDesc.DescriptorCount = count;
				descriptorUpdateDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;

				m_UpdatePipeline.UpdateDescriptorSet("[ParticleCollider] Emitter Index Buffer Descriptor Set 0 Binding 4", setIndex, m_DescriptorHeap.Get(), descriptorUpdateDesc);
			}
			else if (resourceName == SCENE_PARTICLE_ALIVE_BUFFER)
			{
				constexpr uint32 setIndex = 0U;
				constexpr uint32 setBinding = 5U;

				SDescriptorBufferUpdateDesc descriptorUpdateDesc = {};
				descriptorUpdateDesc.ppBuffers = ppBuffers;
				descriptorUpdateDesc.pOffsets = pOffsets;
				descriptorUpdateDesc.pSizes = pSizesInBytes;
				descriptorUpdateDesc.FirstBinding = setBinding;
				descriptorUpdateDesc.DescriptorCount = count;
				descriptorUpdateDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;

				m_UpdatePipeline.UpdateDescriptorSet("[ParticleCollider] Alive Particle Buffer Descriptor Set 0 Binding 5", setIndex, m_DescriptorHeap.Get(), descriptorUpdateDesc);
			}
		}
	}

	void ParticleCollider::Render(uint32 modFrameIndex, uint32 backBufferIndex, CommandList** ppFirstExecutionStage, CommandList** ppSecondaryExecutionStage, bool Sleeping)
	{
		UNREFERENCED_VARIABLE(backBufferIndex);
		UNREFERENCED_VARIABLE(ppSecondaryExecutionStage);
		UNREFERENCED_VARIABLE(Sleeping);

		if (m_InlineRayTracingEnabled)
		{
			if (m_ParticleCount == 0)
				return;

			CommandList* pCommandList = m_ppComputeCommandLists[modFrameIndex];
			m_ppComputeCommandAllocators[modFrameIndex]->Reset();
			pCommandList->Begin(nullptr);

			static constexpr const PipelineMemoryBarrierDesc MEMORY_BARRIER1
			{
				.SrcMemoryAccessFlags = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE,
				.DstMemoryAccessFlags = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_READ,
			};

			pCommandList->PipelineMemoryBarriers(
				FPipelineStageFlag::PIPELINE_STAGE_FLAG_BOTTOM,
				FPipelineStageFlag::PIPELINE_STAGE_FLAG_COMPUTE_SHADER,
				&MEMORY_BARRIER1,
				1);

			m_UpdatePipeline.Bind(pCommandList);
			m_UpdatePipeline.BindConstantRange(pCommandList, (void*)&m_PushConstant, sizeof(PushConstantData), 0U);

			constexpr uint32 WORK_GROUP_INVOCATIONS = 32;
			uint32 workGroupX = uint32(std::ceilf(float(m_ParticleCount) / float(WORK_GROUP_INVOCATIONS)));
			pCommandList->Dispatch(workGroupX, 1U, 1U);

			pCommandList->End();
			(*ppFirstExecutionStage) = pCommandList;
		}
	}
}