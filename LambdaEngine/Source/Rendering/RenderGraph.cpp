#include "Rendering/RenderGraph.h"
#include "Rendering/ICustomRenderer.h"
#include "Rendering/ImGuiRenderer.h"

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
#include "Rendering/Core/API/ISampler.h"
#include "Rendering/Core/API/ITextureView.h"
#include "Rendering/Core/API/ICommandQueue.h"
#include "Rendering/Core/API/IFence.h"
#include "Rendering/Core/API/IShader.h"

#include "Rendering/RenderSystem.h"
#include "Rendering/PipelineStateManager.h"

#include "Game/Scene.h"

#include "Log/Log.h"

#include "Application/API/CommonApplication.h"

namespace LambdaEngine
{
	constexpr const uint32 SAME_QUEUE_BACK_BUFFER_SYNCHRONIZATION_INDEX		= 0;
	constexpr const uint32 SAME_QUEUE_TEXTURE_SYNCHRONIZATION_INDEX			= 1;
	constexpr const uint32 OTHER_QUEUE_BACK_BUFFER_SYNCHRONIZATION_INDEX	= 2;
	constexpr const uint32 OTHER_QUEUE_TEXTURE_SYNCHRONIZATION_INDEX		= 3;

	constexpr const uint32 SAME_QUEUE_BUFFER_SYNCHRONIZATION_INDEX			= 0;
	constexpr const uint32 OTHER_QUEUE_BUFFER_SYNCHRONIZATION_INDEX			= 1;

	RenderGraph::RenderGraph(const IGraphicsDevice* pGraphicsDevice) :
		m_pGraphicsDevice(pGraphicsDevice)
	{
		CommonApplication::Get()->AddEventHandler(this);
	}

	RenderGraph::~RenderGraph()
	{
        m_pFence->Wait(m_SignalValue - 1, UINT64_MAX);
        SAFERELEASE(m_pFence);

        SAFERELEASE(m_pDescriptorHeap);
		SAFEDELETE_ARRAY(m_ppExecutionStages);

		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			SAFERELEASE(m_ppGraphicsCopyCommandAllocators[b]);
			SAFERELEASE(m_ppGraphicsCopyCommandLists[b]);

			SAFERELEASE(m_ppComputeCopyCommandAllocators[b]);
			SAFERELEASE(m_ppComputeCopyCommandLists[b]);
		}

		SAFEDELETE_ARRAY(m_ppGraphicsCopyCommandAllocators);
		SAFEDELETE_ARRAY(m_ppGraphicsCopyCommandLists);

		SAFEDELETE_ARRAY(m_ppComputeCopyCommandAllocators);
		SAFEDELETE_ARRAY(m_ppComputeCopyCommandLists);

		for (auto it = m_ResourceMap.begin(); it != m_ResourceMap.end(); it++)
		{
			Resource* pResource = &it->second;

			if (pResource->OwnershipType == EResourceOwnershipType::INTERNAL)
			{
				if (pResource->Type == ERenderGraphResourceType::BUFFER)
				{
					for (uint32 sr = 0; sr < pResource->SubResourceCount; sr++)
					{
						SAFERELEASE(pResource->Buffer.Buffers[sr]);
					}
				}
				else if (pResource->Type == ERenderGraphResourceType::TEXTURE)
				{
					for (uint32 sr = 0; sr < pResource->SubResourceCount; sr++)
					{
						SAFERELEASE(pResource->Texture.Textures[sr]);
						SAFERELEASE(pResource->Texture.TextureViews[sr]);
						SAFERELEASE(pResource->Texture.Samplers[sr]);
					}
				}
			}
		}

		for (uint32 i = 0; i < m_PipelineStageCount; i++)
		{
			PipelineStage* pPipelineStage = &m_pPipelineStages[i];

			for (uint32 b = 0; b < m_BackBufferCount; b++)
			{
				SAFERELEASE(pPipelineStage->ppComputeCommandAllocators[b]);
				SAFERELEASE(pPipelineStage->ppGraphicsCommandAllocators[b]);
				SAFERELEASE(pPipelineStage->ppComputeCommandLists[b]);
				SAFERELEASE(pPipelineStage->ppGraphicsCommandLists[b]);
			}

			SAFEDELETE_ARRAY(pPipelineStage->ppComputeCommandAllocators);
			SAFEDELETE_ARRAY(pPipelineStage->ppGraphicsCommandAllocators);
			SAFEDELETE_ARRAY(pPipelineStage->ppComputeCommandLists);
			SAFEDELETE_ARRAY(pPipelineStage->ppGraphicsCommandLists);

			if (pPipelineStage->Type == ERenderGraphPipelineStageType::RENDER)
			{
				RenderStage* pRenderStage = &m_pRenderStages[pPipelineStage->StageIndex];

				for (uint32 b = 0; b < m_BackBufferCount; b++)
				{
					if (pRenderStage->ppTextureDescriptorSets != nullptr)
					{
						for (uint32 s = 0; s < pRenderStage->TextureSubDescriptorSetCount; s++)
						{
							SAFERELEASE(pRenderStage->ppTextureDescriptorSets[b * pRenderStage->TextureSubDescriptorSetCount + s]);
						}
					}

					if (pRenderStage->ppBufferDescriptorSets != nullptr)
						SAFERELEASE(pRenderStage->ppBufferDescriptorSets[b]);
				}
				
				SAFEDELETE_ARRAY(pRenderStage->ppTextureDescriptorSets);
				SAFEDELETE_ARRAY(pRenderStage->ppBufferDescriptorSets);
				SAFERELEASE(pRenderStage->pPipelineLayout);
				SAFERELEASE(pRenderStage->pRenderPass);
				PipelineStateManager::ReleasePipelineState(pRenderStage->PipelineStateID);
			}
			else if (pPipelineStage->Type == ERenderGraphPipelineStageType::SYNCHRONIZATION)
			{
				SynchronizationStage* pSynchronizationStage = &m_pSynchronizationStages[pPipelineStage->StageIndex];

				UNREFERENCED_VARIABLE(pSynchronizationStage);
			}
		}

		SAFEDELETE_ARRAY(m_pRenderStages);
		SAFEDELETE_ARRAY(m_pSynchronizationStages);
		SAFEDELETE_ARRAY(m_pPipelineStages);

		for (uint32 r = 0; r < m_DebugRenderers.size(); r++)
		{
			SAFEDELETE(m_DebugRenderers[r]);
		}

