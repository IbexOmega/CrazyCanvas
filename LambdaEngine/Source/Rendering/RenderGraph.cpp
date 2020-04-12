#include "Rendering/RenderGraph.h"
#include "Rendering/RenderGraphDescriptionParser.h"

#include "Rendering/Core/API/IGraphicsDevice.h"
#include "Rendering/Core/API/IDescriptorHeap.h"
#include "Rendering/Core/API/IPipelineLayout.h"
#include "Rendering/Core/API/IDescriptorSet.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	RenderGraph::RenderGraph(const IGraphicsDevice* pGraphicsDevice) :
		m_pGraphicsDevice(pGraphicsDevice),
		m_pRenderStages(nullptr),
		m_RenderStageCount(0)
	{
	}

	RenderGraph::~RenderGraph()
	{
		SAFEDELETEARR(m_pRenderStages);
	}

	bool RenderGraph::Init(const RenderGraphDesc& desc)
	{
		std::vector<RenderStageDesc>			renderStageDescriptions;
		std::vector<SynchronizationStageDesc>	synchronizationStageDescriptions;
		std::vector<PipelineStageDesc>			pipelineStageDescriptions;

		if (!RenderGraphDescriptionParser::Parse(desc, renderStageDescriptions, synchronizationStageDescriptions, pipelineStageDescriptions))
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" could not be parsed", desc.pName);
			return false;
		}

		//if (!CreateDescriptorHeap())
		//{
		//	LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Descriptor Heap", desc.pName);
		//	return false;
		//}

		//if (!CreateRenderStages(renderStageDescriptions))
		//{
		//	LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Render Stages", desc.pName);
		//	return false;
		//}

		return true;
	}

	void RenderGraph::UpdateResource(const ResourceUpdateDesc& desc)
	{
	}

	bool RenderGraph::CreateDescriptorHeap()
	{
		constexpr uint32 DESCRIPTOR_COUNT = 1024;

		DescriptorCountDesc descriptorCountDesc = {};
		descriptorCountDesc.DescriptorSetCount							= DESCRIPTOR_COUNT;
		descriptorCountDesc.SamplerDescriptorCount						= DESCRIPTOR_COUNT;
		descriptorCountDesc.TextureDescriptorCount						= DESCRIPTOR_COUNT;
		descriptorCountDesc.TextureCombinedSamplerDescriptorCount		= DESCRIPTOR_COUNT;
		descriptorCountDesc.ConstantBufferDescriptorCount				= DESCRIPTOR_COUNT;
		descriptorCountDesc.UnorderedAccessBufferDescriptorCount		= DESCRIPTOR_COUNT;
		descriptorCountDesc.UnorderedAccessTextureDescriptorCount		= DESCRIPTOR_COUNT;
		descriptorCountDesc.AccelerationStructureDescriptorCount		= DESCRIPTOR_COUNT;

		DescriptorHeapDesc descriptorHeapDesc = {};
		descriptorHeapDesc.pName			= "Render Graph Descriptor Heap";
		descriptorHeapDesc.DescriptorCount	= descriptorCountDesc;

		m_pDescriptorHeap = m_pGraphicsDevice->CreateDescriptorHeap(descriptorHeapDesc);

		return m_pDescriptorHeap != nullptr;
	}

	bool RenderGraph::CreateRenderStages(const std::vector<RenderStageDesc>& renderStageDescriptions)
	{
		m_pRenderStages = new RenderStage[renderStageDescriptions.size()];

		for (uint32 i = 0; i < renderStageDescriptions.size(); i++)
		{
			const RenderStageDesc* pRenderStageDesc = &renderStageDescriptions[i];

			RenderStage* pRenderStage = &m_pRenderStages[i];

			switch (pRenderStageDesc->PipelineType)
			{
			case EPipelineStateType::GRAPHICS:		pRenderStage->pPipelineState = m_pGraphicsDevice->CreateGraphicsPipelineState(*pRenderStageDesc->Pipeline.pGraphicsDesc);		break;
			case EPipelineStateType::COMPUTE:		pRenderStage->pPipelineState = m_pGraphicsDevice->CreateComputePipelineState(*pRenderStageDesc->Pipeline.pComputeDesc);			break;
			case EPipelineStateType::RAY_TRACING:	pRenderStage->pPipelineState = m_pGraphicsDevice->CreateRayTracingPipelineState(*pRenderStageDesc->Pipeline.pRayTracingDesc);	break;
			}

			std::vector<DescriptorBindingDesc> descriptorSetDescriptions;
			descriptorSetDescriptions.reserve(pRenderStageDesc->AttachmentCount);

			uint32 descriptorBindingIndex = 0;

			for (uint32 a = 0; a < pRenderStageDesc->AttachmentCount; a++)
			{
				const RenderStageAttachment* pAttachment = &pRenderStageDesc->pAttachments[a];
				
				if (AttachmentsNeedsDescriptor(pAttachment->Type))
				{
					DescriptorBindingDesc descriptorBinding = {};
					descriptorBinding.Binding				= descriptorBindingIndex;
					descriptorBinding.DescriptorCount		= pAttachment->DescriptorCount;
					descriptorBinding.DescriptorType		= GetAttachmentDescriptorType(pAttachment->Type);
					descriptorBinding.ppImmutableSamplers	= nullptr;
					descriptorBinding.ShaderStageMask		= pAttachment->StageMask;

					descriptorSetDescriptions.push_back(descriptorBinding);

					descriptorBindingIndex++;
				}
			}

			DescriptorSetLayoutDesc descriptorSetLayout = {};
			descriptorSetLayout.pDescriptorBindings		= descriptorSetDescriptions.data();
			descriptorSetLayout.DescriptorBindingCount	= descriptorSetDescriptions.size();

			PipelineLayoutDesc pipelineLayoutDesc		= {};
			pipelineLayoutDesc.pDescriptorSetLayouts	= &descriptorSetLayout;
			pipelineLayoutDesc.DescriptorSetLayoutCount = 1;

			pRenderStage->pPipelineLayout = m_pGraphicsDevice->CreatePipelineLayout(pipelineLayoutDesc);
		}

		return false;
	}
}