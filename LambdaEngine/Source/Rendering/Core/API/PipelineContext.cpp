#include "Rendering/Core/API/PipelineContext.h"

#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/DescriptorHeap.h"
#include "Rendering/Core/API/DescriptorSet.h"
#include "Rendering/Core/API/PipelineState.h"

#include "Rendering/RenderAPI.h"

namespace LambdaEngine
{
	bool PipelineContext::Init(const String& debugname)
	{
		// Create pipeline layout
		if (!m_Initilized)
		{
			PipelineLayoutDesc pipelineLayoutDesc = { };
			pipelineLayoutDesc.DebugName = debugname + " Pipeline Layout";
			pipelineLayoutDesc.DescriptorSetLayouts = m_DescriptorSetLayoutDescs;
			pipelineLayoutDesc.ConstantRanges = m_ConstantRangeDescs;

			m_PipelineLayout = RenderAPI::GetDevice()->CreatePipelineLayout(&pipelineLayoutDesc);

			ManagedComputePipelineStateDesc pipelineDesc = { };
			pipelineDesc.Shader.ShaderGUID = m_ComputeShaderGUID;
			pipelineDesc.DebugName = debugname + " Pipeline State";
			pipelineDesc.PipelineLayout = m_PipelineLayout;

			m_PipelineStateID = PipelineStateManager::CreateComputePipelineState(&pipelineDesc);
		
		
			m_Initilized = m_PipelineStateID != NULL;
		}
		else
		{
			LOG_WARNING("[PipelineContext]: Can't Init, context has already been initilized");
		}

		return m_Initilized;
	}

	void PipelineContext::Update(Timestamp delta, uint32 modFrameIndex, uint32 backBufferIndex)
	{
		UNREFERENCED_VARIABLE(delta);
		UNREFERENCED_VARIABLE(backBufferIndex);
		m_DescriptorCache.HandleUnavailableDescriptors(modFrameIndex);
	}

	void PipelineContext::BindConstantRange(CommandList* pCommandList, void* pPushConstantData, uint32 pushConstantSize, uint32 pushConstantOffset)
	{
		pCommandList->SetConstantRange(m_PipelineLayout.Get(), FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER, pPushConstantData, pushConstantSize, pushConstantOffset);
	}

	void PipelineContext::Bind(CommandList* pCommandList)
	{
		pCommandList->BindComputePipeline(PipelineStateManager::GetPipelineState(m_PipelineStateID));
	
		for (auto [key, descriptorSet] : m_DescriptorSets)
		{
			pCommandList->BindDescriptorSetCompute(descriptorSet.Get(), m_PipelineLayout.Get(), key);	
		}
	}

	void PipelineContext::CreateDescriptorSetLayout(const TArray<DescriptorBindingDesc>& descriptorBindings)
	{
		if (!m_Initilized)
		{
			DescriptorSetLayoutDesc descriptorSetLayoutDesc = {};
			descriptorSetLayoutDesc.DescriptorBindings = descriptorBindings;

			m_DescriptorSetLayoutDescs.PushBack(descriptorSetLayoutDesc);
		}
		else
		{
			LOG_WARNING("[PipelineContext]: Can't add DescriptorSetLayout, context has already been initilized");
		}
	}

	void PipelineContext::CreateConstantRange(const ConstantRangeDesc& constantRangeDesc)
	{
		if (!m_Initilized)
		{
			m_ConstantRangeDescs.PushBack(constantRangeDesc);
		}
		else
		{
			LOG_WARNING("[PipelineContext]: Can't add constantRange, context has already been initilized");
		}
	}

	void PipelineContext::UpdateDescriptorSet(const String& debugname, uint32 setIndex, DescriptorHeap* pDescriptorHeap, const SDescriptorBufferUpdateDesc& descriptorUpdateDesc, bool shouldCopy)
	{
		auto descriptorSet = m_DescriptorCache.GetDescriptorSet(debugname, m_PipelineLayout.Get(), setIndex, pDescriptorHeap, shouldCopy);
		if (descriptorSet != nullptr)
		{
			descriptorSet->WriteBufferDescriptors(
				descriptorUpdateDesc.ppBuffers,
				descriptorUpdateDesc.pOffsets,
				descriptorUpdateDesc.pSizes,
				descriptorUpdateDesc.FirstBinding,
				descriptorUpdateDesc.DescriptorCount,
				descriptorUpdateDesc.DescriptorType
			);

			m_DescriptorSets[setIndex] = descriptorSet;
		}
		else
		{
			LOG_ERROR("[ParticleUpdater]: Failed to update DescriptorSet[%d]", 0);
		}
	}

	void PipelineContext::UpdateDescriptorSet(const String& debugname, uint32 setIndex, DescriptorHeap* pDescriptorHeap, const SDescriptorTextureUpdateDesc& descriptorUpdateDesc, bool shouldCopy)
	{
		auto descriptorSet = m_DescriptorCache.GetDescriptorSet(debugname, m_PipelineLayout.Get(), setIndex, pDescriptorHeap, shouldCopy);
		if (descriptorSet != nullptr)
		{
			descriptorSet->WriteTextureDescriptors(
				descriptorUpdateDesc.ppTextures,
				descriptorUpdateDesc.ppSamplers,
				descriptorUpdateDesc.TextureState,
				descriptorUpdateDesc.FirstBinding,
				descriptorUpdateDesc.DescriptorCount,
				descriptorUpdateDesc.DescriptorType,
				descriptorUpdateDesc.UniqueSamplers
			);

			m_DescriptorSets[setIndex] = descriptorSet;
		}
		else
		{
			LOG_ERROR("[ParticleUpdater]: Failed to update DescriptorSet[%d]", 0);
		}

	}
}
