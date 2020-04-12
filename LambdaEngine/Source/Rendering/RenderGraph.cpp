#include "Rendering/RenderGraph.h"
#include "Rendering/RenderGraphDescriptionParser.h"

#include "Rendering/Core/API/IGraphicsDevice.h"
#include "Rendering/Core/API/IDescriptorHeap.h"
#include "Rendering/Core/API/IPipelineLayout.h"
#include "Rendering/Core/API/IDescriptorSet.h"
#include "Rendering/Core/API/IRenderPass.h"

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

		if (!CreateDescriptorHeap())
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Descriptor Heap", desc.pName);
			return false;
		}

		if (!CreateRenderStages(renderStageDescriptions))
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Render Stages", desc.pName);
			return false;
		}

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

			std::vector<DescriptorBindingDesc> descriptorSetDescriptions;
			descriptorSetDescriptions.reserve(pRenderStageDesc->AttachmentCount);
			uint32 descriptorBindingIndex = 0;

			std::vector<RenderPassAttachmentDesc> renderPassAttachmentDescriptions;
			renderPassAttachmentDescriptions.reserve(pRenderStageDesc->AttachmentCount);
			std::vector<ETextureState> renderPassRenderTargetStates;
			renderPassRenderTargetStates.reserve(pRenderStageDesc->AttachmentCount);

			//Create Descriptors and RenderPass Attachments from RenderStage Attachments
			for (uint32 a = 0; a < pRenderStageDesc->AttachmentCount; a++)
			{
				const RenderStageAttachment* pAttachment = &pRenderStageDesc->pAttachments[a];
				
				//Descriptors
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
				//RenderPass Attachments
				else
				{
					if (pAttachment->Type == EAttachmentType::OUTPUT_COLOR)
					{
						RenderPassAttachmentDesc renderPassAttachmentDesc = {};
						renderPassAttachmentDesc.Format			= EFormat::FORMAT_R8G8B8A8_UNORM;
						renderPassAttachmentDesc.SampleCount	= 1;
						renderPassAttachmentDesc.LoadOp			= ELoadOp::CLEAR;
						renderPassAttachmentDesc.StoreOp		= EStoreOp::STORE;
						renderPassAttachmentDesc.StencilLoadOp	= ELoadOp::DONT_CARE;
						renderPassAttachmentDesc.StencilStoreOp	= EStoreOp::DONT_CARE;
						renderPassAttachmentDesc.InitialState	= ETextureState::TEXTURE_STATE_UNKNOWN;
						renderPassAttachmentDesc.FinalState		= ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;

						renderPassAttachmentDescriptions.push_back(renderPassAttachmentDesc);

						renderPassRenderTargetStates.push_back(ETextureState::TEXTURE_STATE_RENDER_TARGET);
					}
					else if (pAttachment->Type == EAttachmentType::OUTPUT_DEPTH_STENCIL)
					{
						RenderPassAttachmentDesc renderPassAttachmentDesc = {};
						renderPassAttachmentDesc.Format			= EFormat::FORMAT_D24_UNORM_S8_UINT;
						renderPassAttachmentDesc.SampleCount	= 1;
						renderPassAttachmentDesc.LoadOp			= ELoadOp::CLEAR;
						renderPassAttachmentDesc.StoreOp		= EStoreOp::STORE;
						renderPassAttachmentDesc.StencilLoadOp	= ELoadOp::CLEAR;
						renderPassAttachmentDesc.StencilStoreOp = EStoreOp::STORE;
						renderPassAttachmentDesc.InitialState	= ETextureState::TEXTURE_STATE_UNKNOWN;
						renderPassAttachmentDesc.FinalState		= ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;

						renderPassAttachmentDescriptions.push_back(renderPassAttachmentDesc);
					}
				}
			}

			//Create Pipeline Layout
			DescriptorSetLayoutDesc descriptorSetLayout = {};
			descriptorSetLayout.pDescriptorBindings		= descriptorSetDescriptions.data();
			descriptorSetLayout.DescriptorBindingCount	= descriptorSetDescriptions.size();

			PipelineLayoutDesc pipelineLayoutDesc = {};
			pipelineLayoutDesc.pDescriptorSetLayouts	= &descriptorSetLayout;
			pipelineLayoutDesc.DescriptorSetLayoutCount = 1;

			pRenderStage->pPipelineLayout = m_pGraphicsDevice->CreatePipelineLayout(pipelineLayoutDesc);
			

			//Create Pipeline State
			if (pRenderStageDesc->PipelineType == EPipelineStateType::GRAPHICS)
			{
				pRenderStageDesc->Pipeline.pGraphicsDesc->pPipelineLayout = pRenderStage->pPipelineLayout;

				//Create RenderPass
				{
					RenderPassSubpassDesc renderPassSubpassDesc = {};
					renderPassSubpassDesc.pInputAttachmentStates		= nullptr;
					renderPassSubpassDesc.InputAttachmentCount			= 0;
					renderPassSubpassDesc.pResolveAttachmentStates		= nullptr;
					renderPassSubpassDesc.pRenderTargetStates			= renderPassRenderTargetStates.data();
					renderPassSubpassDesc.RenderTargetCount				= renderPassRenderTargetStates.size();
					renderPassSubpassDesc.DepthStencilAttachmentState	= ETextureState::TEXTURE_STATE_DEPTH_STENCIL_READ_ONLY;

					RenderPassDesc renderPassDesc = {};
					renderPassDesc.pName					= "";
					renderPassDesc.pAttachments				= renderPassAttachmentDescriptions.data();
					renderPassDesc.AttachmentCount			= renderPassAttachmentDescriptions.size();
					renderPassDesc.pSubpasses				= &renderPassSubpassDesc;
					renderPassDesc.SubpassCount				= 1;
					renderPassDesc.pSubpassDependencies		= nullptr;
					renderPassDesc.SubpassDependencyCount	= 0;

					IRenderPass* pRenderPass = m_pGraphicsDevice->CreateRenderPass(renderPassDesc);
					pRenderStageDesc->Pipeline.pGraphicsDesc->pRenderPass = pRenderPass;
				}

				pRenderStage->pPipelineState = m_pGraphicsDevice->CreateGraphicsPipelineState(*pRenderStageDesc->Pipeline.pGraphicsDesc);
			}
			else if (pRenderStageDesc->PipelineType == EPipelineStateType::COMPUTE)
			{
				pRenderStage->pPipelineState = m_pGraphicsDevice->CreateComputePipelineState(*pRenderStageDesc->Pipeline.pComputeDesc);
			}
			else if (pRenderStageDesc->PipelineType == EPipelineStateType::RAY_TRACING)
			{
				pRenderStage->pPipelineState = m_pGraphicsDevice->CreateRayTracingPipelineState(*pRenderStageDesc->Pipeline.pRayTracingDesc);
			}

				int agfsg = 0;
			
		}

		return true;
	}
}