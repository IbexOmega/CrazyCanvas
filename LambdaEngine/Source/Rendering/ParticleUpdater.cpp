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

	bool LambdaEngine::ParticleUpdater::CreatePipelineLayout()
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

		// Set 0
		m_UpdatePipeline.CreateDescriptorSetLayout({ instanceBindingDesc0, instanceBindingDesc1, instanceBindingDesc2, instanceBindingDesc3, instanceBindingDesc4, instanceBindingDesc5 });

		DescriptorBindingDesc depthBindingDesc = {};
		depthBindingDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		depthBindingDesc.DescriptorCount = 1;
		depthBindingDesc.Binding = 0;
		depthBindingDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

		// Set 1
		m_UpdatePipeline.CreateDescriptorSetLayout({ depthBindingDesc });

		DescriptorBindingDesc normalBindingDesc = {};
		normalBindingDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		normalBindingDesc.DescriptorCount = 1;
		normalBindingDesc.Binding = 0;
		normalBindingDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

		// Set 2
		m_UpdatePipeline.CreateDescriptorSetLayout({ normalBindingDesc });

		ConstantRangeDesc constantRange = {};
		constantRange.ShaderStageFlags = FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;
		constantRange.SizeInBytes = sizeof(float) * 2;
		constantRange.OffsetInBytes = 0;

		m_UpdatePipeline.CreateConstantRange(constantRange);

		return true;
	}

	bool LambdaEngine::ParticleUpdater::CreateDescriptorSets()
	{
		DescriptorHeapInfo descriptorCountDesc = { };
		descriptorCountDesc.SamplerDescriptorCount = 0;
		descriptorCountDesc.TextureDescriptorCount = 1;
		descriptorCountDesc.TextureCombinedSamplerDescriptorCount = 2;
		descriptorCountDesc.ConstantBufferDescriptorCount = 1;
		descriptorCountDesc.UnorderedAccessBufferDescriptorCount = 6;
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

		GUID_Lambda computeShaderGUID = ResourceManager::LoadShaderFromFile("/Particles/Particle.comp", FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER, EShaderLang::SHADER_LANG_GLSL);
		success &= computeShaderGUID != GUID_NONE;

		m_UpdatePipeline.SetComputeShader(computeShaderGUID);

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

			if (!m_UpdatePipeline.Init("Updater Compute"))
			{
				LOG_ERROR("[ParticleUpdater]: Failed to init Updater Pipeline Context");
				return false;
			}

			m_Initilized = true;
		}

		return true;
	}

	void ParticleUpdater::Update(Timestamp delta, uint32 modFrameIndex, uint32 backBufferIndex)
	{
		UNREFERENCED_VARIABLE(delta);

		m_PushConstant.delta = float(delta.AsSeconds());

		m_UpdatePipeline.Update(delta, modFrameIndex, backBufferIndex);
	}

	void ParticleUpdater::UpdateTextureResource(const String& resourceName, const TextureView* const* ppPerImageTextureViews, const TextureView* const* ppPerSubImageTextureViews, uint32 imageCount, uint32 subImageCount, bool backBufferBound)
	{
		UNREFERENCED_VARIABLE(ppPerSubImageTextureViews);
		UNREFERENCED_VARIABLE(imageCount);
		UNREFERENCED_VARIABLE(subImageCount);
		UNREFERENCED_VARIABLE(backBufferBound);

		if (resourceName == "G_BUFFER_DEPTH_STENCIL")
		{
			constexpr uint32 setIndex = 1U;
			constexpr uint32 setBinding = 0U;

			if (!m_Sampler)
			{
				SamplerDesc samplerDesc = {};
				samplerDesc.DebugName = "Depth Sampler";
				samplerDesc.MinFilter = EFilterType::FILTER_TYPE_NEAREST;
				samplerDesc.MagFilter = EFilterType::FILTER_TYPE_NEAREST;
				samplerDesc.MipmapMode = EMipmapMode::MIPMAP_MODE_NEAREST;
				samplerDesc.AddressModeU = ESamplerAddressMode::SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
				samplerDesc.AddressModeV = ESamplerAddressMode::SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
				samplerDesc.AddressModeW = ESamplerAddressMode::SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
				samplerDesc.borderColor = ESamplerBorderColor::SAMPLER_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
				m_Sampler = MakeSharedRef<Sampler>(RenderAPI::GetDevice()->CreateSampler(&samplerDesc));
			}
			Sampler* pSampler = m_Sampler.Get();
			SDescriptorTextureUpdateDesc descriptorUpdateDesc = {};
			descriptorUpdateDesc.ppTextures = &ppPerImageTextureViews[0];
			descriptorUpdateDesc.ppSamplers = &pSampler;
			descriptorUpdateDesc.TextureState = ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;
			descriptorUpdateDesc.FirstBinding = setBinding;
			descriptorUpdateDesc.DescriptorCount = 1;
			descriptorUpdateDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
			descriptorUpdateDesc.UniqueSamplers = true;

			m_UpdatePipeline.UpdateDescriptorSet("Particle Depth Texture Descriptor Set 1 Binding 0", setIndex, m_DescriptorHeap.Get(), descriptorUpdateDesc);
		}
		else if (resourceName == "G_BUFFER_COMPACT_NORMAL")
		{
			constexpr uint32 setIndex = 2U;
			constexpr uint32 setBinding = 0U;

			Sampler* sampler = Sampler::GetNearestSampler();
			SDescriptorTextureUpdateDesc descriptorUpdateDesc = {};
			descriptorUpdateDesc.ppTextures = &ppPerImageTextureViews[0];
			descriptorUpdateDesc.ppSamplers = &sampler;
			descriptorUpdateDesc.TextureState = ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;
			descriptorUpdateDesc.FirstBinding = setBinding;
			descriptorUpdateDesc.DescriptorCount = 1;
			descriptorUpdateDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
			descriptorUpdateDesc.UniqueSamplers = true;

			m_UpdatePipeline.UpdateDescriptorSet("Particle Normal Texture Descriptor Set 2 Binding 0", setIndex, m_DescriptorHeap.Get(), descriptorUpdateDesc, false);
		}
	}

	void ParticleUpdater::UpdateBufferResource(const String& resourceName, const Buffer* const* ppBuffers, uint64* pOffsets, uint64* pSizesInBytes, uint32 count, bool backBufferBound)
	{
		UNREFERENCED_VARIABLE(resourceName);
		UNREFERENCED_VARIABLE(ppBuffers);
		UNREFERENCED_VARIABLE(pOffsets);
		UNREFERENCED_VARIABLE(pSizesInBytes);
		UNREFERENCED_VARIABLE(count);
		UNREFERENCED_VARIABLE(backBufferBound);

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

			m_UpdatePipeline.UpdateDescriptorSet("Particle Instance Buffer Descriptor Set 0 Binding 3", setIndex, m_DescriptorHeap.Get(), descriptorUpdateDesc);
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

			m_UpdatePipeline.UpdateDescriptorSet("Particle Instance Buffer Descriptor Set 0 Binding 0", setIndex, m_DescriptorHeap.Get(), descriptorUpdateDesc);
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

			m_UpdatePipeline.UpdateDescriptorSet("Emitter Instance Buffer Descriptor Set 0 Binding 1", setIndex, m_DescriptorHeap.Get(), descriptorUpdateDesc);
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

			m_UpdatePipeline.UpdateDescriptorSet("Emitter Transform Buffer Descriptor Set 0 Binding 3", setIndex, m_DescriptorHeap.Get(), descriptorUpdateDesc);
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

			m_UpdatePipeline.UpdateDescriptorSet("Emitter Index Buffer Descriptor Set 0 Binding 4", setIndex, m_DescriptorHeap.Get(), descriptorUpdateDesc);
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

			m_UpdatePipeline.UpdateDescriptorSet("Alive Particle Buffer Descriptor Set 0 Binding 5", setIndex, m_DescriptorHeap.Get(), descriptorUpdateDesc);
		}
	}

	void ParticleUpdater::Render(uint32 modFrameIndex, uint32 backBufferIndex, CommandList** ppFirstExecutionStage, CommandList** ppSecondaryExecutionStage, bool Sleeping)
	{
		UNREFERENCED_VARIABLE(backBufferIndex);
		UNREFERENCED_VARIABLE(ppSecondaryExecutionStage);
		UNREFERENCED_VARIABLE(Sleeping);

		if (m_ParticleCount == 0)
			return;

		CommandList* pCommandList = m_ppComputeCommandLists[modFrameIndex];
		m_ppComputeCommandAllocators[modFrameIndex]->Reset();
		pCommandList->Begin(nullptr);

		m_UpdatePipeline.Bind(pCommandList);
		m_UpdatePipeline.BindConstantRange(pCommandList, (void*)&m_PushConstant, sizeof(PushConstantData), 0U);

		constexpr uint32 WORK_GROUP_INVOCATIONS = 32;
		uint32 workGroupX = uint32(std::ceilf(float(m_ParticleCount) / float(WORK_GROUP_INVOCATIONS)));
		pCommandList->Dispatch(workGroupX, 1U, 1U);

		pCommandList->End();
		(*ppFirstExecutionStage) = pCommandList;
	}
}