#include "Rendering/AARenderer.h"
#include "Rendering/PipelineStateManager.h"
#include "Rendering/RenderAPI.h"
#include "Rendering/Core/API/Texture.h"
#include "Rendering/Core/API/TextureView.h"
#include "Rendering/Core/API/PipelineState.h"
#include "Rendering/Core/API/PipelineLayout.h"

namespace LambdaEngine
{
	constexpr uint32 MAX_SPECULAR_MIPS = 10;
	constexpr uint32 DIFFUSE_RESOURCE_COUNT = 1;
	constexpr uint32 NUM_FRAMES = 3;

	AARenderer::AARenderer()
		: CustomRenderer()
		, TAAHistory()
		, TAAHistoryViews()
		, m_GraphicsCommandLists()
		, m_GraphicsCommandAllocators()
		, m_TAAState()
		, m_TAALayout()
		, m_FXAAState()
		, m_FXAALayout()
		, m_DescriptorHeap()
		, m_TAADescriptorSets()
		, m_FXAADescriptorSets()
	{
	}

	bool AARenderer::Init()
	{
		{
			DescriptorBindingDesc intermediateBinding = { };
			intermediateBinding.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
			intermediateBinding.DescriptorCount	= 1;
			intermediateBinding.Binding			= 0;
			intermediateBinding.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

			DescriptorBindingDesc cubeMapBinding = { };
			cubeMapBinding.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_TEXTURE;
			cubeMapBinding.DescriptorCount	= 1;
			cubeMapBinding.Binding			= 1;
			cubeMapBinding.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

			DescriptorSetLayoutDesc descriptorSetLayoutDesc = { };
			descriptorSetLayoutDesc.DescriptorBindings =
			{
				environmentMapBinding,
				cubeMapBinding,
			};

			ConstantRangeDesc pushConstantRange;
			pushConstantRange.OffsetInBytes = 0;
			pushConstantRange.SizeInBytes = 12;
			pushConstantRange.ShaderStageFlags = FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

			PipelineLayoutDesc specularLayoutDesc;
			specularLayoutDesc.DebugName = "Specular Cubemap Filter Layout";
			specularLayoutDesc.ConstantRanges =
			{
				pushConstantRange
			};
			specularLayoutDesc.DescriptorSetLayouts =
			{
				descriptorSetLayoutDesc
			};

			m_SpecularFilterLayout = RenderAPI::GetDevice()->CreatePipelineLayout(&specularLayoutDesc);
			if (!m_SpecularFilterLayout)
			{
				LOG_ERROR("[LightProbeRenderer]: Failed to create Specular Cubemap Filter Layout");
				DEBUGBREAK();
				return false;
			}
		}

		{
			GUID_Lambda SpecularFilterShader = ResourceManager::LoadShaderFromFile(
				"Skybox/SpecularCubemapFilter.comp",
				FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER,
				EShaderLang::SHADER_LANG_GLSL,
				"main");
			if (SpecularFilterShader == GUID_NONE)
			{
				DEBUGBREAK();
				return false;
			}

			ManagedComputePipelineStateDesc specularStateDesc;
			specularStateDesc.DebugName = "Specular Cubemap Filter State";
			specularStateDesc.PipelineLayout = m_SpecularFilterLayout;
			specularStateDesc.Shader.ShaderGUID = SpecularFilterShader;

			m_SpecularFilterState = PipelineStateManager::CreateComputePipelineState(&specularStateDesc);
			if (m_SpecularFilterState == 0)
			{
				DEBUGBREAK();
				return false;
			}
		}

		{
			DescriptorBindingDesc environmentMapBinding = { };
			environmentMapBinding.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
			environmentMapBinding.DescriptorCount = 1;
			environmentMapBinding.Binding = 0;
			environmentMapBinding.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

			DescriptorBindingDesc cubeMapBinding = { };
			cubeMapBinding.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_TEXTURE;
			cubeMapBinding.DescriptorCount = 1;
			cubeMapBinding.Binding = 1;
			cubeMapBinding.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

			DescriptorSetLayoutDesc descriptorSetLayoutDesc = { };
			descriptorSetLayoutDesc.DescriptorBindings =
			{
				environmentMapBinding,
				cubeMapBinding,
			};

			ConstantRangeDesc pushConstantRange;
			pushConstantRange.OffsetInBytes = 0;
			pushConstantRange.SizeInBytes = 4;
			pushConstantRange.ShaderStageFlags = FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

			PipelineLayoutDesc diffuseLayoutDesc;
			diffuseLayoutDesc.DebugName = "Diffuse Cubemap Filter Layout";
			diffuseLayoutDesc.ConstantRanges =
			{
				pushConstantRange
			};
			diffuseLayoutDesc.DescriptorSetLayouts =
			{
				descriptorSetLayoutDesc
			};

			m_DiffuseFilterLayout = RenderAPI::GetDevice()->CreatePipelineLayout(&diffuseLayoutDesc);
			if (!m_DiffuseFilterLayout)
			{
				LOG_ERROR("[LightProbeRenderer]: Failed to create Diffuse Cubemap Filter Layout");
				DEBUGBREAK();
				return false;
			}
		}

		{
			GUID_Lambda diffuseFilterShader = ResourceManager::LoadShaderFromFile(
				"Skybox/DiffuseCubemapFilter.comp",
				FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER,
				EShaderLang::SHADER_LANG_GLSL,
				"main");
			if (diffuseFilterShader == GUID_NONE)
			{
				DEBUGBREAK();
				return false;
			}

			ManagedComputePipelineStateDesc diffuseStateDesc;
			diffuseStateDesc.DebugName = "Diffuse Cubemap Filter State";
			diffuseStateDesc.PipelineLayout = m_DiffuseFilterLayout;
			diffuseStateDesc.Shader.ShaderGUID = diffuseFilterShader;

			m_DiffuseFilterState = PipelineStateManager::CreateComputePipelineState(&diffuseStateDesc);
			if (m_DiffuseFilterState == 0)
			{
				DEBUGBREAK();
				return false;
			}
		}

		// Create Descriptor Heap and allocate descriptorsets
		{
			constexpr uint32 NEEDED_RESOURCE_COUNT = (MAX_SPECULAR_MIPS + DIFFUSE_RESOURCE_COUNT) * NUM_FRAMES;

			DescriptorHeapInfo descriptorCountDesc = { };
			descriptorCountDesc.TextureCombinedSamplerDescriptorCount = 16;
			descriptorCountDesc.UnorderedAccessTextureDescriptorCount = NEEDED_RESOURCE_COUNT;

			DescriptorHeapDesc descriptorHeapDesc = { };
			descriptorHeapDesc.DebugName = "LightProbe Filter DescriptorHeap";
			descriptorHeapDesc.DescriptorSetCount = NEEDED_RESOURCE_COUNT;
			descriptorHeapDesc.DescriptorCount = descriptorCountDesc;

			m_FilterDescriptorHeap = RenderAPI::GetDevice()->CreateDescriptorHeap(&descriptorHeapDesc);
			if (!m_FilterDescriptorHeap)
			{
				LOG_ERROR("Failed to create LightProbe Filter DescriptorHeap");
				return false;
			}

			for (uint32 i = 0; i < NUM_FRAMES; i++)
			{
				for (uint32 mip = 0; mip < MAX_SPECULAR_MIPS; mip++)
				{
					String name = "Specular DescriptorSet[" + std::to_string(i) + ", " + std::to_string(mip) + "]";
					TSharedRef<DescriptorSet> specularSet = RenderAPI::GetDevice()->CreateDescriptorSet(
						name,
						m_SpecularFilterLayout.Get(),
						0,
						m_FilterDescriptorHeap.Get());
					if (!specularSet)
					{
						LOG_ERROR("Failed to create '%s'", name.c_str());
						DEBUGBREAK();
						return false;
					}
					else
					{
						m_SpecularFilterDescriptorSets.EmplaceBack(specularSet);
					}
				}

				String name = "Diffuse DescriptorSet[" + std::to_string(i) + "]";
				TSharedRef<DescriptorSet> diffuseSet = RenderAPI::GetDevice()->CreateDescriptorSet(
					name,
					m_DiffuseFilterLayout.Get(),
					0,
					m_FilterDescriptorHeap.Get());
				if (!diffuseSet)
				{
					LOG_ERROR("Failed to create '%s'", name.c_str());
					DEBUGBREAK();
					return false;
				}
				else
				{
					m_DiffuseFilterDescriptorSets.EmplaceBack(diffuseSet);
				}
			}
		}

		// Create compute commandlists
		{
			for (uint32 i = 0; i < NUM_FRAMES; i++)
			{
				String name = "LightProbe Compute CommandAllocator[" + std::to_string(i) + "]";
				TSharedRef<CommandAllocator> computeCommandAllocator = RenderAPI::GetDevice()->CreateCommandAllocator(
					name,
					ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE);
				if (!computeCommandAllocator)
				{
					LOG_ERROR("Failed to create '%s'", name.c_str());
					DEBUGBREAK();
					return false;
				}
				else
				{
					m_ComputeCommandAllocators.EmplaceBack(computeCommandAllocator);
				}

				name = "LightProbe Compute CommandList[" + std::to_string(i) + "]";
				CommandListDesc commandListDesc = {};
				commandListDesc.DebugName = name;
				commandListDesc.CommandListType = ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
				commandListDesc.Flags = FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

				TSharedRef<CommandList> computeCommandList = RenderAPI::GetDevice()->CreateCommandList(
					computeCommandAllocator.Get(),
					&commandListDesc);
				if (!computeCommandList)
				{
					LOG_ERROR("Failed to create '%s'", name.c_str());
					DEBUGBREAK();
					return false;
				}
				else
				{
					m_ComputeCommandLists.EmplaceBack(computeCommandList);
				}
			}
		}

		return true;
	}