		m_DebugRenderers.clear();
	}

	bool RenderGraph::Init(const RenderGraphDesc* pDesc)
	{
		m_pScene						= pDesc->pScene;
		m_BackBufferCount				= pDesc->BackBufferCount;
		m_MaxTexturesPerDescriptorSet	= pDesc->MaxTexturesPerDescriptorSet;

		/*if (!RenderGraphDescriptionParser::Parse(pDesc, renderStageDescriptions, synchronizationStageDescriptions, pipelineStageDescriptions, resourceDescriptions))
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" could not be parsed", pDesc->pName);
			return false;
		}*/

		if (!CreateFence())
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Fence", pDesc->Name.c_str());
			return false;
		}

		if (!CreateDescriptorHeap())
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Descriptor Heap", pDesc->Name.c_str());
			return false;
		}

		if (!CreateCopyCommandLists())
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Copy Command Lists", pDesc->Name.c_str());
			return false;
		}

		if (!CreateResources(pDesc->pRenderGraphStructureDesc->ResourceDescriptions))
		{ 
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Resources", pDesc->Name.c_str());
			return false;
		}

		if (!CreateRenderStages(pDesc->pRenderGraphStructureDesc->RenderStageDescriptions))
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Render Stages", pDesc->Name.c_str());
			return false;
		}

		if (!CreateSynchronizationStages(pDesc->pRenderGraphStructureDesc->SynchronizationStageDescriptions))
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Synchronization Stages", pDesc->Name.c_str());
			return false;
		}

		if (!CreatePipelineStages(pDesc->pRenderGraphStructureDesc->PipelineStageDescriptions))
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Pipeline Stages", pDesc->Name.c_str());
			return false;
		}

		return true;
	}

	void RenderGraph::UpdateResource(const ResourceUpdateDesc& desc)
	{
		auto it = m_ResourceMap.find(desc.ResourceName);

		if (it != m_ResourceMap.end())
		{
			Resource* pResource = &it->second;

			switch (pResource->Type)
			{
				case ERenderGraphResourceType::TEXTURE:					UpdateResourceTexture(pResource, desc);					break;
				case ERenderGraphResourceType::BUFFER:					UpdateResourceBuffer(pResource, desc);					break;
				case ERenderGraphResourceType::ACCELERATION_STRUCTURE:	UpdateResourceAccelerationStructure(pResource, desc);	break;
				default:
				{
					LOG_WARNING("[RenderGraph]: Resource \"%s\" in Render Graph has unsupported Type", desc.ResourceName.c_str());
					return;
				}
			}
		}
		else
		{
			LOG_WARNING("[RenderGraph]: Resource \"%s\" in Render Graph could not be found in Resource Map", desc.ResourceName.c_str());
			return;
		}
	}

	void RenderGraph::UpdateRenderStageDimensions(const String& renderStageName, uint32 x, uint32 y, uint32 z)
	{
		auto it = m_RenderStageMap.find(renderStageName);

		if (it != m_RenderStageMap.end())
		{
			RenderStage* pRenderStage = &m_pRenderStages[it->second];

			if (pRenderStage->Parameters.XDimType == ERenderStageDimensionType::EXTERNAL) pRenderStage->Dimensions.x = x;
			if (pRenderStage->Parameters.YDimType == ERenderStageDimensionType::EXTERNAL) pRenderStage->Dimensions.y = y;
			if (pRenderStage->Parameters.ZDimType == ERenderStageDimensionType::EXTERNAL) pRenderStage->Dimensions.z = z;
		}
		else
		{
			LOG_WARNING("[RenderGraph]: UpdateRenderStageParameters failed, render stage with name \"%s\" could not be found", renderStageName.c_str());
			return;
		}
	}

	void RenderGraph::GetAndIncrementFence(IFence** ppFence, uint64* pSignalValue)
	{
		(*pSignalValue) = m_SignalValue++;
		(*ppFence) = m_pFence;
	}

	void RenderGraph::Update()
	{
		if (m_DirtyDescriptorSetBuffers.size()					> 0 || 
			m_DirtyDescriptorSetAccelerationStructures.size()	> 0)
		{
			//Copy old descriptor set and replace old with copy, then write into the new copy
			for (uint32 r = 0; r < m_RenderStageCount; r++)
			{
				RenderStage* pRenderStage		= &m_pRenderStages[r];

				if (pRenderStage->ppBufferDescriptorSets != nullptr)
				{
					for (uint32 b = 0; b < m_BackBufferCount; b++)
					{
						IDescriptorSet* pDescriptorSet = m_pGraphicsDevice->CreateDescriptorSet("Render Stage Buffer Descriptor Set", pRenderStage->pPipelineLayout, 0, m_pDescriptorHeap);
						m_pGraphicsDevice->CopyDescriptorSet(pRenderStage->ppBufferDescriptorSets[b], pDescriptorSet);
						SAFERELEASE(pRenderStage->ppBufferDescriptorSets[b]);
						pRenderStage->ppBufferDescriptorSets[b] = pDescriptorSet;
					}
				}
				else if (pRenderStage->UsesCustomRenderer)
				{
					pRenderStage->pCustomRenderer->PreBuffersDescriptorSetWrite();
				}
			}

			if (m_DirtyDescriptorSetBuffers.size() > 0)
			{
				for (Resource* pResource : m_DirtyDescriptorSetBuffers)
				{
					for (uint32 rb = 0; rb < pResource->ResourceBindings.size(); rb++)
					{
						ResourceBinding* pResourceBinding = &pResource->ResourceBindings[rb];
						RenderStage* pRenderStage = pResourceBinding->pRenderStage;

						if (pRenderStage->UsesCustomRenderer)
						{
							pRenderStage->pCustomRenderer->UpdateBufferResource(
								pResource->Name,
								pResource->Buffer.Buffers.data(),
								pResource->Buffer.Offsets.data(),
								pResource->Buffer.SizesInBytes.data(),
								pResource->SubResourceCount);
						}
						else if (pResourceBinding->DescriptorType != EDescriptorType::DESCRIPTOR_UNKNOWN)
						{
							for (uint32 b = 0; b < m_BackBufferCount; b++)
							{
								pResourceBinding->pRenderStage->ppBufferDescriptorSets[b]->WriteBufferDescriptors(
									pResource->Buffer.Buffers.data(),
									pResource->Buffer.Offsets.data(),
									pResource->Buffer.SizesInBytes.data(),
									pResourceBinding->Binding,
									pResource->SubResourceCount,
									pResourceBinding->DescriptorType);
							}
						}
					}
				}

				m_DirtyDescriptorSetBuffers.clear();
			}

			//Acceleration Structures
			if (m_DirtyDescriptorSetAccelerationStructures.size() > 0)
			{
				for (Resource* pResource : m_DirtyDescriptorSetAccelerationStructures)
				{
					ResourceBinding* pResourceBinding = &pResource->ResourceBindings[0]; //Assume only one acceleration structure
					RenderStage* pRenderStage = pResourceBinding->pRenderStage;

					if (pRenderStage->UsesCustomRenderer)
					{
						pRenderStage->pCustomRenderer->UpdateAccelerationStructureResource(
							pResource->Name,
							pResource->AccelerationStructure.pTLAS);
					}
					else if (pResourceBinding->DescriptorType != EDescriptorType::DESCRIPTOR_UNKNOWN)
					{
						for (uint32 b = 0; b < m_BackBufferCount; b++)
						{
							pResourceBinding->pRenderStage->ppBufferDescriptorSets[b]->WriteAccelerationStructureDescriptors(
								&pResource->AccelerationStructure.pTLAS,
								pResourceBinding->Binding,
								1);
						}
					}
				}

				m_DirtyDescriptorSetAccelerationStructures.clear();
			}
		}

		if (m_DirtyDescriptorSetTextures.size() > 0)
		{
			//Copy old descriptor set and replace old with copy, then write into the new copy
			for (uint32 r = 0; r < m_RenderStageCount; r++)
			{
				RenderStage* pRenderStage = &m_pRenderStages[r];

				if (pRenderStage->ppTextureDescriptorSets != nullptr)
				{
					for (uint32 b = 0; b < m_BackBufferCount; b++)
					{
						for (uint32 s = 0; s < pRenderStage->TextureSubDescriptorSetCount; s++)
						{
							uint32 descriptorSetIndex = b * pRenderStage->TextureSubDescriptorSetCount + s;
							IDescriptorSet* pDescriptorSet = m_pGraphicsDevice->CreateDescriptorSet("Render Stage Texture Descriptor Set", pRenderStage->pPipelineLayout, pRenderStage->ppBufferDescriptorSets != nullptr ? 1 : 0, m_pDescriptorHeap);
							m_pGraphicsDevice->CopyDescriptorSet(pRenderStage->ppTextureDescriptorSets[descriptorSetIndex], pDescriptorSet);
							SAFERELEASE(pRenderStage->ppTextureDescriptorSets[descriptorSetIndex]);
							pRenderStage->ppTextureDescriptorSets[descriptorSetIndex] = pDescriptorSet;
						}
					}
				}
				else if (pRenderStage->UsesCustomRenderer)
				{
					pRenderStage->pCustomRenderer->PreTexturesDescriptorSetWrite();
				}
			}

			for (Resource* pResource : m_DirtyDescriptorSetTextures)
			{
				for (uint32 rb = 0; rb < pResource->ResourceBindings.size(); rb++)
				{
					ResourceBinding* pResourceBinding = &pResource->ResourceBindings[rb];
					RenderStage* pRenderStage = pResourceBinding->pRenderStage;

					if (pRenderStage->UsesCustomRenderer)
					{
						pRenderStage->pCustomRenderer->UpdateTextureResource(
							pResource->Name,
							pResource->Texture.TextureViews.data(),
							pResource->SubResourceCount);
					}
					else if (pResourceBinding->DescriptorType != EDescriptorType::DESCRIPTOR_UNKNOWN)
					{
						if (pResource->IsBackBuffer)
						{
							for (uint32 b = 0; b < m_BackBufferCount; b++)
							{
								pRenderStage->ppTextureDescriptorSets[b]->WriteTextureDescriptors(
									&pResource->Texture.TextureViews[b],
									&pResource->Texture.Samplers[b],
									pResourceBinding->TextureState,
									pResourceBinding->Binding,
									1,
									pResourceBinding->DescriptorType);
							}
						}
						else
						{
							uint32 actualSubResourceCount = pResource->SubResourceCount / pRenderStage->TextureSubDescriptorSetCount;

							for (uint32 b = 0; b < m_BackBufferCount; b++)
							{
								for (uint32 s = 0; s < pRenderStage->TextureSubDescriptorSetCount; s++)
								{
									uint32 index = b * pRenderStage->TextureSubDescriptorSetCount + s;

									uint32 descriptorSetIndex = b * pRenderStage->TextureSubDescriptorSetCount + s;
									uint32 subResourceIndex = s * actualSubResourceCount;

									pRenderStage->ppTextureDescriptorSets[descriptorSetIndex]->WriteTextureDescriptors(
										&pResource->Texture.TextureViews[subResourceIndex],
										&pResource->Texture.Samplers[subResourceIndex],
										pResourceBinding->TextureState,
										pResourceBinding->Binding,
										actualSubResourceCount,
										pResourceBinding->DescriptorType);
								}
							}
						}
					}
				}
			}

			m_DirtyDescriptorSetTextures.clear();
		}
	}

	void RenderGraph::NewFrame(Timestamp delta)
	{
		for (uint32 r = 0; r < (uint32)m_CustomRenderers.size(); r++)
		{
			ICustomRenderer* pCustomRenderer = m_CustomRenderers[r];
			pCustomRenderer->NewFrame(delta);
		}
	}

	void RenderGraph::PrepareRender(Timestamp delta)
	{
		for (uint32 r = 0; r < (uint32)m_CustomRenderers.size(); r++)
		{
			ICustomRenderer* pCustomRenderer = m_CustomRenderers[r];
			pCustomRenderer->PrepareRender(delta);
		}
	}

	void RenderGraph::Render(uint64 modFrameIndex, uint32 backBufferIndex)
	{
		m_ModFrameIndex		= modFrameIndex;
		m_BackBufferIndex	= backBufferIndex;

		ZERO_MEMORY(m_ppExecutionStages, m_ExecutionStageCount * sizeof(ICommandList*));

		uint32 currentExecutionStage = 0;

		if (m_SignalValue > 3)
			m_pFence->Wait(m_SignalValue - 3, UINT64_MAX);

		for (uint32 p = 0; p < m_PipelineStageCount; p++)
		{
			//Seperate Thread
			{
				PipelineStage* pPipelineStage = &m_pPipelineStages[p];

				if (pPipelineStage->Type == ERenderGraphPipelineStageType::RENDER)
				{
					RenderStage* pRenderStage = &m_pRenderStages[pPipelineStage->StageIndex];
					IPipelineState* pPipelineState = PipelineStateManager::GetPipelineState(pRenderStage->PipelineStateID);
					EPipelineStateType stateType = pPipelineState->GetType();

					if (pRenderStage->UsesCustomRenderer)
					{
						ICustomRenderer* pCustomRenderer = pRenderStage->pCustomRenderer;

						switch (stateType)
						{
						case EPipelineStateType::GRAPHICS:		pCustomRenderer->Render(pPipelineStage->ppGraphicsCommandAllocators[m_ModFrameIndex],	pPipelineStage->ppGraphicsCommandLists[m_ModFrameIndex],	&m_ppExecutionStages[currentExecutionStage], m_ModFrameIndex, m_BackBufferIndex); break;
						case EPipelineStateType::COMPUTE:		pCustomRenderer->Render(pPipelineStage->ppComputeCommandAllocators[m_ModFrameIndex],	pPipelineStage->ppComputeCommandLists[m_ModFrameIndex],		&m_ppExecutionStages[currentExecutionStage], m_ModFrameIndex, m_BackBufferIndex); break;
						case EPipelineStateType::RAY_TRACING:	pCustomRenderer->Render(pPipelineStage->ppComputeCommandAllocators[m_ModFrameIndex],	pPipelineStage->ppComputeCommandLists[m_ModFrameIndex],		&m_ppExecutionStages[currentExecutionStage], m_ModFrameIndex, m_BackBufferIndex); break;
						}
					}
					else
					{
						switch (stateType)
						{
						case EPipelineStateType::GRAPHICS:		ExecuteGraphicsRenderStage(pRenderStage,	pPipelineState, pPipelineStage->ppGraphicsCommandAllocators[m_ModFrameIndex],		pPipelineStage->ppGraphicsCommandLists[m_ModFrameIndex],	&m_ppExecutionStages[currentExecutionStage]);	break;
						case EPipelineStateType::COMPUTE:		ExecuteComputeRenderStage(pRenderStage,		pPipelineState, pPipelineStage->ppComputeCommandAllocators[m_ModFrameIndex],		pPipelineStage->ppComputeCommandLists[m_ModFrameIndex],		&m_ppExecutionStages[currentExecutionStage]);	break;
						case EPipelineStateType::RAY_TRACING:	ExecuteRayTracingRenderStage(pRenderStage,	pPipelineState, pPipelineStage->ppComputeCommandAllocators[m_ModFrameIndex],		pPipelineStage->ppComputeCommandLists[m_ModFrameIndex],		&m_ppExecutionStages[currentExecutionStage]);	break;
						}
					}
					currentExecutionStage++;
				}
				else if (pPipelineStage->Type == ERenderGraphPipelineStageType::SYNCHRONIZATION)
				{
					SynchronizationStage* pSynchronizationStage = &m_pSynchronizationStages[pPipelineStage->StageIndex];

					ExecuteSynchronizationStage(
						pSynchronizationStage,
						pPipelineStage->ppGraphicsCommandAllocators[m_ModFrameIndex],
						pPipelineStage->ppGraphicsCommandLists[m_ModFrameIndex],
						pPipelineStage->ppComputeCommandAllocators[m_ModFrameIndex],
						pPipelineStage->ppComputeCommandLists[m_ModFrameIndex],
						&m_ppExecutionStages[currentExecutionStage],
						&m_ppExecutionStages[currentExecutionStage + 1]);

					currentExecutionStage += 2;
				}
			}
		}

		//Execute Copy Command Lists
		{
			if (m_ExecuteGraphicsCopy)
			{
				m_ExecuteGraphicsCopy = false;

				ICommandList* pGraphicsCopyCommandList = m_ppGraphicsCopyCommandLists[m_ModFrameIndex];

				pGraphicsCopyCommandList->End();
				RenderSystem::GetGraphicsQueue()->ExecuteCommandLists(&pGraphicsCopyCommandList, 1, FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, m_pFence, m_SignalValue - 1, m_pFence, m_SignalValue);
				m_SignalValue++;
			}

			if (m_ExecuteComputeCopy)
			{
				m_ExecuteComputeCopy = false;

				ICommandList* pComputeCopyCommandList = m_ppComputeCopyCommandLists[m_ModFrameIndex];

				pComputeCopyCommandList->End();
				RenderSystem::GetComputeQueue()->ExecuteCommandLists(&pComputeCopyCommandList, 1, FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, m_pFence, m_SignalValue - 1, m_pFence, m_SignalValue);
				m_SignalValue++;
			}
		}

		//Wait Threads

		//Execute the recorded Command Lists, we do this in a Batched mode where we batch as many "same queue" command lists that execute in succession together. This reduced the overhead caused by QueueSubmit
		{
			//This is safe since the first Execution Stage should never be nullptr
			ECommandQueueType currentBatchType = m_ppExecutionStages[0]->GetType();

			static TArray<ICommandList*> currentBatch;

			for (uint32 e = 0; e < m_ExecutionStageCount; e++)
			{
				ICommandList* pCommandList = m_ppExecutionStages[e];

				if (pCommandList != nullptr)
				{
					ECommandQueueType currentType = pCommandList->GetType();

					if (currentType != currentBatchType)
					{
						if (currentBatchType == ECommandQueueType::COMMAND_QUEUE_GRAPHICS)
						{
							RenderSystem::GetGraphicsQueue()->ExecuteCommandLists(currentBatch.data(), currentBatch.size(), FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, m_pFence, m_SignalValue - 1, m_pFence, m_SignalValue);
						}
						else if (currentBatchType == ECommandQueueType::COMMAND_QUEUE_COMPUTE)
						{
							RenderSystem::GetComputeQueue()->ExecuteCommandLists(currentBatch.data(), currentBatch.size(), FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, m_pFence, m_SignalValue - 1, m_pFence, m_SignalValue);
						}

						m_SignalValue++;
						currentBatch.clear();

						currentBatchType = currentType;
					}

					currentBatch.push_back(pCommandList);
				}
			}

			if (!currentBatch.empty())
			{
				if (currentBatchType == ECommandQueueType::COMMAND_QUEUE_GRAPHICS)
				{
					RenderSystem::GetGraphicsQueue()->ExecuteCommandLists(currentBatch.data(), currentBatch.size(), FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, m_pFence, m_SignalValue - 1, m_pFence, m_SignalValue);
				}
				else if (currentBatchType == ECommandQueueType::COMMAND_QUEUE_COMPUTE)
				{
					RenderSystem::GetComputeQueue()->ExecuteCommandLists(currentBatch.data(), currentBatch.size(), FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, m_pFence, m_SignalValue - 1, m_pFence, m_SignalValue);
				}

				m_SignalValue++;
				currentBatch.clear();
			}
		}
	}

	bool RenderGraph::GetResourceTextures(const char* pResourceName, ITexture* const ** pppTexture, uint32* pTextureView) const
	{
		auto it = m_ResourceMap.find(pResourceName);

		if (it != m_ResourceMap.end())
		{
			(*pppTexture)		= it->second.Texture.Textures.data();
			(*pTextureView)		= (uint32)it->second.Texture.Textures.size();
			return true;
		}

		return false;
	}

	bool RenderGraph::GetResourceTextureViews(const char* pResourceName, ITextureView* const ** pppTextureViews, uint32* pTextureViewCount) const
	{
		auto it = m_ResourceMap.find(pResourceName);

		if (it != m_ResourceMap.end())
		{
			(*pppTextureViews)		= it->second.Texture.TextureViews.data();
			(*pTextureViewCount)	= (uint32)it->second.Texture.TextureViews.size();
			return true;
		}

		return false;
	}

	bool RenderGraph::GetResourceBuffers(const char* pResourceName, IBuffer* const ** pppBuffers, uint32* pBufferCount) const
	{
		auto it = m_ResourceMap.find(pResourceName);

		if (it != m_ResourceMap.end())
		{
			(*pppBuffers)			= it->second.Buffer.Buffers.data();
			(*pBufferCount)			= (uint32)it->second.Buffer.Buffers.size();
			return true;
		}

		return false;
	}

	bool RenderGraph::GetResourceAccelerationStructure(const char* pResourceName, IAccelerationStructure const ** ppAccelerationStructure) const
	{
		auto it = m_ResourceMap.find(pResourceName);

		if (it != m_ResourceMap.end())
		{
			(*ppAccelerationStructure) = it->second.AccelerationStructure.pTLAS;
			return true;
		}

		return false;
	}

	void RenderGraph::OnWindowResized(Window* pWindow, uint16 width, uint16 height, EResizeType type)
	{
		m_WindowWidth	= (float32)width;
		m_WindowHeight	= (float32)height;

		for (auto relativeRenderStageIt = m_WindowRelativeRenderStages.begin(); relativeRenderStageIt != m_WindowRelativeRenderStages.end(); relativeRenderStageIt++)
		{
			RenderStage* pRenderStage = &m_pRenderStages[*relativeRenderStageIt];

			UpdateRelativeRenderStageDimensions(pRenderStage);
		}
	}

	bool RenderGraph::CreateFence()
	{
		FenceDesc fenceDesc = {};
		fenceDesc.pName			= "Render Stage Fence";
		fenceDesc.InitalValue	= 0;

		m_pFence = m_pGraphicsDevice->CreateFence(&fenceDesc);

		if (m_pFence == nullptr)
		{
			LOG_ERROR("[RenderGraph]: Could not create RenderGraph fence");
			return false;
		}

		return true;
	}

	bool RenderGraph::CreateDescriptorHeap()
	{
		constexpr uint32 DESCRIPTOR_COUNT = 1024;

		DescriptorCountDesc descriptorCountDesc = { };
		descriptorCountDesc.DescriptorSetCount							= DESCRIPTOR_COUNT;
		descriptorCountDesc.SamplerDescriptorCount						= DESCRIPTOR_COUNT;
		descriptorCountDesc.TextureDescriptorCount						= DESCRIPTOR_COUNT;
		descriptorCountDesc.TextureCombinedSamplerDescriptorCount		= DESCRIPTOR_COUNT;
		descriptorCountDesc.ConstantBufferDescriptorCount				= DESCRIPTOR_COUNT;
		descriptorCountDesc.UnorderedAccessBufferDescriptorCount		= DESCRIPTOR_COUNT;
		descriptorCountDesc.UnorderedAccessTextureDescriptorCount		= DESCRIPTOR_COUNT;
		descriptorCountDesc.AccelerationStructureDescriptorCount		= DESCRIPTOR_COUNT;

		DescriptorHeapDesc descriptorHeapDesc = { };
		descriptorHeapDesc.pName			= "Render Graph Descriptor Heap";
		descriptorHeapDesc.DescriptorCount	= descriptorCountDesc;

		m_pDescriptorHeap = m_pGraphicsDevice->CreateDescriptorHeap(&descriptorHeapDesc);

		return m_pDescriptorHeap != nullptr;
	}

	bool RenderGraph::CreateCopyCommandLists()
	{
		m_ppGraphicsCopyCommandAllocators		= DBG_NEW ICommandAllocator*[m_BackBufferCount];
		m_ppGraphicsCopyCommandLists			= DBG_NEW ICommandList*[m_BackBufferCount];
		m_ppComputeCopyCommandAllocators		= DBG_NEW ICommandAllocator*[m_BackBufferCount];
		m_ppComputeCopyCommandLists				= DBG_NEW ICommandList *[m_BackBufferCount];

		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			//Graphics
			{
				m_ppGraphicsCopyCommandAllocators[b]		= m_pGraphicsDevice->CreateCommandAllocator("Render Graph Graphics Copy Command Allocator", ECommandQueueType::COMMAND_QUEUE_GRAPHICS);

				if (m_ppGraphicsCopyCommandAllocators[b] == nullptr)
				{
					return false;
				}

				CommandListDesc graphicsCopyCommandListDesc = {};
				graphicsCopyCommandListDesc.pName				= "Render Graph Graphics Copy Command List";
				graphicsCopyCommandListDesc.CommandListType		= ECommandListType::COMMAND_LIST_PRIMARY;
				graphicsCopyCommandListDesc.Flags				= FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

				m_ppGraphicsCopyCommandLists[b]			= m_pGraphicsDevice->CreateCommandList(m_ppGraphicsCopyCommandAllocators[b], &graphicsCopyCommandListDesc);

				if (m_ppGraphicsCopyCommandLists[b] == nullptr)
				{
					return false;
				}
			}

			//Compute
			{
				m_ppComputeCopyCommandAllocators[b] = m_pGraphicsDevice->CreateCommandAllocator("Render Graph Compute Copy Command Allocator", ECommandQueueType::COMMAND_QUEUE_COMPUTE);

				if (m_ppComputeCopyCommandAllocators[b] == nullptr)
				{
					return false;
				}

				CommandListDesc computeCopyCommandListDesc = {};
				computeCopyCommandListDesc.pName				= "Render Graph Compute Copy Command List";
				computeCopyCommandListDesc.CommandListType		= ECommandListType::COMMAND_LIST_PRIMARY;
				computeCopyCommandListDesc.Flags				= FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

				m_ppComputeCopyCommandLists[b] = m_pGraphicsDevice->CreateCommandList(m_ppComputeCopyCommandAllocators[b], &computeCopyCommandListDesc);

				if (m_ppComputeCopyCommandLists[b] == nullptr)
				{
					return false;
				}
			}
		}

		return true;
	}

	bool RenderGraph::CreateResources(const TArray<RenderGraphResourceDesc>& resources)
	{
		m_ResourceMap.reserve(resources.size());

		for (uint32 i = 0; i < resources.size(); i++)
		{
			const RenderGraphResourceDesc* pResourceDesc = &resources[i];

			Resource* pResource = &m_ResourceMap[pResourceDesc->Name];

			pResource->Name				= pResourceDesc->Name;
			pResource->IsBackBuffer		= pResourceDesc->Name == RENDER_GRAPH_BACK_BUFFER_ATTACHMENT;

			if (pResource->IsBackBuffer)
			{
				pResource->SubResourceCount = m_BackBufferCount;
			}
			else
			{
				pResource->SubResourceCount = pResourceDesc->SubResourceCount;
			}
					
			if (!pResourceDesc->External)
			{
				//Internal
				if (pResourceDesc->Type == ERenderGraphResourceType::TEXTURE)
				{
					

					pResource->Type					= ERenderGraphResourceType::TEXTURE;
					pResource->OwnershipType		= pResource->IsBackBuffer ? EResourceOwnershipType::EXTERNAL : EResourceOwnershipType::INTERNAL;
					pResource->Texture.Format		= pResourceDesc->TextureFormat; 
					pResource->Texture.Textures.resize(pResource->SubResourceCount);
					pResource->Texture.TextureViews.resize(pResource->SubResourceCount);
					pResource->Texture.Samplers.resize(pResource->SubResourceCount);
				}
				else if (pResourceDesc->Type == ERenderGraphResourceType::BUFFER)
				{
					pResource->Type				= ERenderGraphResourceType::BUFFER;
					pResource->OwnershipType	= EResourceOwnershipType::INTERNAL;
					pResource->Buffer.Buffers.resize(pResource->SubResourceCount);
					pResource->Buffer.Offsets.resize(pResource->SubResourceCount);
					pResource->Buffer.SizesInBytes.resize(pResource->SubResourceCount);
				}
				else
				{
					LOG_ERROR("[RenderGraph]: Unsupported resource type for internal resource \"%s\"", pResource->Name.c_str());
					return false;
				}
			}
			else
			{
				//External
				if (pResourceDesc->Type == ERenderGraphResourceType::TEXTURE)
				{
					pResource->Type				= ERenderGraphResourceType::TEXTURE;
					pResource->OwnershipType	= EResourceOwnershipType::EXTERNAL;
					pResource->Texture.Format	= pResourceDesc->TextureFormat; 
					pResource->Texture.Textures.resize(pResource->SubResourceCount);
					pResource->Texture.TextureViews.resize(pResource->SubResourceCount);
					pResource->Texture.Samplers.resize(pResource->SubResourceCount);
				}
				else if (pResourceDesc->Type == ERenderGraphResourceType::BUFFER)
				{
					pResource->Type				= ERenderGraphResourceType::BUFFER;
					pResource->OwnershipType	= EResourceOwnershipType::EXTERNAL;
					pResource->Buffer.Buffers.resize(pResource->SubResourceCount);
					pResource->Buffer.Offsets.resize(pResource->SubResourceCount);
					pResource->Buffer.SizesInBytes.resize(pResource->SubResourceCount);
				}
				else if (pResourceDesc->Type == ERenderGraphResourceType::ACCELERATION_STRUCTURE)
				{ 
					pResource->Type				= ERenderGraphResourceType::ACCELERATION_STRUCTURE;
					pResource->OwnershipType	= EResourceOwnershipType::EXTERNAL;
				}
				else
				{
					LOG_ERROR("[RenderGraph]: Unsupported resource type for external resource \"%s\"", pResource->Name.c_str());
					return false;
				}
			}
		}

		return true;
	}

	bool RenderGraph::CreateRenderStages(const TArray<RenderStageDesc>& renderStages)
	{
		m_RenderStageCount = (uint32)renderStages.size();
		m_RenderStageMap.reserve(m_RenderStageCount);
		m_pRenderStages = DBG_NEW RenderStage[m_RenderStageCount];

		for (uint32 renderStageIndex = 0; renderStageIndex < m_RenderStageCount; renderStageIndex++)
		{
			const RenderStageDesc* pRenderStageDesc = &renderStages[renderStageIndex];

			RenderStage* pRenderStage = &m_pRenderStages[renderStageIndex];
			m_RenderStageMap[pRenderStageDesc->Name] = renderStageIndex;

			pRenderStage->Parameters = pRenderStageDesc->Parameters;

			if (pRenderStage->Parameters.XDimType == ERenderStageDimensionType::RELATIVE ||
				pRenderStage->Parameters.XDimType == ERenderStageDimensionType::RELATIVE_1D ||
				pRenderStage->Parameters.YDimType == ERenderStageDimensionType::RELATIVE)
			{
				m_WindowRelativeRenderStages.insert(renderStageIndex);

				UpdateRelativeRenderStageDimensions(pRenderStage);
			}

			if (pRenderStage->Parameters.XDimType == ERenderStageDimensionType::CONSTANT)
			{
				pRenderStage->Dimensions.x = pRenderStageDesc->Parameters.XDimVariable;
			}

			if (pRenderStage->Parameters.YDimType == ERenderStageDimensionType::CONSTANT)
			{
				pRenderStage->Dimensions.y = pRenderStageDesc->Parameters.YDimVariable;
			}

			if (pRenderStage->Parameters.ZDimType == ERenderStageDimensionType::CONSTANT)
			{
				pRenderStage->Dimensions.z = pRenderStageDesc->Parameters.ZDimVariable;
			}

			//Calculate the total number of textures we want to bind
			uint32 textureSlots = 0;
			uint32 totalNumberOfTextures = 0;
			uint32 totalNumberOfNonMaterialTextures = 0;
			uint32 textureSubresourceCount = 0;
			bool textureSubResourceCountSame = true;
			for (uint32 rs = 0; rs < pRenderStageDesc->ResourceStates.size(); rs++)
			{
				const RenderGraphResourceState* pResourceStateDesc = &pRenderStageDesc->ResourceStates[rs];

				auto resourceIt = m_ResourceMap.find(pResourceStateDesc->ResourceName);

				if (resourceIt == m_ResourceMap.end())
				{
					LOG_ERROR("[RenderGraph]: Resource State with name \"%s\" has no accompanying Resource", pResourceStateDesc->ResourceName.c_str());
					return false;
				}

				const Resource* pResource = &resourceIt->second;

				if (ResourceStateNeedsDescriptor(pResourceStateDesc->BindingType) && pResource->Type == ERenderGraphResourceType::TEXTURE)
				{
					textureSlots++;

					if (pResourceStateDesc->BindingType == ERenderGraphResourceBindingType::COMBINED_SAMPLER)
					{
						if (textureSubresourceCount > 0 && pResource->SubResourceCount != textureSubresourceCount)
							textureSubResourceCountSame = false;

						textureSubresourceCount = pResource->SubResourceCount;
						totalNumberOfTextures += textureSubresourceCount;

						if (pResource->Name != SCENE_ALBEDO_MAPS	 &&
							pResource->Name != SCENE_NORMAL_MAPS	 &&
							pResource->Name != SCENE_AO_MAPS		 &&
							pResource->Name != SCENE_ROUGHNESS_MAPS	 &&
							pResource->Name != SCENE_METALLIC_MAPS)
						{
							totalNumberOfNonMaterialTextures += textureSubresourceCount;
						}
					}
					else
					{
						totalNumberOfTextures++;
						totalNumberOfNonMaterialTextures++;
					}
				}
			}

			if (textureSlots > m_MaxTexturesPerDescriptorSet)
			{
				LOG_ERROR("[RenderGraph]: Number of required texture slots %u for render stage %s is more than MaxTexturesPerDescriptorSet %u", textureSlots, pRenderStageDesc->Name.c_str(), m_MaxTexturesPerDescriptorSet);
				return false;
			}
			else if (totalNumberOfTextures > m_MaxTexturesPerDescriptorSet && !textureSubResourceCountSame)
			{
				LOG_ERROR("[RenderGraph]: Total number of required texture slots %u for render stage %s is more than MaxTexturesPerDescriptorSet %u. This only works if all texture bindings have either 1 or the same subresource count", textureSlots, pRenderStageDesc->Name.c_str(), m_MaxTexturesPerDescriptorSet);
				return false;
			}
			pRenderStage->MaterialsRenderedPerPass = (m_MaxTexturesPerDescriptorSet - totalNumberOfNonMaterialTextures) / 5; //5 textures per material
			pRenderStage->TextureSubDescriptorSetCount = (uint32)glm::ceil((float)totalNumberOfTextures / float(pRenderStage->MaterialsRenderedPerPass * 5));

			TArray<DescriptorBindingDesc> textureDescriptorSetDescriptions;
			textureDescriptorSetDescriptions.reserve(pRenderStageDesc->ResourceStates.size());
			uint32 textureDescriptorBindingIndex = 0;

			TArray<DescriptorBindingDesc> bufferDescriptorSetDescriptions;
			bufferDescriptorSetDescriptions.reserve(pRenderStageDesc->ResourceStates.size());
			uint32 bufferDescriptorBindingIndex = 0;

			TArray<RenderPassAttachmentDesc>								renderPassAttachmentDescriptions;
			RenderPassAttachmentDesc										renderPassDepthStencilDescription;
			TArray<ETextureState>											renderPassRenderTargetStates;
			TArray<BlendAttachmentState>									renderPassBlendAttachmentStates;
			TArray<std::pair<Resource*, ETextureState>>						renderStageRenderTargets;
			Resource*														pDepthStencilResource = nullptr;
			TArray<std::tuple<Resource*, ETextureState, EDescriptorType>>	renderStageTextureResources;
			TArray<std::tuple<Resource*, ETextureState, EDescriptorType>>	renderStageBufferResources;
			renderPassAttachmentDescriptions.reserve(pRenderStageDesc->ResourceStates.size());
			renderPassRenderTargetStates.reserve(pRenderStageDesc->ResourceStates.size());
			renderPassBlendAttachmentStates.reserve(pRenderStageDesc->ResourceStates.size());
			renderStageRenderTargets.reserve(pRenderStageDesc->ResourceStates.size());
			renderStageTextureResources.reserve(pRenderStageDesc->ResourceStates.size());
			renderStageBufferResources.reserve(pRenderStageDesc->ResourceStates.size());

			//Create Descriptors and RenderPass Attachments from RenderStage Resource States
			for (uint32 rs = 0; rs < pRenderStageDesc->ResourceStates.size(); rs++)
			{
				const RenderGraphResourceState* pResourceStateDesc = &pRenderStageDesc->ResourceStates[rs];

				auto resourceIt = m_ResourceMap.find(pResourceStateDesc->ResourceName);

				if (resourceIt == m_ResourceMap.end())
				{
					LOG_ERROR("[RenderGraph]: Resource State with name \"%s\" has no accompanying Resource", pResourceStateDesc->ResourceName.c_str());
					return false;
				}

				Resource* pResource = &resourceIt->second;

				//Descriptors
				if (ResourceStateNeedsDescriptor(pResourceStateDesc->BindingType))
				{
					EDescriptorType descriptorType		= CalculateResourceStateDescriptorType(pResource->Type, pResourceStateDesc->BindingType);

					if (descriptorType == EDescriptorType::DESCRIPTOR_UNKNOWN)
					{
						LOG_ERROR("[RenderGraph]: Descriptor Type for Resource State with name \"%s\" could not be found", pResourceStateDesc->ResourceName.c_str());
						return false;
					}

					DescriptorBindingDesc descriptorBinding = {};
					descriptorBinding.DescriptorType		= descriptorType;
					descriptorBinding.ppImmutableSamplers	= nullptr;
					descriptorBinding.ShaderStageMask		= CreateShaderStageMask(pRenderStageDesc);


					if (pResource->Type == ERenderGraphResourceType::TEXTURE)
					{
						ETextureState textureState = CalculateResourceTextureState(pResource->Type, pResourceStateDesc->BindingType, pResource->Texture.Format);

						descriptorBinding.DescriptorCount	= pResource->IsBackBuffer ? 1 : pResource->SubResourceCount / pRenderStage->TextureSubDescriptorSetCount;
						descriptorBinding.Binding			= textureDescriptorBindingIndex++;

						textureDescriptorSetDescriptions.push_back(descriptorBinding);
						renderStageTextureResources.push_back(std::make_tuple(pResource, textureState, descriptorType));
					}
					else
					{
						descriptorBinding.DescriptorCount	= pResource->SubResourceCount;
						descriptorBinding.Binding			= bufferDescriptorBindingIndex++;

						bufferDescriptorSetDescriptions.push_back(descriptorBinding);
						renderStageBufferResources.push_back(std::make_tuple(pResource, ETextureState::TEXTURE_STATE_UNKNOWN, descriptorType));
					}
				}
				//RenderPass Attachments
				else if (pResourceStateDesc->BindingType == ERenderGraphResourceBindingType::ATTACHMENT)
				{
					bool isColorAttachment = pResource->Texture.Format != EFormat::FORMAT_D24_UNORM_S8_UINT;

					ETextureState initialState	= CalculateResourceTextureState(pResource->Type, pResourceStateDesc->AttachmentSynchronizations.PrevBindingType, pResource->Texture.Format);
					ETextureState finalState	= CalculateResourceTextureState(pResource->Type, pResourceStateDesc->AttachmentSynchronizations.NextBindingType, pResource->Texture.Format);

					ELoadOp loadOp = ELoadOp::LOAD;

					if (initialState == ETextureState::TEXTURE_STATE_DONT_CARE ||
						initialState == ETextureState::TEXTURE_STATE_UNKNOWN ||
						!pResourceStateDesc->AttachmentSynchronizations.PrevSameFrame)
					{
						loadOp = ELoadOp::CLEAR;
					}

					if (isColorAttachment)
					{
						RenderPassAttachmentDesc renderPassAttachmentDesc = {};
						renderPassAttachmentDesc.Format			= pResource->Texture.Format;
						renderPassAttachmentDesc.SampleCount	= 1;
						renderPassAttachmentDesc.LoadOp			= loadOp;
						renderPassAttachmentDesc.StoreOp		= EStoreOp::STORE;
						renderPassAttachmentDesc.StencilLoadOp	= ELoadOp::DONT_CARE;
						renderPassAttachmentDesc.StencilStoreOp	= EStoreOp::DONT_CARE;
						renderPassAttachmentDesc.InitialState	= initialState;
						renderPassAttachmentDesc.FinalState		= finalState;

						renderPassAttachmentDescriptions.push_back(renderPassAttachmentDesc);

						renderPassRenderTargetStates.push_back(ETextureState::TEXTURE_STATE_RENDER_TARGET);

						BlendAttachmentState blendAttachmentState = {};
						blendAttachmentState.BlendEnabled			= false;
						blendAttachmentState.ColorComponentsMask	= COLOR_COMPONENT_FLAG_R | COLOR_COMPONENT_FLAG_G | COLOR_COMPONENT_FLAG_B | COLOR_COMPONENT_FLAG_A;

						renderPassBlendAttachmentStates.push_back(blendAttachmentState);
						renderStageRenderTargets.push_back(std::make_pair(pResource, finalState));
					}
					else
					{
						RenderPassAttachmentDesc renderPassAttachmentDesc = {};
						renderPassAttachmentDesc.Format			= pResource->Texture.Format;
						renderPassAttachmentDesc.SampleCount	= 1;
						renderPassAttachmentDesc.LoadOp			= loadOp;
						renderPassAttachmentDesc.StoreOp		= EStoreOp::STORE;
						renderPassAttachmentDesc.StencilLoadOp	= loadOp;
						renderPassAttachmentDesc.StencilStoreOp = EStoreOp::STORE;
						renderPassAttachmentDesc.InitialState	= initialState;
						renderPassAttachmentDesc.FinalState		= finalState;

						renderPassDepthStencilDescription = renderPassAttachmentDesc;
						pDepthStencilResource = pResource;
					}
				}
				else if (pResourceStateDesc->BindingType == ERenderGraphResourceBindingType::DRAW_RESOURCE)
				{
					ASSERT(false); //Todo: What todo here? Is this just error?
				}
			}

			if (pRenderStageDesc->CustomRenderer)
			{
				ICustomRenderer* pCustomRenderer = nullptr;

				if (pRenderStageDesc->Name == RENDER_GRAPH_IMGUI_STAGE_NAME)
				{
					ImGuiRenderer* pImGuiRenderer = DBG_NEW ImGuiRenderer(m_pGraphicsDevice);

					ImGuiRendererDesc imguiRendererDesc = {};
					imguiRendererDesc.BackBufferCount	= m_BackBufferCount;
					imguiRendererDesc.VertexBufferSize	= MEGA_BYTE(8);
					imguiRendererDesc.IndexBufferSize	= MEGA_BYTE(8);

					if (!pImGuiRenderer->Init(&imguiRendererDesc))
					{
						LOG_ERROR("[RenderGraph] Could not initialize ImGui Custom Renderer");
						return false;
					}

					pCustomRenderer					= pImGuiRenderer;

					m_CustomRenderers.push_back(pImGuiRenderer);
					m_DebugRenderers.push_back(pImGuiRenderer);
				}
				else
				{
					//Todo: Implement Custom Custom Renderer
					/*pCustomRenderer = pRenderStageDesc->CustomRenderer.pCustomRenderer;
					m_CustomRenderers.push_back(pRenderStageDesc->CustomRenderer.pCustomRenderer);*/
				}

				CustomRendererRenderGraphInitDesc customRendererInitDesc = {};
				customRendererInitDesc.pColorAttachmentDesc			= renderPassAttachmentDescriptions.data();
				customRendererInitDesc.ColorAttachmentCount			= (uint32)renderPassAttachmentDescriptions.size();
				customRendererInitDesc.pDepthStencilAttachmentDesc	= renderPassDepthStencilDescription.Format != EFormat::NONE ? &renderPassDepthStencilDescription : nullptr;

				if (!pCustomRenderer->RenderGraphInit(&customRendererInitDesc))
				{
					LOG_ERROR("[RenderGraph] Could not initialize Custom Renderer");
					return false;
				}

				pRenderStage->UsesCustomRenderer	= true;
				pRenderStage->pCustomRenderer		= pCustomRenderer;
				pRenderStage->FirstPipelineStage	= pCustomRenderer->GetFirstPipelineStage();
				pRenderStage->LastPipelineStage		= pCustomRenderer->GetLastPipelineStage();
			}
			else
			{
				pRenderStage->FirstPipelineStage	= FindEarliestPipelineStage(pRenderStageDesc);
				pRenderStage->LastPipelineStage		= FindLastPipelineStage(pRenderStageDesc);

				//Todo: Implement Constant Range
				//ConstantRangeDesc constantRangeDesc = {};
				//constantRangeDesc.OffsetInBytes			= 0;
				//constantRangeDesc.ShaderStageFlags		= CreateShaderStageMask(pRenderStageDesc);
				//constantRangeDesc.SizeInBytes			= pRenderStageDesc->PushConstants.DataSize;

				//Create Pipeline Layout
				{
					std::vector<DescriptorSetLayoutDesc> descriptorSetLayouts;
					descriptorSetLayouts.reserve(2);

					if (bufferDescriptorSetDescriptions.size() > 0)
					{
						DescriptorSetLayoutDesc descriptorSetLayout = {};
						descriptorSetLayout.pDescriptorBindings		= bufferDescriptorSetDescriptions.data();
						descriptorSetLayout.DescriptorBindingCount	= (uint32)bufferDescriptorSetDescriptions.size();
						descriptorSetLayouts.push_back(descriptorSetLayout);
					}

					if (textureDescriptorSetDescriptions.size() > 0)
					{
						DescriptorSetLayoutDesc descriptorSetLayout = {};
						descriptorSetLayout.pDescriptorBindings		= textureDescriptorSetDescriptions.data();
						descriptorSetLayout.DescriptorBindingCount	= (uint32)textureDescriptorSetDescriptions.size();
						descriptorSetLayouts.push_back(descriptorSetLayout);
					}

					PipelineLayoutDesc pipelineLayoutDesc = {};
					pipelineLayoutDesc.pDescriptorSetLayouts	= descriptorSetLayouts.data();
					pipelineLayoutDesc.DescriptorSetLayoutCount = (uint32)descriptorSetLayouts.size();
					//pipelineLayoutDesc.pConstantRanges			= &constantRangeDesc;
					//pipelineLayoutDesc.ConstantRangeCount		= constantRangeDesc.SizeInBytes > 0 ? 1 : 0;

					pRenderStage->pPipelineLayout = m_pGraphicsDevice->CreatePipelineLayout(&pipelineLayoutDesc);
				}

				//Create Descriptor Set
				{
					if (bufferDescriptorSetDescriptions.size() > 0)
					{
						pRenderStage->ppBufferDescriptorSets = DBG_NEW IDescriptorSet*[m_BackBufferCount];

						for (uint32 i = 0; i < m_BackBufferCount; i++)
						{
							IDescriptorSet* pDescriptorSet = m_pGraphicsDevice->CreateDescriptorSet("Render Stage Buffer Descriptor Set", pRenderStage->pPipelineLayout, 0, m_pDescriptorHeap);
							pRenderStage->ppBufferDescriptorSets[i] = pDescriptorSet;
						}
					}

					if (textureDescriptorSetDescriptions.size() > 0)
					{
						uint32 textureDescriptorSetCount = m_BackBufferCount * pRenderStage->TextureSubDescriptorSetCount;
						pRenderStage->ppTextureDescriptorSets = DBG_NEW IDescriptorSet*[textureDescriptorSetCount];

						for (uint32 i = 0; i < textureDescriptorSetCount; i++)
						{
							IDescriptorSet* pDescriptorSet = m_pGraphicsDevice->CreateDescriptorSet("Render Stage Texture Descriptor Set", pRenderStage->pPipelineLayout, pRenderStage->ppBufferDescriptorSets != nullptr ? 1 : 0, m_pDescriptorHeap);
							pRenderStage->ppTextureDescriptorSets[i] = pDescriptorSet;
						}
					}

				
				}

				//Create Pipeline State
				if (pRenderStageDesc->Type == EPipelineStateType::GRAPHICS)
				{
					GraphicsManagedPipelineStateDesc pipelineDesc = {};
					pipelineDesc.pName							= pRenderStageDesc->Name.c_str();
					pipelineDesc.pPipelineLayout				= pRenderStage->pPipelineLayout;
					pipelineDesc.TaskShader						= ResourceManager::GetGUID(pRenderStageDesc->Graphics.Shaders.TaskShaderName);
					pipelineDesc.MeshShader						= ResourceManager::GetGUID(pRenderStageDesc->Graphics.Shaders.MeshShaderName);	
					pipelineDesc.VertexShader					= ResourceManager::GetGUID(pRenderStageDesc->Graphics.Shaders.VertexShaderName);
					pipelineDesc.GeometryShader					= ResourceManager::GetGUID(pRenderStageDesc->Graphics.Shaders.GeometryShaderName);
					pipelineDesc.HullShader						= ResourceManager::GetGUID(pRenderStageDesc->Graphics.Shaders.HullShaderName);
					pipelineDesc.DomainShader					= ResourceManager::GetGUID(pRenderStageDesc->Graphics.Shaders.DomainShaderName);
					pipelineDesc.PixelShader					= ResourceManager::GetGUID(pRenderStageDesc->Graphics.Shaders.PixelShaderName);

					memcpy(pipelineDesc.pBlendAttachmentStates, renderPassBlendAttachmentStates.data(), renderPassBlendAttachmentStates.size() * sizeof(BlendAttachmentState));
					pipelineDesc.BlendAttachmentStateCount		= (uint32)renderPassBlendAttachmentStates.size();

					//Create RenderPass
					{
						RenderPassSubpassDesc renderPassSubpassDesc = {};
						renderPassSubpassDesc.pInputAttachmentStates		= nullptr;
						renderPassSubpassDesc.InputAttachmentCount			= 0;
						renderPassSubpassDesc.pResolveAttachmentStates		= nullptr;
						renderPassSubpassDesc.pRenderTargetStates			= renderPassRenderTargetStates.data();
						renderPassSubpassDesc.RenderTargetCount				= (uint32)renderPassRenderTargetStates.size();
						renderPassSubpassDesc.DepthStencilAttachmentState	= pDepthStencilResource != nullptr ? ETextureState::TEXTURE_STATE_DEPTH_STENCIL_ATTACHMENT : ETextureState::TEXTURE_STATE_DONT_CARE;

						RenderPassSubpassDependencyDesc renderPassSubpassDependencyDesc = {};
						renderPassSubpassDependencyDesc.SrcSubpass		= EXTERNAL_SUBPASS;
						renderPassSubpassDependencyDesc.DstSubpass		= 0;
						renderPassSubpassDependencyDesc.SrcAccessMask	= 0;
						renderPassSubpassDependencyDesc.DstAccessMask	= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_READ | FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
						renderPassSubpassDependencyDesc.SrcStageMask	= FPipelineStageFlags::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;
						renderPassSubpassDependencyDesc.DstStageMask	= FPipelineStageFlags::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;

						if (renderPassDepthStencilDescription.Format != EFormat::NONE) 
							renderPassAttachmentDescriptions.push_back(renderPassDepthStencilDescription);

						RenderPassDesc renderPassDesc = {};
						renderPassDesc.pName					= "";
						renderPassDesc.pAttachments				= renderPassAttachmentDescriptions.data();
						renderPassDesc.AttachmentCount			= (uint32)renderPassAttachmentDescriptions.size();
						renderPassDesc.pSubpasses				= &renderPassSubpassDesc;
						renderPassDesc.SubpassCount				= 1;
						renderPassDesc.pSubpassDependencies		= &renderPassSubpassDependencyDesc;
						renderPassDesc.SubpassDependencyCount	= 1;

						IRenderPass* pRenderPass		= m_pGraphicsDevice->CreateRenderPass(&renderPassDesc);
						pipelineDesc.pRenderPass		= pRenderPass;

						pRenderStage->pRenderPass		= pRenderPass;
					}

					//Set Draw Type and Draw Resource
					{
						pRenderStage->DrawType = pRenderStageDesc->Graphics.DrawType;

						if (pRenderStageDesc->Graphics.IndexBufferName.size() > 0)
						{
							auto indexBufferIt = m_ResourceMap.find(pRenderStageDesc->Graphics.IndexBufferName);

							if (indexBufferIt == m_ResourceMap.end())
							{
								LOG_ERROR("[RenderGraph]: Resource \"%s\" is referenced as index buffer resource by render stage, but it cannot be found in Resource Map", pRenderStageDesc->Graphics.IndexBufferName.c_str());
								return false;
							}

							pRenderStage->pIndexBufferResource = &indexBufferIt->second;
						}

						if (pRenderStageDesc->Graphics.IndirectArgsBufferName.size() > 0)
						{
							auto indirectArgsBufferIt = m_ResourceMap.find(pRenderStageDesc->Graphics.IndirectArgsBufferName);

							if (indirectArgsBufferIt == m_ResourceMap.end())
							{
								LOG_ERROR("[RenderGraph]: Resource \"%s\" is referenced as mesh index buffer resource by render stage, but it cannot be found in Resource Map", pRenderStageDesc->Graphics.IndirectArgsBufferName.c_str());
								return false;
							}

							pRenderStage->pIndirectArgsBufferResource = &indirectArgsBufferIt->second;
						}
					}

					pRenderStage->PipelineStateID = PipelineStateManager::CreateGraphicsPipelineState(&pipelineDesc);
				}
				else if (pRenderStageDesc->Type == EPipelineStateType::COMPUTE)
				{
					ComputeManagedPipelineStateDesc pipelineDesc = {};
					pipelineDesc.pName				= pRenderStageDesc->Name.c_str();
					pipelineDesc.pPipelineLayout	= pRenderStage->pPipelineLayout;
					pipelineDesc.Shader				= ResourceManager::GetGUID(pRenderStageDesc->Compute.ShaderName);

					pRenderStage->PipelineStateID = PipelineStateManager::CreateComputePipelineState(&pipelineDesc);
				}
				else if (pRenderStageDesc->Type == EPipelineStateType::RAY_TRACING)
				{
					RayTracingManagedPipelineStateDesc pipelineDesc = {};
					pipelineDesc.pPipelineLayout	= pRenderStage->pPipelineLayout;
					pipelineDesc.MaxRecursionDepth	= 1;
					pipelineDesc.RaygenShader		= ResourceManager::GetGUID(pRenderStageDesc->RayTracing.Shaders.RaygenShaderName);

					for (uint32 ch = 0; ch < pRenderStageDesc->RayTracing.Shaders.ClosestHitShaderCount; ch++)
					{
						pipelineDesc.pClosestHitShaders[ch] = ResourceManager::GetGUID(pRenderStageDesc->RayTracing.Shaders.pClosestHitShaderNames[ch]);
					}

					for (uint32 m = 0; m < pRenderStageDesc->RayTracing.Shaders.MissShaderCount; m++)
					{
						pipelineDesc.pMissShaders[m] = ResourceManager::GetGUID(pRenderStageDesc->RayTracing.Shaders.pMissShaderNames[m]);
					}

					pipelineDesc.ClosestHitShaderCount	= pRenderStageDesc->RayTracing.Shaders.ClosestHitShaderCount;
					pipelineDesc.MissShaderCount		= pRenderStageDesc->RayTracing.Shaders.MissShaderCount;

					pRenderStage->PipelineStateID = PipelineStateManager::CreateRayTracingPipelineState(&pipelineDesc);
				}
			}

			//Create Resource Bindings to Render Stage
			{
				if (renderStageRenderTargets.size() > 0)
				{
					if (pRenderStageDesc->Type != EPipelineStateType::GRAPHICS)
					{
						LOG_ERROR("[RenderGraph]: There are resources that a RenderPass should be linked to, but Render Stage %u is not a Graphics Pipeline State", renderStageIndex);
						return false;
					}

					for (uint32 r = 0; r < renderStageRenderTargets.size(); r++)
					{
						auto resourcePair = renderStageRenderTargets[r];
						Resource* pResource = resourcePair.first;

						ResourceBinding resourceBinding = {};
						resourceBinding.pRenderStage	= pRenderStage;
						resourceBinding.DescriptorType	= EDescriptorType::DESCRIPTOR_UNKNOWN;
						resourceBinding.Binding			= UINT32_MAX;
						resourceBinding.TextureState	= resourcePair.second;

						pResource->ResourceBindings.push_back(resourceBinding);		//Create Binding to notify Custom Renderers
						pRenderStage->RenderTargetResources.push_back(pResource);
					}
				}

				if (pDepthStencilResource != nullptr)
				{
					if (pRenderStageDesc->Type != EPipelineStateType::GRAPHICS)
					{
						LOG_ERROR("[RenderGraph]: There are resources that a RenderPass should be linked to, but Render Stage %u is not a Graphics Pipeline State", renderStageIndex);
						return false;
					}

					ResourceBinding resourceBinding = {};
					resourceBinding.pRenderStage	= pRenderStage;
					resourceBinding.DescriptorType	= EDescriptorType::DESCRIPTOR_UNKNOWN;
					resourceBinding.Binding			= UINT32_MAX;
					resourceBinding.TextureState	= ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;

					pDepthStencilResource->ResourceBindings.push_back(resourceBinding); //Create Binding to notify Custom Renderers
					pRenderStage->pDepthStencilAttachment = pDepthStencilResource;
				}

				for (uint32 r = 0; r < renderStageTextureResources.size(); r++)
				{
					auto& resourceTuple = renderStageTextureResources[r];
					Resource* pResource = std::get<0>(resourceTuple);

					ResourceBinding resourceBinding = {};
					resourceBinding.pRenderStage	= pRenderStage;
					resourceBinding.DescriptorType	= std::get<2>(resourceTuple);
					resourceBinding.Binding			= r;
					resourceBinding.TextureState	= std::get<1>(resourceTuple);

					pResource->ResourceBindings.push_back(resourceBinding);
				}

				for (uint32 r = 0; r < renderStageBufferResources.size(); r++)
				{
					auto& resourceTuple = renderStageBufferResources[r];
					Resource* pResource = std::get<0>(resourceTuple);

					ResourceBinding resourceBinding = {};
					resourceBinding.pRenderStage	= pRenderStage;
					resourceBinding.DescriptorType	= std::get<2>(resourceTuple);
					resourceBinding.Binding			= r;
					resourceBinding.TextureState	= std::get<1>(resourceTuple);

					pResource->ResourceBindings.push_back(resourceBinding);
				}
			}

			//Link Render Stage to Push Constant Resource
			/*if (pRenderStageDesc->PushConstants.DataSize > 0)
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
			}*/
		}

		return true;
	}

	bool RenderGraph::CreateSynchronizationStages(const TArray<SynchronizationStageDesc>& synchronizationStageDescriptions)
	{
		m_pSynchronizationStages = DBG_NEW SynchronizationStage[synchronizationStageDescriptions.size()];

		bool firstTimeEnvounteringBackBuffer = false;

		for (uint32 s = 0; s < synchronizationStageDescriptions.size(); s++)
		{
			const SynchronizationStageDesc* pSynchronizationStageDesc = &synchronizationStageDescriptions[s];

			SynchronizationStage* pSynchronizationStage = &m_pSynchronizationStages[s];

			for (auto synchronizationIt = pSynchronizationStageDesc->Synchronizations.begin(); synchronizationIt != pSynchronizationStageDesc->Synchronizations.end(); synchronizationIt++)
			{
				const RenderGraphResourceSynchronizationDesc* pResourceSynchronizationDesc = &(*synchronizationIt);

				//En massa skit kommer nog behöva göras om här, nu när Parsern tar hand om Back Buffer States korrekt.

				auto it = m_ResourceMap.find(pResourceSynchronizationDesc->ResourceName);

				if (it == m_ResourceMap.end())
				{
					LOG_ERROR("[RenderGraph]: Resource found in Synchronization Stage but not in Resource Map \"%s\"", pResourceSynchronizationDesc->ResourceName.c_str());
					return false;
				}

				Resource* pResource = &it->second;

				auto prevRenderStageIt = m_RenderStageMap.find(pResourceSynchronizationDesc->PrevRenderStage);
				auto nextRenderStageIt = m_RenderStageMap.find(pResourceSynchronizationDesc->NextRenderStage);

				if (prevRenderStageIt == m_RenderStageMap.end())
				{
					LOG_ERROR("[RenderGraph]: Render Stage found in Synchronization but not in Render Stage Map \"%s\"", pResourceSynchronizationDesc->PrevRenderStage.c_str());
					return false;
				}

				if (nextRenderStageIt == m_RenderStageMap.end())
				{
					LOG_ERROR("[RenderGraph]: Render Stage found in Synchronization but not in Render Stage Map \"%s\"", pResourceSynchronizationDesc->NextRenderStage.c_str());
					return false;
				}

				const RenderStage* pPrevRenderStage = &m_pRenderStages[prevRenderStageIt->second];
				const RenderStage* pNextRenderStage = &m_pRenderStages[nextRenderStageIt->second];

				pSynchronizationStage->SrcPipelineStage = FindLastPipelineStage(pSynchronizationStage->SrcPipelineStage | pPrevRenderStage->LastPipelineStage);
				pSynchronizationStage->DstPipelineStage = FindEarliestPipelineStage(pSynchronizationStage->DstPipelineStage | pNextRenderStage->FirstPipelineStage);

				ECommandQueueType prevQueue 	= pResourceSynchronizationDesc->PrevQueue;
				ECommandQueueType nextQueue		= pResourceSynchronizationDesc->NextQueue;
				uint32 srcMemoryAccessFlags		= CalculateResourceAccessFlags(pResourceSynchronizationDesc->PrevBindingType);
				uint32 dstMemoryAccessFlags		= CalculateResourceAccessFlags(pResourceSynchronizationDesc->NextBindingType);

				if (pSynchronizationStage->ExecutionQueue == ECommandQueueType::COMMAND_QUEUE_NONE)
				{
					pSynchronizationStage->ExecutionQueue = prevQueue;
				}
				else if (pSynchronizationStage->ExecutionQueue != prevQueue)
				{
					LOG_ERROR("[RenderGraph]: SynchronizationStage \"%s\" contains synchronizations that have different Previous Queues");
					return false;
				}

				if (pResource->Type == ERenderGraphResourceType::TEXTURE)
				{
					PipelineTextureBarrierDesc textureBarrier = {};
					textureBarrier.QueueBefore			= prevQueue;
					textureBarrier.QueueAfter			= nextQueue;
					textureBarrier.SrcMemoryAccessFlags = srcMemoryAccessFlags;
					textureBarrier.DstMemoryAccessFlags = dstMemoryAccessFlags;
					textureBarrier.StateBefore			= CalculateResourceTextureState(pResource->Type, pResourceSynchronizationDesc->PrevBindingType, pResource->Texture.Format);
					textureBarrier.StateAfter			= CalculateResourceTextureState(pResource->Type, pResourceSynchronizationDesc->NextBindingType, pResource->Texture.Format);
					textureBarrier.TextureFlags			= pResource->Texture.Format == EFormat::FORMAT_D24_UNORM_S8_UINT ? FTextureFlags::TEXTURE_FLAG_DEPTH_STENCIL : 0;

					uint32 targetSynchronizationIndex = 0;

					if (prevQueue == nextQueue)
					{
						if (pResource->IsBackBuffer)
						{
							targetSynchronizationIndex = SAME_QUEUE_BACK_BUFFER_SYNCHRONIZATION_INDEX;
						}
						else
						{
							targetSynchronizationIndex = SAME_QUEUE_TEXTURE_SYNCHRONIZATION_INDEX;
						}
					}
					else
					{
						if (pResource->IsBackBuffer)
						{
							targetSynchronizationIndex = OTHER_QUEUE_BACK_BUFFER_SYNCHRONIZATION_INDEX;
						}
						else
						{
							targetSynchronizationIndex = OTHER_QUEUE_TEXTURE_SYNCHRONIZATION_INDEX;
						}
					}

					for (uint32 sr = 0; sr < pResource->SubResourceCount; sr++)
					{
						TArray<PipelineTextureBarrierDesc>& targetArray = pSynchronizationStage->TextureBarriers[targetSynchronizationIndex];
						targetArray.push_back(textureBarrier);
						uint32 barrierIndex = targetArray.size() - 1;

						ResourceBarrierInfo barrierInfo = {};
						barrierInfo.SynchronizationStageIndex	= s;
						barrierInfo.SynchronizationTypeIndex	= targetSynchronizationIndex;
						barrierInfo.BarrierIndex				= barrierIndex;

						pResource->Texture.BarriersPerSynchronizationStage.push_back(barrierInfo);
					}
				}
				else if (pResource->Type == ERenderGraphResourceType::BUFFER)
				{
					PipelineBufferBarrierDesc bufferBarrier = {};
					bufferBarrier.QueueBefore			= prevQueue;
					bufferBarrier.QueueAfter			= nextQueue;
					bufferBarrier.SrcMemoryAccessFlags	= srcMemoryAccessFlags;
					bufferBarrier.DstMemoryAccessFlags	= dstMemoryAccessFlags;

					uint32 targetSynchronizationIndex = 0;

					if (prevQueue == nextQueue)
					{
						targetSynchronizationIndex = SAME_QUEUE_BUFFER_SYNCHRONIZATION_INDEX;
					}
					else
					{
						targetSynchronizationIndex = OTHER_QUEUE_BUFFER_SYNCHRONIZATION_INDEX;
					}

					for (uint32 sr = 0; sr < pResource->SubResourceCount; sr++)
					{
						TArray<PipelineBufferBarrierDesc>& targetArray = pSynchronizationStage->BufferBarriers[targetSynchronizationIndex];
						targetArray.push_back(bufferBarrier);
						uint32 barrierIndex = targetArray.size() - 1;

						ResourceBarrierInfo barrierInfo = {};
						barrierInfo.SynchronizationStageIndex	= s;
						barrierInfo.SynchronizationTypeIndex	= targetSynchronizationIndex;
						barrierInfo.BarrierIndex				= barrierIndex;

						pResource->Buffer.BarriersPerSynchronizationStage.push_back(barrierInfo);
					}
				}
			}
		}

		return true;
	}

	bool RenderGraph::CreatePipelineStages(const TArray<PipelineStageDesc>& pipelineStageDescriptions)
	{
		m_PipelineStageCount = (uint32)pipelineStageDescriptions.size();
		m_pPipelineStages = DBG_NEW PipelineStage[m_PipelineStageCount];

		for (uint32 i = 0; i < m_PipelineStageCount; i++)
		{
			const PipelineStageDesc* pPipelineStageDesc = &pipelineStageDescriptions[i];

			PipelineStage* pPipelineStage = &m_pPipelineStages[i];

			m_ExecutionStageCount += pPipelineStageDesc->Type == ERenderGraphPipelineStageType::SYNCHRONIZATION ? 2 : 1;

			pPipelineStage->Type		= pPipelineStageDesc->Type;
			pPipelineStage->StageIndex	= pPipelineStageDesc->StageIndex;

			pPipelineStage->ppGraphicsCommandAllocators		= DBG_NEW ICommandAllocator*[m_BackBufferCount];
			pPipelineStage->ppComputeCommandAllocators		= DBG_NEW ICommandAllocator*[m_BackBufferCount];
			pPipelineStage->ppGraphicsCommandLists			= DBG_NEW ICommandList*[m_BackBufferCount];
			pPipelineStage->ppComputeCommandLists			= DBG_NEW ICommandList*[m_BackBufferCount];
			
			for (uint32 f = 0; f < m_BackBufferCount; f++)
			{
				pPipelineStage->ppGraphicsCommandAllocators[f]	= m_pGraphicsDevice->CreateCommandAllocator("Render Graph Graphics Command Allocator", ECommandQueueType::COMMAND_QUEUE_GRAPHICS);
				pPipelineStage->ppComputeCommandAllocators[f]	= m_pGraphicsDevice->CreateCommandAllocator("Render Graph Compute Command Allocator", ECommandQueueType::COMMAND_QUEUE_COMPUTE);

				CommandListDesc graphicsCommandListDesc = {};
				graphicsCommandListDesc.pName					= "Render Graph Graphics Command List";
				graphicsCommandListDesc.CommandListType			= ECommandListType::COMMAND_LIST_PRIMARY;
				graphicsCommandListDesc.Flags					= FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

				pPipelineStage->ppGraphicsCommandLists[f]		= m_pGraphicsDevice->CreateCommandList(pPipelineStage->ppGraphicsCommandAllocators[f], &graphicsCommandListDesc);

				CommandListDesc computeCommandListDesc = {};
				computeCommandListDesc.pName					= "Render Graph Compute Command List";
				computeCommandListDesc.CommandListType			= ECommandListType::COMMAND_LIST_PRIMARY;
				computeCommandListDesc.Flags					= FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

				pPipelineStage->ppComputeCommandLists[f]		= m_pGraphicsDevice->CreateCommandList(pPipelineStage->ppComputeCommandAllocators[f], &computeCommandListDesc);
			}
		}

		m_ppExecutionStages = DBG_NEW ICommandList*[m_ExecutionStageCount];

		return true;
	}

	void RenderGraph::UpdateResourceTexture(Resource* pResource, const ResourceUpdateDesc& desc)
	{
		for (uint32 sr = 0; sr < pResource->SubResourceCount; sr++)
		{
			ITexture** ppTexture = &pResource->Texture.Textures[sr];
			ITextureView** ppTextureView = &pResource->Texture.TextureViews[sr];
			ISampler** ppSampler = &pResource->Texture.Samplers[sr];

			ITexture* pTexture			= nullptr;
			ITextureView* pTextureView	= nullptr;
			ISampler* pSampler			= nullptr;

			if (pResource->OwnershipType == EResourceOwnershipType::INTERNAL)
			{
				const TextureDesc& textureDesc	= *desc.InternalTextureUpdate.ppTextureDesc[sr];
				TextureViewDesc textureViewDesc = *desc.InternalTextureUpdate.ppTextureViewDesc[sr];
				
				SAFERELEASE(*ppTexture);
				SAFERELEASE(*ppTextureView);

				pTexture			= m_pGraphicsDevice->CreateTexture(&textureDesc, nullptr);

				textureViewDesc.pTexture = pTexture;
				pTextureView	= m_pGraphicsDevice->CreateTextureView(&textureViewDesc);

				//Update Sampler
				if (desc.InternalTextureUpdate.ppSamplerDesc != nullptr)
				{
					const SamplerDesc& samplerDesc = *desc.InternalTextureUpdate.ppSamplerDesc[sr];

					SAFERELEASE(*ppSampler);
					pSampler = m_pGraphicsDevice->CreateSampler(&samplerDesc);
				}
			}
			else if (pResource->OwnershipType == EResourceOwnershipType::EXTERNAL)
			{
				pTexture			= desc.ExternalTextureUpdate.ppTextures[sr];
				pTextureView		= desc.ExternalTextureUpdate.ppTextureViews[sr];

				//Update Sampler
				if (desc.ExternalTextureUpdate.ppSamplers != nullptr)
				{
					pSampler = desc.ExternalTextureUpdate.ppSamplers[sr];
				}
			}
			else
			{
				LOG_ERROR("[RenderGraph]: UpdateResourceTexture called for resource with invalid OwnershipType");
				return;
			}

			(*ppTexture)		= pTexture;
			(*ppTextureView)	= pTextureView;
			(*ppSampler)		= pSampler;

			if (pResource->Texture.BarriersPerSynchronizationStage.size() > 0)
			{
				PipelineTextureBarrierDesc* pFirstBarrier = nullptr;

				for (uint32 b = sr; b < pResource->Texture.BarriersPerSynchronizationStage.size(); b += pResource->SubResourceCount)
				{
					const ResourceBarrierInfo* pBarrierInfo = &pResource->Texture.BarriersPerSynchronizationStage[b];
					SynchronizationStage* pSynchronizationStage = &m_pSynchronizationStages[pBarrierInfo->SynchronizationStageIndex];

					PipelineTextureBarrierDesc* pTextureBarrier = &pSynchronizationStage->TextureBarriers[pBarrierInfo->SynchronizationTypeIndex][pBarrierInfo->BarrierIndex];

					pTextureBarrier->pTexture		= pTexture;
					pTextureBarrier->Miplevel		= 0;
					pTextureBarrier->MiplevelCount	= pTexture->GetDesc().Miplevels;
					pTextureBarrier->ArrayIndex		= 0;
					pTextureBarrier->ArrayCount		= pTexture->GetDesc().ArrayCount;

					if (pFirstBarrier == nullptr)
						pFirstBarrier = pTextureBarrier;
				}

				//Transfer to Initial State
				if (pFirstBarrier != nullptr)
				{
					PipelineTextureBarrierDesc  initialBarrier = {};

					initialBarrier.pTexture						= pTexture;
					initialBarrier.StateBefore					= ETextureState::TEXTURE_STATE_DONT_CARE;
					initialBarrier.StateAfter					= pFirstBarrier->StateBefore;
					initialBarrier.QueueBefore					= pFirstBarrier->QueueBefore;
					initialBarrier.QueueAfter					= pFirstBarrier->QueueBefore;
					initialBarrier.SrcMemoryAccessFlags			= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_UNKNOWN;
					initialBarrier.DstMemoryAccessFlags			= pFirstBarrier->SrcMemoryAccessFlags;
					initialBarrier.TextureFlags					= pFirstBarrier->TextureFlags;
					initialBarrier.Miplevel						= 0;
					initialBarrier.MiplevelCount				= pFirstBarrier->MiplevelCount;
					initialBarrier.ArrayIndex					= 0;
					initialBarrier.ArrayCount					= pFirstBarrier->ArrayCount;

					FPipelineStageFlags srcPipelineStage = pResource->ResourceBindings[0].pRenderStage->FirstPipelineStage;
					FPipelineStageFlags dstPipelineStage = pResource->ResourceBindings[0].pRenderStage->LastPipelineStage;

					if (initialBarrier.QueueAfter == ECommandQueueType::COMMAND_QUEUE_GRAPHICS)
					{
						ICommandList* pCommandList = m_ppGraphicsCopyCommandLists[m_ModFrameIndex];

						if (!m_ExecuteGraphicsCopy)
						{
							m_ExecuteGraphicsCopy = true;
								
							m_ppGraphicsCopyCommandAllocators[m_ModFrameIndex]->Reset();
							pCommandList->Begin(nullptr);
						}

						pCommandList->PipelineTextureBarriers(srcPipelineStage, dstPipelineStage, &initialBarrier, 1);
					}
					else if (initialBarrier.QueueAfter == ECommandQueueType::COMMAND_QUEUE_COMPUTE)
					{
						ICommandList* pCommandList = m_ppComputeCopyCommandLists[m_ModFrameIndex];

						if (!m_ExecuteComputeCopy)
						{
							m_ExecuteComputeCopy = true;

							m_ppComputeCopyCommandAllocators[m_ModFrameIndex]->Reset();
							pCommandList->Begin(nullptr);
						}

						pCommandList->PipelineTextureBarriers(srcPipelineStage, dstPipelineStage, &initialBarrier, 1);
					}
				}
			}
		}

		if (pResource->ResourceBindings.size() > 0)
			m_DirtyDescriptorSetTextures.insert(pResource);
	}

	void RenderGraph::UpdateResourceBuffer(Resource* pResource, const ResourceUpdateDesc& desc)
	{
		//Update Buffer
		for (uint32 sr = 0; sr < pResource->SubResourceCount; sr++)
		{
			const BufferDesc& bufferDesc = *desc.InternalBufferUpdate.ppBufferDesc[sr];

			IBuffer** ppBuffer		= &pResource->Buffer.Buffers[sr];
			uint64* pOffset			= &pResource->Buffer.Offsets[sr];
			uint64* pSizeInBytes	= &pResource->Buffer.SizesInBytes[sr];

			IBuffer* pBuffer = nullptr;

			if (pResource->OwnershipType == EResourceOwnershipType::INTERNAL)
			{
				SAFERELEASE(*ppBuffer);
				pBuffer = m_pGraphicsDevice->CreateBuffer(desc.InternalBufferUpdate.ppBufferDesc[sr], nullptr);
			}
			else if (pResource->OwnershipType == EResourceOwnershipType::EXTERNAL)
			{
				pBuffer = desc.ExternalBufferUpdate.ppBuffer[sr];
			}
			else
			{
				LOG_ERROR("[RenderGraph]: UpdateResourceBuffer called for Resource with unknown OwnershipType, \"%s\"", pResource->Name.c_str());
				return;
			}

			(*ppBuffer)		= pBuffer;
			(*pSizeInBytes) = pBuffer->GetDesc().SizeInBytes;
			(*pOffset)		= 0;

			for (uint32 b = sr; b < pResource->Buffer.BarriersPerSynchronizationStage.size(); b += pResource->SubResourceCount)
			{
				const ResourceBarrierInfo* pBarrierInfo = &pResource->Buffer.BarriersPerSynchronizationStage[b];
				SynchronizationStage* pSynchronizationStage = &m_pSynchronizationStages[pBarrierInfo->SynchronizationStageIndex];

				PipelineBufferBarrierDesc* pBufferBarrier = &pSynchronizationStage->BufferBarriers[pBarrierInfo->SynchronizationTypeIndex][pBarrierInfo->BarrierIndex];

				pBufferBarrier->pBuffer		= pBuffer;
				pBufferBarrier->SizeInBytes = pBuffer->GetDesc().SizeInBytes;
				pBufferBarrier->Offset		= 0;
			}
		}

		if (pResource->ResourceBindings.size() > 0)
			m_DirtyDescriptorSetBuffers.insert(pResource);
	}

	void RenderGraph::UpdateResourceAccelerationStructure(Resource* pResource, const ResourceUpdateDesc& desc)
	{
		//Update Acceleration Structure
		pResource->AccelerationStructure.pTLAS = desc.ExternalAccelerationStructure.pTLAS;

		m_DirtyDescriptorSetAccelerationStructures.insert(pResource);
	}

	void RenderGraph::UpdateRelativeRenderStageDimensions(RenderStage* pRenderStage)
	{
		if (pRenderStage->Parameters.XDimType == ERenderStageDimensionType::RELATIVE_1D)
		{
			pRenderStage->Dimensions.x = uint32(pRenderStage->Parameters.XDimVariable * m_WindowWidth * m_WindowHeight);
		}
		else if (pRenderStage->Parameters.XDimType == ERenderStageDimensionType::RELATIVE)
		{
			pRenderStage->Dimensions.x = uint32(pRenderStage->Parameters.XDimVariable * m_WindowWidth);
		}

		if (pRenderStage->Parameters.YDimType == ERenderStageDimensionType::RELATIVE)
		{
			pRenderStage->Dimensions.y = uint32(pRenderStage->Parameters.YDimVariable * m_WindowHeight);
		}
	}

	void RenderGraph::ExecuteSynchronizationStage(
		SynchronizationStage*	pSynchronizationStage, 
		ICommandAllocator*		pGraphicsCommandAllocator, 
		ICommandList*			pGraphicsCommandList, 
		ICommandAllocator*		pComputeCommandAllocator, 
		ICommandList*			pComputeCommandList,
		ICommandList**			ppFirstExecutionStage,
		ICommandList**			ppSecondExecutionStage)
	{
		pGraphicsCommandAllocator->Reset();
		pGraphicsCommandList->Begin(nullptr);

		pComputeCommandAllocator->Reset();
		pComputeCommandList->Begin(nullptr);

		ICommandList* pFirstExecutionCommandList = nullptr;
		ICommandList* pSecondExecutionCommandList = nullptr;

		if (pSynchronizationStage->ExecutionQueue == ECommandQueueType::COMMAND_QUEUE_GRAPHICS)
		{
			(*ppFirstExecutionStage) = pGraphicsCommandList;
			pFirstExecutionCommandList = pGraphicsCommandList;
			pSecondExecutionCommandList = pComputeCommandList;
		}
		else if (pSynchronizationStage->ExecutionQueue == ECommandQueueType::COMMAND_QUEUE_COMPUTE)
		{
			(*ppFirstExecutionStage) = pComputeCommandList;
			pFirstExecutionCommandList = pComputeCommandList;
			pSecondExecutionCommandList = pGraphicsCommandList;
		}

		//Texture Synchronizations
		{
			const TArray<PipelineTextureBarrierDesc>& sameQueueBackBufferBarriers	= pSynchronizationStage->TextureBarriers[SAME_QUEUE_BACK_BUFFER_SYNCHRONIZATION_INDEX];
			const TArray<PipelineTextureBarrierDesc>& sameQueueTextureBarriers		= pSynchronizationStage->TextureBarriers[SAME_QUEUE_TEXTURE_SYNCHRONIZATION_INDEX];
			const TArray<PipelineTextureBarrierDesc>& otherQueueBackBufferBarriers	= pSynchronizationStage->TextureBarriers[OTHER_QUEUE_BACK_BUFFER_SYNCHRONIZATION_INDEX];
			const TArray<PipelineTextureBarrierDesc>& otherQueueTextureBarriers		= pSynchronizationStage->TextureBarriers[OTHER_QUEUE_TEXTURE_SYNCHRONIZATION_INDEX];

			if (sameQueueBackBufferBarriers.size() > 0)
			{
				pFirstExecutionCommandList->PipelineTextureBarriers(pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->DstPipelineStage, sameQueueBackBufferBarriers.data(), sameQueueBackBufferBarriers.size());
			}

			if (sameQueueTextureBarriers.size() > 0)
			{
				pFirstExecutionCommandList->PipelineTextureBarriers(pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->DstPipelineStage, sameQueueTextureBarriers.data(), sameQueueTextureBarriers.size());
			}

			if (otherQueueBackBufferBarriers.size() > 0)
			{
				pFirstExecutionCommandList->PipelineTextureBarriers(pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->DstPipelineStage, otherQueueBackBufferBarriers.data(), otherQueueBackBufferBarriers.size());
				pSecondExecutionCommandList->PipelineTextureBarriers(pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->DstPipelineStage, otherQueueBackBufferBarriers.data(), otherQueueBackBufferBarriers.size());
				(*ppSecondExecutionStage) = pSecondExecutionCommandList;
			}

			if (otherQueueTextureBarriers.size() > 0)
			{
				pFirstExecutionCommandList->PipelineTextureBarriers(pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->DstPipelineStage, otherQueueTextureBarriers.data(), otherQueueTextureBarriers.size());
				pSecondExecutionCommandList->PipelineTextureBarriers(pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->DstPipelineStage, otherQueueTextureBarriers.data(), otherQueueTextureBarriers.size());
				(*ppSecondExecutionStage) = pSecondExecutionCommandList;
			}
		}

		//Buffer Synchronization
		{
			const TArray<PipelineBufferBarrierDesc>& sameQueueBufferBarriers		= pSynchronizationStage->BufferBarriers[SAME_QUEUE_BUFFER_SYNCHRONIZATION_INDEX];
			const TArray<PipelineBufferBarrierDesc>& otherQueueBufferBarriers		= pSynchronizationStage->BufferBarriers[OTHER_QUEUE_BUFFER_SYNCHRONIZATION_INDEX];

			if (sameQueueBufferBarriers.size() > 0)
			{
				pFirstExecutionCommandList->PipelineBufferBarriers(pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->DstPipelineStage, sameQueueBufferBarriers.data(), sameQueueBufferBarriers.size());
			}

			if (otherQueueBufferBarriers.size() > 0)
			{
				pFirstExecutionCommandList->PipelineBufferBarriers(pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->DstPipelineStage, otherQueueBufferBarriers.data(), otherQueueBufferBarriers.size());
				pSecondExecutionCommandList->PipelineBufferBarriers(pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->DstPipelineStage, otherQueueBufferBarriers.data(), otherQueueBufferBarriers.size());
				(*ppSecondExecutionStage) = pSecondExecutionCommandList;
			}
		}

		pGraphicsCommandList->End();
		pComputeCommandList->End();
	}

	void RenderGraph::ExecuteGraphicsRenderStage(
		RenderStage*		pRenderStage, 
		IPipelineState*		pPipelineState,
		ICommandAllocator*	pGraphicsCommandAllocator, 
		ICommandList*		pGraphicsCommandList, 
		ICommandList**		ppExecutionStage)
	{
		pGraphicsCommandAllocator->Reset();
		pGraphicsCommandList->Begin(nullptr);

		uint32 flags = FRenderPassBeginFlags::RENDER_PASS_BEGIN_FLAG_INLINE;

		ITextureView* ppTextureViews[MAX_COLOR_ATTACHMENTS];
		ITextureView* pDeptchStencilTextureView = nullptr;
		ClearColorDesc clearColorDescriptions[MAX_COLOR_ATTACHMENTS + 1];
		
		uint32 textureViewCount = 0;
		uint32 clearColorCount = 0;
		for (auto it = pRenderStage->RenderTargetResources.begin(); it != pRenderStage->RenderTargetResources.end(); it++)
		{
			ppTextureViews[textureViewCount++] = (*it)->Texture.TextureViews.size() > 1 ? (*it)->Texture.TextureViews[m_BackBufferIndex] : (*it)->Texture.TextureViews[0];

			clearColorDescriptions[clearColorCount].Color[0]	= 0.0f;
			clearColorDescriptions[clearColorCount].Color[1]	= 0.0f;
			clearColorDescriptions[clearColorCount].Color[2]	= 0.0f;
			clearColorDescriptions[clearColorCount].Color[3]	= 0.0f;

			clearColorCount++;
		}

		if (pRenderStage->pDepthStencilAttachment != nullptr)
		{
			pDeptchStencilTextureView = pRenderStage->pDepthStencilAttachment->Texture.TextureViews.size() > 1 ? pRenderStage->pDepthStencilAttachment->Texture.TextureViews[m_BackBufferIndex] : pRenderStage->pDepthStencilAttachment->Texture.TextureViews[0];

			clearColorDescriptions[clearColorCount].Depth		= 1.0f;
			clearColorDescriptions[clearColorCount].Stencil		= 0;

			clearColorCount++;
		}
		
		
		BeginRenderPassDesc beginRenderPassDesc = {};
		beginRenderPassDesc.pRenderPass			= pRenderStage->pRenderPass;
		beginRenderPassDesc.ppRenderTargets		= ppTextureViews;
		beginRenderPassDesc.RenderTargetCount	= textureViewCount;
		beginRenderPassDesc.pDepthStencil		= pDeptchStencilTextureView;
		beginRenderPassDesc.Width				= pRenderStage->Dimensions.x;
		beginRenderPassDesc.Height				= pRenderStage->Dimensions.y;
		beginRenderPassDesc.Flags				= flags;
		beginRenderPassDesc.pClearColors		= clearColorDescriptions;
		beginRenderPassDesc.ClearColorCount		= clearColorCount;
		beginRenderPassDesc.Offset.x			= 0;
		beginRenderPassDesc.Offset.y			= 0;

		pGraphicsCommandList->BeginRenderPass(&beginRenderPassDesc);

		Viewport viewport = {};
		viewport.MinDepth	= 0.0f;
		viewport.MaxDepth	= 1.0f;
		viewport.Width		= (float)pRenderStage->Dimensions.x;
		viewport.Height		= (float)pRenderStage->Dimensions.y;
		viewport.x			= 0.0f;
		viewport.y			= 0.0f;

		pGraphicsCommandList->SetViewports(&viewport, 0, 1);

		ScissorRect scissorRect = {};
		scissorRect.Width	= pRenderStage->Dimensions.x;
		scissorRect.Height	= pRenderStage->Dimensions.y;
		scissorRect.x		= 0;
		scissorRect.y		= 0;

		pGraphicsCommandList->SetScissorRects(&scissorRect, 0, 1);

		pGraphicsCommandList->BindGraphicsPipeline(pPipelineState);

		uint32 textureDescriptorSetBindingIndex = 0;

		if (pRenderStage->ppBufferDescriptorSets != nullptr)
		{
			pGraphicsCommandList->BindDescriptorSetGraphics(pRenderStage->ppBufferDescriptorSets[m_BackBufferIndex], pRenderStage->pPipelineLayout, 0);
			textureDescriptorSetBindingIndex = 1;
		}

		if (pRenderStage->DrawType == ERenderStageDrawType::SCENE_INDIRECT)
		{
			pGraphicsCommandList->BindIndexBuffer(pRenderStage->pIndexBufferResource->Buffer.Buffers[0], 0, EIndexType::UINT32);

			IBuffer* pDrawBuffer		= pRenderStage->pIndirectArgsBufferResource->Buffer.Buffers[0];
			uint32 totalDrawCount		= uint32(pDrawBuffer->GetDesc().SizeInBytes / sizeof(IndexedIndirectMeshArgument));
			uint32 indirectArgStride	= sizeof(IndexedIndirectMeshArgument);

			uint32 drawOffset = 0;
			for (uint32 i = 0; i < pRenderStage->TextureSubDescriptorSetCount; i++)
			{
				uint32 newBaseMaterialIndex		= (i + 1) * pRenderStage->MaterialsRenderedPerPass;
				uint32 newDrawOffset			= m_pScene->GetIndirectArgumentOffset(newBaseMaterialIndex);
				uint32 drawCount				= newDrawOffset - drawOffset;

				if (pRenderStage->ppTextureDescriptorSets != nullptr)
					pGraphicsCommandList->BindDescriptorSetGraphics(pRenderStage->ppTextureDescriptorSets[m_BackBufferIndex * pRenderStage->TextureSubDescriptorSetCount + i], pRenderStage->pPipelineLayout, textureDescriptorSetBindingIndex);

				pGraphicsCommandList->DrawIndexedIndirect(pDrawBuffer, drawOffset * indirectArgStride, drawCount, indirectArgStride);
				drawOffset = newDrawOffset;

				if (newDrawOffset >= totalDrawCount)
					break;
			}
		}
		else if (pRenderStage->DrawType == ERenderStageDrawType::FULLSCREEN_QUAD)
		{
			if (pRenderStage->TextureSubDescriptorSetCount > 1)
			{
				LOG_WARNING("[RenderGraph]: Render Stage has TextureSubDescriptor > 1 and DrawType FULLSCREEN_QUAD, this is currently not supported");
			}

			if (pRenderStage->ppTextureDescriptorSets != nullptr)
				pGraphicsCommandList->BindDescriptorSetGraphics(pRenderStage->ppTextureDescriptorSets[m_BackBufferIndex], pRenderStage->pPipelineLayout, textureDescriptorSetBindingIndex);

			pGraphicsCommandList->DrawInstanced(3, 1, 0, 0);
		}

		pGraphicsCommandList->EndRenderPass();
		pGraphicsCommandList->End();

		(*ppExecutionStage) = pGraphicsCommandList;
	}

	void RenderGraph::ExecuteComputeRenderStage(
		RenderStage*		pRenderStage, 
		IPipelineState*		pPipelineState,
		ICommandAllocator*	pComputeCommandAllocator,
		ICommandList*		pComputeCommandList,
		ICommandList**		ppExecutionStage)
	{
		pComputeCommandAllocator->Reset();
		pComputeCommandList->Begin(nullptr);

		pComputeCommandList->BindComputePipeline(pPipelineState);

		uint32 textureDescriptorSetBindingIndex = 0;

		if (pRenderStage->ppBufferDescriptorSets != nullptr)
		{
			pComputeCommandList->BindDescriptorSetCompute(pRenderStage->ppBufferDescriptorSets[m_BackBufferIndex], pRenderStage->pPipelineLayout, 0);
			textureDescriptorSetBindingIndex = 1;
		}

		if (pRenderStage->TextureSubDescriptorSetCount > 1)
		{
			LOG_WARNING("[RenderGraph]: Render Stage has TextureSubDescriptor > 1 and is Compute, this is currently not supported");
		}

		if (pRenderStage->ppTextureDescriptorSets != nullptr)
			pComputeCommandList->BindDescriptorSetCompute(pRenderStage->ppTextureDescriptorSets[m_BackBufferIndex], pRenderStage->pPipelineLayout, textureDescriptorSetBindingIndex);

		pComputeCommandList->Dispatch(pRenderStage->Dimensions.x, pRenderStage->Dimensions.y, pRenderStage->Dimensions.z);

		pComputeCommandList->End();

		(*ppExecutionStage) = pComputeCommandList;
	}

	void RenderGraph::ExecuteRayTracingRenderStage(
		RenderStage*		pRenderStage, 
		IPipelineState*		pPipelineState,
		ICommandAllocator*	pComputeCommandAllocator,
		ICommandList*		pComputeCommandList,
		ICommandList**		ppExecutionStage)
	{
		pComputeCommandAllocator->Reset();
		pComputeCommandList->Begin(nullptr);

		pComputeCommandList->BindRayTracingPipeline(pPipelineState);

		uint32 textureDescriptorSetBindingIndex = 0;

		if (pRenderStage->ppBufferDescriptorSets != nullptr)
		{
			pComputeCommandList->BindDescriptorSetRayTracing(pRenderStage->ppBufferDescriptorSets[m_BackBufferIndex], pRenderStage->pPipelineLayout, 0);
			textureDescriptorSetBindingIndex = 1;
		}

		if (pRenderStage->TextureSubDescriptorSetCount > 1)
		{
			LOG_WARNING("[RenderGraph]: Render Stage has TextureSubDescriptor > 1 and is Ray Tracing, this is currently not supported");
		}

		if (pRenderStage->ppTextureDescriptorSets != nullptr)
			pComputeCommandList->BindDescriptorSetRayTracing(pRenderStage->ppTextureDescriptorSets[m_BackBufferIndex], pRenderStage->pPipelineLayout, textureDescriptorSetBindingIndex);

		pComputeCommandList->TraceRays(pRenderStage->Dimensions.x, pRenderStage->Dimensions.y, pRenderStage->Dimensions.z);

		pComputeCommandList->End();

		(*ppExecutionStage) = pComputeCommandList;
	}
}
