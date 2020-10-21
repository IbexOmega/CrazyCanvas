#include "Rendering/Core/API/PipelineContext.h"

#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/DescriptorHeap.h"
#include "Rendering/Core/API/DescriptorSet.h"
#include "Rendering/Core/API/PipelineState.h"

#include "Rendering/RenderAPI.h"

bool LambdaEngine::PipelineContext::Init(const String& debugname)
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

void LambdaEngine::PipelineContext::Update(Timestamp delta, uint32 modFrameIndex, uint32 backBufferIndex)
{
	m_DescriptorCache.HandleUnavailableDescriptors(modFrameIndex);
}

void LambdaEngine::PipelineContext::BindConstantRange(CommandList* pCommandList, void* pPushConstantData, uint32 pushConstantSize, uint32 pushConstantOffset)
{
	pCommandList->SetConstantRange(m_PipelineLayout.Get(), FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER, pPushConstantData, pushConstantSize, pushConstantOffset);
}

void LambdaEngine::PipelineContext::Bind(CommandList* pCommandList)
{
	pCommandList->BindComputePipeline(PipelineStateManager::GetPipelineState(m_PipelineStateID));
	
	for (auto [key, descriptorSet] : m_DescriptorSets)
	{
		pCommandList->BindDescriptorSetCompute(descriptorSet.Get(), m_PipelineLayout.Get(), key);	
	}
}

void LambdaEngine::PipelineContext::CreateDescriptorSetLayout(const TArray<DescriptorBindingDesc>& descriptorBindings)
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

void LambdaEngine::PipelineContext::CreateConstantRange(const ConstantRangeDesc& constantRangeDesc)
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

void LambdaEngine::PipelineContext::UpdateDescriptorSet(const String& debugname, uint32 setIndex, DescriptorHeap* pDescriptorHeap, const SDescriptorUpdateDesc& descriptorUpdateDesc)
{
	auto& descriptorSet = m_DescriptorSets[setIndex];
	descriptorSet = m_DescriptorCache.GetDescriptorSet(debugname, m_PipelineLayout.Get(), setIndex, pDescriptorHeap);
	if (descriptorSet != nullptr)
	{
		descriptorSet->WriteBufferDescriptors(
			descriptorUpdateDesc.ppBuffers,
			descriptorUpdateDesc.pOffsets,
			descriptorUpdateDesc.pSizes,
			descriptorUpdateDesc.firstBinding,
			descriptorUpdateDesc.descriptorCount,
			EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER
		);
	}
	else
	{
		LOG_ERROR("[ParticleUpdater]: Failed to update DescriptorSet[%d]", 0);
	}
}