	void AARenderer::UpdateTextureResource(
		const String& resourceName,
		const TextureView* const* ppPerImageTextureViews,
		const TextureView* const* ppPerSubImageTextureViews,
		const Sampler* const* ppPerImageSamplers,
		uint32 imageCount,
		uint32 subImageCount,
		bool backBufferBound)
	{
		UNREFERENCED_VARIABLE(ppPerSubImageTextureViews);
		UNREFERENCED_VARIABLE(ppPerImageSamplers);
		UNREFERENCED_VARIABLE(subImageCount);
		UNREFERENCED_VARIABLE(backBufferBound);

		if (resourceName == "SKYBOX")
		{
			VALIDATE(imageCount > 0);
			VALIDATE(ppPerImageTextureViews[0] != nullptr);

			m_pEnvironmentMapView = ppPerImageTextureViews[0];
			m_pEnvironmentMap = m_pEnvironmentMapView->GetDesc().pTexture;
			m_NeedsUpdate = true;
		}
		else if (resourceName == "GLOBAL_SPECULAR_PROBE")
		{
			VALIDATE(imageCount > 0);
			VALIDATE(ppPerImageTextureViews[0] != nullptr);

			m_pGlobalSpecularView = ppPerImageTextureViews[0];
			m_pGlobalSpecular = m_pGlobalSpecularView->GetDesc().pTexture;
			VALIDATE(m_pGlobalSpecular->GetDesc().Miplevels < MAX_SPECULAR_MIPS);

			m_GlobalSpecularWriteViews.Reserve(subImageCount);
			for (uint32 i = 0; i < subImageCount; i++)
			{
				m_GlobalSpecularWriteViews.EmplaceBack(ppPerSubImageTextureViews[i]);
			}

			m_NeedsUpdate = true;
		}
		else if (resourceName == "GLOBAL_DIFFUSE_PROBE")
		{
			VALIDATE(imageCount > 0);
			VALIDATE(ppPerImageTextureViews[0] != nullptr);

			m_pGlobalDiffuseView = ppPerImageTextureViews[0];
			m_pGlobalDiffuse = m_pGlobalDiffuseView->GetDesc().pTexture;
			m_NeedsUpdate = true;
		}
	}

