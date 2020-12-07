#include "Rendering/RT/ReflectionsDenoisePass.h"

#include "Rendering/RenderAPI.h"
#include "Rendering/PipelineStateManager.h"

#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/PipelineLayout.h"
#include "Rendering/Core/API/DescriptorSet.h"
#include "Rendering/Core/API/DescriptorHeap.h"
#include "Rendering/Core/API/TextureView.h"
#include "Rendering/Core/API/Texture.h"

#include "Resources/ResourceManager.h"

namespace LambdaEngine
{
	ReflectionsDenoisePass::~ReflectionsDenoisePass()
	{
		if (m_Initialized)
		{
			Release();
		}
	}

	bool ReflectionsDenoisePass::Init()
	{
		return true;
	}

	bool ReflectionsDenoisePass::RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc)
	{
		VALIDATE(pPreInitDesc != nullptr);

		if (m_Initialized)
		{
			if (!Release())
			{
				return false;
			}
		}

		m_BackBufferCount = pPreInitDesc->BackBufferCount;

		m_pDeviceResourcesToRemove = DBG_NEW TArray<DeviceChild*>[m_BackBufferCount];

		if (!CreateCommandLists())
		{
			LOG_ERROR("[ReflectionsDenoisePass]: Failed to create Command Lists");
			return false;
		}

		if (!CreatePipelineLayouts())
		{
			LOG_ERROR("[ReflectionsDenoisePass]: Failed to create Pipeline Layouts");
			return false;
		}

		if (!CreateDescriptorSets())
		{
			LOG_ERROR("[ReflectionsDenoisePass]: Failed to create Descriptor Sets");
			return false;
		}

		if (!CreatePipelineStates())
		{
			LOG_ERROR("[ReflectionsDenoisePass]: Failed to create Pipeline States");
			return false;
		}

		if (!CreateBarriers())
		{
			LOG_ERROR("[ReflectionsDenoisePass]: Failed to create Barriers");
			return false;
		}

		m_Initialized = true;
		return true;
	}

	void ReflectionsDenoisePass::PreTexturesDescriptorSetWrite()
	{
		TArray<DeviceChild*>& deviceResourcesToRemove = m_pDeviceResourcesToRemove[m_ModFrameIndex];
		deviceResourcesToRemove.PushBack(m_pSpatioTemporalPassDescriptorSet);
		deviceResourcesToRemove.PushBack(m_pHorizontalGaussianPassDescriptorSet);
		deviceResourcesToRemove.PushBack(m_pVerticalGaussianPassDescriptorSet);

		{
			DescriptorSet* pDescriptorSet = RenderAPI::GetDevice()->CreateDescriptorSet(m_pSpatioTemporalPassDescriptorSet->GetName(), m_pSpatioTemporalPassPipelineLayout, 0, m_pDescriptorHeap);
			RenderAPI::GetDevice()->CopyDescriptorSet(m_pSpatioTemporalPassDescriptorSet, pDescriptorSet);
			m_pSpatioTemporalPassDescriptorSet = pDescriptorSet;
		}

		{
			DescriptorSet* pDescriptorSet = RenderAPI::GetDevice()->CreateDescriptorSet(m_pHorizontalGaussianPassDescriptorSet->GetName(), m_pGaussianPassPipelineLayout, 0, m_pDescriptorHeap);
			RenderAPI::GetDevice()->CopyDescriptorSet(m_pHorizontalGaussianPassDescriptorSet, pDescriptorSet);
			m_pHorizontalGaussianPassDescriptorSet = pDescriptorSet;
		}

		{
			DescriptorSet* pDescriptorSet = RenderAPI::GetDevice()->CreateDescriptorSet(m_pVerticalGaussianPassDescriptorSet->GetName(), m_pGaussianPassPipelineLayout, 0, m_pDescriptorHeap);
			RenderAPI::GetDevice()->CopyDescriptorSet(m_pVerticalGaussianPassDescriptorSet, pDescriptorSet);
			m_pVerticalGaussianPassDescriptorSet = pDescriptorSet;
		}
	}

	void ReflectionsDenoisePass::UpdateTextureResource(
		const String& resourceName,
		const TextureView* const* ppPerImageTextureViews,
		const TextureView* const* ppPerSubImageTextureViews,
		const Sampler* const* ppPerImageSamplers,
		uint32 imageCount,
		uint32 subImageCount,
		bool backBufferBound)
	{
		UNREFERENCED_VARIABLE(ppPerSubImageTextureViews);
		UNREFERENCED_VARIABLE(backBufferBound);

		if (imageCount == 1 && subImageCount == 1)
		{
			const TextureView* pTextureView = ppPerImageTextureViews[0];
			const Sampler* pSampler = ppPerImageSamplers[0];

			if (resourceName == "DENOISED_REFLECTIONS_TEXTURE")
			{
				m_pSpatioTemporalPassDescriptorSet->WriteTextureDescriptors(&pTextureView, &pSampler, ETextureState::TEXTURE_STATE_GENERAL, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_TEXTURE, true);
				m_pHorizontalGaussianPassDescriptorSet->WriteTextureDescriptors(&pTextureView, &pSampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, true);
				m_pVerticalGaussianPassDescriptorSet->WriteTextureDescriptors(&pTextureView, &pSampler, ETextureState::TEXTURE_STATE_GENERAL, 3, 1, EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_TEXTURE, true);

				Texture* pTexture = pTextureView->GetDesc().pTexture;
				m_FirstStagePipelineBarriers[DENOISED_REFLECTIONS_BARRIER_INDEX].pTexture = pTexture;
				m_SecondStagePipelineBarriers[DENOISED_REFLECTIONS_BARRIER_INDEX].pTexture = pTexture;

				m_DispatchWidth = uint32(ceil(float32(pTexture->GetDesc().Width) / float32(WORKGROUP_WIDTH_HEIGHT)));
				m_DispatchHeight = uint32(ceil(float32(pTexture->GetDesc().Height) / float32(WORKGROUP_WIDTH_HEIGHT)));
			}
			else if (resourceName == "PREV_DENOISED_REFLECTIONS_TEXTURE")
			{
				m_pSpatioTemporalPassDescriptorSet->WriteTextureDescriptors(&pTextureView, &pSampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 3, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, true);
			}
			else if (resourceName == "REFLECTIONS_TEXTURE")
			{
				m_pSpatioTemporalPassDescriptorSet->WriteTextureDescriptors(&pTextureView, &pSampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 1, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, true);
			}
			else if (resourceName == "BRDF_PDF_TEXTURE")
			{
				m_pSpatioTemporalPassDescriptorSet->WriteTextureDescriptors(&pTextureView, &pSampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 2, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, true);
			}
			else if (resourceName == "G_BUFFER_AO_ROUGH_METAL_VALID")
			{
				m_pSpatioTemporalPassDescriptorSet->WriteTextureDescriptors(&pTextureView, &pSampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 4, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, true);
				m_pHorizontalGaussianPassDescriptorSet->WriteTextureDescriptors(&pTextureView, &pSampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 1, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, true);
				m_pVerticalGaussianPassDescriptorSet->WriteTextureDescriptors(&pTextureView, &pSampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 1, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, true);
			}
			else if (resourceName == "G_BUFFER_VELOCITY_FWIDTH_NORMAL")
			{
				m_pSpatioTemporalPassDescriptorSet->WriteTextureDescriptors(&pTextureView, &pSampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 5, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, true);
			}
			else if (resourceName == "G_BUFFER_PACKED_GEOMETRIC_NORMAL")
			{
				m_pSpatioTemporalPassDescriptorSet->WriteTextureDescriptors(&pTextureView, &pSampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 6, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, true);
			}
			else if (resourceName == "G_BUFFER_PREV_PACKED_GEOMETRIC_NORMAL")
			{
				m_pSpatioTemporalPassDescriptorSet->WriteTextureDescriptors(&pTextureView, &pSampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 7, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, true);
			}
			else if (resourceName == "G_BUFFER_COMPACT_NORMAL")
			{
				m_pHorizontalGaussianPassDescriptorSet->WriteTextureDescriptors(&pTextureView, &pSampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 2, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, true);
				m_pVerticalGaussianPassDescriptorSet->WriteTextureDescriptors(&pTextureView, &pSampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 2, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, true);
			}
			else if (resourceName == "INTERMEDIATE_OUTPUT_IMAGE")
			{
				m_pHorizontalGaussianPassDescriptorSet->WriteTextureDescriptors(&pTextureView, &pSampler, ETextureState::TEXTURE_STATE_GENERAL, 3, 1, EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_TEXTURE, true);
				m_pVerticalGaussianPassDescriptorSet->WriteTextureDescriptors(&pTextureView, &pSampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, true);

				Texture* pTexture = pTextureView->GetDesc().pTexture;
				m_FirstStagePipelineBarriers[INTERMEDIATE_OUTPUT_BARRIER_INDEX].pTexture = pTexture;
				m_SecondStagePipelineBarriers[INTERMEDIATE_OUTPUT_BARRIER_INDEX].pTexture = pTexture;
			}
		}
	}

	void ReflectionsDenoisePass::Render(
		uint32 modFrameIndex,
		uint32 backBufferIndex,
		CommandList** ppFirstExecutionStage,
		CommandList** ppSecondaryExecutionStage,
		bool sleeping)
	{
		UNREFERENCED_VARIABLE(backBufferIndex);
		UNREFERENCED_VARIABLE(ppSecondaryExecutionStage);

		if (sleeping)
			return;

		m_ModFrameIndex = modFrameIndex;

		//Release Device Resources
		{
			TArray<DeviceChild*>& deviceResourcesToRemove = m_pDeviceResourcesToRemove[m_ModFrameIndex];

			for (DeviceChild* pDeviceResource : deviceResourcesToRemove)
			{
				SAFERELEASE(pDeviceResource);
			}

			deviceResourcesToRemove.Clear();
		}

		m_ppCommandAllocators[m_ModFrameIndex]->Reset();
		CommandList* pCommandList = m_ppCommandLists[m_ModFrameIndex];

		pCommandList->Begin(nullptr);

		//Spatio-Temporal Denoise Pass
		{
			pCommandList->BindDescriptorSetCompute(m_pSpatioTemporalPassDescriptorSet, m_pSpatioTemporalPassPipelineLayout, 0);
			pCommandList->BindComputePipeline(PipelineStateManager::GetPipelineState(m_SpatioTemporalPassPipelineStateID));
			pCommandList->Dispatch(m_DispatchWidth, m_DispatchHeight, 1);
		}

		//First Synchronization
		{
			pCommandList->PipelineTextureBarriers(
				FPipelineStageFlag::PIPELINE_STAGE_FLAG_COMPUTE_SHADER, 
				FPipelineStageFlag::PIPELINE_STAGE_FLAG_COMPUTE_SHADER,
				m_FirstStagePipelineBarriers,
				ARR_SIZE(m_FirstStagePipelineBarriers));
		}

		//Horizontal Gaussian Denoise Pass
		{
			pCommandList->BindDescriptorSetCompute(m_pHorizontalGaussianPassDescriptorSet, m_pGaussianPassPipelineLayout, 0);
			pCommandList->BindComputePipeline(PipelineStateManager::GetPipelineState(m_HorizontalGaussianPassPipelineStateID));
			pCommandList->Dispatch(m_DispatchWidth, m_DispatchHeight, 1);
		}

		//Second Synchronization
		{
			pCommandList->PipelineTextureBarriers(
				FPipelineStageFlag::PIPELINE_STAGE_FLAG_COMPUTE_SHADER,
				FPipelineStageFlag::PIPELINE_STAGE_FLAG_COMPUTE_SHADER,
				m_SecondStagePipelineBarriers,
				ARR_SIZE(m_SecondStagePipelineBarriers));
		}

		//Vertical Gaussian Denoise Pass
		{
			pCommandList->BindDescriptorSetCompute(m_pVerticalGaussianPassDescriptorSet, m_pGaussianPassPipelineLayout, 0);
			pCommandList->BindComputePipeline(PipelineStateManager::GetPipelineState(m_VerticalGaussianPassPipelineStateID));
			pCommandList->Dispatch(m_DispatchWidth, m_DispatchHeight, 1);
		}

		pCommandList->End();

		(*ppFirstExecutionStage) = pCommandList;
	}

	bool ReflectionsDenoisePass::Release()
	{
		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			SAFERELEASE(m_ppCommandAllocators[b]);
			SAFERELEASE(m_ppCommandLists[b]);

			TArray<DeviceChild*>& deviceResourcesToRemove = m_pDeviceResourcesToRemove[b];

			for (DeviceChild* pDeviceResource : deviceResourcesToRemove)
			{
				SAFERELEASE(pDeviceResource);
			}

			deviceResourcesToRemove.Clear();
		}
		SAFEDELETE_ARRAY(m_pDeviceResourcesToRemove);
		SAFEDELETE_ARRAY(m_ppCommandAllocators);
		SAFEDELETE_ARRAY(m_ppCommandLists);

		SAFERELEASE(m_pSpatioTemporalPassPipelineLayout);
		SAFERELEASE(m_pGaussianPassPipelineLayout);

		SAFERELEASE(m_pDescriptorHeap);
		SAFERELEASE(m_pSpatioTemporalPassDescriptorSet);
		SAFERELEASE(m_pHorizontalGaussianPassDescriptorSet);
		SAFERELEASE(m_pVerticalGaussianPassDescriptorSet);

		m_Initialized = false;
		return true;
	}

	bool ReflectionsDenoisePass::CreateCommandLists()
	{
		m_ppCommandAllocators = DBG_NEW CommandAllocator * [m_BackBufferCount];
		m_ppCommandLists = DBG_NEW CommandList * [m_BackBufferCount];

		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			CommandAllocator* pCommandAllocator = RenderAPI::GetDevice()->CreateCommandAllocator("Reflections Denoise Command Allocator " + std::to_string(b), ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE);

			if (pCommandAllocator == nullptr)
			{
				return false;
			}

			CommandListDesc commandListDesc = {};
			commandListDesc.DebugName		= "Reflections Denoise Command List " + std::to_string(b);
			commandListDesc.CommandListType	= ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
			commandListDesc.Flags			= FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

			m_ppCommandAllocators[b] = pCommandAllocator;
			CommandList* pCommandList = RenderAPI::GetDevice()->CreateCommandList(pCommandAllocator, &commandListDesc);

			if (pCommandList == nullptr)
			{
				return false;
			}

			m_ppCommandLists[b] = pCommandList;
		}

		return true;
	}

	bool ReflectionsDenoisePass::CreatePipelineLayouts()
	{
		//Spatio-Temporal Denoise Pass
		{
			DescriptorSetLayoutDesc descriptorSetLayout = {};

			//Denoised Reflections Texture
			{
				DescriptorBindingDesc descriptorBindingDesc = {};
				descriptorBindingDesc.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_TEXTURE;
				descriptorBindingDesc.DescriptorCount	= 1;
				descriptorBindingDesc.Binding			= 0;
				descriptorBindingDesc.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;
				descriptorSetLayout.DescriptorBindings.PushBack(descriptorBindingDesc);
			}

			//Reflections Texture
			{
				DescriptorBindingDesc descriptorBindingDesc = {};
				descriptorBindingDesc.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
				descriptorBindingDesc.DescriptorCount	= 1;
				descriptorBindingDesc.Binding			= 1;
				descriptorBindingDesc.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;
				descriptorSetLayout.DescriptorBindings.PushBack(descriptorBindingDesc);
			}

			//BRDF & PDF Texture
			{
				DescriptorBindingDesc descriptorBindingDesc = {};
				descriptorBindingDesc.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
				descriptorBindingDesc.DescriptorCount	= 1;
				descriptorBindingDesc.Binding			= 2;
				descriptorBindingDesc.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;
				descriptorSetLayout.DescriptorBindings.PushBack(descriptorBindingDesc);
			}

			//Prev Denoised Reflections Texture
			{
				DescriptorBindingDesc descriptorBindingDesc = {};
				descriptorBindingDesc.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
				descriptorBindingDesc.DescriptorCount	= 1;
				descriptorBindingDesc.Binding			= 3;
				descriptorBindingDesc.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;
				descriptorSetLayout.DescriptorBindings.PushBack(descriptorBindingDesc);
			}

			//G-Buffer AO/Roughness/Metallic/Valid Texture
			{
				DescriptorBindingDesc descriptorBindingDesc = {};
				descriptorBindingDesc.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
				descriptorBindingDesc.DescriptorCount	= 1;
				descriptorBindingDesc.Binding			= 4;
				descriptorBindingDesc.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;
				descriptorSetLayout.DescriptorBindings.PushBack(descriptorBindingDesc);
			}

			//G-Buffer Velocity/FWidth Normal
			{
				DescriptorBindingDesc descriptorBindingDesc = {};
				descriptorBindingDesc.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
				descriptorBindingDesc.DescriptorCount	= 1;
				descriptorBindingDesc.Binding			= 5;
				descriptorBindingDesc.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;
				descriptorSetLayout.DescriptorBindings.PushBack(descriptorBindingDesc);
			}

			//G-Buffer Packed Geometric Normal
			{
				DescriptorBindingDesc descriptorBindingDesc = {};
				descriptorBindingDesc.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
				descriptorBindingDesc.DescriptorCount	= 1;
				descriptorBindingDesc.Binding			= 6;
				descriptorBindingDesc.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;
				descriptorSetLayout.DescriptorBindings.PushBack(descriptorBindingDesc);
			}

			//Prev G-Buffer Packed Geometric Normal
			{
				DescriptorBindingDesc descriptorBindingDesc = {};
				descriptorBindingDesc.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
				descriptorBindingDesc.DescriptorCount	= 1;
				descriptorBindingDesc.Binding			= 7;
				descriptorBindingDesc.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;
				descriptorSetLayout.DescriptorBindings.PushBack(descriptorBindingDesc);
			}

			PipelineLayoutDesc pipelineLayoutDesc = { };
			pipelineLayoutDesc.DebugName			= "Spatio-Temporal Reflections Denoise Pass";
			pipelineLayoutDesc.DescriptorSetLayouts = { descriptorSetLayout };

			m_pSpatioTemporalPassPipelineLayout = RenderAPI::GetDevice()->CreatePipelineLayout(&pipelineLayoutDesc);
		}

		//Gaussian Denoise pass
		{
			DescriptorSetLayoutDesc descriptorSetLayout = {};

			//Denoised Reflections Texture
			{
				DescriptorBindingDesc descriptorBindingDesc = {};
				descriptorBindingDesc.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
				descriptorBindingDesc.DescriptorCount	= 1;
				descriptorBindingDesc.Binding			= 0;
				descriptorBindingDesc.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;
				descriptorSetLayout.DescriptorBindings.PushBack(descriptorBindingDesc);
			}

			//G-Buffer AO/Roughness/Metallic/Valid Texture
			{
				DescriptorBindingDesc descriptorBindingDesc = {};
				descriptorBindingDesc.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
				descriptorBindingDesc.DescriptorCount	= 1;
				descriptorBindingDesc.Binding			= 1;
				descriptorBindingDesc.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;
				descriptorSetLayout.DescriptorBindings.PushBack(descriptorBindingDesc);
			}

			//G-Buffer Compact Normal Texture
			{
				DescriptorBindingDesc descriptorBindingDesc = {};
				descriptorBindingDesc.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
				descriptorBindingDesc.DescriptorCount	= 1;
				descriptorBindingDesc.Binding			= 2;
				descriptorBindingDesc.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;
				descriptorSetLayout.DescriptorBindings.PushBack(descriptorBindingDesc);
			}

			//Gaussian Denoised Output Texture
			{
				DescriptorBindingDesc descriptorBindingDesc = {};
				descriptorBindingDesc.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_TEXTURE;
				descriptorBindingDesc.DescriptorCount	= 1;
				descriptorBindingDesc.Binding			= 3;
				descriptorBindingDesc.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;
				descriptorSetLayout.DescriptorBindings.PushBack(descriptorBindingDesc);
			}

			PipelineLayoutDesc pipelineLayoutDesc = { };
			pipelineLayoutDesc.DebugName			= "Gaussian Denoise Pass";
			pipelineLayoutDesc.DescriptorSetLayouts = { descriptorSetLayout };

			m_pGaussianPassPipelineLayout = RenderAPI::GetDevice()->CreatePipelineLayout(&pipelineLayoutDesc);
		}

		return true;
	}

	bool ReflectionsDenoisePass::CreateDescriptorSets()
	{
		DescriptorHeapInfo descriptorCountDesc = { };
		descriptorCountDesc.SamplerDescriptorCount					= 0;
		descriptorCountDesc.TextureDescriptorCount					= 0;
		descriptorCountDesc.TextureCombinedSamplerDescriptorCount	= 32;
		descriptorCountDesc.ConstantBufferDescriptorCount			= 0;
		descriptorCountDesc.UnorderedAccessBufferDescriptorCount	= 8;
		descriptorCountDesc.UnorderedAccessTextureDescriptorCount	= 0;
		descriptorCountDesc.AccelerationStructureDescriptorCount	= 0;

		DescriptorHeapDesc descriptorHeapDesc = { };
		descriptorHeapDesc.DebugName			= "Reflections Denoise Pass Descriptor Heap";
		descriptorHeapDesc.DescriptorSetCount	= 64;
		descriptorHeapDesc.DescriptorCount		= descriptorCountDesc;

		m_pDescriptorHeap = RenderAPI::GetDevice()->CreateDescriptorHeap(&descriptorHeapDesc);
		if (m_pDescriptorHeap == nullptr)
		{
			return false;
		}

		m_pSpatioTemporalPassDescriptorSet = RenderAPI::GetDevice()->CreateDescriptorSet("Spatio-Temporal Denoise Descriptor Set", m_pSpatioTemporalPassPipelineLayout, 0, m_pDescriptorHeap);
		if (m_pSpatioTemporalPassDescriptorSet == nullptr)
		{
			LOG_ERROR("[ReflectionsDenoisePass]: Failed to create m_pSpatioTemporalPassDescriptorSet");
			return false;
		}

		m_pVerticalGaussianPassDescriptorSet = RenderAPI::GetDevice()->CreateDescriptorSet("Gaussian Vertical Pass Descriptor Set", m_pGaussianPassPipelineLayout, 0, m_pDescriptorHeap);
		if (m_pVerticalGaussianPassDescriptorSet == nullptr)
		{
			LOG_ERROR("[ReflectionsDenoisePass]: Failed to create m_pVerticalGaussianPassDescriptorSet");
			return false;
		}

		m_pHorizontalGaussianPassDescriptorSet = RenderAPI::GetDevice()->CreateDescriptorSet("Gaussian Horizontal Pass Descriptor Set", m_pGaussianPassPipelineLayout, 0, m_pDescriptorHeap);
		if (m_pHorizontalGaussianPassDescriptorSet == nullptr)
		{
			LOG_ERROR("[ReflectionsDenoisePass]: Failed to create m_pHorizontalGaussianPassDescriptorSet");
			return false;
		}

		return true;
	}

	bool ReflectionsDenoisePass::CreatePipelineStates()
	{
		ShaderConstant workgroupWidthDirection = {};
		workgroupWidthDirection.Integer = WORKGROUP_WIDTH_HEIGHT;

		ShaderConstant workgroupHeightDirection = {};
		workgroupHeightDirection.Integer = WORKGROUP_WIDTH_HEIGHT;

		//Spatio-Temporal Denoise Pass
		{
			GUID_Lambda shaderGUID = ResourceManager::LoadShaderFromFile("/RayTracing/SpatioTemporalReflectionsDenoiser.comp", FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER, EShaderLang::SHADER_LANG_GLSL);

			ManagedComputePipelineStateDesc computePiplineStateDesc = {};
			computePiplineStateDesc.DebugName				= "Spatio-Temporal Denoise Pass Pipeline State";
			computePiplineStateDesc.PipelineLayout			= MakeSharedRef(m_pSpatioTemporalPassPipelineLayout);
			computePiplineStateDesc.Shader.ShaderGUID		= shaderGUID;
			computePiplineStateDesc.Shader.ShaderConstants = { workgroupWidthDirection, workgroupHeightDirection };

			m_SpatioTemporalPassPipelineStateID = PipelineStateManager::CreateComputePipelineState(&computePiplineStateDesc);
		}

		//Gaussian Denoise Pass
		{
			GUID_Lambda shaderGUID = ResourceManager::LoadShaderFromFile("/RayTracing/GaussianDenoiser.comp", FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER, EShaderLang::SHADER_LANG_GLSL);

			//Horizontal Pass
			{
				ShaderConstant verticalBlurDirection = {};
				verticalBlurDirection.Float = 0.0f;

				ManagedComputePipelineStateDesc computePiplineStateDesc = {};
				computePiplineStateDesc.DebugName				= "Horizontal Gaussian Denoise Pass Pipeline State";
				computePiplineStateDesc.PipelineLayout			= MakeSharedRef(m_pGaussianPassPipelineLayout);
				computePiplineStateDesc.Shader.ShaderGUID		= shaderGUID;
				computePiplineStateDesc.Shader.ShaderConstants	= { workgroupWidthDirection, workgroupHeightDirection, verticalBlurDirection };

				m_HorizontalGaussianPassPipelineStateID = PipelineStateManager::CreateComputePipelineState(&computePiplineStateDesc);
			}

			//Vertical Pass
			{
				ShaderConstant verticalBlurDirection = {};
				verticalBlurDirection.Float = 1.0f;

				ManagedComputePipelineStateDesc computePiplineStateDesc = {};
				computePiplineStateDesc.DebugName				= "Vertical Gaussian Denoise Pass Pipeline State";
				computePiplineStateDesc.PipelineLayout			= MakeSharedRef(m_pGaussianPassPipelineLayout);
				computePiplineStateDesc.Shader.ShaderGUID		= shaderGUID;
				computePiplineStateDesc.Shader.ShaderConstants	= { workgroupWidthDirection, workgroupHeightDirection, verticalBlurDirection };

				m_VerticalGaussianPassPipelineStateID = PipelineStateManager::CreateComputePipelineState(&computePiplineStateDesc);
			}
		}

		return true;
	}

	bool ReflectionsDenoisePass::CreateBarriers()
	{
		//Denoised Reflections Texture Unordered Access -> Shader Resource
		{
			PipelineTextureBarrierDesc textureBarrier = {};
			textureBarrier.pTexture				= nullptr;
			textureBarrier.StateBefore			= ETextureState::TEXTURE_STATE_GENERAL;
			textureBarrier.StateAfter			= ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;
			textureBarrier.QueueBefore			= ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE;
			textureBarrier.QueueAfter			= ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE;
			textureBarrier.SrcMemoryAccessFlags	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_SHADER_WRITE;
			textureBarrier.DstMemoryAccessFlags	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_SHADER_READ;
			textureBarrier.Miplevel				= 0;
			textureBarrier.MiplevelCount		= 1;
			textureBarrier.ArrayIndex			= 0;
			textureBarrier.ArrayCount			= 1;

			m_FirstStagePipelineBarriers[DENOISED_REFLECTIONS_BARRIER_INDEX] = textureBarrier;
		}

		//Intermediate Output Image Shader Resource -> Unordered Access
		{
			PipelineTextureBarrierDesc textureBarrier = {};
			textureBarrier.pTexture				= nullptr;
			textureBarrier.StateBefore			= ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;
			textureBarrier.StateAfter			= ETextureState::TEXTURE_STATE_GENERAL;
			textureBarrier.QueueBefore			= ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE;
			textureBarrier.QueueAfter			= ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE;
			textureBarrier.SrcMemoryAccessFlags	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_SHADER_READ;
			textureBarrier.DstMemoryAccessFlags	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_SHADER_WRITE;
			textureBarrier.Miplevel				= 0;
			textureBarrier.MiplevelCount		= 1;
			textureBarrier.ArrayIndex			= 0;
			textureBarrier.ArrayCount			= 1;

			m_FirstStagePipelineBarriers[INTERMEDIATE_OUTPUT_BARRIER_INDEX] = textureBarrier;
		}

		//Denoised Reflections Texture Shader Resource -> Unordered Access
		{
			PipelineTextureBarrierDesc textureBarrier = {};
			textureBarrier.pTexture				= nullptr;
			textureBarrier.StateBefore			= ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;
			textureBarrier.StateAfter			= ETextureState::TEXTURE_STATE_GENERAL;
			textureBarrier.QueueBefore			= ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE;
			textureBarrier.QueueAfter			= ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE;
			textureBarrier.SrcMemoryAccessFlags	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_SHADER_READ;
			textureBarrier.DstMemoryAccessFlags	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_SHADER_WRITE;
			textureBarrier.Miplevel				= 0;
			textureBarrier.MiplevelCount		= 1;
			textureBarrier.ArrayIndex			= 0;
			textureBarrier.ArrayCount			= 1;

			m_SecondStagePipelineBarriers[DENOISED_REFLECTIONS_BARRIER_INDEX] = textureBarrier;
		}

		//Intermediate Output Image Unordered Access -> Shader Resource
		{
			PipelineTextureBarrierDesc textureBarrier = {};
			textureBarrier.pTexture				= nullptr;
			textureBarrier.StateBefore			= ETextureState::TEXTURE_STATE_GENERAL;
			textureBarrier.StateAfter			= ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;
			textureBarrier.QueueBefore			= ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE;
			textureBarrier.QueueAfter			= ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE;
			textureBarrier.SrcMemoryAccessFlags	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_SHADER_WRITE;
			textureBarrier.DstMemoryAccessFlags	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_SHADER_READ;
			textureBarrier.Miplevel				= 0;
			textureBarrier.MiplevelCount		= 1;
			textureBarrier.ArrayIndex			= 0;
			textureBarrier.ArrayCount			= 1;

			m_SecondStagePipelineBarriers[INTERMEDIATE_OUTPUT_BARRIER_INDEX] = textureBarrier;
		}

		return true;
	}
}