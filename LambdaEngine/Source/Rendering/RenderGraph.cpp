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
		auto it = m_ResourceMap.find(desc.pName);

		if (it != m_ResourceMap.end())
		{
			const char* pResourceName = it->first;
			Resource* pResource = &it->second;

			switch (pResource->Type)
			{
			case EResourceType::PUSH_CONSTANTS:						UpdateResourcePushConstants(pResourceName, pResource, desc);					break;
			case EResourceType::INTERNAL_TEXTURE:					UpdateResourceInternalTexture(pResourceName, pResource, desc);					break;
			case EResourceType::INTERNAL_BUFFER:					UpdateResourceInternalBuffer(pResourceName, pResource, desc);					break;
			case EResourceType::INTERNAL_COLOR_ATTACHMENT:			UpdateResourceInternalColorAttachment(pResourceName, pResource, desc);			break;
			case EResourceType::INTERNAL_DEPTH_STENCIL_ATTACHMENT:	UpdateResourceInternalDepthStencilAttachment(pResourceName, pResource, desc);	break;
			case EResourceType::EXTERNAL_TEXTURE:					UpdateResourceExternalTexture(pResourceName, pResource, desc);					break;
			case EResourceType::EXTERNAL_BUFFER:					UpdateResourceExternalBuffer(pResourceName, pResource, desc);					break;
			case EResourceType::EXTERNAL_ACCELERATION_STRUCTURE:	UpdateResourceExternalAccelerationStructure(pResourceName, pResource, desc);	break;
			default:
				{
					LOG_WARNING("[RenderGraph]: Resource \"%s\" in Render Graph has unsupported Type", desc.pName);
					return;
				}
			}
		}
		else
		{
			LOG_WARNING("[RenderGraph]: Resource \"%s\" in Render Graph could not be found in Resource Map", desc.pName);
			return;
		}
	}

	void RenderGraph::Update()
	{
		if (m_DirtyDescriptorSetTextures.size() > 0)
		{
			//m_pGraphicsDevice->Wait();

			for (Resource* pResource : m_DirtyDescriptorSetTextures)
			{
				for (uint32 rb = 0; rb < pResource->ResourceBindings.size(); rb++)
				{
					ResourceBinding* pResourceBinding = &pResource->ResourceBindings[rb];

					pResourceBinding->pDescriptorSet->WriteSamplerDescriptors();
				}
			}

			m_DirtyDescriptorSetTextures.clear();
		}

		if (m_DirtyDescriptorSetBuffers.size() > 0)
		{
			//m_pGraphicsDevice->Wait();

			for (Resource* pResource : m_DirtyDescriptorSetBuffers)
			{
				for (uint32 rb = 0; rb < pResource->ResourceBindings.size(); rb++)
				{
					ResourceBinding* pResourceBinding = &pResource->ResourceBindings[rb];

					pResourceBinding->pDescriptorSet->WriteBufferDescriptors();
				}
			}

			m_DirtyDescriptorSetBuffers.clear();
		}

		if (m_DirtyDescriptorSetAccelerationStructures.size() > 0)
		{
			//m_pGraphicsDevice->Wait();

			for (Resource* pResource : m_DirtyDescriptorSetAccelerationStructures)
			{
				for (uint32 rb = 0; rb < pResource->ResourceBindings.size(); rb++)
				{
					ResourceBinding* pResourceBinding = &pResource->ResourceBindings[rb];

					pResourceBinding->pDescriptorSet->WriteAccelerationStructureDescriptors();
				}
			}

			m_DirtyDescriptorSetAccelerationStructures.clear();
		}
	}

	void RenderGraph::Render()
	{
		for (uint32 i = 0; i < m_PipelineStageCount; i++)
		{
			PipelineStage* pPipelineStage = &m_pPipelineStages[i];

			
		}
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
						else if (simpleType == ESimpleResourceType::COLOR_ATTACHMENT)
						{
							pResource->Type = EResourceType::INTERNAL_COLOR_ATTACHMENT;
						}
						else if (simpleType == ESimpleResourceType::DEPTH_STENCIL_ATTACHMENT)
						{
							pResource->Type = EResourceType::INTERNAL_DEPTH_STENCIL_ATTACHMENT;
						}
						else
						{
							LOG_ERROR("[RenderGraph]: Unsupported resource type for internal resource \"%s\"", pAttachment->pName);
							return false;
						}
					}
					else
					{
						//Externl
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

				pResource->Type							= EResourceType::PUSH_CONSTANTS;
				pResource->PushConstants.DataSize		= pResourceDesc->pPushConstantsDesc->DataSize;
				pResource->PushConstants.pData			= DBG_NEW byte[pResource->PushConstants.DataSize];
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
			std::vector<Resource*> descriptorSetResources;
			descriptorSetResources.reserve(pRenderStageDesc->AttachmentCount);
			std::vector<Resource*> renderPassResources;
			renderPassResources.reserve(pRenderStageDesc->AttachmentCount);

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
						descriptorBinding.ShaderStageMask		= pAttachment->ShaderStage;

						descriptorSetDescriptions.push_back(descriptorBinding);

						descriptorBindingIndex++;

						descriptorSetResources.push_back(&it->second);
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

						renderPassResources.push_back(&it->second);
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
				pRenderStageDesc->GraphicsPipeline.pGraphicsDesc->pPipelineLayout			= pRenderStage->pPipelineLayout;
				pRenderStageDesc->GraphicsPipeline.pGraphicsDesc->pBlendAttachmentStates	= renderPassBlendAttachmentStates.data();
				pRenderStageDesc->GraphicsPipeline.pGraphicsDesc->BlendAttachmentStateCount = renderPassBlendAttachmentStates.size();

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
					pRenderStageDesc->GraphicsPipeline.pGraphicsDesc->pRenderPass = pRenderPass;
				}

				pRenderStage->DrawType			= pRenderStageDesc->GraphicsPipeline.DrawType;
				pRenderStage->pPipelineState	= m_pGraphicsDevice->CreateGraphicsPipelineState(*pRenderStageDesc->GraphicsPipeline.pGraphicsDesc);
			}
			else if (pRenderStageDesc->PipelineType == EPipelineStateType::COMPUTE)
			{
				pRenderStageDesc->ComputePipeline.pComputeDesc->pPipelineLayout = pRenderStage->pPipelineLayout;

				pRenderStage->pPipelineState = m_pGraphicsDevice->CreateComputePipelineState(*pRenderStageDesc->ComputePipeline.pComputeDesc);
			}
			else if (pRenderStageDesc->PipelineType == EPipelineStateType::RAY_TRACING)
			{
				pRenderStageDesc->RayTracingPipeline.pRayTracingDesc->pPipelineLayout = pRenderStage->pPipelineLayout;

				pRenderStage->pPipelineState = m_pGraphicsDevice->CreateRayTracingPipelineState(*pRenderStageDesc->RayTracingPipeline.pRayTracingDesc);
			}

			//Link Attachment Resources to Render Stage (Descriptor Set)
			for (uint32 r = 0; r < descriptorSetResources.size(); r++)
			{
				Resource* pResource = descriptorSetResources[r];

				ResourceBinding resourceBinding = {};
				//resourceBinding.pDescriptorSet	= pDescriptorSet; //TODO
				resourceBinding.Binding			= r;

				pResource->ResourceBindings.push_back(resourceBinding);
			}

			//Link Render Stage to Push Constant Resource
			if (pRenderStageDesc->PushConstants.DataSize > 0)
			{
				auto it = m_ResourceMap.find(pRenderStageDesc->PushConstants.pName);

				if (it != m_ResourceMap.end())
				{
					pRenderStage->pPushConstantsResource = &it->second;
				}
				else
				{
					LOG_ERROR("[RenderGraph]: Push Constants resource found in Render Stage but not in Resource Map \"%s\"", pRenderStageDesc->PushConstants.pName);
					return false;
				}
			}

			//Link RenderPass to RenderPass Attachments
			if (renderPassResources.size() > 0)
			{
				if (pRenderStageDesc->PipelineType != EPipelineStateType::GRAPHICS)
				{
					LOG_ERROR("[RenderGraph]: There are resources that a RenderPass should be linked to, but Render Stage %u is not a Graphics Pipeline State", i);
					return false;
				}

				for (uint32 r = 0; r < renderPassResources.size(); r++)
				{
					Resource* pResource = descriptorSetResources[r];

					pRenderStage->pRenderPassResources.insert(pResource);
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

				auto it = m_ResourceMap.find(attachmentSynchronizationDesc.FromAttachment.pName);

				if (it == m_ResourceMap.end())
				{
					LOG_ERROR("[RenderGraph]: Resource found in Synchronization Stage but not in Resource Map \"%s\"", attachmentSynchronizationDesc.FromAttachment.pName);
					return false;
				}

				if (barrierType == ESimpleResourceType::TEXTURE)
				{
					PipelineTextureBarrier textureBarrier = {};
					textureBarrier.QueueBefore	= ConvertPipelineStateTypeToQueue(attachmentSynchronizationDesc.FromQueueOwner);
					textureBarrier.QueueAfter	= ConvertPipelineStateTypeToQueue(attachmentSynchronizationDesc.ToQueueOwner);
					textureBarrier.StateBefore	= ConvertAttachmentTypeToTextureState(attachmentSynchronizationDesc.FromAttachment.Type);
					textureBarrier.StateAfter	= ConvertAttachmentTypeToTextureState(attachmentSynchronizationDesc.ToAttachment.Type);

					TextureSynchronization textureSynchronizations = {};
					textureSynchronizations.SrcShaderStage		= attachmentSynchronizationDesc.FromAttachment.ShaderStage;
					textureSynchronizations.DstShaderStage		= attachmentSynchronizationDesc.ToAttachment.ShaderStage;
					textureSynchronizations.Barrier				= textureBarrier;

					pSynchronizationStage->TextureSynchronizations[attachmentSynchronizationDesc.FromAttachment.pName] = textureSynchronizations;

					it->second.Texture.Barriers.push_back(&textureBarrier);
				}
				else if (barrierType == ESimpleResourceType::BUFFER)
				{
					//Implement
					LOG_WARNING("[RenderGraph]: Barrier for Buffers has not been implemented");

					//it->second.Buffer.Barriers.push_back(&bufferBarrier);
				}
			}
		}

		return true;
	}

	bool RenderGraph::CreatePipelineStages(const std::vector<PipelineStageDesc>& pipelineStageDescriptions)
	{
		m_PipelineStageCount = pipelineStageDescriptions.size();
		m_pPipelineStages = DBG_NEW PipelineStage[m_PipelineStageCount];

		for (uint32 i = 0; i < m_PipelineStageCount; i++)
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

	void RenderGraph::UpdateResourcePushConstants(const char* pResourceName, Resource* pResource, const ResourceUpdateDesc& desc)
	{
		if (pResource->PushConstants.DataSize != desc.PushConstantUpdate.DataSize)
		{
			LOG_WARNING("[RenderGraph]: Resource \"%s\" could not be updated as the Data Size is not equivalent for Push Constants Resource", pResourceName);
			return;
		}

		memcpy(pResource->PushConstants.pData, desc.PushConstantUpdate.pData, pResource->PushConstants.DataSize);
	}

	void RenderGraph::UpdateResourceInternalTexture(const char* pResourceName, Resource* pResource, const ResourceUpdateDesc& desc)
	{
		//Update Texture
		{
			SAFEDELETE(pResource->Texture.pTexture);
			SAFEDELETE(pResource->Texture.pTextureView);
			pResource->Texture.pTexture		= m_pGraphicsDevice->CreateTexture(*desc.InternalTextureUpdate.pTextureDesc);
			pResource->Texture.pTextureView = m_pGraphicsDevice->CreateTextureView(*desc.InternalTextureUpdate.pTextureViewDesc);

			for (PipelineTextureBarrier* pTextureBarrier : pResource->Texture.Barriers)
			{
				pTextureBarrier->pTexture		= pResource->Texture.pTexture;
				pTextureBarrier->Miplevel		= 0;
				pTextureBarrier->MiplevelCount	= pResource->Texture.pTexture->GetDesc().Miplevels;
				pTextureBarrier->ArrayIndex		= 0;
				pTextureBarrier->ArrayCount		= pResource->Texture.pTexture->GetDesc().ArrayCount;
			}
		}

		//Update Sampler
		if (desc.InternalTextureUpdate.pSamplerDesc != nullptr)
		{
			SAFEDELETE(pResource->Texture.pSampler);
			pResource->Texture.pSampler		= m_pGraphicsDevice->CreateSampler(*desc.InternalTextureUpdate.pSamplerDesc);
		}

		m_DirtyDescriptorSetTextures.insert(pResource);
	}

	void RenderGraph::UpdateResourceInternalBuffer(const char* pResourceName, Resource* pResource, const ResourceUpdateDesc& desc)
	{
		//Update Buffer
		{
			SAFEDELETE(pResource->Buffer.pBuffer);
			pResource->Buffer.pBuffer = m_pGraphicsDevice->CreateBuffer(*desc.InternalBufferUpdate.pBufferDesc);

			//for (PipelineBufferBarrier* pBufferBarrier : pResource->Buffer.Barriers)
			{

			}
		}

		m_DirtyDescriptorSetBuffers.insert(pResource);
	}

	void RenderGraph::UpdateResourceInternalColorAttachment(const char* pResourceName, Resource* pResource, const ResourceUpdateDesc& desc)
	{
		//Update Texture
		{
			SAFEDELETE(pResource->Texture.pTexture);
			SAFEDELETE(pResource->Texture.pTextureView);
			pResource->Texture.pTexture		= m_pGraphicsDevice->CreateTexture(*desc.InternalTextureUpdate.pTextureDesc);
			pResource->Texture.pTextureView = m_pGraphicsDevice->CreateTextureView(*desc.InternalTextureUpdate.pTextureViewDesc);

			for (PipelineTextureBarrier* pTextureBarrier : pResource->Texture.Barriers)
			{
				pTextureBarrier->pTexture		= pResource->Texture.pTexture;
				pTextureBarrier->Miplevel		= 0;
				pTextureBarrier->MiplevelCount	= pResource->Texture.pTexture->GetDesc().Miplevels;
				pTextureBarrier->ArrayIndex		= 0;
				pTextureBarrier->ArrayCount		= pResource->Texture.pTexture->GetDesc().ArrayCount;
			}
		}
	}

	void RenderGraph::UpdateResourceInternalDepthStencilAttachment(const char* pResourceName, Resource* pResource, const ResourceUpdateDesc& desc)
	{
		//Update Texture
		{
			SAFEDELETE(pResource->Texture.pTexture);
			SAFEDELETE(pResource->Texture.pTextureView);
			pResource->Texture.pTexture		= m_pGraphicsDevice->CreateTexture(*desc.InternalTextureUpdate.pTextureDesc);
			pResource->Texture.pTextureView = m_pGraphicsDevice->CreateTextureView(*desc.InternalTextureUpdate.pTextureViewDesc);

			for (PipelineTextureBarrier* pTextureBarrier : pResource->Texture.Barriers)
			{
				pTextureBarrier->pTexture		= pResource->Texture.pTexture;
				pTextureBarrier->Miplevel		= 0;
				pTextureBarrier->MiplevelCount	= pResource->Texture.pTexture->GetDesc().Miplevels;
				pTextureBarrier->ArrayIndex		= 0;
				pTextureBarrier->ArrayCount		= pResource->Texture.pTexture->GetDesc().ArrayCount;
			}
		}
	}

	void RenderGraph::UpdateResourceExternalTexture(const char* pResourceName, Resource* pResource, const ResourceUpdateDesc& desc)
	{
		//Update Texture
		{
			pResource->Texture.pTexture		= desc.ExternalTextureUpdate.pTexture;
			pResource->Texture.pTextureView = desc.ExternalTextureUpdate.pTextureView;

			for (PipelineTextureBarrier* pTextureBarrier : pResource->Texture.Barriers)
			{
				pTextureBarrier->pTexture		= pResource->Texture.pTexture;
				pTextureBarrier->Miplevel		= 0;
				pTextureBarrier->MiplevelCount	= pResource->Texture.pTexture->GetDesc().Miplevels;
				pTextureBarrier->ArrayIndex		= 0;
				pTextureBarrier->ArrayCount		= pResource->Texture.pTexture->GetDesc().ArrayCount;
			}
		}

		//Update Sampler
		if (desc.ExternalTextureUpdate.pSampler != nullptr)
		{
			pResource->Texture.pSampler		= desc.ExternalTextureUpdate.pSampler;
		}

		m_DirtyDescriptorSetTextures.insert(pResource);
	}

	void RenderGraph::UpdateResourceExternalBuffer(const char* pResourceName, Resource* pResource, const ResourceUpdateDesc& desc)
	{
		//Update Buffer
		{
			pResource->Buffer.pBuffer = desc.ExternalBufferUpdate.pBuffer;

			//for (PipelineBufferBarrier* pBufferBarrier : pResource->Buffer.Barriers)
			{

			}
		}

		m_DirtyDescriptorSetBuffers.insert(pResource);
	}

	void RenderGraph::UpdateResourceExternalAccelerationStructure(const char* pResourceName, Resource* pResource, const ResourceUpdateDesc& desc)
	{
		//Update Acceleration Structure
		{
			pResource->AccelerationStructure.pTLAS = desc.ExternalAccelerationStructure.pTLAS;
		}

		m_DirtyDescriptorSetAccelerationStructures.insert(pResource);
	}

	void RenderGraph::ExecuteSynchronizationStage(SynchronizationStage* pSynchronizationStage, ICommandAllocator* pGraphicsCommandAllocator, ICommandList* pGraphicsCommandList, ICommandAllocator* pComputeCommandAllocator, ICommandList* pComputeCommandList)
	{
		pGraphicsCommandAllocator->Reset();
		pGraphicsCommandList->Reset();
		pGraphicsCommandList->Begin(nullptr);

		pComputeCommandAllocator->Reset();
		pComputeCommandList->Reset();
		pComputeCommandList->Begin(nullptr);

		for (auto it = pSynchronizationStage->TextureSynchronizations.begin(); it != pSynchronizationStage->TextureSynchronizations.end(); it++)
		{
			const TextureSynchronization* pTextureSynchronization = &it->second;

			const PipelineTextureBarrier* pBarrier = &pTextureSynchronization->Barrier;
			FPipelineStageFlags srcPipelineStage = ConvertShaderStageToPipelineStage(pTextureSynchronization->SrcShaderStage);
			FPipelineStageFlags dstPipelineStage = ConvertShaderStageToPipelineStage(pTextureSynchronization->SrcShaderStage);

			if (pBarrier->QueueBefore == ECommandQueueType::COMMAND_QUEUE_GRAPHICS)
			{
				if (pBarrier->QueueAfter == ECommandQueueType::COMMAND_QUEUE_COMPUTE)
				{
					//Graphics -> Compute
					pGraphicsCommandList->PipelineTextureBarriers(srcPipelineStage, FPipelineStageFlags::PIPELINE_STAGE_FLAG_BOTTOM, pBarrier, 1);
					pComputeCommandList->PipelineTextureBarriers(FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, dstPipelineStage, pBarrier, 1);
				}
				else if (pBarrier->QueueAfter == ECommandQueueType::COMMAND_QUEUE_GRAPHICS)
				{
					//Graphics -> Graphics
					pGraphicsCommandList->PipelineTextureBarriers(srcPipelineStage, dstPipelineStage, pBarrier, 1);
				}
			}
			else if (pBarrier->QueueBefore == ECommandQueueType::COMMAND_QUEUE_COMPUTE)
			{
				if (pBarrier->QueueAfter == ECommandQueueType::COMMAND_QUEUE_GRAPHICS)
				{
					//Compute -> Graphics
					pGraphicsCommandList->PipelineTextureBarriers(srcPipelineStage, FPipelineStageFlags::PIPELINE_STAGE_FLAG_BOTTOM, pBarrier, 1);
					pComputeCommandList->PipelineTextureBarriers(FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, dstPipelineStage, pBarrier, 1);
				}
				else if (pBarrier->QueueAfter == ECommandQueueType::COMMAND_QUEUE_COMPUTE)
				{
					//Compute -> Compute
					pComputeCommandList->PipelineTextureBarriers(srcPipelineStage, dstPipelineStage, pBarrier, 1);
				}
			}
		}

		//Buffer Synchronization

		pGraphicsCommandList->End();
		pComputeCommandList->End();
	}

	void RenderGraph::ExecuteGraphicsRenderStage(RenderStage* pRenderStage, ICommandAllocator* pGraphicsCommandAllocator, ICommandList* pGraphicsCommandList)
	{
		pGraphicsCommandAllocator->Reset();

		pGraphicsCommandList->Reset();
		pGraphicsCommandList->Begin(nullptr);

		if (pRenderStage->DrawType == EDrawType::SCENE_INDIRECT)
		{

		}
		else if (pRenderStage->DrawType == EDrawType::FULLSCREEN_QUAD)
		{

		}

		pGraphicsCommandList->End();
	}

	void RenderGraph::ExecuteComputeRenderStage(RenderStage* pRenderStage, ICommandAllocator* pComputeCommandAllocator, ICommandList* pComputeCommandList)
	{
	}

	void RenderGraph::ExecuteRayTracingRenderStage(RenderStage* pRenderStage, ICommandAllocator* pComputeCommandAllocator, ICommandList* pComputeCommandList)
	{
	}

	uint32 RenderGraph::CreateShaderStageMask(const RenderStageDesc* pRenderStageDesc)
	{
		uint32 shaderStageMask = 0;

		if (pRenderStageDesc->PipelineType == EPipelineStateType::GRAPHICS)
		{
			shaderStageMask |= (pRenderStageDesc->GraphicsPipeline.pGraphicsDesc->pMeshShader != nullptr)		? FShaderStageFlags::SHADER_STAGE_FLAG_MESH_SHADER		: 0;

			shaderStageMask |= (pRenderStageDesc->GraphicsPipeline.pGraphicsDesc->pVertexShader != nullptr)		? FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER	: 0;
			shaderStageMask |= (pRenderStageDesc->GraphicsPipeline.pGraphicsDesc->pGeometryShader != nullptr)	? FShaderStageFlags::SHADER_STAGE_FLAG_GEOMETRY_SHADER	: 0;
			shaderStageMask |= (pRenderStageDesc->GraphicsPipeline.pGraphicsDesc->pHullShader != nullptr)		? FShaderStageFlags::SHADER_STAGE_FLAG_HULL_SHADER		: 0;
			shaderStageMask |= (pRenderStageDesc->GraphicsPipeline.pGraphicsDesc->pDomainShader != nullptr)		? FShaderStageFlags::SHADER_STAGE_FLAG_DOMAIN_SHADER	: 0;

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