	void AARenderer::Render(
		uint32 modFrameIndex,
		uint32 backBufferIndex,
		CommandList** ppFirstExecutionStage,
		CommandList** ppSecondaryExecutionStage,
		bool sleeping)
	{
		UNREFERENCED_VARIABLE(modFrameIndex);
		UNREFERENCED_VARIABLE(backBufferIndex);
		UNREFERENCED_VARIABLE(ppFirstExecutionStage);
		UNREFERENCED_VARIABLE(ppSecondaryExecutionStage);
		UNREFERENCED_VARIABLE(sleeping);

		// Return if update is not needed
		if (!m_NeedsUpdate)
		{
			return;
		}

		// Update descriptorset
		const uint32 descriptorSetOffset = modFrameIndex * MAX_SPECULAR_MIPS;
		const uint32 numSpecularMips = m_pGlobalSpecular->GetDesc().Miplevels;
		for (uint32 mip = 0; mip < numSpecularMips; mip++)
		{
			TSharedRef<DescriptorSet>& specularSet = m_SpecularFilterDescriptorSets[descriptorSetOffset + mip];
			specularSet->WriteTextureDescriptors(
				&m_pEnvironmentMapView,
				Sampler::GetLinearSamplerToBind(),
				ETextureState::TEXTURE_STATE_SHADER_READ_ONLY,
				0, 1,
				EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER,
				false);

			const TextureView* pWriteView = m_GlobalSpecularWriteViews[mip];
			specularSet->WriteTextureDescriptors(
				&pWriteView,
				Sampler::GetLinearSamplerToBind(),
				ETextureState::TEXTURE_STATE_GENERAL,
				1, 1,
				EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_TEXTURE,
				false);
		}

		TSharedRef<DescriptorSet>& diffuseSet = m_DiffuseFilterDescriptorSets[modFrameIndex];
		diffuseSet->WriteTextureDescriptors(
			&m_pEnvironmentMapView,
			Sampler::GetLinearSamplerToBind(),
			ETextureState::TEXTURE_STATE_SHADER_READ_ONLY,
			0, 1,
			EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER,
			false);
		diffuseSet->WriteTextureDescriptors(
			&m_pGlobalDiffuseView,
			Sampler::GetLinearSamplerToBind(),
			ETextureState::TEXTURE_STATE_GENERAL,
			1, 1,
			EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_TEXTURE,
			false);

		m_NeedsUpdate = false;

		// Filter lightprobes
		TSharedRef<CommandAllocator>	commandAllocator = m_ComputeCommandAllocators[modFrameIndex];
		TSharedRef<CommandList>			commandList = m_ComputeCommandLists[modFrameIndex];
		commandAllocator->Reset();
		commandList->Begin(nullptr);

		struct FilterSettings
		{
			uint32	EnvironmenMapSize;
			uint32	CubeMapSize;
			float32	Roughness;
		} settings;

		const TextureDesc& environmentDesc = m_pEnvironmentMap->GetDesc();
		settings.EnvironmenMapSize = environmentDesc.Width;

		// Specular
		{
			const PipelineState* pPipeline = PipelineStateManager::GetPipelineState(m_SpecularFilterState);
			commandList->BindComputePipeline(pPipeline);
		}

		const TextureDesc& globalSpecularDesc = m_pGlobalSpecular->GetDesc();
		settings.CubeMapSize = globalSpecularDesc.Width;
		settings.Roughness = 0.0f;

		const float32 roughnessDelta = 1.0f / float32(numSpecularMips);
		for (uint32 mip = 0; mip < numSpecularMips; mip++)
		{
			commandList->SetConstantRange(
				m_SpecularFilterLayout.Get(),
				FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER,
				&settings,
				sizeof(FilterSettings), 0);

			TSharedRef<DescriptorSet>& specularSet = m_SpecularFilterDescriptorSets[descriptorSetOffset + mip];
			commandList->BindDescriptorSetCompute(
				specularSet.Get(),
				m_SpecularFilterLayout.Get(),
				0);

			commandList->Dispatch(settings.CubeMapSize, settings.CubeMapSize, 6);
			settings.CubeMapSize = std::max<uint32>(settings.CubeMapSize / 2, 1u);
			settings.Roughness += roughnessDelta;
		}

		// Diffuse
		{
			const PipelineState* pPipeline = PipelineStateManager::GetPipelineState(m_DiffuseFilterState);
			commandList->BindComputePipeline(pPipeline);
		}

		const TextureDesc& globalDiffuseDesc = m_pGlobalDiffuse->GetDesc();
		const uint32 diffuseSize = globalDiffuseDesc.Width;

		commandList->SetConstantRange(
			m_DiffuseFilterLayout.Get(),
			FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER,
			&diffuseSize,
			sizeof(uint32), 0);

		commandList->BindDescriptorSetCompute(
			diffuseSet.Get(),
			m_DiffuseFilterLayout.Get(),
			0);

		commandList->Dispatch(globalDiffuseDesc.Width, globalDiffuseDesc.Height, 6);

		commandList->End();
		(*ppFirstExecutionStage) = commandList.Get();
	}
}