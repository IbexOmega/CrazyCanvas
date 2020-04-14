#include "Rendering/RenderGraph.h"
#include "Rendering/RenderGraphDescriptionParser.h"

#include "Rendering/Core/API/IGraphicsDevice.h"
#include "Rendering/Core/API/IDescriptorHeap.h"
#include "Rendering/Core/API/IPipelineLayout.h"
#include "Rendering/Core/API/IDescriptorSet.h"
#include "Rendering/Core/API/IRenderPass.h"
#include "Rendering/Core/API/GraphicsHelpers.h"
#include "Rendering/Core/API/ICommandAllocator.h"
#include "Rendering/Core/API/ICommandList.h"
#include "Rendering/Core/API/IBuffer.h"
#include "Rendering/Core/API/ITexture.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	RenderGraph::RenderGraph(const IGraphicsDevice* pGraphicsDevice) :
		m_pGraphicsDevice(pGraphicsDevice),
		m_pRenderStages(nullptr)
	{
	}

	RenderGraph::~RenderGraph()
	{
		SAFEDELETE_ARRAY(m_pRenderStages);
	}

	bool RenderGraph::Init(const RenderGraphDesc& desc)
	{
		std::vector<RenderStageDesc>							renderStageDescriptions;
		std::vector<SynchronizationStageDesc>					synchronizationStageDescriptions;
		std::vector<PipelineStageDesc>							pipelineStageDescriptions;
		std::vector<RenderStageResourceDesc>					resourceDescriptions;

		if (!RenderGraphDescriptionParser::Parse(desc, renderStageDescriptions, synchronizationStageDescriptions, pipelineStageDescriptions, resourceDescriptions))
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" could not be parsed", desc.pName);
			return false;
		}

		if (!CreateDescriptorHeap())
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Descriptor Heap", desc.pName);
			return false;
		}

		if (!CreateResources(resourceDescriptions))
		{ 
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Resources", desc.pName);
			return false;
		}

		if (!CreateRenderStages(renderStageDescriptions))
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Render Stages", desc.pName);
			return false;
		}

		if (!CreateSynchronizationStages(synchronizationStageDescriptions))
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Synchronization Stages", desc.pName);
			return false;
		}

		if (!CreatePipelineStages(pipelineStageDescriptions))
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Pipeline Stages", desc.pName);
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

	bool RenderGraph::CreateResources(const std::vector<RenderStageResourceDesc>& resourceDescriptions)
	{
		m_ResourceMap.reserve(resourceDescriptions.size());

		for (uint32 i = 0; i < resourceDescriptions.size(); i++)
		{
			const RenderStageResourceDesc* pResourceDesc = &resourceDescriptions[i];

			if (pResourceDesc->Type == ERenderStageResourceType::ATTACHMENT)
			{
				const RenderStageAttachment* pAttachment = pResourceDesc->pAttachmentDesc;

				if (!IsAttachmentReserved(pAttachment->pName))
				{
					Resource* pResource = &m_ResourceMap[pAttachment->pName];

					EAttachmentAccessType accessType	= GetAttachmentAccessType(pAttachment->Type);
					ESimpleResourceType simpleType		= GetSimpleType(pAttachment->Type);

					if (accessType != EAttachmentAccessType::EXTERNAL_INPUT)
					{
						//Internal
						if (simpleType == ESimpleResourceType::TEXTURE)
						{
							pResource->Type = EResourceType::INTERNAL_TEXTURE;
						}
						else if (simpleType == ESimpleResourceType::BUFFER)
						{
							pResource->Type = EResourceType::INTERNAL_BUFFER;
						}
						else
						{
							LOG_ERROR("[RenderGraph]: Unsupported resource type for internal resource \"%s\"", pAttachment->pName);
							return false;
						}
					}
					else
					{
						//External
						if (simpleType == ESimpleResourceType::TEXTURE)
						{
							pResource->Type = EResourceType::EXTERNAL_TEXTURE;
						}
						else if (simpleType == ESimpleResourceType::BUFFER)
						{
							pResource->Type = EResourceType::EXTERNAL_BUFFER;
						}
						else if (simpleType == ESimpleResourceType::ACCELERATION_STRUCTURE)
						{ 
							pResource->Type = EResourceType::EXTERNAL_ACCELERATION_STRUCTURE;
						}
						else
						{
							LOG_ERROR("[RenderGraph]: Unsupported resource type for external resource \"%s\"", pAttachment->pName);
							return false;
						}
					}
				}
			}
			else if (pResourceDesc->Type == ERenderStageResourceType::PUSH_CONSTANTS)
			{
				Resource* pResource = &m_ResourceMap[pResourceDesc->pPushConstantsDesc->pName];

				pResource->Type = EResourceType::PUSH_CONSTANTS;
			}
		}

		return true;
	}

	bool RenderGraph::CreateRenderStages(const std::vector<RenderStageDesc>& renderStageDescriptions)
	{
		m_pRenderStages = DBG_NEW RenderStage[renderStageDescriptions.size()];

		for (uint32 i = 0; i < renderStageDescriptions.size(); i++)
		{
			const RenderStageDesc* pRenderStageDesc = &renderStageDescriptions[i];

			RenderStage* pRenderStage = &m_pRenderStages[i];

			pRenderStage->WaitValue			= i > 0 ? i - 1 : UINT64_MAX_;
			pRenderStage->SignalValue		= i;

			std::vector<DescriptorBindingDesc> descriptorSetDescriptions;
			descriptorSetDescriptions.reserve(pRenderStageDesc->AttachmentCount);
			uint32 descriptorBindingIndex = 0;

			ConstantRangeDesc constantRangeDesc = {};
			constantRangeDesc.OffsetInBytes			= 0;
			constantRangeDesc.ShaderStageFlags		= CreateShaderStageMask(pRenderStageDesc);
			constantRangeDesc.SizeInBytes			= pRenderStageDesc->PushConstants.DataSize;

			std::vector<RenderPassAttachmentDesc> renderPassAttachmentDescriptions;
			renderPassAttachmentDescriptions.reserve(pRenderStageDesc->AttachmentCount);
			std::vector<ETextureState> renderPassRenderTargetStates;
			renderPassRenderTargetStates.reserve(pRenderStageDesc->AttachmentCount);
			std::vector<BlendAttachmentState> renderPassBlendAttachmentStates;
			renderPassBlendAttachmentStates.reserve(pRenderStageDesc->AttachmentCount);
			std::vector<Resource*> resourcesToLinkToDescriptorSet;
			resourcesToLinkToDescriptorSet.reserve(pRenderStageDesc->AttachmentCount);
			std::vector<Resource*> resourcesToLinkToRenderPassWith;
			resourcesToLinkToRenderPassWith.reserve(pRenderStageDesc->AttachmentCount);

			//Create Descriptors and RenderPass Attachments from RenderStage Attachments
			for (uint32 a = 0; a < pRenderStageDesc->AttachmentCount; a++)
			{
				const RenderStageAttachment* pAttachment = &pRenderStageDesc->pAttachments[a];

				if (!IsAttachmentReserved(pAttachment->pName))
				{
					auto it = m_ResourceMap.find(pAttachment->pName);

					if (it == m_ResourceMap.end())
					{
						LOG_ERROR("[RenderGraph]: Resource found in Render Stage but not in Resource Map \"%s\"", pAttachment->pName);
						return false;
					}

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

						resourcesToLinkToDescriptorSet.push_back(&it->second);
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

							BlendAttachmentState blendAttachmentState = {};
							blendAttachmentState.BlendEnabled			= false;
							blendAttachmentState.ColorComponentsMask	= COLOR_COMPONENT_FLAG_R | COLOR_COMPONENT_FLAG_G | COLOR_COMPONENT_FLAG_B | COLOR_COMPONENT_FLAG_A;

							renderPassBlendAttachmentStates.push_back(blendAttachmentState);
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

						resourcesToLinkToRenderPassWith.push_back(&it->second);
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
			pipelineLayoutDesc.pConstantRanges			= &constantRangeDesc;
			pipelineLayoutDesc.ConstantRangeCount		= constantRangeDesc.SizeInBytes > 0 ? 1 : 0;

			pRenderStage->pPipelineLayout = m_pGraphicsDevice->CreatePipelineLayout(pipelineLayoutDesc);

			//Create Pipeline State
			if (pRenderStageDesc->PipelineType == EPipelineStateType::GRAPHICS)
			{
				pRenderStageDesc->Pipeline.pGraphicsDesc->pPipelineLayout			= pRenderStage->pPipelineLayout;
				pRenderStageDesc->Pipeline.pGraphicsDesc->pBlendAttachmentStates	= renderPassBlendAttachmentStates.data();
				pRenderStageDesc->Pipeline.pGraphicsDesc->BlendAttachmentStateCount = renderPassBlendAttachmentStates.size();

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
				pRenderStageDesc->Pipeline.pComputeDesc->pPipelineLayout = pRenderStage->pPipelineLayout;

				pRenderStage->pPipelineState = m_pGraphicsDevice->CreateComputePipelineState(*pRenderStageDesc->Pipeline.pComputeDesc);
			}
			else if (pRenderStageDesc->PipelineType == EPipelineStateType::RAY_TRACING)
			{
				pRenderStageDesc->Pipeline.pRayTracingDesc->pPipelineLayout = pRenderStage->pPipelineLayout;

				pRenderStage->pPipelineState = m_pGraphicsDevice->CreateRayTracingPipelineState(*pRenderStageDesc->Pipeline.pRayTracingDesc);
			}

			//Link Attachment Resources to Render Stage (Descriptor Set)
			for (uint32 r = 0; r < resourcesToLinkToDescriptorSet.size(); r++)
			{
				Resource* pResource = resourcesToLinkToDescriptorSet[r];

				ResourceBinding resourceBinding = {};
				resourceBinding.pRenderStage	= pRenderStage;
				resourceBinding.Binding			= r;

				pResource->ResourceBindings.push_back(resourceBinding);
			}

			//Link Push Constant Resource to Render Stage (Pipeline Layout)
			if (pRenderStageDesc->PushConstants.DataSize > 0)
			{
				auto it = m_ResourceMap.find(pRenderStageDesc->PushConstants.pName);

				if (it != m_ResourceMap.end())
				{
					ResourceBinding resourceBinding = {};
					resourceBinding.pRenderStage	= pRenderStage;
					resourceBinding.Binding			= UINT32_MAX_;

					it->second.ResourceBindings.push_back(resourceBinding);
				}
				else
				{
					LOG_ERROR("[RenderGraph]: Push Constants resource found in Render Stage but not in Resource Map \"%s\"", pRenderStageDesc->PushConstants.pName);
					return false;
				}
			}

			//Link RenderPass to RenderPass Attachments
			if (resourcesToLinkToRenderPassWith.size() > 0)
			{
				if (pRenderStageDesc->PipelineType != EPipelineStateType::GRAPHICS)
				{
					LOG_ERROR("[RenderGraph]: There are resources that a RenderPass should be linked to, but Render Stage %u is not a Graphics Pipeline State", i);
					return false;
				}

				for (uint32 r = 0; r < resourcesToLinkToRenderPassWith.size(); r++)
				{
					LOG_WARNING("[RenderGraph]: RenderPass should be linked to Resource but this is not implemented!");
				}
			}
		}

		return true;
	}

	bool RenderGraph::CreateSynchronizationStages(const std::vector<SynchronizationStageDesc>& synchronizationStageDescriptions)
	{
		m_pSynchronizationStages = DBG_NEW SynchronizationStage[synchronizationStageDescriptions.size()];

		for (uint32 i = 0; i < synchronizationStageDescriptions.size(); i++)
		{
			const SynchronizationStageDesc* pSynchronizationStageDesc = &synchronizationStageDescriptions[i];

			SynchronizationStage* pSynchronizationStage = &m_pSynchronizationStages[i];

			for (const AttachmentSynchronizationDesc& attachmentSynchronizationDesc : pSynchronizationStageDesc->Synchronizations)
			{
				ESimpleResourceType barrierType = GetSimpleType(attachmentSynchronizationDesc.FromAttachment.Type);

				if (barrierType == ESimpleResourceType::TEXTURE)
				{
					PipelineTextureBarrier textureBarrier = {};
					textureBarrier.QueueBefore	= ConvertPipelineStateTypeToQueue(attachmentSynchronizationDesc.FromQueueOwner);
					textureBarrier.QueueAfter	= ConvertPipelineStateTypeToQueue(attachmentSynchronizationDesc.ToQueueOwner);
					textureBarrier.StateBefore	= ConvertAttachmentTypeToTextureState(attachmentSynchronizationDesc.FromAttachment.Type);
					textureBarrier.StateAfter	= ConvertAttachmentTypeToTextureState(attachmentSynchronizationDesc.ToAttachment.Type);

					pSynchronizationStage->TextureBarriers[attachmentSynchronizationDesc.FromAttachment.pName] = textureBarrier;
				}
				else if (barrierType == ESimpleResourceType::BUFFER)
				{
					//Implement
					LOG_WARNING("[RenderGraph]: Barrier for Buffers has not been implemented");
				}
			}
		}

		return true;
	}

	bool RenderGraph::CreatePipelineStages(const std::vector<PipelineStageDesc>& pipelineStageDescriptions)
	{
		m_pPipelineStages = DBG_NEW PipelineStage[pipelineStageDescriptions.size()];

		for (uint32 i = 0; i < pipelineStageDescriptions.size(); i++)
		{
			const PipelineStageDesc* pPipelineStageDesc = &pipelineStageDescriptions[i];

			PipelineStage* pPipelineStage = &m_pPipelineStages[i];

			pPipelineStage->Type		= pPipelineStageDesc->Type;
			pPipelineStage->StageIndex	= pPipelineStageDesc->StageIndex;

			for (uint32 f = 0; f < MAX_FRAMES_IN_FLIGHT; f++)
			{
				pPipelineStage->pGraphicsCommandAllocators[f]	= m_pGraphicsDevice->CreateCommandAllocator("Render Graph Graphics Command Allocator", ECommandQueueType::COMMAND_QUEUE_GRAPHICS);
				pPipelineStage->pComputeCommandAllocators[f]	= m_pGraphicsDevice->CreateCommandAllocator("Render Graph Compute Command Allocator", ECommandQueueType::COMMAND_QUEUE_COMPUTE);

				CommandListDesc graphicsCommandListDesc = {};
				graphicsCommandListDesc.pName					= "Render Graph Graphics Command List";
				graphicsCommandListDesc.CommandListType			= ECommandListType::COMMAND_LIST_SECONDARY;
				graphicsCommandListDesc.Flags					= FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

				pPipelineStage->pGraphicsCommandLists[f]		= m_pGraphicsDevice->CreateCommandList(pPipelineStage->pGraphicsCommandAllocators[f], graphicsCommandListDesc);

				CommandListDesc computeCommandListDesc = {};
				computeCommandListDesc.pName					= "Render Graph Compute Command List";
				computeCommandListDesc.CommandListType			= ECommandListType::COMMAND_LIST_SECONDARY;
				computeCommandListDesc.Flags					= FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

				pPipelineStage->pComputeCommandLists[f]			= m_pGraphicsDevice->CreateCommandList(pPipelineStage->pComputeCommandAllocators[f], computeCommandListDesc);
			}
		}

		return true;
	}

	uint32 RenderGraph::CreateShaderStageMask(const RenderStageDesc* pRenderStageDesc)
	{
		uint32 shaderStageMask = 0;

		if (pRenderStageDesc->PipelineType == EPipelineStateType::GRAPHICS)
		{
			shaderStageMask |= (pRenderStageDesc->Pipeline.pGraphicsDesc->pMeshShader != nullptr)		? FShaderStageFlags::SHADER_STAGE_FLAG_MESH_SHADER		: 0;

			shaderStageMask |= (pRenderStageDesc->Pipeline.pGraphicsDesc->pVertexShader != nullptr)		? FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER	: 0;
			shaderStageMask |= (pRenderStageDesc->Pipeline.pGraphicsDesc->pGeometryShader != nullptr)	? FShaderStageFlags::SHADER_STAGE_FLAG_GEOMETRY_SHADER	: 0;
			shaderStageMask |= (pRenderStageDesc->Pipeline.pGraphicsDesc->pHullShader != nullptr)		? FShaderStageFlags::SHADER_STAGE_FLAG_HULL_SHADER		: 0;
			shaderStageMask |= (pRenderStageDesc->Pipeline.pGraphicsDesc->pDomainShader != nullptr)		? FShaderStageFlags::SHADER_STAGE_FLAG_DOMAIN_SHADER	: 0;

			shaderStageMask |= FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER;
		}
		else if (pRenderStageDesc->PipelineType == EPipelineStateType::COMPUTE)
		{
			shaderStageMask |= FShaderStageFlags::SHADER_STAGE_FLAG_COMPUTE_SHADER;
		}
		else if (pRenderStageDesc->PipelineType == EPipelineStateType::RAY_TRACING)
		{
			shaderStageMask |= FShaderStageFlags::SHADER_STAGE_FLAG_RAYGEN_SHADER;
			shaderStageMask |= FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER;
			shaderStageMask |= FShaderStageFlags::SHADER_STAGE_FLAG_MISS_SHADER;
		}

		return shaderStageMask;
	}
}