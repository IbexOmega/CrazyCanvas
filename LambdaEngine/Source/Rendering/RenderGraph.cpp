#include "Rendering/RenderGraph.h"
#include "Rendering/CustomRenderer.h"
#include "Rendering/ImGuiRenderer.h"

#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/DescriptorHeap.h"
#include "Rendering/Core/API/PipelineLayout.h"
#include "Rendering/Core/API/DescriptorSet.h"
#include "Rendering/Core/API/RenderPass.h"
#include "Rendering/Core/API/GraphicsHelpers.h"
#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/Buffer.h"
#include "Rendering/Core/API/Texture.h"
#include "Rendering/Core/API/Sampler.h"
#include "Rendering/Core/API/Texture.h"
#include "Rendering/Core/API/TextureView.h"
#include "Rendering/Core/API/CommandQueue.h"
#include "Rendering/Core/API/Fence.h"
#include "Rendering/Core/API/Shader.h"
#include "Rendering/Core/API/SBT.h"

#include "Rendering/RenderAPI.h"
#include "Rendering/PipelineStateManager.h"
#include "Rendering/IRenderGraphCreateHandler.h"
#include "Rendering/EntityMaskManager.h"

#include "Log/Log.h"

#include "Application/API/CommonApplication.h"
#include "Application/API/Events/EventQueue.h"

#include "Debug/Profiler.h"

namespace LambdaEngine
{
	constexpr const uint32 SAME_QUEUE_BACK_BUFFER_BOUND_SYNCHRONIZATION_INDEX	= 0;
	constexpr const uint32 SAME_QUEUE_TEXTURE_SYNCHRONIZATION_INDEX				= 1;
	constexpr const uint32 OTHER_QUEUE_BACK_BUFFER_BOUND_SYNCHRONIZATION_INDEX	= 2;
	constexpr const uint32 OTHER_QUEUE_TEXTURE_SYNCHRONIZATION_INDEX			= 3;

	constexpr const uint32 SAME_QUEUE_UNBOUNDED_TEXTURE_SYNCHRONIZATION_INDEX	= 0;
	constexpr const uint32 OTHER_QUEUE_UNBOUNDED_TEXTURE_SYNCHRONIZATION_INDEX	= 1;

	constexpr const uint32 SAME_QUEUE_BUFFER_SYNCHRONIZATION_INDEX			= 0;
	constexpr const uint32 OTHER_QUEUE_BUFFER_SYNCHRONIZATION_INDEX			= 1;

	RenderGraph::RenderGraph(const GraphicsDevice* pGraphicsDevice)
		: m_pGraphicsDevice(pGraphicsDevice)
	{
		VALIDATE(m_pGraphicsDevice != nullptr);
		m_pGraphicsDevice->QueryDeviceFeatures(&m_Features);

		EventQueue::RegisterEventHandler<WindowResizedEvent>(this, &RenderGraph::OnWindowResized);
		EventQueue::RegisterEventHandler<PreSwapChainRecreatedEvent>(this, &RenderGraph::OnPreSwapChainRecreated);
		EventQueue::RegisterEventHandler<PostSwapChainRecreatedEvent>(this, &RenderGraph::OnPostSwapChainRecreated);
		EventQueue::RegisterEventHandler<PipelineStatesRecompiledEvent>(this, &RenderGraph::OnPipelineStatesRecompiled);
	}

	RenderGraph::~RenderGraph()
	{
		EventQueue::UnregisterEventHandler(this, &RenderGraph::OnWindowResized);

		s_pMaterialFence->Wait(m_SignalValue - 1, UINT64_MAX);
		SAFERELEASE(s_pMaterialFence);

		SAFERELEASE(m_pDescriptorHeap);

		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			SAFERELEASE(m_ppGraphicsCopyCommandAllocators[b]);
			SAFERELEASE(m_ppGraphicsCopyCommandLists[b]);

			SAFERELEASE(m_ppComputeCopyCommandAllocators[b]);
			SAFERELEASE(m_ppComputeCopyCommandLists[b]);

			for (DeviceChild* pDeviceChild : m_pDeviceResourcesToDestroy[b])
			{
				SAFERELEASE(pDeviceChild);
			}
		}

		SAFEDELETE_ARRAY(m_ppGraphicsCopyCommandAllocators);
		SAFEDELETE_ARRAY(m_ppGraphicsCopyCommandLists);

		SAFEDELETE_ARRAY(m_ppComputeCopyCommandAllocators);
		SAFEDELETE_ARRAY(m_ppComputeCopyCommandLists);

		SAFEDELETE_ARRAY(m_pDeviceResourcesToDestroy);

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
					for (uint32 sr = 0; sr < pResource->Texture.Textures.GetSize(); sr++)
					{
						SAFERELEASE(pResource->Texture.Textures[sr]);
						SAFERELEASE(pResource->Texture.PerImageTextureViews[sr]);
						SAFERELEASE(pResource->Texture.Samplers[sr]);
					}

					if (pResource->Texture.UsedAsRenderTarget && pResource->Texture.PerSubImageUniquelyAllocated)
					{
						for (TextureView* pPerSubImageTextureView : pResource->Texture.PerSubImageTextureViews)
						{
							SAFERELEASE(pPerSubImageTextureView);
						}
					}
				}
			}
		}

		ReleasePipelineStages();

		for (uint32 r = 0; r < m_DebugRenderers.GetSize(); r++)
		{
			SAFEDELETE(m_DebugRenderers[r]);
		}

		m_DebugRenderers.Clear();
	}

	bool RenderGraph::Init(const RenderGraphDesc* pDesc, TSet<DrawArgMaskDesc>& requiredDrawArgMasks)
	{
		m_BackBufferCount				= pDesc->BackBufferCount;
		m_pDeviceResourcesToDestroy		= DBG_NEW TArray<DeviceChild*>[m_BackBufferCount];

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

		if (!CreateProfiler(pDesc->pRenderGraphStructureDesc->PipelineStageDescriptions.GetSize()))
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Profiler", pDesc->Name.c_str());
			return false;
		}

		if (!CreateResources(pDesc->pRenderGraphStructureDesc->ResourceDescriptions))
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Resources", pDesc->Name.c_str());
			return false;
		}

		if (!CreateRenderStages(pDesc->pRenderGraphStructureDesc->RenderStageDescriptions, pDesc->pRenderGraphStructureDesc->ShaderConstants, pDesc->CustomRenderers, requiredDrawArgMasks))
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Render Stages", pDesc->Name.c_str());
			return false;
		}

		if (!CreateSynchronizationStages(pDesc->pRenderGraphStructureDesc->SynchronizationStageDescriptions, requiredDrawArgMasks))
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Synchronization Stages", pDesc->Name.c_str());
			return false;
		}

		if (!CreatePipelineStages(pDesc->pRenderGraphStructureDesc->PipelineStageDescriptions))
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Pipeline Stages", pDesc->Name.c_str());
			return false;
		}
		
		if (!CustomRenderStagesPostInit())
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to Post Init Custom Renderers", pDesc->Name.c_str());
			return false;
		}

		m_WindowWidth	= (float32)pDesc->BackBufferWidth;
		m_WindowHeight	= (float32)pDesc->BackBufferHeight;
		UpdateRelativeParameters();

		return true;
	}

	bool RenderGraph::Recreate(const RenderGraphDesc* pDesc, TSet<DrawArgMaskDesc>& requiredDrawArgMasks)
	{
		RenderAPI::GetGraphicsQueue()->Flush();
		RenderAPI::GetComputeQueue()->Flush();
		RenderAPI::GetCopyQueue()->Flush();

		//Release Old Stuff
		{
			m_DirtyBoundTextureResources.clear();
			m_DirtyBoundBufferResources.clear();
			m_DirtyBoundAccelerationStructureResources.clear();
			m_DirtyBoundDrawArgResources.clear();
			m_DirtyRenderStageTextureSets.clear();
			m_DirtyRenderStageBufferSets.clear();
			m_WindowRelativeRenderStages.clear();
			m_WindowRelativeResources.Clear();

			//Stop Copy Command Lists
			{
				for (uint32 b = 0; b < m_BackBufferCount; b++)
				{
					CommandList* pGraphicsCopyCommandList = m_ppGraphicsCopyCommandLists[b];

					if (pGraphicsCopyCommandList->IsBegin())
						pGraphicsCopyCommandList->End();

					CommandList* pComputeCopyCommandList = m_ppComputeCopyCommandLists[b];

					if (pComputeCopyCommandList->IsBegin())
						pComputeCopyCommandList->End();
				}
			}

			for (uint32 b = 0; b < m_BackBufferCount; b++)
			{
				for (DeviceChild* pDeviceChild : m_pDeviceResourcesToDestroy[b])
				{
					SAFERELEASE(pDeviceChild);
				}
			}

			ReleasePipelineStages();
		}

		if (!CreateProfiler(pDesc->pRenderGraphStructureDesc->PipelineStageDescriptions.GetSize()))
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Profiler", pDesc->Name.c_str());
			return false;
		}

		if (!CreateResources(pDesc->pRenderGraphStructureDesc->ResourceDescriptions))
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Resources", pDesc->Name.c_str());
			return false;
		}

		if (!CreateRenderStages(pDesc->pRenderGraphStructureDesc->RenderStageDescriptions, pDesc->pRenderGraphStructureDesc->ShaderConstants, pDesc->CustomRenderers, requiredDrawArgMasks))
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Render Stages", pDesc->Name.c_str());
			return false;
		}

		if (!CreateSynchronizationStages(pDesc->pRenderGraphStructureDesc->SynchronizationStageDescriptions, requiredDrawArgMasks))
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Synchronization Stages", pDesc->Name.c_str());
			return false;
		}

		if (!CreatePipelineStages(pDesc->pRenderGraphStructureDesc->PipelineStageDescriptions))
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Pipeline Stages", pDesc->Name.c_str());
			return false;
		}

		if (!CustomRenderStagesPostInit())
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to Post Init Custom Renderers", pDesc->Name.c_str());
			return false;
		}

		UpdateRelativeParameters();

		for (IRenderGraphCreateHandler* pCreateHandler : m_CreateHandlers)
		{
			pCreateHandler->OnRenderGraphRecreate(this);
		}

		UpdateResourceBindings();

		m_WindowWidth	= (float32)pDesc->BackBufferWidth;
		m_WindowHeight	= (float32)pDesc->BackBufferHeight;
		UpdateRelativeParameters();

		return true;
	}

	void RenderGraph::AddCreateHandler(IRenderGraphCreateHandler* pCreateHandler)
	{
		m_CreateHandlers.PushBack(pCreateHandler);
	}

	void RenderGraph::UpdateResource(const ResourceUpdateDesc* pDesc)
	{
		auto it = m_ResourceMap.find(pDesc->ResourceName);

		if (it != m_ResourceMap.end())
		{
			Resource* pResource = &it->second;

			switch (pResource->Type)
			{
				case ERenderGraphResourceType::TEXTURE:					UpdateResourceTexture(pResource, pDesc);				break;
				case ERenderGraphResourceType::SCENE_DRAW_ARGS:			UpdateResourceDrawArgs(pResource, pDesc);				break;
				case ERenderGraphResourceType::BUFFER:					UpdateResourceBuffer(pResource, pDesc);					break;
				case ERenderGraphResourceType::ACCELERATION_STRUCTURE:	UpdateResourceAccelerationStructure(pResource, pDesc);	break;
				default:
				{
					LOG_WARNING("[RenderGraph]: Resource \"%s\" in Render Graph has unsupported Type", pDesc->ResourceName.c_str());
					return;
				}
			}
		}
		else
		{
			LOG_WARNING("[RenderGraph]: Resource \"%s\" in Render Graph could not be found in Resource Map", pDesc->ResourceName.c_str());
			return;
		}
	}

	void RenderGraph::UpdatePushConstants(const PushConstantsUpdate* pDesc)
	{
		if (pDesc->pData != nullptr)
		{
			auto renderStageIt = m_RenderStageMap.find(pDesc->RenderStageName);

			if (renderStageIt != m_RenderStageMap.end())
			{
				RenderStage* pRenderStage = &m_pRenderStages[renderStageIt->second];

				if (pDesc->DataSize <= pRenderStage->ExternalPushConstants.MaxDataSize)
				{
					if (pRenderStage->ExternalPushConstants.pData == nullptr)
					{
						pRenderStage->ExternalPushConstants.pData = DBG_NEW byte[pRenderStage->ExternalPushConstants.MaxDataSize];
					}

					memcpy(pRenderStage->ExternalPushConstants.pData, pDesc->pData, pDesc->DataSize);
					pRenderStage->ExternalPushConstants.DataSize = pDesc->DataSize;
				}
				else
				{
					LOG_ERROR("[RenderGraph]: Render Stage \"%s\" has a Max External Push Constant size of %d but Update Desc has a size of %d", pDesc->RenderStageName.c_str(), pRenderStage->ExternalPushConstants.MaxDataSize, pDesc->DataSize);
				}
			}
			else
			{
				LOG_WARNING("[RenderGraph]: Render Stage \"%s\" in Render Graph could not be found in Render Stage Map", pDesc->RenderStageName.c_str());
			}
		}
		else
		{
			LOG_ERROR("[RenderGraph]: PushConstantsUpdate::pData can not be nullptr!");
		}
	}

	void RenderGraph::UpdateGlobalSBT(CommandList* pCommandList, const TArray<SBTRecord>& shaderRecords, TArray<DeviceChild*>& removedDeviceResources)
	{
		m_GlobalShaderRecords = shaderRecords;

		for (uint32 r = 0; r < m_RenderStageCount; r++)
		{
			RenderStage* pRenderStage = &m_pRenderStages[r];

			if (pRenderStage->pPipelineState != nullptr && pRenderStage->pPipelineState->GetType() == EPipelineStateType::PIPELINE_STATE_TYPE_RAY_TRACING)
			{
				SBTDesc sbtDesc = {};
				sbtDesc.DebugName		= "Render Graph Global SBT";
				sbtDesc.pPipelineState	= pRenderStage->pPipelineState;
				sbtDesc.SBTRecords		= m_GlobalShaderRecords;

				if (pRenderStage->pSBT == nullptr)
				{
					pRenderStage->pSBT = RenderAPI::GetDevice()->CreateSBT(pCommandList, &sbtDesc);
				}
				else
				{
					pRenderStage->pSBT->Build(pCommandList, removedDeviceResources, &sbtDesc);
				}
			}
		}
	}

	void RenderGraph::UpdateRenderStageDimensions(const String& renderStageName, uint32 x, uint32 y, uint32 z)
	{
		auto it = m_RenderStageMap.find(renderStageName);

		if (it != m_RenderStageMap.end())
		{
			RenderStage* pRenderStage = &m_pRenderStages[it->second];

			if (pRenderStage->Parameters.XDimType == ERenderGraphDimensionType::EXTERNAL) pRenderStage->Dimensions.x = x;
			if (pRenderStage->Parameters.YDimType == ERenderGraphDimensionType::EXTERNAL) pRenderStage->Dimensions.y = y;
			if (pRenderStage->Parameters.ZDimType == ERenderGraphDimensionType::EXTERNAL) pRenderStage->Dimensions.z = z;
		}
		else
		{
			LOG_WARNING("[RenderGraph]: UpdateRenderStageParameters failed, render stage with name \"%s\" could not be found", renderStageName.c_str());
			return;
		}
	}

	void RenderGraph::UpdateResourceDimensions(const String& resourceName, uint32 x, uint32 y)
	{
		auto resourceDescIt = m_InternalResourceUpdateDescriptions.find(resourceName);

		if (resourceDescIt != m_InternalResourceUpdateDescriptions.end())
		{
			InternalResourceUpdateDesc* pResourceUpdateDesc = &resourceDescIt->second;

			switch (pResourceUpdateDesc->Type)
			{
				case ERenderGraphResourceType::TEXTURE:
				{
					if (pResourceUpdateDesc->TextureUpdate.XDimType == ERenderGraphDimensionType::EXTERNAL) pResourceUpdateDesc->TextureUpdate.TextureDesc.Width = x;
					if (pResourceUpdateDesc->TextureUpdate.YDimType == ERenderGraphDimensionType::EXTERNAL) pResourceUpdateDesc->TextureUpdate.TextureDesc.Height = y;
					m_DirtyInternalResources.insert(resourceName);
					break;
				}
				case ERenderGraphResourceType::BUFFER:
				{
					if (pResourceUpdateDesc->BufferUpdate.SizeType == ERenderGraphDimensionType::EXTERNAL) pResourceUpdateDesc->BufferUpdate.BufferDesc.SizeInBytes = x;
					m_DirtyInternalResources.insert(resourceName);
					break;
				}
				default:
				{
					LOG_WARNING("[RenderGraph]: UpdateResourceDimensions failed, resource \"%s\" has an unsupported type", resourceName.c_str());
					break;
				}
			}
		}
		else
		{
			LOG_WARNING("[RenderGraph]: UpdateResourceDimensions failed, resource with name \"%s\" could not be found", resourceName.c_str());
			return;
		}
	}

	void RenderGraph::TriggerRenderStage(const String& renderStageName)
	{
		auto it = m_RenderStageMap.find(renderStageName);

		if (it != m_RenderStageMap.end())
		{
			RenderStage* pRenderStage = &m_pRenderStages[it->second];

			if (pRenderStage->TriggerType == ERenderStageExecutionTrigger::TRIGGERED)
			{
				pRenderStage->FrameCounter = 0;
			}
			else
			{
				LOG_WARNING("[RenderGraph]: TriggerRenderStage failed, render stage with name \"%s\" does not have TRIGGERED as trigger type", renderStageName.c_str());
				return;
			}
		}
		else
		{
			LOG_WARNING("[RenderGraph]: TriggerRenderStage failed, render stage with name \"%s\" could not be found", renderStageName.c_str());
			return;
		}
	}

	void RenderGraph::SetRenderStageSleeping(const String& renderStageName, bool sleeping)
	{
		auto it = m_RenderStageMap.find(renderStageName);

		if (it != m_RenderStageMap.end())
		{
			RenderStage* pRenderStage = &m_pRenderStages[it->second];
			pRenderStage->Sleeping = sleeping;
		}
		else
		{
			LOG_WARNING("[RenderGraph]: SetRenderStageSleeping failed, render stage with name \"%s\" could not be found", renderStageName.c_str());
			return;
		}
	}

	void RenderGraph::Update(LambdaEngine::Timestamp delta, uint32 modFrameIndex, uint32 backBufferIndex)
	{
		UNREFERENCED_VARIABLE(modFrameIndex);
		UNREFERENCED_VARIABLE(backBufferIndex);

		for (CustomRenderer* pCustomRenderer : m_CustomRenderers)
		{
			pCustomRenderer->Update(delta, (uint32)m_ModFrameIndex, m_BackBufferIndex);
		}

		UpdateResourceBindings();
	}

	void RenderGraph::UpdateResourceBindings()
	{
		//We need to copy descriptor sets here since they may become invalidated after recreating internal resources
		{
			if (!m_DirtyRenderStageTextureSets.empty())
			{
				//Copy old descriptor set and replace old with copy, then write into the new copy
				for (RenderStage* pRenderStage : m_DirtyRenderStageTextureSets)
				{
					if (pRenderStage->ppTextureDescriptorSets != nullptr)
					{
						for (uint32 b = 0; b < m_BackBufferCount; b++)
						{
							DescriptorSet* pSrcDescriptorSet = pRenderStage->ppTextureDescriptorSets[b];
							DescriptorSet* pDescriptorSet = m_pGraphicsDevice->CreateDescriptorSet(pSrcDescriptorSet->GetName(), pRenderStage->pPipelineLayout, pRenderStage->TextureSetIndex, m_pDescriptorHeap);
							m_pGraphicsDevice->CopyDescriptorSet(pSrcDescriptorSet, pDescriptorSet);
							m_pDeviceResourcesToDestroy[b].PushBack(pSrcDescriptorSet);
							pRenderStage->ppTextureDescriptorSets[b] = pDescriptorSet;
						}
					}
					else if (pRenderStage->UsesCustomRenderer)
					{
						pRenderStage->pCustomRenderer->PreTexturesDescriptorSetWrite();
					}
				}

				m_DirtyRenderStageTextureSets.clear();
			}

			if (!m_DirtyRenderStageBufferSets.empty())
			{
				//Copy old descriptor set and replace old with copy, then write into the new copy
				for (RenderStage* pRenderStage : m_DirtyRenderStageBufferSets)
				{
					if (pRenderStage->ppBufferDescriptorSets != nullptr)
					{
						for (uint32 b = 0; b < m_BackBufferCount; b++)
						{
							DescriptorSet* pSrcDescriptorSet = pRenderStage->ppBufferDescriptorSets[b];
							DescriptorSet* pDescriptorSet = m_pGraphicsDevice->CreateDescriptorSet(pSrcDescriptorSet->GetName(), pRenderStage->pPipelineLayout, pRenderStage->BufferSetIndex, m_pDescriptorHeap);
							m_pGraphicsDevice->CopyDescriptorSet(pSrcDescriptorSet, pDescriptorSet);
							m_pDeviceResourcesToDestroy[b].PushBack(pSrcDescriptorSet);
							pRenderStage->ppBufferDescriptorSets[b] = pDescriptorSet;
						}
					}
					else if (pRenderStage->UsesCustomRenderer)
					{
						pRenderStage->pCustomRenderer->PreBuffersDescriptorSetWrite();
					}
				}

				m_DirtyRenderStageBufferSets.clear();
			}
		}

		if (m_DirtyInternalResources.size() > 0)
		{
			for (const String& dirtyInternalResourceDescName : m_DirtyInternalResources)
			{
				UpdateInternalResource(m_InternalResourceUpdateDescriptions[dirtyInternalResourceDescName]);
			}

			m_DirtyInternalResources.clear();
		}

		if (!m_DirtyBoundBufferResources.empty())
		{
			for (Resource* pResource : m_DirtyBoundBufferResources)
			{
				for (uint32 rb = 0; rb < pResource->ResourceBindings.GetSize(); rb++)
				{
					ResourceBinding* pResourceBinding = &pResource->ResourceBindings[rb];
					RenderStage* pRenderStage = pResourceBinding->pRenderStage;

					if (pRenderStage->UsesCustomRenderer)
					{
						pRenderStage->pCustomRenderer->UpdateBufferResource(
							pResource->Name,
							pResource->Buffer.Buffers.GetData(),
							pResource->Buffer.Offsets.GetData(),
							pResource->Buffer.SizesInBytes.GetData(),
							pResource->SubResourceCount,
							pResource->BackBufferBound);
					}
					else if (pResourceBinding->DescriptorType != EDescriptorType::DESCRIPTOR_TYPE_UNKNOWN)
					{
						if (pResource->BackBufferBound)
						{
							for (uint32 b = 0; b < m_BackBufferCount; b++)
							{
								pResourceBinding->pRenderStage->ppBufferDescriptorSets[b]->WriteBufferDescriptors(
									&pResource->Buffer.Buffers[b],
									&pResource->Buffer.Offsets[b],
									&pResource->Buffer.SizesInBytes[b],
									pResourceBinding->Binding,
									1,
									pResourceBinding->DescriptorType);
							}
						}
						else
						{
							for (uint32 b = 0; b < m_BackBufferCount; b++)
							{
								pResourceBinding->pRenderStage->ppBufferDescriptorSets[b]->WriteBufferDescriptors(
									pResource->Buffer.Buffers.GetData(),
									pResource->Buffer.Offsets.GetData(),
									pResource->Buffer.SizesInBytes.GetData(),
									pResourceBinding->Binding,
									pResource->SubResourceCount,
									pResourceBinding->DescriptorType);
							}
						}
					}
				}
			}

			m_DirtyBoundBufferResources.clear();
		}

		//Acceleration Structures
		if (!m_DirtyBoundAccelerationStructureResources.empty())
		{
			for (Resource* pResource : m_DirtyBoundAccelerationStructureResources)
			{
				if (!pResource->ResourceBindings.IsEmpty())
				{
					for (uint32 rb = 0; rb < pResource->ResourceBindings.GetSize(); rb++)
					{
						ResourceBinding* pResourceBinding = &pResource->ResourceBindings[rb];
						RenderStage* pRenderStage = pResourceBinding->pRenderStage;

						if (pRenderStage->UsesCustomRenderer)
						{
							pRenderStage->pCustomRenderer->UpdateAccelerationStructureResource(
								pResource->Name,
								pResource->AccelerationStructure.pTLAS);
						}
						else if (pResourceBinding->DescriptorType != EDescriptorType::DESCRIPTOR_TYPE_UNKNOWN)
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
				}
			}

			m_DirtyBoundAccelerationStructureResources.clear();
		}

		if (!m_DirtyBoundTextureResources.empty())
		{
			for (Resource* pResource : m_DirtyBoundTextureResources)
			{
				for (uint32 rb = 0; rb < pResource->ResourceBindings.GetSize(); rb++)
				{
					ResourceBinding* pResourceBinding = &pResource->ResourceBindings[rb];
					RenderStage* pRenderStage = pResourceBinding->pRenderStage;

					if (pRenderStage->UsesCustomRenderer)
					{
						pRenderStage->pCustomRenderer->UpdateTextureResource(
							pResource->Name,
							pResource->Texture.PerImageTextureViews.GetData(),
							pResource->Texture.PerSubImageTextureViews.GetData(),
							pResource->Texture.PerImageTextureViews.GetSize(),
							pResource->Texture.PerSubImageTextureViews.GetSize(),
							pResource->BackBufferBound);
					}
					else if (pResourceBinding->DescriptorType != EDescriptorType::DESCRIPTOR_TYPE_UNKNOWN)
					{
						if (pResource->BackBufferBound)
						{
							for (uint32 b = 0; b < m_BackBufferCount; b++)
							{
								pRenderStage->ppTextureDescriptorSets[b]->WriteTextureDescriptors(
									&pResource->Texture.PerImageTextureViews[b],
									&pResource->Texture.Samplers[b],
									pResourceBinding->TextureState,
									pResourceBinding->Binding,
									1,
									pResourceBinding->DescriptorType,
									true);
							}
						}
						else
						{
							for (uint32 b = 0; b < m_BackBufferCount; b++)
							{
								pRenderStage->ppTextureDescriptorSets[b]->WriteTextureDescriptors(
									pResource->Texture.PerImageTextureViews.GetData(),
									pResource->Texture.Samplers.GetData(),
									pResourceBinding->TextureState,
									pResourceBinding->Binding,
									pResource->Texture.PerImageTextureViews.GetSize(),
									pResourceBinding->DescriptorType,
									pResource->Texture.Samplers.GetSize() == pResource->Texture.PerImageTextureViews.GetSize());
							}
						}
					}
				}
			}

			m_DirtyBoundTextureResources.clear();
		}

		if (!m_DirtyBoundDrawArgResources.empty())
		{
			for (Resource* pResource : m_DirtyBoundDrawArgResources)
			{
				for (uint32 rb = 0; rb < pResource->ResourceBindings.GetSize(); rb++)
				{
					ResourceBinding* pResourceBinding = &pResource->ResourceBindings[rb];
					RenderStage* pRenderStage = pResourceBinding->pRenderStage;

					auto drawArgsMaskToArgsIt = pResource->DrawArgs.FullMaskToArgs.find(pRenderStage->DrawArgsMaskDesc.FullMask);
					pRenderStage->pDrawArgs = drawArgsMaskToArgsIt->second.Args.GetData();

					if (pRenderStage->UsesCustomRenderer)
					{
						pRenderStage->pCustomRenderer->UpdateDrawArgsResource(
							pResource->Name,
							drawArgsMaskToArgsIt->second.Args.GetData(),
							drawArgsMaskToArgsIt->second.Args.GetSize());
					}
					else
					{
						pRenderStage->pDrawArgs = drawArgsMaskToArgsIt->second.Args.GetData();

						for (uint32 b = 0; b < m_BackBufferCount; b++)
						{
							DescriptorSet** ppPrevDrawArgsPerFrame = pRenderStage->pppDrawArgDescriptorSets[b];
							DescriptorSet** ppNewDrawArgsPerFrame = nullptr;

							DescriptorSet** ppPrevDrawArgsExtensionsPerFrame = pRenderStage->pppDrawArgExtensionsDescriptorSets ? pRenderStage->pppDrawArgExtensionsDescriptorSets[b] : nullptr;
							DescriptorSet** ppNewDrawArgsExtensionsPerFrame = nullptr;

							// Check if it need to expand the list of descriptor sets
							bool resizeArr = pRenderStage->NumDrawArgsPerFrame < drawArgsMaskToArgsIt->second.Args.GetSize();

							if (resizeArr)
							{
								ppNewDrawArgsPerFrame = DBG_NEW DescriptorSet * [drawArgsMaskToArgsIt->second.Args.GetSize()];

								if (pRenderStage->pppDrawArgExtensionsDescriptorSets)
								{
									ppNewDrawArgsExtensionsPerFrame = DBG_NEW DescriptorSet * [drawArgsMaskToArgsIt->second.Args.GetSize()];
								}
							}
							else
							{
								ppNewDrawArgsPerFrame = ppPrevDrawArgsPerFrame;
								ppNewDrawArgsExtensionsPerFrame = ppPrevDrawArgsExtensionsPerFrame;
							}

							for (uint32 d = 0; d < drawArgsMaskToArgsIt->second.Args.GetSize(); d++)
							{
								// Destroy the previous descriptor sets.
								if (d < pRenderStage->NumDrawArgsPerFrame)
								{
									{
										DescriptorSet* pSrcDescriptorSet = ppPrevDrawArgsPerFrame[d];
										m_pDeviceResourcesToDestroy[b].PushBack(pSrcDescriptorSet);
									}

									// Destroy previous extension descriptor sets if it had any.
									if (ppPrevDrawArgsExtensionsPerFrame)
									{
										DescriptorSet* pSrcDescriptorSet = ppPrevDrawArgsExtensionsPerFrame[d];
										if(pSrcDescriptorSet)
											m_pDeviceResourcesToDestroy[b].PushBack(pSrcDescriptorSet);
									}
								}

								DescriptorSet* pWriteDescriptorSet = m_pGraphicsDevice->CreateDescriptorSet("Draw Args Descriptor Set", pRenderStage->pPipelineLayout, pRenderStage->DrawSetIndex, m_pDescriptorHeap);

								static uint64 offset = 0;

								const DrawArg& drawArg = drawArgsMaskToArgsIt->second.Args[d];
								VALIDATE(drawArg.pVertexBuffer);
								pWriteDescriptorSet->WriteBufferDescriptors(&drawArg.pVertexBuffer, &offset, &drawArg.pVertexBuffer->GetDesc().SizeInBytes, 0, 1, pResourceBinding->DescriptorType);

								VALIDATE(drawArg.pInstanceBuffer);
								pWriteDescriptorSet->WriteBufferDescriptors(&drawArg.pInstanceBuffer, &offset, &drawArg.pInstanceBuffer->GetDesc().SizeInBytes, 1, 1, pResourceBinding->DescriptorType);

								// If meshletbuffer is nullptr we assume that meshshaders are disabled
								if (drawArg.pMeshletBuffer)
								{
									pWriteDescriptorSet->WriteBufferDescriptors(&drawArg.pMeshletBuffer, &offset, &drawArg.pMeshletBuffer->GetDesc().SizeInBytes, 2, 1, pResourceBinding->DescriptorType);

									VALIDATE(drawArg.pUniqueIndicesBuffer);
									pWriteDescriptorSet->WriteBufferDescriptors(&drawArg.pUniqueIndicesBuffer, &offset, &drawArg.pUniqueIndicesBuffer->GetDesc().SizeInBytes, 3, 1, pResourceBinding->DescriptorType);

									VALIDATE(drawArg.pPrimitiveIndices);
									pWriteDescriptorSet->WriteBufferDescriptors(&drawArg.pPrimitiveIndices, &offset, &drawArg.pPrimitiveIndices->GetDesc().SizeInBytes, 4, 1, pResourceBinding->DescriptorType);
								}

								ppNewDrawArgsPerFrame[d] = pWriteDescriptorSet;

								// Only create a desciptor set for the extensions if it is needed.
								if (drawArg.HasExtensions && ppNewDrawArgsExtensionsPerFrame)
								{
									DescriptorSet* pExtensionsWriteDescriptorSet = m_pGraphicsDevice->CreateDescriptorSet("Draw Args Extensions Descriptor Set", pRenderStage->pPipelineLayout, pRenderStage->DrawExtensionSetIndex, m_pDescriptorHeap);

									// Fetch data for the write texture descirptors.
									THashTable<uint32, std::tuple<TArray<TextureView*>, TArray<Sampler*>>> bindingToDataMap;
									for (uint32 eGroup = 0; eGroup < drawArg.InstanceCount; eGroup++)
									{
										DrawArgExtensionGroup* extensionGroup = drawArg.ppExtensionGroups[eGroup];
										if (extensionGroup)
										{
											uint32 numExtensions = extensionGroup->ExtensionCount;
											for (uint32 e = 0; e < numExtensions; e++)
											{
												DrawArgExtensionData& extension = extensionGroup->pExtensions[e];
												VALIDATE(extension.ExtensionID != 0);

												// TODO: This might be wrong!
												uint32 extensionID = extension.ExtensionID;
												for (uint32 t = 0; t < extension.TextureCount; t++)
												{
													// If it is the first, set it to the default texture.
													if (auto it = bindingToDataMap.find(extensionID); it == bindingToDataMap.end())
													{
														TextureView* pDefaultMaskMapView = ResourceManager::GetTextureView(GUID_TEXTURE_DEFAULT_MASK_MAP);
														get<0>(bindingToDataMap[extensionID]).PushBack(pDefaultMaskMapView);
														get<1>(bindingToDataMap[extensionID]).PushBack(Sampler::GetLinearSampler());
													}

													get<0>(bindingToDataMap[extensionID]).PushBack(extension.ppTextureViews[t]);
													get<1>(bindingToDataMap[extensionID]).PushBack(extension.ppSamplers[t]);
												}
											}
										}
									}

									// Write texture descirptors
									for (uint32 binding = 0; auto& entry : bindingToDataMap)
									{
										// TODO: Make this more general. Do not hardcode the texture state and descriptor type!
										const uint32 textureCount = get<0>(entry.second).GetSize();
										pExtensionsWriteDescriptorSet->WriteTextureDescriptors(
											get<0>(entry.second).GetData(),
											get<1>(entry.second).GetData(),
											ETextureState::TEXTURE_STATE_SHADER_READ_ONLY,
											binding++,
											textureCount,
											EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER,
											true);
									}

									ppNewDrawArgsExtensionsPerFrame[d] = pExtensionsWriteDescriptorSet;
								}
								else if (ppNewDrawArgsExtensionsPerFrame)
								{
									DescriptorSet* pExtensionsWriteDescriptorSet = m_pGraphicsDevice->CreateDescriptorSet("Draw Args Extensions Descriptor Set", pRenderStage->pPipelineLayout, pRenderStage->DrawExtensionSetIndex, m_pDescriptorHeap);

									TextureView* pDefaultMaskMapView = ResourceManager::GetTextureView(GUID_TEXTURE_DEFAULT_MASK_MAP);
									Sampler* pDefaultSampler = Sampler::GetNearestSampler();
									pExtensionsWriteDescriptorSet->WriteTextureDescriptors(
										&pDefaultMaskMapView,
										&pDefaultSampler,
										ETextureState::TEXTURE_STATE_SHADER_READ_ONLY,
										0,
										1,
										EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER,
										true
									);

									ppNewDrawArgsExtensionsPerFrame[d] = pExtensionsWriteDescriptorSet;
								}
							}

							//If drawArgsMaskToArgsIt->second.Args.GetSize() is smaller than pRenderStage->NumDrawArgsPerFrame then some Descriptor Sets are not destroyed that should be destroyed
							for (uint32 d = drawArgsMaskToArgsIt->second.Args.GetSize(); d < pRenderStage->NumDrawArgsPerFrame; d++)
							{
								{
									DescriptorSet* pSrcDescriptorSet = ppPrevDrawArgsPerFrame[d];
									m_pDeviceResourcesToDestroy[b].PushBack(pSrcDescriptorSet);
								}

								if (ppPrevDrawArgsExtensionsPerFrame)
								{
									DescriptorSet* pSrcDescriptorSet = ppPrevDrawArgsExtensionsPerFrame[d];
									if(pSrcDescriptorSet)
										m_pDeviceResourcesToDestroy[b].PushBack(pSrcDescriptorSet);
								}
							}

							if (resizeArr) SAFEDELETE_ARRAY(pRenderStage->pppDrawArgDescriptorSets[b]);

							pRenderStage->pppDrawArgDescriptorSets[b] = ppNewDrawArgsPerFrame;

							if (pRenderStage->pppDrawArgExtensionsDescriptorSets)
							{
								if (resizeArr) SAFEDELETE_ARRAY(pRenderStage->pppDrawArgExtensionsDescriptorSets[b]);
								pRenderStage->pppDrawArgExtensionsDescriptorSets[b] = ppNewDrawArgsExtensionsPerFrame;
							}
						}

						pRenderStage->NumDrawArgsPerFrame = drawArgsMaskToArgsIt->second.Args.GetSize();
					}
				}
			}

			m_DirtyBoundDrawArgResources.clear();
		}
	}

	void RenderGraph::Render(uint64 modFrameIndex, uint32 backBufferIndex)
	{
		m_BackBufferIndex = backBufferIndex;

		ZERO_MEMORY(m_ppExecutionStages, m_ExecutionStageCount * sizeof(CommandList*));

		uint32 currentExecutionStage = 0;

		s_pMaterialFence->Wait(m_SignalValue - 1, UINT64_MAX);

		TArray<DeviceChild*>& currentFrameDeviceResourcesToDestroy = m_pDeviceResourcesToDestroy[m_ModFrameIndex];

		if (!currentFrameDeviceResourcesToDestroy.IsEmpty())
		{
			for (DeviceChild* pDeviceChild : currentFrameDeviceResourcesToDestroy)
			{
				SAFERELEASE(pDeviceChild);
			}

			currentFrameDeviceResourcesToDestroy.Clear();
		}

		for (uint32 p = 0; p < m_PipelineStageCount; p++)
		{
			//Seperate Thread
			{
				PipelineStage* pPipelineStage = &m_pPipelineStages[p];

				if (pPipelineStage->Type == ERenderGraphPipelineStageType::RENDER)
				{
					RenderStage* pRenderStage = &m_pRenderStages[pPipelineStage->StageIndex];

					if (pRenderStage->UsesCustomRenderer)
					{
						if ((pRenderStage->FrameCounter != pRenderStage->FrameOffset) && pRenderStage->pDisabledRenderPass == nullptr)
							continue;

						CustomRenderer* pCustomRenderer = pRenderStage->pCustomRenderer;
						pCustomRenderer->Render(
							uint32(m_ModFrameIndex),
							m_BackBufferIndex,
							&m_ppExecutionStages[currentExecutionStage],
							&m_ppExecutionStages[currentExecutionStage + 1],
							pRenderStage->Sleeping);

						currentExecutionStage += 2;
					}
					else
					{
						switch (pRenderStage->pPipelineState->GetType())
						{
						case EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS:		ExecuteGraphicsRenderStage(pRenderStage,	pPipelineStage->ppGraphicsCommandAllocators[m_ModFrameIndex],	pPipelineStage->ppGraphicsCommandLists[m_ModFrameIndex],	&m_ppExecutionStages[currentExecutionStage]);	break;
						case EPipelineStateType::PIPELINE_STATE_TYPE_COMPUTE:		ExecuteComputeRenderStage(pRenderStage,		pPipelineStage->ppComputeCommandAllocators[m_ModFrameIndex],	pPipelineStage->ppComputeCommandLists[m_ModFrameIndex],		&m_ppExecutionStages[currentExecutionStage]);	break;
						case EPipelineStateType::PIPELINE_STATE_TYPE_RAY_TRACING:	ExecuteRayTracingRenderStage(pRenderStage,	pPipelineStage->ppComputeCommandAllocators[m_ModFrameIndex],	pPipelineStage->ppComputeCommandLists[m_ModFrameIndex],		&m_ppExecutionStages[currentExecutionStage]);	break;
						}
						currentExecutionStage++;
					}

					if (pRenderStage->TriggerType == ERenderStageExecutionTrigger::EVERY)
					{
						pRenderStage->FrameCounter++;

						if (pRenderStage->FrameCounter > pRenderStage->FrameDelay)
						{
							pRenderStage->FrameCounter = 0;
						}
					}
					else
					{
						//We set this to one, DISABLED and TRIGGERED wont trigger unless FrameCounter == 0
						pRenderStage->FrameCounter = 1;
					}
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
			CommandList* pGraphicsCopyCommandList = m_ppGraphicsCopyCommandLists[m_ModFrameIndex];

			if (pGraphicsCopyCommandList->IsBegin())
			{
				pGraphicsCopyCommandList->End();
				RenderAPI::GetGraphicsQueue()->ExecuteCommandLists(&pGraphicsCopyCommandList, 1, FPipelineStageFlag::PIPELINE_STAGE_FLAG_TOP, s_pMaterialFence, m_SignalValue - 1, s_pMaterialFence, m_SignalValue);
				m_SignalValue++;
			}

			CommandList* pComputeCopyCommandList = m_ppComputeCopyCommandLists[m_ModFrameIndex];

			if (pComputeCopyCommandList->IsBegin())
			{
				pComputeCopyCommandList->End();
				RenderAPI::GetComputeQueue()->ExecuteCommandLists(&pComputeCopyCommandList, 1, FPipelineStageFlag::PIPELINE_STAGE_FLAG_TOP, s_pMaterialFence, m_SignalValue - 1, s_pMaterialFence, m_SignalValue);
				m_SignalValue++;
			}
		}

		//Wait Threads

		//Execute the recorded Command Lists, we do this in a Batched mode where we batch as many "same queue" command lists that execute in succession together. This reduced the overhead caused by QueueSubmit
		{
			//This is safe since the first Execution Stage should never be nullptr
			ECommandQueueType currentBatchType = ECommandQueueType::COMMAND_QUEUE_TYPE_NONE;

			static TArray<CommandList*> currentBatch;

			for (uint32 e = 0; e < m_ExecutionStageCount; e++)
			{
				CommandList* pCommandList = m_ppExecutionStages[e];
				if (pCommandList != nullptr)
				{
					ECommandQueueType currentType = pCommandList->GetType();

					if (currentBatchType == ECommandQueueType::COMMAND_QUEUE_TYPE_NONE)
					{
						currentBatchType = currentType;
					}

					if (currentType != currentBatchType)
					{
						if (currentBatchType == ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS)
						{
							RenderAPI::GetGraphicsQueue()->ExecuteCommandLists(currentBatch.GetData(), currentBatch.GetSize(), FPipelineStageFlag::PIPELINE_STAGE_FLAG_TOP, s_pMaterialFence, m_SignalValue - 1, s_pMaterialFence, m_SignalValue);
						}
						else if (currentBatchType == ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE)
						{
							RenderAPI::GetComputeQueue()->ExecuteCommandLists(currentBatch.GetData(), currentBatch.GetSize(), FPipelineStageFlag::PIPELINE_STAGE_FLAG_TOP, s_pMaterialFence, m_SignalValue - 1, s_pMaterialFence, m_SignalValue);
						}

						m_SignalValue++;
						currentBatch.Clear();

						currentBatchType = currentType;
					}

					currentBatch.PushBack(pCommandList);
				}
			}

			if (!currentBatch.IsEmpty())
			{
				if (currentBatchType == ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS)
				{
					RenderAPI::GetGraphicsQueue()->ExecuteCommandLists(currentBatch.GetData(), currentBatch.GetSize(), FPipelineStageFlag::PIPELINE_STAGE_FLAG_TOP, s_pMaterialFence, m_SignalValue - 1, s_pMaterialFence, m_SignalValue);
				}
				else if (currentBatchType == ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE)
				{
					RenderAPI::GetComputeQueue()->ExecuteCommandLists(currentBatch.GetData(), currentBatch.GetSize(), FPipelineStageFlag::PIPELINE_STAGE_FLAG_TOP, s_pMaterialFence, m_SignalValue - 1, s_pMaterialFence, m_SignalValue);
				}

				m_SignalValue++;
				currentBatch.Clear();
			}
		}

		m_ModFrameIndex = modFrameIndex;
	}

	CommandList* RenderGraph::AcquireGraphicsCopyCommandList()
	{
		CommandList* pCommandList = m_ppGraphicsCopyCommandLists[m_ModFrameIndex];

		if (!pCommandList->IsBegin())
		{
			m_ppGraphicsCopyCommandAllocators[m_ModFrameIndex]->Reset();
			pCommandList->Begin(nullptr);
		}

		return pCommandList;
	}

	CommandList* RenderGraph::AcquireComputeCopyCommandList()
	{
		CommandList* pCommandList = m_ppComputeCopyCommandLists[m_ModFrameIndex];

		if (!pCommandList->IsBegin())
		{
			m_ppComputeCopyCommandAllocators[m_ModFrameIndex]->Reset();
			pCommandList->Begin(nullptr);
		}

		return pCommandList;
	}

	bool RenderGraph::GetResourceTextures(const char* pResourceName, Texture* const ** pppTexture, uint32* pTextureView) const
	{
		auto it = m_ResourceMap.find(pResourceName);

		if (it != m_ResourceMap.end())
		{
			(*pppTexture)		= it->second.Texture.Textures.GetData();
			(*pTextureView)		= (uint32)it->second.Texture.Textures.GetSize();
			return true;
		}

		return false;
	}

	bool RenderGraph::GetResourcePerImageTextureViews(const char* pResourceName, TextureView* const ** pppTextureViews, uint32* pTextureViewCount) const
	{
		auto it = m_ResourceMap.find(pResourceName);

		if (it != m_ResourceMap.end())
		{
			(*pppTextureViews)		= it->second.Texture.PerImageTextureViews.GetData();
			(*pTextureViewCount)	= (uint32)it->second.Texture.PerImageTextureViews.GetSize();
			return true;
		}

		return false;
	}

	bool RenderGraph::GetResourcePerSubImageTextureViews(const char* pResourceName, TextureView* const** pppTextureViews, uint32* pTextureViewCount) const
	{
		auto it = m_ResourceMap.find(pResourceName);

		if (it != m_ResourceMap.end())
		{
			(*pppTextureViews) = it->second.Texture.PerSubImageTextureViews.GetData();
			(*pTextureViewCount) = (uint32)it->second.Texture.PerSubImageTextureViews.GetSize();
			return true;
		}

		return false;
	}

	bool RenderGraph::GetResourceBuffers(const char* pResourceName, Buffer* const ** pppBuffers, uint32* pBufferCount) const
	{
		auto it = m_ResourceMap.find(pResourceName);

		if (it != m_ResourceMap.end())
		{
			(*pppBuffers)			= it->second.Buffer.Buffers.GetData();
			(*pBufferCount)			= (uint32)it->second.Buffer.Buffers.GetSize();
			return true;
		}

		return false;
	}

	bool RenderGraph::GetResourceAccelerationStructure(const char* pResourceName, AccelerationStructure const ** ppAccelerationStructure) const
	{
		auto it = m_ResourceMap.find(pResourceName);

		if (it != m_ResourceMap.end())
		{
			(*ppAccelerationStructure) = it->second.AccelerationStructure.pTLAS;
			return true;
		}

		return false;
	}

	bool RenderGraph::OnWindowResized(const WindowResizedEvent& windowEvent)
	{
		if (IsEventOfType<WindowResizedEvent>(windowEvent))
		{
			m_WindowWidth	= (float32)windowEvent.Width;
			m_WindowHeight	= (float32)windowEvent.Height;

			UpdateRelativeParameters();

			return true;
		}
		else
		{
			return false;
		}
	}

	bool RenderGraph::OnPreSwapChainRecreated(const PreSwapChainRecreatedEvent& swapChainEvent)
	{
		UNREFERENCED_VARIABLE(swapChainEvent);

		auto backBufferResourceIt = m_ResourceMap.find(RENDER_GRAPH_BACK_BUFFER_ATTACHMENT);

		if (backBufferResourceIt != m_ResourceMap.end())
		{
			for (const ResourceBinding& binding : backBufferResourceIt->second.ResourceBindings)
			{
				if (binding.pRenderStage->UsesCustomRenderer)
				{
					binding.pRenderStage->pCustomRenderer->PreTexturesDescriptorSetWrite();
				}
				else if (binding.DescriptorType != EDescriptorType::DESCRIPTOR_TYPE_UNKNOWN)
				{
					for (uint32 b = 0; b < m_BackBufferCount; b++)
					{
						DescriptorSet* pSrcDescriptorSet = binding.pRenderStage->ppTextureDescriptorSets[b];
						DescriptorSet* pDescriptorSet = m_pGraphicsDevice->CreateDescriptorSet(pSrcDescriptorSet->GetName(), binding.pRenderStage->pPipelineLayout, binding.pRenderStage->TextureSetIndex, m_pDescriptorHeap);
						m_pGraphicsDevice->CopyDescriptorSet(pSrcDescriptorSet, pDescriptorSet);
						m_pDeviceResourcesToDestroy[b].PushBack(pSrcDescriptorSet);
						binding.pRenderStage->ppTextureDescriptorSets[b] = pDescriptorSet;
					}
				}
			}
		}

		return true;
	}

	bool RenderGraph::OnPostSwapChainRecreated(const PostSwapChainRecreatedEvent& swapChainEvent)
	{
		UNREFERENCED_VARIABLE(swapChainEvent);

		auto backBufferResourceIt = m_ResourceMap.find(RENDER_GRAPH_BACK_BUFFER_ATTACHMENT);

		if (backBufferResourceIt != m_ResourceMap.end())
		{
			for (const ResourceBinding& binding : backBufferResourceIt->second.ResourceBindings)
			{
				if (binding.pRenderStage->UsesCustomRenderer)
				{
					binding.pRenderStage->pCustomRenderer->UpdateTextureResource(
						backBufferResourceIt->second.Name,
						backBufferResourceIt->second.Texture.PerImageTextureViews.GetData(),
						backBufferResourceIt->second.Texture.PerSubImageTextureViews.GetData(),
						backBufferResourceIt->second.Texture.PerImageTextureViews.GetSize(),
						backBufferResourceIt->second.Texture.PerSubImageTextureViews.GetSize(),
						true);
				}
				else if (binding.DescriptorType != EDescriptorType::DESCRIPTOR_TYPE_UNKNOWN)
				{
					for (uint32 b = 0; b < m_BackBufferCount; b++)
					{
						binding.pRenderStage->ppTextureDescriptorSets[b]->WriteTextureDescriptors(
							&backBufferResourceIt->second.Texture.PerImageTextureViews[b],
							&backBufferResourceIt->second.Texture.Samplers[b],
							binding.TextureState,
							binding.Binding,
							1,
							binding.DescriptorType,
							true);
					}
				}
			}
		}

		return true;
	}

	bool RenderGraph::OnPipelineStatesRecompiled(const PipelineStatesRecompiledEvent& event)
	{
		UNREFERENCED_VARIABLE(event);

		for (uint32 r = 0; r < m_RenderStageCount; r++)
		{
			RenderStage* pRenderStage = &m_pRenderStages[r];
			if (!pRenderStage->UsesCustomRenderer)
			{
				pRenderStage->pPipelineState = PipelineStateManager::GetPipelineState(pRenderStage->PipelineStateID);

				if (pRenderStage->pPipelineState->GetType() == EPipelineStateType::PIPELINE_STATE_TYPE_RAY_TRACING)
				{
					m_pDeviceResourcesToDestroy[m_ModFrameIndex].PushBack(pRenderStage->pSBT);

					SBTDesc sbtDesc = {};
					sbtDesc.DebugName = "Render Graph Global SBT";
					sbtDesc.pPipelineState = pRenderStage->pPipelineState;
					sbtDesc.SBTRecords = m_GlobalShaderRecords;

					pRenderStage->pSBT = RenderAPI::GetDevice()->CreateSBT(AcquireComputeCopyCommandList(), &sbtDesc);
				}
			}
		}

		return true;
	}

	void RenderGraph::ReleasePipelineStages()
	{
		SAFEDELETE_ARRAY(m_ppExecutionStages);

		for (uint32 i = 0; i < m_PipelineStageCount; i++)
		{
			PipelineStage* pPipelineStage = &m_pPipelineStages[i];

			if (pPipelineStage->ppComputeCommandAllocators != nullptr)
			{
				for (uint32 b = 0; b < m_BackBufferCount; b++)
				{
					SAFERELEASE(pPipelineStage->ppComputeCommandAllocators[b]);
					SAFERELEASE(pPipelineStage->ppComputeCommandLists[b]);
				}

				SAFEDELETE_ARRAY(pPipelineStage->ppComputeCommandAllocators);
				SAFEDELETE_ARRAY(pPipelineStage->ppComputeCommandLists);
			}

			if (pPipelineStage->ppGraphicsCommandAllocators != nullptr)
			{
				for (uint32 b = 0; b < m_BackBufferCount; b++)
				{
					SAFERELEASE(pPipelineStage->ppGraphicsCommandAllocators[b]);
					SAFERELEASE(pPipelineStage->ppGraphicsCommandLists[b]);
				}

				SAFEDELETE_ARRAY(pPipelineStage->ppGraphicsCommandAllocators);
				SAFEDELETE_ARRAY(pPipelineStage->ppGraphicsCommandLists);
			}

			if (pPipelineStage->Type == ERenderGraphPipelineStageType::RENDER)
			{
				RenderStage* pRenderStage = &m_pRenderStages[pPipelineStage->StageIndex];

				for (uint32 b = 0; b < m_BackBufferCount; b++)
				{
					for (uint32 ipc = 0; ipc < NUM_INTERNAL_PUSH_CONSTANTS_TYPES; ipc++)
					{
						SAFEDELETE_ARRAY(pRenderStage->pInternalPushConstants[ipc].pData);
					}

					SAFEDELETE_ARRAY(pRenderStage->ExternalPushConstants.pData);

					if (pRenderStage->ppTextureDescriptorSets != nullptr)
						SAFERELEASE(pRenderStage->ppTextureDescriptorSets[b]);

					if (pRenderStage->ppBufferDescriptorSets != nullptr)
						SAFERELEASE(pRenderStage->ppBufferDescriptorSets[b]);

					if (pRenderStage->pppDrawArgDescriptorSets != nullptr)
					{
						DescriptorSet** ppDrawArgDescriptorSets = pRenderStage->pppDrawArgDescriptorSets[b];

						for (uint32 d = 0; d < pRenderStage->NumDrawArgsPerFrame; d++)
						{
							SAFERELEASE(ppDrawArgDescriptorSets[d]);
						}

						SAFEDELETE_ARRAY(ppDrawArgDescriptorSets);
					}

					// Release draw arg extensions descriptor sets.
					if (pRenderStage->pppDrawArgExtensionsDescriptorSets != nullptr)
					{
						DescriptorSet** ppDrawArgExtensionsDescriptrSet = pRenderStage->pppDrawArgExtensionsDescriptorSets[b];

						if (ppDrawArgExtensionsDescriptrSet)
						{
							for (uint32 d = 0; d < pRenderStage->NumDrawArgsPerFrame; d++)
							{
								SAFERELEASE(ppDrawArgExtensionsDescriptrSet[d]);
							}
						}

						SAFEDELETE_ARRAY(ppDrawArgExtensionsDescriptrSet);
					}
				}

				SAFEDELETE_ARRAY(pRenderStage->ppTextureDescriptorSets);
				SAFEDELETE_ARRAY(pRenderStage->ppBufferDescriptorSets);
				SAFEDELETE_ARRAY(pRenderStage->pppDrawArgDescriptorSets);
				SAFEDELETE_ARRAY(pRenderStage->pppDrawArgExtensionsDescriptorSets);
				SAFERELEASE(pRenderStage->pPipelineLayout);
				SAFERELEASE(pRenderStage->pSBT);
				SAFERELEASE(pRenderStage->pRenderPass);
				SAFERELEASE(pRenderStage->pDisabledRenderPass);
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

		Profiler::GetGPUProfiler()->Release();
	}

	bool RenderGraph::CreateFence()
	{
		FenceDesc fenceDesc = {};
		fenceDesc.DebugName		= "Render Stage Fence";
		fenceDesc.InitalValue	= 0;

		s_pMaterialFence = m_pGraphicsDevice->CreateFence(&fenceDesc);

		if (s_pMaterialFence == nullptr)
		{
			LOG_ERROR("[RenderGraph]: Could not create RenderGraph fence");
			return false;
		}

		return true;
	}

	bool RenderGraph::CreateDescriptorHeap()
	{
		constexpr uint32 DESCRIPTOR_COUNT = 4096;

		DescriptorHeapInfo descriptorCountDesc = { };
		descriptorCountDesc.SamplerDescriptorCount						= DESCRIPTOR_COUNT;
		descriptorCountDesc.TextureDescriptorCount						= DESCRIPTOR_COUNT;
		descriptorCountDesc.TextureCombinedSamplerDescriptorCount		= DESCRIPTOR_COUNT;
		descriptorCountDesc.ConstantBufferDescriptorCount				= DESCRIPTOR_COUNT;
		descriptorCountDesc.UnorderedAccessBufferDescriptorCount		= DESCRIPTOR_COUNT;
		descriptorCountDesc.UnorderedAccessTextureDescriptorCount		= DESCRIPTOR_COUNT;
		descriptorCountDesc.AccelerationStructureDescriptorCount		= DESCRIPTOR_COUNT;

		DescriptorHeapDesc descriptorHeapDesc = { };
		descriptorHeapDesc.DebugName			= "Render Graph Descriptor Heap";
		descriptorHeapDesc.DescriptorSetCount	= DESCRIPTOR_COUNT;
		descriptorHeapDesc.DescriptorCount		= descriptorCountDesc;

		m_pDescriptorHeap = m_pGraphicsDevice->CreateDescriptorHeap(&descriptorHeapDesc);

		return m_pDescriptorHeap != nullptr;
	}

	bool RenderGraph::CreateCopyCommandLists()
	{
		m_ppGraphicsCopyCommandAllocators	= DBG_NEW CommandAllocator*[m_BackBufferCount];
		m_ppGraphicsCopyCommandLists		= DBG_NEW CommandList*[m_BackBufferCount];
		m_ppComputeCopyCommandAllocators	= DBG_NEW CommandAllocator*[m_BackBufferCount];
		m_ppComputeCopyCommandLists			= DBG_NEW CommandList *[m_BackBufferCount];

		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			//Graphics
			{
				m_ppGraphicsCopyCommandAllocators[b]		= m_pGraphicsDevice->CreateCommandAllocator("Render Graph Graphics Copy Command Allocator", ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);

				if (m_ppGraphicsCopyCommandAllocators[b] == nullptr)
				{
					return false;
				}

				CommandListDesc graphicsCopyCommandListDesc		= {};
				graphicsCopyCommandListDesc.DebugName			= "Render Graph Graphics Copy Command List";
				graphicsCopyCommandListDesc.CommandListType		= ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
				graphicsCopyCommandListDesc.Flags				= FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

				m_ppGraphicsCopyCommandLists[b] = m_pGraphicsDevice->CreateCommandList(m_ppGraphicsCopyCommandAllocators[b], &graphicsCopyCommandListDesc);

				if (m_ppGraphicsCopyCommandLists[b] == nullptr)
				{
					return false;
				}
			}

			//Compute
			{
				m_ppComputeCopyCommandAllocators[b] = m_pGraphicsDevice->CreateCommandAllocator("Render Graph Compute Copy Command Allocator", ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE);

				if (m_ppComputeCopyCommandAllocators[b] == nullptr)
				{
					return false;
				}

				CommandListDesc computeCopyCommandListDesc		= {};
				computeCopyCommandListDesc.DebugName			= "Render Graph Compute Copy Command List";
				computeCopyCommandListDesc.CommandListType		= ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
				computeCopyCommandListDesc.Flags				= FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

				m_ppComputeCopyCommandLists[b] = m_pGraphicsDevice->CreateCommandList(m_ppComputeCopyCommandAllocators[b], &computeCopyCommandListDesc);

				if (m_ppComputeCopyCommandLists[b] == nullptr)
				{
					return false;
				}
			}
		}

		return true;
	}

	bool RenderGraph::CreateProfiler(uint32 pipelineStageCount)
	{
		Profiler::GetGPUProfiler()->Init(GPUProfiler::TimeUnit::MICRO);
		Profiler::GetGPUProfiler()->CreateTimestamps(pipelineStageCount * m_BackBufferCount * 2);
		Profiler::GetGPUProfiler()->CreateGraphicsPipelineStats();

		return true;
	}

	bool RenderGraph::CreateResources(const TArray<RenderGraphResourceDesc>& resourceDescriptions)
	{
		m_ResourceMap.reserve(resourceDescriptions.GetSize());
		TArray<String> addedResourceNames;
		addedResourceNames.Reserve(resourceDescriptions.GetSize());

		for (uint32 i = 0; i < resourceDescriptions.GetSize(); i++)
		{
			const RenderGraphResourceDesc* pResourceDesc = &resourceDescriptions[i];
			addedResourceNames.PushBack(pResourceDesc->Name);

			Resource newResource;
			InternalResourceUpdateDesc internalResourceUpdateDesc = {};
			bool isRelativeInternal = false;

			newResource.Name				= pResourceDesc->Name;
			newResource.IsBackBuffer		= pResourceDesc->Name == RENDER_GRAPH_BACK_BUFFER_ATTACHMENT;
			newResource.BackBufferBound		= newResource.IsBackBuffer || newResource.BackBufferBound;
			newResource.ShouldSynchronize	= pResourceDesc->ShouldSynchronize;

			if (newResource.BackBufferBound)
			{
				newResource.SubResourceCount = m_BackBufferCount;
			}
			else
			{
				newResource.SubResourceCount = pResourceDesc->SubResourceCount;
			}

			uint32 arrayCount					= 0;
			ETextureViewType textureViewType	= ETextureViewType::TEXTURE_VIEW_TYPE_NONE;
			bool isCubeTexture					= pResourceDesc->TextureParams.TextureType == ERenderGraphTextureType::TEXTURE_CUBE;

			//Create Resource Entries, this is independent of whether the resource is internal/external
			if (pResourceDesc->Type == ERenderGraphResourceType::TEXTURE)
			{
				newResource.Type					= ERenderGraphResourceType::TEXTURE;
				newResource.Texture.TextureType		= pResourceDesc->TextureParams.TextureType;
				newResource.Texture.IsOfArrayType	= pResourceDesc->TextureParams.IsOfArrayType;
				newResource.Texture.UnboundedArray	= pResourceDesc->TextureParams.UnboundedArray;
				newResource.Texture.Format			= pResourceDesc->TextureParams.TextureFormat;
				newResource.Texture.TextureType		= pResourceDesc->TextureParams.TextureType;

				if (!pResourceDesc->TextureParams.UnboundedArray)
				{
					if (!pResourceDesc->TextureParams.IsOfArrayType)
					{
						//Not of Array Type -> we need space for SubResourceCount Textures/PerImageTextureViews/Samplers
						newResource.Texture.Textures.Resize(newResource.SubResourceCount);
						newResource.Texture.PerImageTextureViews.Resize(newResource.SubResourceCount);
						newResource.Texture.Samplers.Resize(newResource.SubResourceCount);

						//If Cube Texture we also need space for per sub image texture views
						if (isCubeTexture)
						{
							arrayCount = 6;
							textureViewType = ETextureViewType::TEXTURE_VIEW_TYPE_CUBE;
							newResource.Texture.PerSubImageTextureViews.Resize(newResource.SubResourceCount * arrayCount);

							newResource.Texture.PerSubImageUniquelyAllocated = true;
						}
						else
						{
							arrayCount = 1;
							textureViewType = ETextureViewType::TEXTURE_VIEW_TYPE_2D;
							newResource.Texture.PerSubImageTextureViews.Resize(newResource.SubResourceCount);
						}
					}
					else
					{
						//Of Array Type -> we only need space for 1 Textures/PerImageTextureViews/Samplers
						newResource.Texture.Textures.Resize(1);
						newResource.Texture.PerImageTextureViews.Resize(1);
						newResource.Texture.Samplers.Resize(1);

						//If Cube Texture we also need space for per sub image texture views
						if (isCubeTexture)
						{
							arrayCount = newResource.SubResourceCount * 6;
							textureViewType = ETextureViewType::TEXTURE_VIEW_TYPE_CUBE_ARRAY;
							newResource.Texture.PerSubImageTextureViews.Resize(arrayCount);

							newResource.Texture.PerSubImageUniquelyAllocated = true;
						}
						else
						{
							arrayCount = newResource.SubResourceCount;
							textureViewType = ETextureViewType::TEXTURE_VIEW_TYPE_2D_ARRAY;
							newResource.Texture.PerSubImageTextureViews.Resize(1);

							newResource.Texture.PerSubImageUniquelyAllocated = true;
						}
					}
				}
			}
			else if (pResourceDesc->Type == ERenderGraphResourceType::SCENE_DRAW_ARGS)
			{
				newResource.Type = ERenderGraphResourceType::SCENE_DRAW_ARGS;
			}
			else if (pResourceDesc->Type == ERenderGraphResourceType::BUFFER)
			{
				newResource.Type = ERenderGraphResourceType::BUFFER;

				newResource.Buffer.Buffers.Resize(newResource.SubResourceCount);
				newResource.Buffer.Offsets.Resize(newResource.SubResourceCount);
				newResource.Buffer.SizesInBytes.Resize(newResource.SubResourceCount);
			}
			else if (pResourceDesc->Type == ERenderGraphResourceType::ACCELERATION_STRUCTURE)
			{
				newResource.Type = ERenderGraphResourceType::ACCELERATION_STRUCTURE;
			}

			//Create Internal Update Descriptions if the resource is internal, otherwise just set that it's external
			if (!pResourceDesc->External)
			{
				//Internal
				if (pResourceDesc->Type == ERenderGraphResourceType::TEXTURE)
				{
					newResource.OwnershipType				= newResource.IsBackBuffer ? EResourceOwnershipType::EXTERNAL : EResourceOwnershipType::INTERNAL;

					if (!newResource.IsBackBuffer)
					{
						TextureDesc		textureDesc			= {};
						TextureViewDesc textureViewDesc		= {};
						SamplerDesc		samplerDesc			= {};

						textureDesc.DebugName			= !isCubeTexture ? pResourceDesc->Name + " Texture" : pResourceDesc->Name + " Texture Cube";
						textureDesc.MemoryType			= pResourceDesc->MemoryType;
						textureDesc.Format				= pResourceDesc->TextureParams.TextureFormat;
						textureDesc.Type				= ETextureType::TEXTURE_TYPE_2D;
						textureDesc.Flags				= pResourceDesc->TextureParams.TextureFlags;
						textureDesc.Width				= uint32(pResourceDesc->TextureParams.XDimVariable);
						textureDesc.Height				= uint32(pResourceDesc->TextureParams.YDimVariable);
						textureDesc.Depth				= 1U;
						textureDesc.ArrayCount			= arrayCount;
						textureDesc.Miplevels			= pResourceDesc->TextureParams.MiplevelCount;
						textureDesc.SampleCount			= pResourceDesc->TextureParams.SampleCount;

						textureViewDesc.DebugName		= !isCubeTexture ? pResourceDesc->Name + " Texture View" : pResourceDesc->Name + " Texture Cube View";
						textureViewDesc.pTexture		= nullptr;
						textureViewDesc.Flags			= pResourceDesc->TextureParams.TextureViewFlags;
						textureViewDesc.Format			= pResourceDesc->TextureParams.TextureFormat;
						textureViewDesc.Type			= textureViewType;
						textureViewDesc.MiplevelCount	= pResourceDesc->TextureParams.MiplevelCount;
						textureViewDesc.ArrayCount		= arrayCount;
						textureViewDesc.Miplevel		= 0U;
						textureViewDesc.ArrayIndex		= 0U;

						samplerDesc.DebugName			= pResourceDesc->Name + " Sampler";
						samplerDesc.MinFilter			= RenderGraphSamplerToFilter(pResourceDesc->TextureParams.SamplerType);
						samplerDesc.MagFilter			= RenderGraphSamplerToFilter(pResourceDesc->TextureParams.SamplerType);
						samplerDesc.MipmapMode			= RenderGraphSamplerToMipmapMode(pResourceDesc->TextureParams.SamplerType);
						samplerDesc.AddressModeU		= RenderGraphSamplerAddressMode(pResourceDesc->TextureParams.SamplerAddressMode);
						samplerDesc.AddressModeV		= RenderGraphSamplerAddressMode(pResourceDesc->TextureParams.SamplerAddressMode);
						samplerDesc.AddressModeW		= RenderGraphSamplerAddressMode(pResourceDesc->TextureParams.SamplerAddressMode);
						samplerDesc.borderColor			= RenderGraphSamplerBorderColor(pResourceDesc->TextureParams.SamplerBorderColor);
						samplerDesc.MipLODBias			= 0.0f;
						samplerDesc.AnisotropyEnabled	= false;
						samplerDesc.MaxAnisotropy		= 16;
						samplerDesc.MinLOD				= 0.0f;
						samplerDesc.MaxLOD				= 1.0f;

						internalResourceUpdateDesc.ResourceName						= pResourceDesc->Name;
						internalResourceUpdateDesc.Type								= ERenderGraphResourceType::TEXTURE;
						internalResourceUpdateDesc.TextureUpdate.XDimType			= pResourceDesc->TextureParams.XDimType;
						internalResourceUpdateDesc.TextureUpdate.YDimType			= pResourceDesc->TextureParams.YDimType;
						internalResourceUpdateDesc.TextureUpdate.XDimVariable		= pResourceDesc->TextureParams.XDimVariable;
						internalResourceUpdateDesc.TextureUpdate.YDimVariable		= pResourceDesc->TextureParams.YDimVariable;
						internalResourceUpdateDesc.TextureUpdate.TextureDesc		= textureDesc;
						internalResourceUpdateDesc.TextureUpdate.TextureViewDesc	= textureViewDesc;
						internalResourceUpdateDesc.TextureUpdate.SamplerDesc		= samplerDesc;

						if (pResourceDesc->TextureParams.XDimType == ERenderGraphDimensionType::RELATIVE ||
							pResourceDesc->TextureParams.YDimType == ERenderGraphDimensionType::RELATIVE)
						{
							isRelativeInternal = true;
						}
					}
				}
				else if (pResourceDesc->Type == ERenderGraphResourceType::BUFFER)
				{
					newResource.Type			= ERenderGraphResourceType::BUFFER;

					BufferDesc bufferDesc = {};
					bufferDesc.DebugName		= pResourceDesc->Name + " Buffer";
					bufferDesc.MemoryType		= pResourceDesc->MemoryType;
					bufferDesc.Flags			= pResourceDesc->BufferParams.BufferFlags;
					bufferDesc.SizeInBytes		= pResourceDesc->BufferParams.Size;

					internalResourceUpdateDesc.ResourceName						= pResourceDesc->Name;
					internalResourceUpdateDesc.Type								= ERenderGraphResourceType::BUFFER;
					internalResourceUpdateDesc.BufferUpdate.SizeType			= pResourceDesc->BufferParams.SizeType;
					internalResourceUpdateDesc.BufferUpdate.BufferDesc			= bufferDesc;

					if (pResourceDesc->BufferParams.SizeType == ERenderGraphDimensionType::RELATIVE)
					{
						isRelativeInternal = true;
					}
				}
				else
				{
					LOG_ERROR("[RenderGraph]: Unsupported resource type for internal resource \"%s\"", newResource.Name.c_str());
					return false;
				}
			}
			else
			{
				newResource.OwnershipType = EResourceOwnershipType::EXTERNAL;
			}

			//Now that we have created a resource(desc), we should check if the same resource exists (all important parameters need to be the same)
			bool alreadyExists = false;
			auto previousResourceIt = m_ResourceMap.find(newResource.Name);
			if (previousResourceIt != m_ResourceMap.end())
			{
				alreadyExists = true;

				Resource& previousResource = previousResourceIt->second;
				previousResource.ResourceBindings.Clear();

				alreadyExists = alreadyExists && newResource.IsBackBuffer		== previousResource.IsBackBuffer;
				alreadyExists = alreadyExists && newResource.BackBufferBound	== previousResource.BackBufferBound;
				alreadyExists = alreadyExists && newResource.SubResourceCount	== previousResource.SubResourceCount;
				alreadyExists = alreadyExists && newResource.OwnershipType		== previousResource.OwnershipType;
				alreadyExists = alreadyExists && newResource.Type				== previousResource.Type;

				//Should be safe to clear, as this resource will either be the one we use or be overwritten
				previousResource.BarriersPerSynchronizationStage.Clear();

				if (newResource.Type == ERenderGraphResourceType::TEXTURE)
				{
					alreadyExists = alreadyExists && newResource.Texture.TextureType						== previousResource.Texture.TextureType;
					alreadyExists = alreadyExists && newResource.Texture.IsOfArrayType						== previousResource.Texture.IsOfArrayType;
					alreadyExists = alreadyExists && newResource.Texture.Format								== previousResource.Texture.Format;
					alreadyExists = alreadyExists && newResource.Texture.Textures.GetSize()					== previousResource.Texture.Textures.GetSize();
					alreadyExists = alreadyExists && newResource.Texture.PerImageTextureViews.GetSize()		== previousResource.Texture.PerImageTextureViews.GetSize();
					alreadyExists = alreadyExists && newResource.Texture.PerSubImageTextureViews.GetSize()	== previousResource.Texture.PerSubImageTextureViews.GetSize();
					alreadyExists = alreadyExists && newResource.Texture.Samplers.GetSize()					== previousResource.Texture.Samplers.GetSize();

					previousResource.Texture.InitialTransitionBarrier = {};

					//If the resource is discovered as nonexisiting here, we need to release internal subresources
					if (!alreadyExists && previousResource.OwnershipType == EResourceOwnershipType::INTERNAL)
					{
						for (uint32 sr = 0; sr < previousResource.SubResourceCount; sr++)
						{
							SAFERELEASE(previousResource.Texture.Textures[sr]);
							SAFERELEASE(previousResource.Texture.PerImageTextureViews[sr]);
							SAFERELEASE(previousResource.Texture.Samplers[sr]);
						}

						if (previousResource.Texture.UsedAsRenderTarget && previousResource.Texture.PerSubImageUniquelyAllocated)
						{
							for (TextureView* pPerSubImageTextureView : previousResource.Texture.PerSubImageTextureViews)
							{
								SAFERELEASE(pPerSubImageTextureView);
							}
						}
					}
				}
				else if (newResource.Type == ERenderGraphResourceType::SCENE_DRAW_ARGS)
				{
					//Nothing to check here
				}
				else if (newResource.Type == ERenderGraphResourceType::BUFFER)
				{
					alreadyExists = alreadyExists && newResource.Buffer.Buffers.GetSize()			== previousResource.Buffer.Buffers.GetSize();
					alreadyExists = alreadyExists && newResource.Buffer.Offsets.GetSize()			== previousResource.Buffer.Offsets.GetSize();
					alreadyExists = alreadyExists && newResource.Buffer.SizesInBytes.GetSize()		== previousResource.Buffer.SizesInBytes.GetSize();

					previousResource.Buffer.InitialTransitionBarrier = {};

					//If the resource is discovered as nonexisiting here, we need to release internal subresources
					if (!alreadyExists && previousResource.OwnershipType == EResourceOwnershipType::INTERNAL)
					{
						for (uint32 sr = 0; sr < previousResource.SubResourceCount; sr++)
						{
							SAFERELEASE(previousResource.Buffer.Buffers[sr]);
						}
					}
				}
				else if (newResource.Type == ERenderGraphResourceType::ACCELERATION_STRUCTURE)
				{
					//Nothing to check here
				}
			}

			if (!alreadyExists)
			{
				m_ResourceMap[newResource.Name] = newResource;
			}

			if (newResource.OwnershipType == EResourceOwnershipType::INTERNAL && !newResource.IsBackBuffer)
			{
				m_InternalResourceUpdateDescriptions[pResourceDesc->Name] = internalResourceUpdateDesc;
				m_DirtyInternalResources.insert(pResourceDesc->Name);
			}

			if (isRelativeInternal)
			{
				m_WindowRelativeResources.PushBack(pResourceDesc->Name);
			}
		}

		//Remove Resources that are left from previous Creations
		for (auto resourceIt = m_ResourceMap.begin(); resourceIt != m_ResourceMap.end();)
		{
			auto addedResourceNamesIt = std::find_if(addedResourceNames.Begin(), addedResourceNames.End(), [resourceIt](const String& addedResourceName) { return resourceIt->second.Name == addedResourceName; });

			if (addedResourceNamesIt == addedResourceNames.end())
			{
				Resource* pResource = &resourceIt->second;

				if (pResource->OwnershipType == EResourceOwnershipType::INTERNAL)
				{
					if (pResource->Type == ERenderGraphResourceType::TEXTURE)
					{
						for (uint32 sr = 0; sr < pResource->SubResourceCount; sr++)
						{
							SAFERELEASE(pResource->Texture.Textures[sr]);
							SAFERELEASE(pResource->Texture.PerImageTextureViews[sr]);
							SAFERELEASE(pResource->Texture.Samplers[sr]);
						}

						if (pResource->Texture.UsedAsRenderTarget && pResource->Texture.PerSubImageUniquelyAllocated)
						{
							for (TextureView* pPerSubImageTextureView : pResource->Texture.PerSubImageTextureViews)
							{
								SAFERELEASE(pPerSubImageTextureView);
							}
						}
					}
					else if (pResource->Type == ERenderGraphResourceType::BUFFER)
					{
						for (uint32 sr = 0; sr < pResource->SubResourceCount; sr++)
						{
							SAFERELEASE(pResource->Buffer.Buffers[sr]);
						}
					}
				}

				resourceIt = m_ResourceMap.erase(resourceIt);
			}
			else
			{
				resourceIt++;
			}
		}

		return true;
	}

	bool RenderGraph::CreateRenderStages(const TArray<RenderStageDesc>& renderStages, const THashTable<String, RenderGraphShaderConstants>& shaderConstants, const TArray<CustomRenderer*>& customRenderers, TSet<DrawArgMaskDesc>& requiredDrawArgMasks)
	{
		m_RenderStageCount = (uint32)renderStages.GetSize();
		m_RenderStageMap.reserve(m_RenderStageCount);
		m_pRenderStages = DBG_NEW RenderStage[m_RenderStageCount];

		for (uint32 renderStageIndex = 0; renderStageIndex < m_RenderStageCount; renderStageIndex++)
		{
			const RenderStageDesc* pRenderStageDesc = &renderStages[renderStageIndex];

			RenderStage* pRenderStage = &m_pRenderStages[renderStageIndex];
			m_RenderStageMap[pRenderStageDesc->Name] = renderStageIndex;

			bool isImGuiStage = pRenderStageDesc->Name == RENDER_GRAPH_IMGUI_STAGE_NAME;

			pRenderStage->Name			= pRenderStageDesc->Name;
			pRenderStage->Parameters	= pRenderStageDesc->Parameters;

			if (pRenderStage->Parameters.XDimType == ERenderGraphDimensionType::RELATIVE ||
				pRenderStage->Parameters.XDimType == ERenderGraphDimensionType::RELATIVE_1D ||
				pRenderStage->Parameters.YDimType == ERenderGraphDimensionType::RELATIVE)
			{
				m_WindowRelativeRenderStages.insert(renderStageIndex);

				UpdateRelativeRenderStageDimensions(pRenderStage);
			}

			if (pRenderStage->Parameters.XDimType == ERenderGraphDimensionType::CONSTANT)
			{
				pRenderStage->Dimensions.x = uint32(pRenderStageDesc->Parameters.XDimVariable);
			}

			if (pRenderStage->Parameters.YDimType == ERenderGraphDimensionType::CONSTANT)
			{
				pRenderStage->Dimensions.y = uint32(pRenderStageDesc->Parameters.YDimVariable);
			}

			if (pRenderStage->Parameters.ZDimType == ERenderGraphDimensionType::CONSTANT)
			{
				pRenderStage->Dimensions.z = uint32(pRenderStageDesc->Parameters.ZDimVariable);
			}

			if (pRenderStageDesc->Type == EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS)
			{
				pRenderStage->DrawType = pRenderStageDesc->Graphics.DrawType;
				if (pRenderStageDesc->Graphics.DrawType == ERenderStageDrawType::SCENE_INSTANCES)
				{
					if (!pRenderStageDesc->Graphics.Shaders.MeshShaderName.empty())
					{
						pRenderStage->DrawType = ERenderStageDrawType::SCENE_INSTANCES_MESH_SHADER;
					}
				}
			}

			pRenderStage->Dimensions.x = glm::max<uint32>(1, pRenderStage->Dimensions.x);
			pRenderStage->Dimensions.y = glm::max<uint32>(1, pRenderStage->Dimensions.y);
			pRenderStage->Dimensions.z = glm::max<uint32>(1, pRenderStage->Dimensions.z);

			TArray<DescriptorBindingDesc> textureDescriptorSetDescriptions;
			textureDescriptorSetDescriptions.Reserve(pRenderStageDesc->ResourceStates.GetSize());
			uint32 textureDescriptorBindingIndex = 0;

			TArray<DescriptorBindingDesc> bufferDescriptorSetDescriptions;
			bufferDescriptorSetDescriptions.Reserve(pRenderStageDesc->ResourceStates.GetSize());
			uint32 bufferDescriptorBindingIndex = 0;

			TArray<DescriptorBindingDesc> drawArgDescriptorSetDescriptions;
			drawArgDescriptorSetDescriptions.Reserve(pRenderStageDesc->ResourceStates.GetSize());

			TArray<DescriptorBindingDesc> drawArgExtensionDescriptorSetDescriptions;
			drawArgExtensionDescriptorSetDescriptions.Reserve(pRenderStageDesc->ResourceStates.GetSize());

			TArray<RenderPassAttachmentDesc>								renderPassAttachmentDescriptions;
			RenderPassAttachmentDesc										renderPassDepthStencilDescription;
			TArray<ETextureState>											renderPassRenderTargetStates;
			TArray<BlendAttachmentStateDesc>								renderPassBlendAttachmentStates;
			TArray<std::pair<Resource*, ETextureState>>						renderStageRenderTargets;
			Resource*														pDepthStencilResource = nullptr;
			TArray<std::tuple<Resource*, ETextureState, EDescriptorType>>	renderStageTextureResources;
			TArray<std::tuple<Resource*, EDescriptorType>>					renderStageBufferResources;
			TArray<std::tuple<Resource*, EDescriptorType>>					renderStageDrawArgResources;
			renderPassAttachmentDescriptions.Reserve(pRenderStageDesc->ResourceStates.GetSize());
			renderPassRenderTargetStates.Reserve(pRenderStageDesc->ResourceStates.GetSize());
			renderPassBlendAttachmentStates.Reserve(pRenderStageDesc->ResourceStates.GetSize());
			renderStageRenderTargets.Reserve(pRenderStageDesc->ResourceStates.GetSize());
			renderStageTextureResources.Reserve(pRenderStageDesc->ResourceStates.GetSize());
			renderStageBufferResources.Reserve(pRenderStageDesc->ResourceStates.GetSize());
			renderStageDrawArgResources.Reserve(pRenderStageDesc->ResourceStates.GetSize());

			bool						attachmentStateUnchanged			= true;
			float32						renderPassAttachmentsWidth			= 0;
			float32						renderPassAttachmentsHeight			= 0;
			ERenderGraphDimensionType	renderPassAttachmentDimensionTypeX	= ERenderGraphDimensionType::NONE;
			ERenderGraphDimensionType	renderPassAttachmentDimensionTypeY	= ERenderGraphDimensionType::NONE;

			//Special Types of Render Stage Variable
			uint32 renderStageExecutionCount = 1;

			//Create PipelineStageMask here, to be used for InitialBarriers Creation
			uint32 pipelineStageMask = CreateShaderStageMask(pRenderStageDesc);
			FPipelineStageFlags lastPipelineStageFlags = FindLastPipelineStage(pRenderStageDesc);

			//Create Descriptors and RenderPass Attachments from RenderStage Resource States
			for (uint32 rs = 0; rs < pRenderStageDesc->ResourceStates.GetSize(); rs++)
			{
				const RenderGraphResourceState* pResourceStateDesc = &pRenderStageDesc->ResourceStates[rs];

				auto resourceIt = m_ResourceMap.find(pResourceStateDesc->ResourceName);

				if (resourceIt == m_ResourceMap.end())
				{
					LOG_ERROR("[RenderGraph]: Resource State with name \"%s\" has no accompanying Resource", pResourceStateDesc->ResourceName.c_str());
					return false;
				}

				Resource* pResource = &resourceIt->second;

				//Create Initital Transition Barrier and Params, we don't want to create them for the ImGui stage because it "backtransitions" unless it is the only stage
				if (!isImGuiStage || m_RenderStageCount == 1)
				{
					if (pResource->Type == ERenderGraphResourceType::TEXTURE && !pResource->IsBackBuffer)
					{
						uint32 numInitialBarriers = 0;
						if (pResource->BackBufferBound)
						{
							numInitialBarriers = m_BackBufferCount;
						}
						else if (pResource->Texture.IsOfArrayType)
						{
							numInitialBarriers = 1;
						}
						else
						{
							numInitialBarriers = pResource->SubResourceCount;
						}

						if (pResource->Texture.InitialTransitionBarrier.QueueBefore == ECommandQueueType::COMMAND_QUEUE_TYPE_UNKNOWN)
						{
							pResource->LastPipelineStageOfFirstRenderStage = lastPipelineStageFlags;

							pResource->Texture.InitialTransitionBarrier.pTexture				= nullptr;
							pResource->Texture.InitialTransitionBarrier.StateBefore				= pResource->OwnershipType == EResourceOwnershipType::INTERNAL ? ETextureState::TEXTURE_STATE_UNKNOWN : ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;
							pResource->Texture.InitialTransitionBarrier.StateAfter				= CalculateResourceTextureState(pResource->Type, pResourceStateDesc->BindingType == ERenderGraphResourceBindingType::ATTACHMENT ? pResourceStateDesc->AttachmentSynchronizations.PrevBindingType : pResourceStateDesc->BindingType, pResource->Texture.Format);
							pResource->Texture.InitialTransitionBarrier.QueueBefore				= ConvertPipelineStateTypeToQueue(pRenderStageDesc->Type);
							pResource->Texture.InitialTransitionBarrier.QueueAfter				= pResource->Texture.InitialTransitionBarrier.QueueBefore;
							pResource->Texture.InitialTransitionBarrier.SrcMemoryAccessFlags	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_READ;
							pResource->Texture.InitialTransitionBarrier.DstMemoryAccessFlags	= CalculateResourceAccessFlags(pResourceStateDesc->BindingType);
							pResource->Texture.InitialTransitionBarrier.TextureFlags			= pResource->Texture.Format == EFormat::FORMAT_D24_UNORM_S8_UINT ? FTextureFlag::TEXTURE_FLAG_DEPTH_STENCIL : 0;
						}
					}
					else if (pResource->Type == ERenderGraphResourceType::SCENE_DRAW_ARGS && pResourceStateDesc->DrawArgsIncludeMask != 0x0 && pResourceStateDesc->DrawArgsExcludeMask != UINT32_MAX)
					{
						if (pRenderStage->pDrawArgsResource != nullptr)
						{
							LOG_ERROR("[RenderGraph]: Multiple Draw Buffer Bindings are currently not supported for a single RenderStage, %s", pRenderStage->Name.c_str());
							return false;
						}

						if (pRenderStageDesc->Type != EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS || pRenderStageDesc->Graphics.DrawType != ERenderStageDrawType::SCENE_INSTANCES)
						{
							LOG_ERROR("[RenderGraph]: Unfortunately, only GRAPHICS Render Stages with Draw Type SCENE_INSTANCES is allowed to have a resource of binding type DRAW_BUFFERS");
							return false;
						}

						DrawArgMaskDesc maskDesc = {};
						maskDesc.IncludeMask = pResourceStateDesc->DrawArgsIncludeMask;
						maskDesc.ExcludeMask = pResourceStateDesc->DrawArgsExcludeMask;

						requiredDrawArgMasks.insert(maskDesc);
						pRenderStage->pDrawArgsResource	= pResource;
						pRenderStage->DrawArgsMaskDesc	= maskDesc;

						//Set Initial Template only if Mask has not been found before
						auto maskToBuffersIt = pResource->DrawArgs.FullMaskToArgs.find(maskDesc.FullMask);
						if (maskToBuffersIt == pResource->DrawArgs.FullMaskToArgs.end())
						{
							pResource->LastPipelineStageOfFirstRenderStage = lastPipelineStageFlags;
							DrawArgsData drawArgsData = {};
							drawArgsData.InitialTransitionBarrierTemplate.pBuffer				= nullptr;
							drawArgsData.InitialTransitionBarrierTemplate.QueueBefore			= ConvertPipelineStateTypeToQueue(pRenderStageDesc->Type);
							drawArgsData.InitialTransitionBarrierTemplate.QueueAfter			= drawArgsData.InitialTransitionBarrierTemplate.QueueBefore;
							drawArgsData.InitialTransitionBarrierTemplate.SrcMemoryAccessFlags	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
							drawArgsData.InitialTransitionBarrierTemplate.DstMemoryAccessFlags	= CalculateResourceAccessFlags(pResourceStateDesc->BindingType);

							drawArgsData.InitialTextureTransitionBarrierTemplate.QueueBefore			= ConvertPipelineStateTypeToQueue(pRenderStageDesc->Type);
							drawArgsData.InitialTextureTransitionBarrierTemplate.QueueAfter				= drawArgsData.InitialTextureTransitionBarrierTemplate.QueueBefore;
							drawArgsData.InitialTextureTransitionBarrierTemplate.SrcMemoryAccessFlags	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
							drawArgsData.InitialTextureTransitionBarrierTemplate.DstMemoryAccessFlags	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_READ;
							drawArgsData.InitialTextureTransitionBarrierTemplate.StateBefore			= ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;
							drawArgsData.InitialTextureTransitionBarrierTemplate.StateAfter				= ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;

							pResource->DrawArgs.FullMaskToArgs[maskDesc.FullMask] = drawArgsData;
						}
					}
					else if (pResource->Type == ERenderGraphResourceType::BUFFER)
					{
						uint32 numInitialBarriers = 0;
						if (pResource->BackBufferBound)
						{
							numInitialBarriers = m_BackBufferCount;
						}
						else
						{
							numInitialBarriers = pResource->SubResourceCount;
						}

						if (pResource->Buffer.InitialTransitionBarrier.QueueBefore == ECommandQueueType::COMMAND_QUEUE_TYPE_UNKNOWN)
						{
							pResource->LastPipelineStageOfFirstRenderStage = lastPipelineStageFlags;

							pResource->Buffer.InitialTransitionBarrier.pBuffer				= nullptr;
							pResource->Buffer.InitialTransitionBarrier.QueueBefore			= ConvertPipelineStateTypeToQueue(pRenderStageDesc->Type);
							pResource->Buffer.InitialTransitionBarrier.QueueAfter			= pResource->Buffer.InitialTransitionBarrier.QueueBefore;
							pResource->Buffer.InitialTransitionBarrier.SrcMemoryAccessFlags	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_UNKNOWN;
							pResource->Buffer.InitialTransitionBarrier.DstMemoryAccessFlags	= CalculateResourceAccessFlags(pResourceStateDesc->BindingType);
						}
					}
				}

				//Descriptors
				if (ResourceStateNeedsDescriptor(pResourceStateDesc->BindingType))
				{
					EDescriptorType descriptorType		= CalculateResourceStateDescriptorType(pResource->Type, pResourceStateDesc->BindingType);

					if (descriptorType == EDescriptorType::DESCRIPTOR_TYPE_UNKNOWN)
					{
						LOG_ERROR("[RenderGraph]: Descriptor Type for Resource State with name \"%s\" could not be found", pResourceStateDesc->ResourceName.c_str());
						return false;
					}

					DescriptorBindingDesc descriptorBinding = {};
					descriptorBinding.DescriptorType		= descriptorType;
					descriptorBinding.ShaderStageMask		= pipelineStageMask;

					if (pResource->Type == ERenderGraphResourceType::TEXTURE)
					{
						ETextureState textureState = CalculateResourceTextureState(pResource->Type, pResourceStateDesc->BindingType, pResource->Texture.Format);

						descriptorBinding.Binding			= textureDescriptorBindingIndex++;

						if (pResource->Texture.UnboundedArray)
						{
							descriptorBinding.Flags				= FDescriptorSetLayoutBindingFlag::DESCRIPTOR_SET_LAYOUT_BINDING_FLAG_PARTIALLY_BOUND;
							descriptorBinding.DescriptorCount	= PARTIALLY_BOUND_DESCRIPTOR_COUNT;
						}
						else
						{
							uint32 actualSubResourceCount		= (pResource->BackBufferBound || pResource->Texture.IsOfArrayType) ? 1 : pResource->SubResourceCount;
							descriptorBinding.DescriptorCount	= actualSubResourceCount;
						}

						textureDescriptorSetDescriptions.PushBack(descriptorBinding);
						renderStageTextureResources.PushBack(std::make_tuple(pResource, textureState, descriptorType));
					}
					else if (pResource->Type == ERenderGraphResourceType::SCENE_DRAW_ARGS)
					{
						// Vertex Buffer
						descriptorBinding.DescriptorCount	= 1;
						descriptorBinding.Binding			= 0;
						drawArgDescriptorSetDescriptions.PushBack(descriptorBinding);

						// Instance Buffer
						descriptorBinding.DescriptorCount	= 1;
						descriptorBinding.Binding			= 1;
						drawArgDescriptorSetDescriptions.PushBack(descriptorBinding);

						// Meshlet Buffer
						descriptorBinding.DescriptorCount	= 1;
						descriptorBinding.Binding			= 2;
						drawArgDescriptorSetDescriptions.PushBack(descriptorBinding);

						// Unique Indices Buffer
						descriptorBinding.DescriptorCount	= 1;
						descriptorBinding.Binding			= 3;
						drawArgDescriptorSetDescriptions.PushBack(descriptorBinding);

						// Primitive Indices Buffer
						descriptorBinding.DescriptorCount	= 1;
						descriptorBinding.Binding			= 4;
						drawArgDescriptorSetDescriptions.PushBack(descriptorBinding);

						/*
						*	Create a new descriptor set for extensions.
						*	If the render stage uses two extensions and the first extension has two textures and the second has one, the binding will be like this:
						*
						*	First extension's first texture has binding 0
						*	First extension's second texture has binding 1
						*	Second extension's texture has binding 2
						*
						*	Each holding a array of textures for each instance which uses an extension.
						*
						*	The Instance buffer has an ExtensionIndex to point to the right element in the array.
						*	The first element is used for instances which does not have an extension.
						*/
						TArray<uint32> extensionMasks = EntityMaskManager::ExtractComponentMasksFromEntityMask(pRenderStage->DrawArgsMaskDesc.IncludeMask & ~pRenderStage->DrawArgsMaskDesc.ExcludeMask);
						uint32 binding = 0;
						for (uint32 mask : extensionMasks)
						{
							const DrawArgExtensionDesc& extensionDesc = EntityMaskManager::GetExtensionDescFromExtensionMask(mask);
							for (uint32 t = 0; t < extensionDesc.TextureCount; t++)
							{
								// TODO: Do not hardcode the descriptor type!
								descriptorBinding.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
								descriptorBinding.DescriptorCount	= 1000u;
								descriptorBinding.Binding			= binding++;
								descriptorBinding.Flags				= FDescriptorSetLayoutBindingFlag::DESCRIPTOR_SET_LAYOUT_BINDING_FLAG_PARTIALLY_BOUND;
								drawArgExtensionDescriptorSetDescriptions.PushBack(descriptorBinding);
							}
						}

						renderStageDrawArgResources.PushBack(std::make_tuple(pResource, descriptorType));
					}
					else
					{
						descriptorBinding.DescriptorCount	= pResource->SubResourceCount;
						descriptorBinding.Binding			= bufferDescriptorBindingIndex++;

						bufferDescriptorSetDescriptions.PushBack(descriptorBinding);
						renderStageBufferResources.PushBack(std::make_tuple(pResource, descriptorType));
					}
				}
				//RenderPass Attachments
				else if (pResourceStateDesc->BindingType == ERenderGraphResourceBindingType::ATTACHMENT)
				{
					// Check if attachment is unchanged after renderstage
					auto prevBinding = pResourceStateDesc->AttachmentSynchronizations.PrevBindingType;
					auto nextBinding = pResourceStateDesc->AttachmentSynchronizations.NextBindingType;
					if (prevBinding != nextBinding)
					{
						attachmentStateUnchanged = false;
					}

					if (pResource->OwnershipType != EResourceOwnershipType::EXTERNAL)
					{

						float32						xDimVariable;
						float32						yDimVariable;
						ERenderGraphDimensionType	xDimType;
						ERenderGraphDimensionType	yDimType;

						if (!pResource->IsBackBuffer)
						{
							auto resourceUpdateDescIt = m_InternalResourceUpdateDescriptions.find(pResourceStateDesc->ResourceName);

							if (resourceUpdateDescIt == m_InternalResourceUpdateDescriptions.end())
							{
								LOG_ERROR("[RenderGraph]: Resource State with name \"%s\" has no accompanying InternalResourceUpdateDesc", pResourceStateDesc->ResourceName.c_str());
								return false;
							}

							xDimVariable = resourceUpdateDescIt->second.TextureUpdate.XDimVariable;
							yDimVariable = resourceUpdateDescIt->second.TextureUpdate.YDimVariable;
							xDimType = resourceUpdateDescIt->second.TextureUpdate.XDimType;
							yDimType = resourceUpdateDescIt->second.TextureUpdate.YDimType;
						}
						else
						{
							xDimVariable = 1.0f;
							yDimVariable = 1.0f;
							xDimType = ERenderGraphDimensionType::RELATIVE;
							yDimType = ERenderGraphDimensionType::RELATIVE;
						}

						//Just use the width to check if its ever been set
						if (renderPassAttachmentsWidth == 0)
						{
							renderPassAttachmentsWidth = xDimVariable;
							renderPassAttachmentsHeight = yDimVariable;
							renderPassAttachmentDimensionTypeX = xDimType;
							renderPassAttachmentDimensionTypeY = yDimType;
						}
						else
						{
							bool success = true;

							if (renderPassAttachmentsWidth != xDimVariable)
							{
								LOG_ERROR("[RenderGraph]: Resource State with name \"%s\" is bound as Attachment but does not share the same width %d, as previous attachments %d",
									pResourceStateDesc->ResourceName.c_str(),
									xDimVariable,
									renderPassAttachmentsWidth);
								success = false;
							}

							if (renderPassAttachmentsHeight != yDimVariable)
							{
								LOG_ERROR("[RenderGraph]: Resource State with name \"%s\" is bound as Attachment but does not share the same height %d, as previous attachments %d",
									pResourceStateDesc->ResourceName.c_str(),
									yDimVariable,
									renderPassAttachmentsHeight);
								success = false;
							}

							if (renderPassAttachmentDimensionTypeX != xDimType)
							{
								LOG_ERROR("[RenderGraph]: Resource State with name \"%s\" is bound as Attachment but does not share the same XDimType %s, as previous attachments %s",
									pResourceStateDesc->ResourceName.c_str(),
									RenderGraphDimensionTypeToString(xDimType),
									RenderGraphDimensionTypeToString(renderPassAttachmentDimensionTypeX));
								success = false;
							}

							if (renderPassAttachmentDimensionTypeY != yDimType)
							{
								LOG_ERROR("[RenderGraph]: Resource State with name \"%s\" is bound as Attachment but does not share the same XDimType %s, as previous attachments %s",
									pResourceStateDesc->ResourceName.c_str(),
									RenderGraphDimensionTypeToString(yDimType),
									RenderGraphDimensionTypeToString(renderPassAttachmentDimensionTypeY));
								success = false;
							}

							if (!success)
								return false;
						}
					}

					pResource->Texture.UsedAsRenderTarget = true;

					uint32 executionCountFromResource = pResource->Texture.PerSubImageTextureViews.GetSize();

					if (pResource->BackBufferBound) executionCountFromResource /= m_BackBufferCount;

					if (renderStageExecutionCount == 1)
					{
						renderStageExecutionCount = executionCountFromResource;
					}
					else if (executionCountFromResource > 1 && executionCountFromResource != renderStageExecutionCount)
					{
						LOG_ERROR("[RenderGraph]: Resource %s is used as RenderPass Attachment and requires execution count %d, but execution count for this RenderStage has been set to %d from another resource",
							pResource->Name.c_str(),
							executionCountFromResource,
							renderStageExecutionCount);
						return false;
					}

					bool isColorAttachment = pResource->Texture.Format != EFormat::FORMAT_D24_UNORM_S8_UINT;

					ETextureState initialState	= CalculateResourceTextureState(pResource->Type, pResourceStateDesc->AttachmentSynchronizations.PrevBindingType, pResource->Texture.Format);
					ETextureState finalState	= CalculateResourceTextureState(pResource->Type, pResourceStateDesc->AttachmentSynchronizations.NextBindingType, pResource->Texture.Format);

					ELoadOp loadOp = ELoadOp::LOAD_OP_LOAD;

					if (initialState == ETextureState::TEXTURE_STATE_DONT_CARE ||
						initialState == ETextureState::TEXTURE_STATE_UNKNOWN ||
						!pResourceStateDesc->AttachmentSynchronizations.PrevSameFrame)
					{
						loadOp = ELoadOp::LOAD_OP_CLEAR;
					}

					if (isColorAttachment)
					{
						RenderPassAttachmentDesc renderPassAttachmentDesc = {};
						renderPassAttachmentDesc.Format			= pResource->Texture.Format;
						renderPassAttachmentDesc.SampleCount	= 1;
						renderPassAttachmentDesc.LoadOp			= loadOp;
						renderPassAttachmentDesc.StoreOp		= EStoreOp::STORE_OP_STORE;
						renderPassAttachmentDesc.StencilLoadOp	= ELoadOp::LOAD_OP_DONT_CARE;
						renderPassAttachmentDesc.StencilStoreOp	= EStoreOp::STORE_OP_DONT_CARE;
						renderPassAttachmentDesc.InitialState	= initialState;
						renderPassAttachmentDesc.FinalState		= finalState;

						renderPassAttachmentDescriptions.PushBack(renderPassAttachmentDesc);

						renderPassRenderTargetStates.PushBack(ETextureState::TEXTURE_STATE_RENDER_TARGET);

						BlendAttachmentStateDesc blendAttachmentState = {};
						if (pRenderStageDesc->Graphics.AlphaBlendingEnabled)
						{
							blendAttachmentState.BlendOp					= EBlendOp::BLEND_OP_ADD;
							blendAttachmentState.SrcBlend					= EBlendFactor::BLEND_FACTOR_SRC_ALPHA;
							blendAttachmentState.DstBlend					= EBlendFactor::BLEND_FACTOR_INV_SRC_ALPHA;
							blendAttachmentState.BlendOpAlpha				= EBlendOp::BLEND_OP_ADD;
							blendAttachmentState.SrcBlendAlpha				= EBlendFactor::BLEND_FACTOR_SRC_ALPHA;
							blendAttachmentState.DstBlendAlpha				= EBlendFactor::BLEND_FACTOR_INV_SRC_ALPHA;
							blendAttachmentState.RenderTargetComponentMask	= COLOR_COMPONENT_FLAG_R | COLOR_COMPONENT_FLAG_G | COLOR_COMPONENT_FLAG_B | COLOR_COMPONENT_FLAG_A;
							blendAttachmentState.BlendEnabled				= true;
						}
						else
						{
							blendAttachmentState.BlendEnabled				= false;
							blendAttachmentState.RenderTargetComponentMask	= COLOR_COMPONENT_FLAG_R | COLOR_COMPONENT_FLAG_G | COLOR_COMPONENT_FLAG_B | COLOR_COMPONENT_FLAG_A;
						}

						renderPassBlendAttachmentStates.PushBack(blendAttachmentState);
						renderStageRenderTargets.PushBack(std::make_pair(pResource, finalState));
					}
					else
					{
						RenderPassAttachmentDesc renderPassAttachmentDesc = {};
						renderPassAttachmentDesc.Format			= pResource->Texture.Format;
						renderPassAttachmentDesc.SampleCount	= 1;
						renderPassAttachmentDesc.LoadOp			= loadOp;
						renderPassAttachmentDesc.StoreOp		= EStoreOp::STORE_OP_STORE;
						renderPassAttachmentDesc.StencilLoadOp	= loadOp;
						renderPassAttachmentDesc.StencilStoreOp = EStoreOp::STORE_OP_STORE;
						renderPassAttachmentDesc.InitialState	= initialState;
						renderPassAttachmentDesc.FinalState		= finalState;

						renderPassDepthStencilDescription = renderPassAttachmentDesc;
						pDepthStencilResource = pResource;
					}
				}
			}

			//Triggering
			{
				pRenderStage->TriggerType	= pRenderStageDesc->TriggerType;
				pRenderStage->FrameDelay	= pRenderStageDesc->TriggerType == ERenderStageExecutionTrigger::EVERY ? uint32(pRenderStageDesc->FrameDelay) : 0;
				pRenderStage->FrameOffset	= pRenderStageDesc->TriggerType == ERenderStageExecutionTrigger::EVERY ? uint32(pRenderStageDesc->FrameOffset) : 0;
				pRenderStage->FrameCounter	= pRenderStageDesc->TriggerType == ERenderStageExecutionTrigger::EVERY ? 0 : 1; //We only trigger on FrameCounter == FrameDelay
			}

			if (pRenderStageDesc->CustomRenderer)
			{
				CustomRenderer* pCustomRenderer = nullptr;

				if (isImGuiStage)
				{
					auto imGuiRenderStageIt = std::find_if(m_DebugRenderers.Begin(), m_DebugRenderers.End(), [](const CustomRenderer* pCustomRenderer) { return pCustomRenderer->GetName() == RENDER_GRAPH_IMGUI_STAGE_NAME; });

					if (imGuiRenderStageIt == m_DebugRenderers.End())
					{

						ImGuiRendererDesc imguiRendererDesc = {};
						imguiRendererDesc.BackBufferCount	= m_BackBufferCount;
						imguiRendererDesc.VertexBufferSize	= MEGA_BYTE(8);
						imguiRendererDesc.IndexBufferSize	= MEGA_BYTE(8);

						ImGuiRenderer* pImGuiRenderer = DBG_NEW ImGuiRenderer(m_pGraphicsDevice, &imguiRendererDesc);
						if (!pImGuiRenderer->Init())
						{
							LOG_ERROR("[RenderGraph] Could not initialize ImGui Custom Renderer");
							return false;
						}

						m_CustomRenderers.PushBack(pImGuiRenderer);
						m_DebugRenderers.PushBack(pImGuiRenderer);

						pCustomRenderer = pImGuiRenderer;
					}
					else
					{
						pCustomRenderer = *imGuiRenderStageIt;
					}
				}
				else
				{
					auto customRendererIt = std::find_if(customRenderers.Begin(), customRenderers.End(), [pRenderStageDesc](const CustomRenderer* pCustomRenderer) { return pRenderStageDesc->Name == pCustomRenderer->GetName(); });

					if (customRendererIt == customRenderers.end())
					{
						LOG_ERROR("[RenderGraph]: Custom Renderer %s could not be found among Custom Renderers", pRenderStage->Name.c_str());
						pRenderStage->TriggerType = ERenderStageExecutionTrigger::DISABLED;
					}
					else
					{
						pCustomRenderer = *customRendererIt;
					}
				}

				// Track all custom renderers
				m_CustomRenderers.PushBack(pCustomRenderer);

				CustomRendererRenderGraphInitDesc customRendererInitDesc = {};
				customRendererInitDesc.pRenderGraph					= this;
				customRendererInitDesc.BackBufferCount				= m_BackBufferCount;
				customRendererInitDesc.pColorAttachmentDesc			= renderPassAttachmentDescriptions.GetData();
				customRendererInitDesc.ColorAttachmentCount			= (uint32)renderPassAttachmentDescriptions.GetSize();
				customRendererInitDesc.pDepthStencilAttachmentDesc	= renderPassDepthStencilDescription.Format != EFormat::FORMAT_NONE ? &renderPassDepthStencilDescription : nullptr;

				if (!pCustomRenderer->RenderGraphInit(&customRendererInitDesc))
				{
					LOG_ERROR("[RenderGraph] Could not initialize Custom Renderer");
					return false;
				}

				pRenderStage->UsesCustomRenderer	= true;
				pRenderStage->pCustomRenderer		= pCustomRenderer;
				pRenderStage->FirstPipelineStage	= pCustomRenderer->GetFirstPipelineStage();
				pRenderStage->LastPipelineStage		= pCustomRenderer->GetLastPipelineStage();

				//Since FirstPipelineStage of Custom Renderers is not known until we have created the Custom Renderer, we need to update Resources FirstPipelineStageOfLastRenderStage member
				if (!isImGuiStage)
				{
					for (uint32 rs = 0; rs < pRenderStageDesc->ResourceStates.GetSize(); rs++)
					{
						const RenderGraphResourceState* pResourceStateDesc = &pRenderStageDesc->ResourceStates[rs];

						auto resourceIt = m_ResourceMap.find(pResourceStateDesc->ResourceName);

						if (resourceIt == m_ResourceMap.end())
						{
							LOG_ERROR("[RenderGraph]: Resource State with name \"%s\" has no accompanying Resource", pResourceStateDesc->ResourceName.c_str());
							return false;
						}

						//Only set it if it hasn't been set before, if it has been set before a previous Render Stage uses this resource -> we shouldn't set it
						if (resourceIt->second.LastPipelineStageOfFirstRenderStage == FPipelineStageFlag::PIPELINE_STAGE_FLAG_UNKNOWN)
						{
							resourceIt->second.LastPipelineStageOfFirstRenderStage = pRenderStage->LastPipelineStage;
						}
					}
				}
			}
			else
			{
				pRenderStage->PipelineStageMask		= pipelineStageMask;
				pRenderStage->FirstPipelineStage	= FindEarliestPipelineStage(pRenderStageDesc);
				pRenderStage->LastPipelineStage		= lastPipelineStageFlags;
				pRenderStage->ExecutionCount		= renderStageExecutionCount;

				ConstantRangeDesc pushConstantRange = {};

				//Create Push Constants
				{
					uint32 externalMaxSize = MAX_PUSH_CONSTANT_SIZE;

					if (renderStageExecutionCount > 1)
					{
						PushConstants* pPushConstants = &pRenderStage->pInternalPushConstants[DRAW_ITERATION_PUSH_CONSTANTS_INDEX];
						pPushConstants->pData		= DBG_NEW byte[DRAW_ITERATION_PUSH_CONSTANTS_SIZE];
						pPushConstants->DataSize	= DRAW_ITERATION_PUSH_CONSTANTS_SIZE;
						pPushConstants->Offset		= MAX_PUSH_CONSTANT_SIZE - externalMaxSize;
						pPushConstants->MaxDataSize = DRAW_ITERATION_PUSH_CONSTANTS_SIZE;

						externalMaxSize -= DRAW_ITERATION_PUSH_CONSTANTS_SIZE;
					}

					//External Push Constants
					{
						PushConstants* pPushConstants = &pRenderStage->ExternalPushConstants;
						pPushConstants->pData		= nullptr; //This doesn't get allocated until UpdatePushConstants is called
						pPushConstants->DataSize	= 0;
						pPushConstants->Offset		= MAX_PUSH_CONSTANT_SIZE - externalMaxSize;
						pPushConstants->MaxDataSize = externalMaxSize;
					}

					pushConstantRange.ShaderStageFlags	= pipelineStageMask;
					pushConstantRange.SizeInBytes		= MAX_PUSH_CONSTANT_SIZE;
					pushConstantRange.OffsetInBytes		= 0;
				}

				//Create Pipeline Layout
				{
					TArray<DescriptorSetLayoutDesc> descriptorSetLayouts;
					descriptorSetLayouts.Reserve(2);

					if (bufferDescriptorSetDescriptions.GetSize() > 0)
					{
						DescriptorSetLayoutDesc descriptorSetLayout = {};
						descriptorSetLayout.DescriptorBindings		= bufferDescriptorSetDescriptions;
						descriptorSetLayouts.PushBack(descriptorSetLayout);
					}

					if (textureDescriptorSetDescriptions.GetSize() > 0)
					{
						DescriptorSetLayoutDesc descriptorSetLayout = {};
						descriptorSetLayout.DescriptorBindings		= textureDescriptorSetDescriptions;
						descriptorSetLayouts.PushBack(descriptorSetLayout);
					}

					if (pRenderStage->DrawType == ERenderStageDrawType::SCENE_INSTANCES ||
						pRenderStage->DrawType == ERenderStageDrawType::SCENE_INSTANCES_MESH_SHADER)
					{
						if (pRenderStage->pDrawArgsResource == nullptr)
						{
							LOG_ERROR("[RenderGraph]: A RenderStage of DrawType SCENE_INSTANCES and SCENE_INSTANCES_MESH_SHADER must have a binding of typ SCENE_DRAW_BUFFERS");
							return false;
						}
					}

					if (pRenderStage->pDrawArgsResource != nullptr)
					{
						if (pRenderStage->DrawArgsMaskDesc.IncludeMask == 0x0)
						{
							LOG_ERROR("[RenderGraph]: A RenderStage which has a binding of type SCENE_DRAW_BUFFERS should have a non-zero DrawArgsIncludeMask set to that binding");
							return false;
						}

						if (pRenderStage->DrawArgsMaskDesc.ExcludeMask == UINT32_MAX)
						{
							LOG_ERROR("[RenderGraph]: A RenderStage which has a binding of type SCENE_DRAW_BUFFERS should have a DrawArgsExcludeMask not set to 0xFFFFFFFF set to that binding");
							return false;
						}

						if ((pRenderStage->DrawArgsMaskDesc.IncludeMask& pRenderStage->DrawArgsMaskDesc.ExcludeMask) > 0)
						{
							LOG_ERROR("[RenderGraph]: A RenderStage which has a binding of type SCENE_DRAW_BUFFERS should have a DrawArgsIncludeMask and a DrawArgsExcludeMask that do not overlap set to that binding");
							return false;
						}

						{
							DescriptorSetLayoutDesc descriptorSetLayout = {};
							descriptorSetLayout.DescriptorBindings		= drawArgDescriptorSetDescriptions;
							descriptorSetLayouts.PushBack(descriptorSetLayout);
						}

						// Extensions descriptor set layout
						if (drawArgExtensionDescriptorSetDescriptions.GetSize() > 0)
						{
							DescriptorSetLayoutDesc descriptorSetLayout = {};
							descriptorSetLayout.DescriptorBindings		= drawArgExtensionDescriptorSetDescriptions;
							descriptorSetLayouts.PushBack(descriptorSetLayout);
						}
					}

					PipelineLayoutDesc pipelineLayoutDesc = {};
					pipelineLayoutDesc.DescriptorSetLayouts	= descriptorSetLayouts;
					pipelineLayoutDesc.ConstantRanges		= { pushConstantRange };

					pRenderStage->pPipelineLayout = m_pGraphicsDevice->CreatePipelineLayout(&pipelineLayoutDesc);
				}

				//Create Descriptor Set
				{
					uint32 setIndex = 0;

					if (bufferDescriptorSetDescriptions.GetSize() > 0)
					{
						pRenderStage->ppBufferDescriptorSets = DBG_NEW DescriptorSet*[m_BackBufferCount];

						for (uint32 i = 0; i < m_BackBufferCount; i++)
						{
							DescriptorSet* pDescriptorSet = m_pGraphicsDevice->CreateDescriptorSet(pRenderStageDesc->Name + " Buffer Descriptor Set " + std::to_string(i), pRenderStage->pPipelineLayout, setIndex, m_pDescriptorHeap);
							pRenderStage->ppBufferDescriptorSets[i] = pDescriptorSet;
						}

						pRenderStage->BufferSetIndex = setIndex;
						setIndex++;
					}

					if (textureDescriptorSetDescriptions.GetSize() > 0)
					{
						pRenderStage->ppTextureDescriptorSets = DBG_NEW DescriptorSet*[m_BackBufferCount];

						for (uint32 i = 0; i < m_BackBufferCount; i++)
						{
							DescriptorSet* pDescriptorSet = m_pGraphicsDevice->CreateDescriptorSet(pRenderStageDesc->Name + " Texture Descriptor Set " + std::to_string(i), pRenderStage->pPipelineLayout, setIndex, m_pDescriptorHeap);
							pRenderStage->ppTextureDescriptorSets[i] = pDescriptorSet;
						}

						pRenderStage->TextureSetIndex = setIndex;
						setIndex++;
					}

					if (pRenderStageDesc->Type == EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS && pRenderStageDesc->Graphics.DrawType == ERenderStageDrawType::SCENE_INSTANCES)
					{
						pRenderStage->pppDrawArgDescriptorSets = DBG_NEW DescriptorSet**[m_BackBufferCount];
						for (uint32 i = 0; i < m_BackBufferCount; i++)
						{
							pRenderStage->pppDrawArgDescriptorSets[i] = nullptr;
						}

						pRenderStage->DrawSetIndex = setIndex;
						setIndex++;

						// Draw Arg Extensions descriptor set
						if (drawArgExtensionDescriptorSetDescriptions.GetSize() > 0)
						{
							pRenderStage->pppDrawArgExtensionsDescriptorSets = DBG_NEW DescriptorSet**[m_BackBufferCount];
							for (uint32 i = 0; i < m_BackBufferCount; i++)
							{
								pRenderStage->pppDrawArgExtensionsDescriptorSets[i] = nullptr;
							}
							pRenderStage->DrawExtensionSetIndex = setIndex;
							setIndex++;
						}
					}
				}

				//Shader Constants
				const RenderGraphShaderConstants* pShaderConstants = nullptr;
				{
					auto shaderConstantsIt = shaderConstants.find(pRenderStage->Name);

					if (shaderConstantsIt != shaderConstants.end())
					{
						pShaderConstants = &shaderConstantsIt->second;
					}
				}


				//Create Pipeline State
				if (pRenderStageDesc->Type == EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS)
				{
					ManagedGraphicsPipelineStateDesc pipelineDesc = {};
					pipelineDesc.DebugName							= pRenderStageDesc->Name;
					pipelineDesc.PipelineLayout						= MakeSharedRef(pRenderStage->pPipelineLayout);
					pipelineDesc.DepthStencilState.DepthTestEnable	= pRenderStageDesc->Graphics.DepthTestEnabled;
					pipelineDesc.TaskShader.ShaderGUID				= pRenderStageDesc->Graphics.Shaders.TaskShaderName.empty()		? GUID_NONE : ResourceManager::LoadShaderFromFile(pRenderStageDesc->Graphics.Shaders.TaskShaderName,		FShaderStageFlag::SHADER_STAGE_FLAG_TASK_SHADER,		EShaderLang::SHADER_LANG_GLSL);
					pipelineDesc.MeshShader.ShaderGUID				= pRenderStageDesc->Graphics.Shaders.MeshShaderName.empty()		? GUID_NONE : ResourceManager::LoadShaderFromFile(pRenderStageDesc->Graphics.Shaders.MeshShaderName,		FShaderStageFlag::SHADER_STAGE_FLAG_MESH_SHADER,		EShaderLang::SHADER_LANG_GLSL);
					pipelineDesc.VertexShader.ShaderGUID			= pRenderStageDesc->Graphics.Shaders.VertexShaderName.empty()	? GUID_NONE : ResourceManager::LoadShaderFromFile(pRenderStageDesc->Graphics.Shaders.VertexShaderName,		FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER,		EShaderLang::SHADER_LANG_GLSL);
					pipelineDesc.GeometryShader.ShaderGUID			= pRenderStageDesc->Graphics.Shaders.GeometryShaderName.empty() ? GUID_NONE : ResourceManager::LoadShaderFromFile(pRenderStageDesc->Graphics.Shaders.GeometryShaderName,	FShaderStageFlag::SHADER_STAGE_FLAG_GEOMETRY_SHADER,	EShaderLang::SHADER_LANG_GLSL);
					pipelineDesc.HullShader.ShaderGUID				= pRenderStageDesc->Graphics.Shaders.HullShaderName.empty()		? GUID_NONE : ResourceManager::LoadShaderFromFile(pRenderStageDesc->Graphics.Shaders.HullShaderName,		FShaderStageFlag::SHADER_STAGE_FLAG_HULL_SHADER,		EShaderLang::SHADER_LANG_GLSL);
					pipelineDesc.DomainShader.ShaderGUID			= pRenderStageDesc->Graphics.Shaders.DomainShaderName.empty()	? GUID_NONE : ResourceManager::LoadShaderFromFile(pRenderStageDesc->Graphics.Shaders.DomainShaderName,		FShaderStageFlag::SHADER_STAGE_FLAG_DOMAIN_SHADER,		EShaderLang::SHADER_LANG_GLSL);
					pipelineDesc.PixelShader.ShaderGUID				= pRenderStageDesc->Graphics.Shaders.PixelShaderName.empty()	? GUID_NONE : ResourceManager::LoadShaderFromFile(pRenderStageDesc->Graphics.Shaders.PixelShaderName,		FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER,		EShaderLang::SHADER_LANG_GLSL);
					pipelineDesc.BlendState.BlendAttachmentStates	= renderPassBlendAttachmentStates;
					pipelineDesc.RasterizerState.CullMode			= pRenderStageDesc->Graphics.CullMode;
					pipelineDesc.RasterizerState.PolygonMode		= pRenderStageDesc->Graphics.PolygonMode;
					pipelineDesc.RasterizerState.FrontFaceCounterClockWise = false;
					pipelineDesc.InputAssembly.PrimitiveTopology	= pRenderStageDesc->Graphics.PrimitiveTopology;

					if (pShaderConstants != nullptr)
					{
						pipelineDesc.TaskShader.ShaderConstants		= pShaderConstants->Graphics.TaskShaderConstants;
						pipelineDesc.MeshShader.ShaderConstants		= pShaderConstants->Graphics.MeshShaderConstants;
						pipelineDesc.VertexShader.ShaderConstants	= pShaderConstants->Graphics.VertexShaderConstants;
						pipelineDesc.GeometryShader.ShaderConstants = pShaderConstants->Graphics.GeometryShaderConstants;
						pipelineDesc.HullShader.ShaderConstants		= pShaderConstants->Graphics.HullShaderConstants;
						pipelineDesc.DomainShader.ShaderConstants	= pShaderConstants->Graphics.DomainShaderConstants;
						pipelineDesc.PixelShader.ShaderConstants	= pShaderConstants->Graphics.PixelShaderConstants;
					}

					//Create RenderPass
					{
						RenderPassSubpassDesc renderPassSubpassDesc = { };
						renderPassSubpassDesc.RenderTargetStates			= renderPassRenderTargetStates;
						renderPassSubpassDesc.DepthStencilAttachmentState	= pDepthStencilResource != nullptr ? ETextureState::TEXTURE_STATE_DEPTH_STENCIL_ATTACHMENT : ETextureState::TEXTURE_STATE_DONT_CARE;

						RenderPassSubpassDependencyDesc renderPassSubpassDependencyDesc = {};
						renderPassSubpassDependencyDesc.SrcSubpass		= EXTERNAL_SUBPASS;
						renderPassSubpassDependencyDesc.DstSubpass		= 0;
						renderPassSubpassDependencyDesc.SrcAccessMask	= 0;
						renderPassSubpassDependencyDesc.DstAccessMask	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_READ | FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
						renderPassSubpassDependencyDesc.SrcStageMask	= FPipelineStageFlag::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;
						renderPassSubpassDependencyDesc.DstStageMask	= FPipelineStageFlag::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;

						if (renderPassDepthStencilDescription.Format != EFormat::FORMAT_NONE)
							renderPassAttachmentDescriptions.PushBack(renderPassDepthStencilDescription);

						RenderPassDesc renderPassDesc = {};
						renderPassDesc.DebugName			= "";
						renderPassDesc.Attachments			= renderPassAttachmentDescriptions;
						renderPassDesc.Subpasses			= { renderPassSubpassDesc };
						renderPassDesc.SubpassDependencies	= { renderPassSubpassDependencyDesc };

						RenderPass* pRenderPass		= m_pGraphicsDevice->CreateRenderPass(&renderPassDesc);
						pipelineDesc.RenderPass		= MakeSharedRef(pRenderPass);

						pRenderStage->pRenderPass	= pRenderPass;

						//Create duplicate Render Pass (this is fucking retarded) which we use when the RenderStage is Disabled, this Render Pass forces LoadOp to be LOAD
						if (!attachmentStateUnchanged)
						{
							RenderPassDesc disabledRenderPassDesc = renderPassDesc;

							for (RenderPassAttachmentDesc& attachmentDesc : disabledRenderPassDesc.Attachments)
							{
								if (attachmentDesc.InitialState != ETextureState::TEXTURE_STATE_UNKNOWN) attachmentDesc.LoadOp = ELoadOp::LOAD_OP_LOAD;
								if (attachmentDesc.StencilLoadOp != ELoadOp::LOAD_OP_DONT_CARE) attachmentDesc.StencilLoadOp = ELoadOp::LOAD_OP_LOAD;
							}

							pRenderStage->pDisabledRenderPass = m_pGraphicsDevice->CreateRenderPass(&disabledRenderPassDesc);
						}
					}

					pRenderStage->PipelineStateID = PipelineStateManager::CreateGraphicsPipelineState(&pipelineDesc);
					VALIDATE(pRenderStage->PipelineStateID != 0);
					pRenderStage->pPipelineState = PipelineStateManager::GetPipelineState(pRenderStage->PipelineStateID);
				}
				else if (pRenderStageDesc->Type == EPipelineStateType::PIPELINE_STATE_TYPE_COMPUTE)
				{
					ManagedComputePipelineStateDesc pipelineDesc = { };
					pipelineDesc.DebugName		= pRenderStageDesc->Name;
					pipelineDesc.PipelineLayout	= MakeSharedRef(pRenderStage->pPipelineLayout);
					pipelineDesc.Shader.ShaderGUID = pRenderStageDesc->Compute.ShaderName.empty() ? GUID_NONE : ResourceManager::LoadShaderFromFile(pRenderStageDesc->Compute.ShaderName, FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER, EShaderLang::SHADER_LANG_GLSL);

					if (pShaderConstants != nullptr)
					{
						pipelineDesc.Shader.ShaderConstants		= pShaderConstants->Compute.ShaderConstants;
					}

					pRenderStage->PipelineStateID = PipelineStateManager::CreateComputePipelineState(&pipelineDesc);
					VALIDATE(pRenderStage->PipelineStateID != 0);
					pRenderStage->pPipelineState = PipelineStateManager::GetPipelineState(pRenderStage->PipelineStateID);
				}
				else if (pRenderStageDesc->Type == EPipelineStateType::PIPELINE_STATE_TYPE_RAY_TRACING)
				{
					ManagedRayTracingPipelineStateDesc pipelineDesc = {};
					pipelineDesc.PipelineLayout			= MakeSharedRef(pRenderStage->pPipelineLayout);
					pipelineDesc.MaxRecursionDepth		= 1;
					pipelineDesc.RaygenShader.ShaderGUID = pRenderStageDesc->RayTracing.Shaders.RaygenShaderName.empty() ? GUID_NONE : ResourceManager::LoadShaderFromFile(pRenderStageDesc->RayTracing.Shaders.RaygenShaderName, FShaderStageFlag::SHADER_STAGE_FLAG_RAYGEN_SHADER, EShaderLang::SHADER_LANG_GLSL );

					if (pShaderConstants != nullptr)
					{
						pipelineDesc.RaygenShader.ShaderConstants = pShaderConstants->RayTracing.RaygenConstants;
					}

					pipelineDesc.ClosestHitShaders.Resize(pRenderStageDesc->RayTracing.Shaders.ClosestHitShaderCount);
					for (uint32 ch = 0; ch < pRenderStageDesc->RayTracing.Shaders.ClosestHitShaderCount; ch++)
					{
						pipelineDesc.ClosestHitShaders[ch].ShaderGUID = pRenderStageDesc->RayTracing.Shaders.pClosestHitShaderNames[ch].empty() ? GUID_NONE : ResourceManager::LoadShaderFromFile(pRenderStageDesc->RayTracing.Shaders.pClosestHitShaderNames[ch], FShaderStageFlag::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER, EShaderLang::SHADER_LANG_GLSL );

						if (pShaderConstants != nullptr)
						{
							pipelineDesc.ClosestHitShaders[ch].ShaderConstants = pShaderConstants->RayTracing.ClosestHitConstants[ch];
						}
					}

					pipelineDesc.MissShaders.Resize(pRenderStageDesc->RayTracing.Shaders.MissShaderCount);
					for (uint32 m = 0; m < pRenderStageDesc->RayTracing.Shaders.MissShaderCount; m++)
					{
						pipelineDesc.MissShaders[m].ShaderGUID = pRenderStageDesc->RayTracing.Shaders.pMissShaderNames[m].empty() ? GUID_NONE : ResourceManager::LoadShaderFromFile(pRenderStageDesc->RayTracing.Shaders.pMissShaderNames[m], FShaderStageFlag::SHADER_STAGE_FLAG_MISS_SHADER, EShaderLang::SHADER_LANG_GLSL );

						if (pShaderConstants != nullptr)
						{
							pipelineDesc.MissShaders[m].ShaderConstants = pShaderConstants->RayTracing.MissConstants[m];
						}
					}

					pRenderStage->PipelineStateID = PipelineStateManager::CreateRayTracingPipelineState(&pipelineDesc);
					VALIDATE(pRenderStage->PipelineStateID != 0);
					pRenderStage->pPipelineState = PipelineStateManager::GetPipelineState(pRenderStage->PipelineStateID);
				}
			}

			//Create Resource Bindings to Render Stage
			{
				if (renderStageRenderTargets.GetSize() > 0)
				{
					if (pRenderStageDesc->Type != EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS)
					{
						LOG_ERROR("[RenderGraph]: There are resources that a RenderPass should be linked to, but Render Stage %u is not a Graphics Pipeline State", renderStageIndex);
						return false;
					}

					for (uint32 r = 0; r < renderStageRenderTargets.GetSize(); r++)
					{
						auto resourcePair = renderStageRenderTargets[r];
						Resource* pResource = resourcePair.first;

						ResourceBinding resourceBinding = {};
						resourceBinding.pRenderStage	= pRenderStage;
						resourceBinding.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_UNKNOWN;
						resourceBinding.Binding			= UINT32_MAX;
						resourceBinding.TextureState	= resourcePair.second;

						pResource->ResourceBindings.PushBack(resourceBinding);		//Create Binding to notify Custom Renderers
						pRenderStage->RenderTargetResources.PushBack(pResource);
					}
				}

				if (pDepthStencilResource != nullptr)
				{
					if (pRenderStageDesc->Type != EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS)
					{
						LOG_ERROR("[RenderGraph]: There are resources that a RenderPass should be linked to, but Render Stage %u is not a Graphics Pipeline State", renderStageIndex);
						return false;
					}

					ResourceBinding resourceBinding = {};
					resourceBinding.pRenderStage	= pRenderStage;
					resourceBinding.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_UNKNOWN;
					resourceBinding.Binding			= UINT32_MAX;
					resourceBinding.TextureState	= ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;

					pDepthStencilResource->ResourceBindings.PushBack(resourceBinding); //Create Binding to notify Custom Renderers
					pRenderStage->pDepthStencilAttachment = pDepthStencilResource;
				}

				for (uint32 r = 0; r < renderStageTextureResources.GetSize(); r++)
				{
					auto& resourceTuple = renderStageTextureResources[r];
					Resource* pResource = std::get<0>(resourceTuple);

					ResourceBinding resourceBinding = {};
					resourceBinding.pRenderStage	= pRenderStage;
					resourceBinding.DescriptorType	= std::get<2>(resourceTuple);
					resourceBinding.Binding			= r;
					resourceBinding.TextureState	= std::get<1>(resourceTuple);

					pResource->ResourceBindings.PushBack(resourceBinding);
				}

				for (uint32 r = 0; r < renderStageBufferResources.GetSize(); r++)
				{
					auto& resourceTuple = renderStageBufferResources[r];
					Resource* pResource = std::get<0>(resourceTuple);

					ResourceBinding resourceBinding = {};
					resourceBinding.pRenderStage	= pRenderStage;
					resourceBinding.DescriptorType	= std::get<1>(resourceTuple);
					resourceBinding.Binding			= r;
					resourceBinding.TextureState	= ETextureState::TEXTURE_STATE_UNKNOWN;

					pResource->ResourceBindings.PushBack(resourceBinding);
				}

				for (uint32 r = 0; r < renderStageDrawArgResources.GetSize(); r++)
				{
					auto& resourceTuple = renderStageDrawArgResources[r];
					Resource* pResource = std::get<0>(resourceTuple);

					ResourceBinding resourceBinding = {};
					resourceBinding.pRenderStage	= pRenderStage;
					resourceBinding.DescriptorType	= std::get<1>(resourceTuple);
					resourceBinding.Binding			= r;
					resourceBinding.TextureState	= ETextureState::TEXTURE_STATE_UNKNOWN;

					pResource->ResourceBindings.PushBack(resourceBinding);
				}
			}
		}

		return true;
	}

	bool RenderGraph::CreateSynchronizationStages(const TArray<SynchronizationStageDesc>& synchronizationStageDescriptions, TSet<DrawArgMaskDesc>& requiredDrawArgMasks)
	{
		m_pSynchronizationStages = DBG_NEW SynchronizationStage[synchronizationStageDescriptions.GetSize()];

		for (uint32 s = 0; s < synchronizationStageDescriptions.GetSize(); s++)
		{
			const SynchronizationStageDesc* pSynchronizationStageDesc = &synchronizationStageDescriptions[s];

			SynchronizationStage* pSynchronizationStage = &m_pSynchronizationStages[s];
			ECommandQueueType otherQueue = ECommandQueueType::COMMAND_QUEUE_TYPE_NONE;

			for (auto synchronizationIt = pSynchronizationStageDesc->Synchronizations.begin(); synchronizationIt != pSynchronizationStageDesc->Synchronizations.end(); synchronizationIt++)
			{
				const RenderGraphResourceSynchronizationDesc* pResourceSynchronizationDesc = &(*synchronizationIt);

				//En massa skit kommer nog beh�va g�ras om h�r, nu n�r Parsern tar hand om Back Buffer States korrekt.

				auto it = m_ResourceMap.find(pResourceSynchronizationDesc->ResourceName);

				if (it == m_ResourceMap.end())
				{
					LOG_ERROR("[RenderGraph]: Resource found in Synchronization Stage but not in Resource Map \"%s\"", pResourceSynchronizationDesc->ResourceName.c_str());
					return false;
				}

				Resource* pResource = &it->second;

				if (!pResource->ShouldSynchronize)
					continue;

				auto prevRenderStageIt = m_RenderStageMap.find(pResourceSynchronizationDesc->PrevRenderStage);
				auto nextRenderStageIt = m_RenderStageMap.find(pResourceSynchronizationDesc->NextRenderStage);

				FPipelineStageFlags	prevLastPipelineStage;


				if (prevRenderStageIt == m_RenderStageMap.end())
				{
					if (pResourceSynchronizationDesc->PrevRenderStage == "PRESENT")
					{
						prevLastPipelineStage = FPipelineStageFlag::PIPELINE_STAGE_FLAG_BOTTOM;
					}
					else
					{
						LOG_ERROR("[RenderGraph]: Render Stage found in Synchronization but not in Render Stage Map \"%s\"", pResourceSynchronizationDesc->PrevRenderStage.c_str());
						return false;
					}
				}
				else
				{
					prevLastPipelineStage = m_pRenderStages[prevRenderStageIt->second].LastPipelineStage;
				}

				if (nextRenderStageIt == m_RenderStageMap.end())
				{
					LOG_ERROR("[RenderGraph]: Render Stage found in Synchronization but not in Render Stage Map \"%s\"", pResourceSynchronizationDesc->NextRenderStage.c_str());
					return false;
				}

				const RenderStage* pNextRenderStage	= &m_pRenderStages[nextRenderStageIt->second];

				ECommandQueueType prevQueue 	= pResourceSynchronizationDesc->PrevQueue;
				ECommandQueueType nextQueue		= pResourceSynchronizationDesc->NextQueue;
				uint32 srcMemoryAccessFlags		= CalculateResourceAccessFlags(pResourceSynchronizationDesc->PrevBindingType);
				uint32 dstMemoryAccessFlags		= CalculateResourceAccessFlags(pResourceSynchronizationDesc->NextBindingType);

				if (pSynchronizationStage->ExecutionQueue == ECommandQueueType::COMMAND_QUEUE_TYPE_NONE)
				{
					pSynchronizationStage->ExecutionQueue = prevQueue;
					otherQueue = pSynchronizationStage->ExecutionQueue == ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS ? ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE : ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS;
				}
				else if (pSynchronizationStage->ExecutionQueue != prevQueue)
				{
					LOG_ERROR("[RenderGraph]: SynchronizationStage \"%s\" contains synchronizations that have different Previous Queues");
					return false;
				}

				pSynchronizationStage->SrcPipelineStage				= FindLastPipelineStage(pSynchronizationStage->SrcPipelineStage | prevLastPipelineStage);
				pSynchronizationStage->SameQueueDstPipelineStage	= FindEarliestCompatiblePipelineStage(pSynchronizationStage->SameQueueDstPipelineStage | pNextRenderStage->FirstPipelineStage, pSynchronizationStage->ExecutionQueue);
				pSynchronizationStage->OtherQueueDstPipelineStage	= FindEarliestCompatiblePipelineStage(pSynchronizationStage->OtherQueueDstPipelineStage | pNextRenderStage->FirstPipelineStage, otherQueue);

				if (pResource->Type == ERenderGraphResourceType::TEXTURE)
				{
					PipelineTextureBarrierDesc textureBarrier = {};
					textureBarrier.QueueBefore			= prevQueue;
					textureBarrier.QueueAfter			= nextQueue;
					textureBarrier.SrcMemoryAccessFlags = srcMemoryAccessFlags;
					textureBarrier.DstMemoryAccessFlags = dstMemoryAccessFlags;
					textureBarrier.StateBefore			= CalculateResourceTextureState(pResource->Type, pResourceSynchronizationDesc->PrevBindingType, pResource->Texture.Format);
					textureBarrier.StateAfter			= CalculateResourceTextureState(pResource->Type, pResourceSynchronizationDesc->NextBindingType, pResource->Texture.Format);
					textureBarrier.TextureFlags			= pResource->Texture.Format == EFormat::FORMAT_D24_UNORM_S8_UINT ? FTextureFlag::TEXTURE_FLAG_DEPTH_STENCIL : 0;

					uint32 targetSynchronizationIndex = 0;

					if (prevQueue == nextQueue)
					{
						if (pResource->BackBufferBound)
						{
							targetSynchronizationIndex = SAME_QUEUE_BACK_BUFFER_BOUND_SYNCHRONIZATION_INDEX;
						}
						else if (pResource->Texture.UnboundedArray)
						{
							targetSynchronizationIndex = SAME_QUEUE_UNBOUNDED_TEXTURE_SYNCHRONIZATION_INDEX;
						}
						else
						{
							targetSynchronizationIndex = SAME_QUEUE_TEXTURE_SYNCHRONIZATION_INDEX;
						}
					}
					else
					{
						if (pResource->BackBufferBound)
						{
							targetSynchronizationIndex = OTHER_QUEUE_BACK_BUFFER_BOUND_SYNCHRONIZATION_INDEX;
						}
						else if (pResource->Texture.UnboundedArray)
						{
							targetSynchronizationIndex = OTHER_QUEUE_UNBOUNDED_TEXTURE_SYNCHRONIZATION_INDEX;
						}
						else
						{
							targetSynchronizationIndex = OTHER_QUEUE_TEXTURE_SYNCHRONIZATION_INDEX;
						}
					}

					if (pResource->Texture.UnboundedArray)
					{
						//Back Buffer Bound textures are not allowed to be Unbounded Arrays
						if (pResource->BackBufferBound)
						{
							LOG_ERROR("[RenderGraph]: Resource \"%s\" found to be Back Buffer Bound and Unbounded, this is currently not allowed!", pResource->Name.c_str());
							return false;
						}

						//If this is an Unbounded Array of textures we use a similar trick to Draw Args, we create one barrier which we will later use as a template
						TArray<TArray<PipelineTextureBarrierDesc>>& targetArray = pSynchronizationStage->UnboundedTextureBarriers[targetSynchronizationIndex];
						targetArray.PushBack({ textureBarrier });
						uint32 barrierIndex = targetArray.GetSize() - 1;

						ResourceBarrierInfo barrierInfo = {};
						barrierInfo.SynchronizationStageIndex	= s;
						barrierInfo.SynchronizationTypeIndex	= targetSynchronizationIndex;
						barrierInfo.BarrierIndex				= barrierIndex;

						pResource->BarriersPerSynchronizationStage.PushBack(barrierInfo);
					}
					else
					{
						uint32 actualSubResourceCount = 0;

						bool isTextureCube = pResource->Texture.TextureType == ERenderGraphTextureType::TEXTURE_CUBE;
						if (pResource->BackBufferBound)
						{
							actualSubResourceCount = m_BackBufferCount;
							textureBarrier.ArrayCount = isTextureCube ? 6 : 1;
						}
						else if (pResource->Texture.IsOfArrayType)
						{
							actualSubResourceCount = 1;
							textureBarrier.ArrayCount = isTextureCube ? 6 * pResource->SubResourceCount : pResource->SubResourceCount;
						}
						else
						{
							actualSubResourceCount = pResource->SubResourceCount;
							textureBarrier.ArrayCount = isTextureCube ? 6 : 1;
						}

						for (uint32 sr = 0; sr < actualSubResourceCount; sr++)
						{
							TArray<PipelineTextureBarrierDesc>& targetArray = pSynchronizationStage->TextureBarriers[targetSynchronizationIndex];
							targetArray.PushBack(textureBarrier);
							uint32 barrierIndex = targetArray.GetSize() - 1;

							ResourceBarrierInfo barrierInfo = {};
							barrierInfo.SynchronizationStageIndex	= s;
							barrierInfo.SynchronizationTypeIndex	= targetSynchronizationIndex;
							barrierInfo.BarrierIndex				= barrierIndex;

							pResource->BarriersPerSynchronizationStage.PushBack(barrierInfo);
						}
					}
				}
				else if (pResource->Type == ERenderGraphResourceType::SCENE_DRAW_ARGS)
				{
					{
						PipelineBufferBarrierDesc bufferBarrier = {};
						bufferBarrier.QueueBefore = prevQueue;
						bufferBarrier.QueueAfter = nextQueue;
						bufferBarrier.SrcMemoryAccessFlags = srcMemoryAccessFlags;
						bufferBarrier.DstMemoryAccessFlags = dstMemoryAccessFlags;

						uint32 targetSynchronizationIndex = 0;

						if (prevQueue == nextQueue)
						{
							targetSynchronizationIndex = SAME_QUEUE_BUFFER_SYNCHRONIZATION_INDEX;
						}
						else
						{
							targetSynchronizationIndex = OTHER_QUEUE_BUFFER_SYNCHRONIZATION_INDEX;
						}

						TArray<PipelineBufferBarrierDesc>& targetArray = pSynchronizationStage->DrawBufferBarriers[targetSynchronizationIndex];
						targetArray.PushBack(bufferBarrier);
						uint32 barrierIndex = targetArray.GetSize() - 1;

						//We ignore SubResourceCount since DRAW_BUFFERS don't have predetermined SubResourceCount, instead it is determined at runtime

						DrawArgMaskDesc maskDesc = {};
						maskDesc.IncludeMask = synchronizationIt->DrawArgsIncludeMask;
						maskDesc.ExcludeMask = synchronizationIt->DrawArgsExcludeMask;

						ResourceBarrierInfo barrierInfo = {};
						barrierInfo.SynchronizationStageIndex	= s;
						barrierInfo.SynchronizationTypeIndex	= targetSynchronizationIndex;
						barrierInfo.BarrierIndex				= barrierIndex;
						barrierInfo.DrawArgsMaskDesc			= maskDesc;

						requiredDrawArgMasks.insert(maskDesc);

						pResource->BarriersPerSynchronizationStage.PushBack(barrierInfo);

						DrawArgsData drawArgsData = {};
						drawArgsData.InitialTransitionBarrierTemplate.pBuffer				= nullptr;
						drawArgsData.InitialTransitionBarrierTemplate.QueueBefore			= prevQueue;
						drawArgsData.InitialTransitionBarrierTemplate.QueueAfter			= drawArgsData.InitialTransitionBarrierTemplate.QueueBefore;
						drawArgsData.InitialTransitionBarrierTemplate.SrcMemoryAccessFlags	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
						drawArgsData.InitialTransitionBarrierTemplate.DstMemoryAccessFlags	= srcMemoryAccessFlags;

						drawArgsData.InitialTextureTransitionBarrierTemplate.QueueBefore			= prevQueue;
						drawArgsData.InitialTextureTransitionBarrierTemplate.QueueAfter				= drawArgsData.InitialTextureTransitionBarrierTemplate.QueueBefore;
						drawArgsData.InitialTextureTransitionBarrierTemplate.SrcMemoryAccessFlags	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
						drawArgsData.InitialTextureTransitionBarrierTemplate.DstMemoryAccessFlags	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_READ;
						drawArgsData.InitialTextureTransitionBarrierTemplate.StateBefore			= ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;
						drawArgsData.InitialTextureTransitionBarrierTemplate.StateAfter				= ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;

						pResource->DrawArgs.FullMaskToArgs[maskDesc.FullMask] = drawArgsData;
					}

					// Textures from draw arg extensions.
					// (This is the same code as in the Texture Resource, but uses DrawTextureBarriers instead of TextureBarriers, Might want to make a function for this.)
					{
						PipelineTextureBarrierDesc textureBarrier = {};
						textureBarrier.QueueBefore = prevQueue;
						textureBarrier.QueueAfter = nextQueue;
						textureBarrier.SrcMemoryAccessFlags = srcMemoryAccessFlags;
						textureBarrier.DstMemoryAccessFlags = dstMemoryAccessFlags;
						textureBarrier.StateBefore = CalculateResourceTextureState(ERenderGraphResourceType::TEXTURE, pResourceSynchronizationDesc->PrevBindingType, pResource->Texture.Format);
						textureBarrier.StateAfter = CalculateResourceTextureState(ERenderGraphResourceType::TEXTURE, pResourceSynchronizationDesc->NextBindingType, pResource->Texture.Format);
						textureBarrier.TextureFlags = pResource->Texture.Format == EFormat::FORMAT_D24_UNORM_S8_UINT ? FTextureFlag::TEXTURE_FLAG_DEPTH_STENCIL : 0;

						uint32 targetSynchronizationIndex = 0;

						if (prevQueue == nextQueue)
						{
							targetSynchronizationIndex = 0; // SAME_QUEUE_TEXTURE_SYNCHRONIZATION_INDEX
						}
						else
						{
							targetSynchronizationIndex = 1; // OTHER_QUEUE_TEXTURE_SYNCHRONIZATION_INDEX
						}

						uint32 actualSubResourceCount = 0;
						bool isTextureCube = pResource->Texture.TextureType == ERenderGraphTextureType::TEXTURE_CUBE;
						if (pResource->BackBufferBound)
						{
							actualSubResourceCount = m_BackBufferCount;
							textureBarrier.ArrayCount = isTextureCube ? 6 : 1;
						}
						else if (pResource->Texture.IsOfArrayType)
						{
							actualSubResourceCount = 1;
							textureBarrier.ArrayCount = isTextureCube ? 6 * pResource->SubResourceCount : pResource->SubResourceCount;
						}
						else
						{
							actualSubResourceCount = pResource->SubResourceCount;
							textureBarrier.ArrayCount = isTextureCube ? 6 : 1;
						}

						for (uint32 sr = 0; sr < actualSubResourceCount; sr++)
						{
							TArray<PipelineTextureBarrierDesc>& targetArray = pSynchronizationStage->DrawTextureBarriers[targetSynchronizationIndex];
							targetArray.PushBack(textureBarrier);

							/*
							uint32 barrierIndex = targetArray.GetSize() - 1;
							ResourceBarrierInfo barrierInfo = {};
							barrierInfo.SynchronizationStageIndex = s;
							barrierInfo.SynchronizationTypeIndex = targetSynchronizationIndex;
							barrierInfo.BarrierIndex = barrierIndex;

							pResource->BarriersPerSynchronizationStage.PushBack(barrierInfo);
							*/
						}
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

					uint32 actualSubResourceCount = 0;
					if (pResource->BackBufferBound)
					{
						actualSubResourceCount = m_BackBufferCount;
					}
					else
					{
						actualSubResourceCount = pResource->SubResourceCount;
					}

					for (uint32 sr = 0; sr < actualSubResourceCount; sr++)
					{
						TArray<PipelineBufferBarrierDesc>& targetArray = pSynchronizationStage->BufferBarriers[targetSynchronizationIndex];
						targetArray.PushBack(bufferBarrier);
						uint32 barrierIndex = targetArray.GetSize() - 1;

						ResourceBarrierInfo barrierInfo = {};
						barrierInfo.SynchronizationStageIndex	= s;
						barrierInfo.SynchronizationTypeIndex	= targetSynchronizationIndex;
						barrierInfo.BarrierIndex				= barrierIndex;

						pResource->BarriersPerSynchronizationStage.PushBack(barrierInfo);
					}
				}
			}
		}

		return true;
	}

	bool RenderGraph::CreatePipelineStages(const TArray<PipelineStageDesc>& pipelineStageDescriptions)
	{
		m_PipelineStageCount = (uint32)pipelineStageDescriptions.GetSize();
		m_pPipelineStages = DBG_NEW PipelineStage[m_PipelineStageCount];

		String pipelineStageName = "";

		for (uint32 i = 0; i < m_PipelineStageCount; i++)
		{
			const PipelineStageDesc* pPipelineStageDesc = &pipelineStageDescriptions[i];

			PipelineStage* pPipelineStage = &m_pPipelineStages[i];

			bool createCommandLists = true;

			if (pPipelineStageDesc->Type == ERenderGraphPipelineStageType::RENDER)
			{
				bool usesCustomRenderer = m_pRenderStages[pPipelineStageDesc->StageIndex].UsesCustomRenderer;
				createCommandLists = !usesCustomRenderer;
				m_ExecutionStageCount += usesCustomRenderer ? 2 : 1;
				pipelineStageName = m_pRenderStages[pPipelineStageDesc->StageIndex].Name;
			}
			else if (pPipelineStageDesc->Type == ERenderGraphPipelineStageType::SYNCHRONIZATION)
			{
				m_ExecutionStageCount += 2;
				pipelineStageName = "Synchronization Stage " + std::to_string(pPipelineStageDesc->StageIndex);
			}

			pPipelineStage->Type		= pPipelineStageDesc->Type;
			pPipelineStage->StageIndex	= pPipelineStageDesc->StageIndex;

			if (createCommandLists)
			{
				pPipelineStage->ppGraphicsCommandAllocators		= DBG_NEW CommandAllocator*[m_BackBufferCount];
				pPipelineStage->ppComputeCommandAllocators		= DBG_NEW CommandAllocator*[m_BackBufferCount];
				pPipelineStage->ppGraphicsCommandLists			= DBG_NEW CommandList*[m_BackBufferCount];
				pPipelineStage->ppComputeCommandLists			= DBG_NEW CommandList*[m_BackBufferCount];

				for (uint32 f = 0; f < m_BackBufferCount; f++)
				{
					//Todo: Don't always allocate 2 command lists (timestamps also do this)
					pPipelineStage->ppGraphicsCommandAllocators[f]	= m_pGraphicsDevice->CreateCommandAllocator("Render Graph Graphics Command Allocator", ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);
					pPipelineStage->ppComputeCommandAllocators[f]	= m_pGraphicsDevice->CreateCommandAllocator("Render Graph Compute Command Allocator", ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE);

					CommandListDesc graphicsCommandListDesc = {};
					graphicsCommandListDesc.DebugName				= pipelineStageName + " Graphics Command List";
					graphicsCommandListDesc.CommandListType			= ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
					graphicsCommandListDesc.Flags					= FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

					pPipelineStage->ppGraphicsCommandLists[f]		= m_pGraphicsDevice->CreateCommandList(pPipelineStage->ppGraphicsCommandAllocators[f], &graphicsCommandListDesc);

					// Add graphics timestamps
					Profiler::GetGPUProfiler()->AddTimestamp(pPipelineStage->ppGraphicsCommandLists[f], pipelineStageName + " GRAPHICS");

					CommandListDesc computeCommandListDesc = {};
					computeCommandListDesc.DebugName				= pipelineStageName + " Compute Command List";
					computeCommandListDesc.CommandListType			= ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
					computeCommandListDesc.Flags					= FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

					pPipelineStage->ppComputeCommandLists[f]		= m_pGraphicsDevice->CreateCommandList(pPipelineStage->ppComputeCommandAllocators[f], &computeCommandListDesc);

					// Add compute timestamps
					Profiler::GetGPUProfiler()->AddTimestamp(pPipelineStage->ppComputeCommandLists[f], pipelineStageName + " COMPUTE");
				}

				// Reset all timestamps and pipeline statistics before first use of them
				for (uint32 f = 0; f < m_BackBufferCount; f++)
				{
					pPipelineStage->ppGraphicsCommandLists[f]->Begin(nullptr);
					Profiler::GetGPUProfiler()->ResetTimestamp(pPipelineStage->ppGraphicsCommandLists[f]);
					Profiler::GetGPUProfiler()->ResetGraphicsPipelineStat(pPipelineStage->ppGraphicsCommandLists[f]);
					Profiler::GetGPUProfiler()->StartGraphicsPipelineStat(pPipelineStage->ppGraphicsCommandLists[f]);
					Profiler::GetGPUProfiler()->EndGraphicsPipelineStat(pPipelineStage->ppGraphicsCommandLists[f]);
					pPipelineStage->ppGraphicsCommandLists[f]->End();
					RenderAPI::GetGraphicsQueue()->ExecuteCommandLists(&pPipelineStage->ppGraphicsCommandLists[f], 1, FPipelineStageFlag::PIPELINE_STAGE_FLAG_UNKNOWN, nullptr, 0, nullptr, 0);
					RenderAPI::GetGraphicsQueue()->Flush();

					pPipelineStage->ppComputeCommandLists[f]->Begin(nullptr);
					Profiler::GetGPUProfiler()->ResetTimestamp(pPipelineStage->ppComputeCommandLists[f]);
					pPipelineStage->ppComputeCommandLists[f]->End();
					RenderAPI::GetComputeQueue()->ExecuteCommandLists(&pPipelineStage->ppComputeCommandLists[f], 1, FPipelineStageFlag::PIPELINE_STAGE_FLAG_UNKNOWN, nullptr, 0, nullptr, 0);
					RenderAPI::GetComputeQueue()->Flush();
				}
			}
		}

		m_ppExecutionStages = DBG_NEW CommandList*[m_ExecutionStageCount];

		return true;
	}

	bool RenderGraph::CustomRenderStagesPostInit()
	{
		for (CustomRenderer* pCustomRenderer : m_CustomRenderers)
		{
			if (!pCustomRenderer->RenderGraphPostInit())
			{
				return false;
			}
		}

		return true;
	}

	void RenderGraph::UpdateRelativeParameters()
	{
		RenderAPI::GetGraphicsQueue()->Flush();
		RenderAPI::GetComputeQueue()->Flush();
		RenderAPI::GetCopyQueue()->Flush();

		for (uint32 renderStageIndex : m_WindowRelativeRenderStages)
		{
			RenderStage* pRenderStage = &m_pRenderStages[renderStageIndex];

			UpdateRelativeRenderStageDimensions(pRenderStage);
		}

		for (const String& resourceName : m_WindowRelativeResources)
		{
			InternalResourceUpdateDesc* pInternalResourceUpdateDesc = &m_InternalResourceUpdateDescriptions[resourceName];

			UpdateRelativeResourceDimensions(pInternalResourceUpdateDesc);
		}
	}

	void RenderGraph::UpdateInternalResource(InternalResourceUpdateDesc& desc)
	{
		auto it = m_ResourceMap.find(desc.ResourceName);

		if (it != m_ResourceMap.end())
		{
			Resource* pResource = &it->second;
			ResourceUpdateDesc resourceUpdateDesc;
			resourceUpdateDesc.ResourceName = desc.ResourceName;

			switch (desc.Type)
			{
				case ERenderGraphResourceType::TEXTURE:
				{
					resourceUpdateDesc.InternalTextureUpdate.pTextureDesc		= &desc.TextureUpdate.TextureDesc;
					resourceUpdateDesc.InternalTextureUpdate.pTextureViewDesc	= &desc.TextureUpdate.TextureViewDesc;
					resourceUpdateDesc.InternalTextureUpdate.pSamplerDesc		= &desc.TextureUpdate.SamplerDesc;
					UpdateResourceTexture(pResource, &resourceUpdateDesc);
					break;
				}
				case ERenderGraphResourceType::BUFFER:
				{
					resourceUpdateDesc.InternalBufferUpdate.pBufferDesc			= &desc.BufferUpdate.BufferDesc;
					UpdateResourceBuffer(pResource, &resourceUpdateDesc);
					break;
				}
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

	void RenderGraph::UpdateResourceTexture(Resource* pResource, const ResourceUpdateDesc* pDesc)
	{
		uint32 actualSubResourceCount = 0;
		// If true, every texture has a unique sampler (or atleast a sampler array the same size of the texture array)
		bool uniqueSamplers = true;

		//Unbounded arrays are handled differently compared to normal textures
		if (pResource->Texture.UnboundedArray)
		{
			uniqueSamplers = pDesc->ExternalTextureUpdate.TextureCount == pDesc->ExternalTextureUpdate.SamplerCount;

			if (!uniqueSamplers && pDesc->ExternalTextureUpdate.SamplerCount > 1)
			{
				LOG_WARNING("[RenderGraph, UpdateResourceTexture]: SamplerCount does not match TextureCount and is not equal to 1. Only the first sampler will be used. TextureCount = %d, SamplerCount = %d",
				pDesc->ExternalTextureUpdate.TextureCount, pDesc->ExternalTextureUpdate.SamplerCount);
			}

			//We don't know the subresource count until now so we must update all container arrays
			actualSubResourceCount = pDesc->ExternalTextureUpdate.TextureCount;
			pResource->Texture.Textures.Resize(actualSubResourceCount);
			pResource->Texture.PerImageTextureViews.Resize(actualSubResourceCount);
			pResource->Texture.Samplers.Resize(uniqueSamplers ? pDesc->ExternalTextureUpdate.TextureCount : 1);
			pResource->Texture.PerSubImageTextureViews.Resize(actualSubResourceCount * (pDesc->ExternalTextureUpdate.ppPerSubImageTextureViews != nullptr ? pDesc->ExternalTextureUpdate.PerImageSubImageTextureViewCount : 1));

			//We must clear all non-template barriers
			for (uint32 b = 0; b < pResource->BarriersPerSynchronizationStage.GetSize(); b++)
			{
				const ResourceBarrierInfo* pBarrierInfo = &pResource->BarriersPerSynchronizationStage[b];
				SynchronizationStage* pSynchronizationStage = &m_pSynchronizationStages[pBarrierInfo->SynchronizationStageIndex];

				TArray<PipelineTextureBarrierDesc>& textureBarriers = pSynchronizationStage->UnboundedTextureBarriers[pBarrierInfo->SynchronizationTypeIndex][pBarrierInfo->BarrierIndex];
				textureBarriers.Resize(1);
				textureBarriers[0].pTexture = nullptr;
			}
		}
		else
		{
			if (pResource->BackBufferBound)
			{
				actualSubResourceCount = m_BackBufferCount;
			}
			else if (pResource->Texture.IsOfArrayType)
			{
				actualSubResourceCount = 1;
			}
			else
			{
				actualSubResourceCount = pResource->SubResourceCount;
			}
		}

		for (uint32 sr = 0; sr < actualSubResourceCount; sr++)
		{
			Texture** ppTexture			= &pResource->Texture.Textures[sr];
			TextureView** ppTextureView = &pResource->Texture.PerImageTextureViews[sr];
			Sampler** ppSampler			= &pResource->Texture.Samplers[uniqueSamplers ? sr : 0];

			Texture* pTexture						= nullptr;
			TextureView* pTextureView				= nullptr;
			Sampler* pSampler						= nullptr;

			if (pResource->OwnershipType == EResourceOwnershipType::INTERNAL)
			{
				const TextureDesc* pTextureDesc	= pDesc->InternalTextureUpdate.pTextureDesc;
				TextureViewDesc textureViewDesc = *pDesc->InternalTextureUpdate.pTextureViewDesc; //Make a copy so we can change TextureViewDesc::pTexture

				SAFERELEASE(*ppTexture);
				SAFERELEASE(*ppTextureView);

				pTexture = m_pGraphicsDevice->CreateTexture(pTextureDesc);

				textureViewDesc.pTexture = pTexture;
				pTextureView = m_pGraphicsDevice->CreateTextureView(&textureViewDesc);

				//Create texture views for sub images to be used as Render Targets
				if (pResource->Texture.UsedAsRenderTarget)
				{
					if (pResource->Texture.PerSubImageUniquelyAllocated)
					{
						TextureViewDesc subImageTextureViewDesc = {};
						subImageTextureViewDesc.pTexture		= pTexture;
						subImageTextureViewDesc.Flags			= (textureViewDesc.Flags & FTextureViewFlag::TEXTURE_VIEW_FLAG_DEPTH_STENCIL) > 0 ? FTextureViewFlag::TEXTURE_VIEW_FLAG_DEPTH_STENCIL : FTextureViewFlag::TEXTURE_VIEW_FLAG_RENDER_TARGET;
						subImageTextureViewDesc.Format			= pResource->Texture.Format;
						subImageTextureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_TYPE_2D;
						subImageTextureViewDesc.Miplevel		= textureViewDesc.Miplevel;
						subImageTextureViewDesc.MiplevelCount	= textureViewDesc.MiplevelCount;
						subImageTextureViewDesc.ArrayCount		= 1;

						for (uint32 si = 0; si < pTextureDesc->ArrayCount; si++)
						{
							TextureView** ppPerSubImageTextureView = &pResource->Texture.PerSubImageTextureViews[sr * pTextureDesc->ArrayCount + si];

							subImageTextureViewDesc.DebugName	= pResource->Name + " Sub Image Texture View " + std::to_string(si);
							subImageTextureViewDesc.ArrayIndex	= si;

							SAFERELEASE(*ppPerSubImageTextureView);
							(*ppPerSubImageTextureView)	= m_pGraphicsDevice->CreateTextureView(&subImageTextureViewDesc);
						}
					}
					else
					{
						pResource->Texture.PerSubImageTextureViews[sr] = pTextureView;
					}
				}

				//Update Sampler
				if (pDesc->InternalTextureUpdate.pSamplerDesc != nullptr)
				{
					SAFERELEASE(*ppSampler);
					pSampler = m_pGraphicsDevice->CreateSampler(pDesc->InternalTextureUpdate.pSamplerDesc);
				}
			}
			else if (pResource->OwnershipType == EResourceOwnershipType::EXTERNAL)
			{
				pTexture			= pDesc->ExternalTextureUpdate.ppTextures[sr];
				pTextureView		= pDesc->ExternalTextureUpdate.ppTextureViews[sr];

				if (pDesc->ExternalTextureUpdate.ppPerSubImageTextureViews != nullptr)
				{
					uint32 subImageBaseIndex = sr * pDesc->ExternalTextureUpdate.PerImageSubImageTextureViewCount;

					for (uint32 si = 0; si < pDesc->ExternalTextureUpdate.PerImageSubImageTextureViewCount; si++)
					{
						pResource->Texture.PerSubImageTextureViews[subImageBaseIndex + si] = pDesc->ExternalTextureUpdate.ppPerSubImageTextureViews[subImageBaseIndex + si];
					}
				}
				else
				{
					pResource->Texture.PerSubImageTextureViews[sr] = pTextureView;
				}

				//Update Sampler
				if (pDesc->ExternalTextureUpdate.ppSamplers != nullptr)
				{
					pSampler = pDesc->ExternalTextureUpdate.ppSamplers[uniqueSamplers ? sr : 0];
				}
			}
			else
			{
				LOG_ERROR("[RenderGraph]: UpdateResourceTexture called for resource with invalid OwnershipType");
				return;
			}

			const TextureDesc& textureDesc = pTexture->GetDesc();

			if (pResource->Texture.IsOfArrayType)
			{
				if (pResource->Texture.TextureType == ERenderGraphTextureType::TEXTURE_CUBE)
				{
					if (textureDesc.ArrayCount != pResource->SubResourceCount * 6)
					{
						LOG_ERROR("[RenderGraph]: UpdateResourceTexture for resource of array type with length %u and type TextureCube but ArrayCount was %u", pResource->SubResourceCount, textureDesc.ArrayCount);
						return;
					}
				}
				else
				{
					if (textureDesc.ArrayCount != pResource->SubResourceCount)
					{
						LOG_ERROR("[RenderGraph]: UpdateResourceTexture for resource of array type with length %u and type Texture2D but ArrayCount was %u", pResource->SubResourceCount, textureDesc.ArrayCount);
						return;
					}
				}
			}

			(*ppTexture)		= pTexture;
			(*ppTextureView)	= pTextureView;
			(*ppSampler)		= pSampler;

			if (pResource->BarriersPerSynchronizationStage.GetSize() > 0)
			{
				if (pResource->Texture.UnboundedArray)
				{
					for (uint32 b = 0; b < pResource->BarriersPerSynchronizationStage.GetSize(); b++)
					{
						const ResourceBarrierInfo* pBarrierInfo = &pResource->BarriersPerSynchronizationStage[b];
						SynchronizationStage* pSynchronizationStage = &m_pSynchronizationStages[pBarrierInfo->SynchronizationStageIndex];

						TArray<PipelineTextureBarrierDesc>& textureBarriers = pSynchronizationStage->UnboundedTextureBarriers[pBarrierInfo->SynchronizationTypeIndex][pBarrierInfo->BarrierIndex];
						PipelineTextureBarrierDesc& templateBarrier = textureBarriers[0];

						if (templateBarrier.pTexture == nullptr)
						{
							templateBarrier.pTexture		= pTexture;
							templateBarrier.Miplevel		= 0;
							templateBarrier.MiplevelCount	= textureDesc.Miplevels;
							templateBarrier.ArrayIndex		= 0;
							templateBarrier.ArrayCount		= textureDesc.ArrayCount;
						}
						else
						{
							PipelineTextureBarrierDesc newBarrier = templateBarrier;
							newBarrier.pTexture			= pTexture;
							newBarrier.Miplevel			= 0;
							newBarrier.MiplevelCount	= textureDesc.Miplevels;
							newBarrier.ArrayIndex		= 0;
							newBarrier.ArrayCount		= textureDesc.ArrayCount;
							textureBarriers.PushBack(newBarrier);
						}
					}
				}
				else
				{
					for (uint32 b = sr; b < pResource->BarriersPerSynchronizationStage.GetSize(); b += pResource->SubResourceCount)
					{
						const ResourceBarrierInfo* pBarrierInfo = &pResource->BarriersPerSynchronizationStage[b];
						SynchronizationStage* pSynchronizationStage = &m_pSynchronizationStages[pBarrierInfo->SynchronizationStageIndex];

						PipelineTextureBarrierDesc* pTextureBarrier = &pSynchronizationStage->TextureBarriers[pBarrierInfo->SynchronizationTypeIndex][pBarrierInfo->BarrierIndex];

						pTextureBarrier->pTexture		= pTexture;
						pTextureBarrier->Miplevel		= 0;
						pTextureBarrier->MiplevelCount	= textureDesc.Miplevels;
						pTextureBarrier->ArrayIndex		= 0;
						pTextureBarrier->ArrayCount		= textureDesc.ArrayCount;
					}
				}
			}

			//Transfer to Initial State
			if (pResource->Texture.InitialTransitionBarrier.QueueBefore != ECommandQueueType::COMMAND_QUEUE_TYPE_UNKNOWN && pResource->ShouldSynchronize)
			{
				PipelineTextureBarrierDesc& initialBarrier = pResource->Texture.InitialTransitionBarrier;

				initialBarrier.pTexture				= pTexture;
				initialBarrier.Miplevel				= 0;
				initialBarrier.MiplevelCount		= textureDesc.Miplevels;
				initialBarrier.ArrayIndex			= 0;
				initialBarrier.ArrayCount			= textureDesc.ArrayCount;

				FPipelineStageFlags srcPipelineStage = pResource->LastPipelineStageOfFirstRenderStage;
				FPipelineStageFlags dstPipelineStage = pResource->LastPipelineStageOfFirstRenderStage;

				if (initialBarrier.QueueAfter == ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS)
				{
					CommandList* pCommandList = m_ppGraphicsCopyCommandLists[m_ModFrameIndex];

					if (!pCommandList->IsBegin())
					{
						m_ppGraphicsCopyCommandAllocators[m_ModFrameIndex]->Reset();
						pCommandList->Begin(nullptr);
					}

					pCommandList->PipelineTextureBarriers(srcPipelineStage, dstPipelineStage, &initialBarrier, 1);
				}
				else if (initialBarrier.QueueAfter == ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE)
				{
					CommandList* pCommandList = m_ppComputeCopyCommandLists[m_ModFrameIndex];

					if (!pCommandList->IsBegin())
					{
						m_ppComputeCopyCommandAllocators[m_ModFrameIndex]->Reset();
						pCommandList->Begin(nullptr);
					}

					pCommandList->PipelineTextureBarriers(srcPipelineStage, dstPipelineStage, &initialBarrier, 1);
				}
			}
		}

		if (pResource->ResourceBindings.GetSize() > 0)
		{
			m_DirtyBoundTextureResources.insert(pResource);

			for (const ResourceBinding& binding : pResource->ResourceBindings)
			{
				m_DirtyRenderStageTextureSets.insert(binding.pRenderStage);
			}
		}
	}

	void RenderGraph::UpdateResourceDrawArgs(Resource* pResource, const ResourceUpdateDesc* pDesc)
	{
		auto drawArgsArgsIt = pResource->DrawArgs.FullMaskToArgs.find(pDesc->ExternalDrawArgsUpdate.DrawArgsMaskDesc.FullMask);

		if (drawArgsArgsIt != pResource->DrawArgs.FullMaskToArgs.end())
		{
			drawArgsArgsIt->second.Args.Clear();

			drawArgsArgsIt->second.Args.Resize(pDesc->ExternalDrawArgsUpdate.Count);

			for (uint32 d = 0; d < pDesc->ExternalDrawArgsUpdate.Count; d++)
			{
				drawArgsArgsIt->second.Args[d] = pDesc->ExternalDrawArgsUpdate.pDrawArgs[d];
			}

			//Update Synchronization Stage Barriers
			for (uint32 b = 0; b < pResource->BarriersPerSynchronizationStage.GetSize(); b += pResource->SubResourceCount)
			{
				const ResourceBarrierInfo* pBarrierInfo = &pResource->BarriersPerSynchronizationStage[b];

				if (pDesc->ExternalDrawArgsUpdate.DrawArgsMaskDesc == pBarrierInfo->DrawArgsMaskDesc)
				{
					SynchronizationStage* pSynchronizationStage = &m_pSynchronizationStages[pBarrierInfo->SynchronizationStageIndex];

					//Todo: This only works while there is a single Draw Args resource, this is OK for now but should be changed when generalizing
					TArray<PipelineBufferBarrierDesc>& drawBufferBarriers = pSynchronizationStage->DrawBufferBarriers[pBarrierInfo->SynchronizationTypeIndex];
					TArray<PipelineTextureBarrierDesc>& drawTextureBarriers = pSynchronizationStage->DrawTextureBarriers[pBarrierInfo->SynchronizationTypeIndex];

					PipelineBufferBarrierDesc bufferBarrierTemplate = drawBufferBarriers.GetFront();
					PipelineTextureBarrierDesc textureBarrierTemplate = drawTextureBarriers.GetFront();

					if (pDesc->ExternalDrawArgsUpdate.Count != 0)
					{
						drawBufferBarriers.Clear();
						drawTextureBarriers.Clear();
					}
					else
					{
						drawBufferBarriers.GetFront().pBuffer = nullptr;
						drawTextureBarriers.GetFront().pTexture = nullptr;
						continue;
					}

					for (uint32 d = 0; d < pDesc->ExternalDrawArgsUpdate.Count; d++)
					{
						DrawArg* pDrawArg = &pDesc->ExternalDrawArgsUpdate.pDrawArgs[d];

						// Vertex Buffer
						{
							VALIDATE(pDrawArg->pVertexBuffer);

							bufferBarrierTemplate.pBuffer		= pDrawArg->pVertexBuffer;
							bufferBarrierTemplate.SizeInBytes	= pDrawArg->pVertexBuffer->GetDesc().SizeInBytes;
							bufferBarrierTemplate.Offset		= 0;
							drawBufferBarriers.PushBack(bufferBarrierTemplate);
						}

						// Instance Buffer
						{
							VALIDATE(pDrawArg->pInstanceBuffer);

							bufferBarrierTemplate.pBuffer		= pDrawArg->pInstanceBuffer;
							bufferBarrierTemplate.SizeInBytes	= pDrawArg->pInstanceBuffer->GetDesc().SizeInBytes;
							bufferBarrierTemplate.Offset		= 0;
							drawBufferBarriers.PushBack(bufferBarrierTemplate);
						}

						// Index Buffer
						{
							VALIDATE(pDrawArg->pIndexBuffer);

							bufferBarrierTemplate.pBuffer		= pDrawArg->pIndexBuffer;
							bufferBarrierTemplate.SizeInBytes	= pDrawArg->pIndexBuffer->GetDesc().SizeInBytes;
							bufferBarrierTemplate.Offset		= 0;
							drawBufferBarriers.PushBack(bufferBarrierTemplate);
						}

						// Meshlet Buffer
						if(pDrawArg->pMeshletBuffer)
						{
							bufferBarrierTemplate.pBuffer		= pDrawArg->pMeshletBuffer;
							bufferBarrierTemplate.SizeInBytes	= pDrawArg->pMeshletBuffer->GetDesc().SizeInBytes;
							bufferBarrierTemplate.Offset		= 0;
							drawBufferBarriers.PushBack(bufferBarrierTemplate);
						}

						// UniqueIndices Buffer
						if (pDrawArg->pUniqueIndicesBuffer)
						{
							bufferBarrierTemplate.pBuffer		= pDrawArg->pUniqueIndicesBuffer;
							bufferBarrierTemplate.SizeInBytes	= pDrawArg->pUniqueIndicesBuffer->GetDesc().SizeInBytes;
							bufferBarrierTemplate.Offset		= 0;
							drawBufferBarriers.PushBack(bufferBarrierTemplate);
						}

						// PrimitiveIndices Buffer
						if (pDrawArg->pPrimitiveIndices)
						{
							bufferBarrierTemplate.pBuffer		= pDrawArg->pPrimitiveIndices;
							bufferBarrierTemplate.SizeInBytes	= pDrawArg->pPrimitiveIndices->GetDesc().SizeInBytes;
							bufferBarrierTemplate.Offset		= 0;
							drawBufferBarriers.PushBack(bufferBarrierTemplate);
						}


						// For draw arg extensions
						if (pDrawArg->HasExtensions)
						{
							uint32 numExtensionGroups = pDrawArg->InstanceCount;
							for (uint32 i = 0; i < numExtensionGroups; i++)
							{
								DrawArgExtensionGroup* extensionGroup = pDrawArg->ppExtensionGroups[i];
								if (extensionGroup)
								{
									uint32 extensionCount = extensionGroup->ExtensionCount;
									for (uint32 e = 0; e < extensionCount; e++)
									{
										DrawArgExtensionData& extension = extensionGroup->pExtensions[e];
										uint32 numTextures = extension.TextureCount;
										for (uint32 t = 0; t < numTextures; t++)
										{
											//uint32 masks = extensionGroup->pExtensionMasks[e];
											const TextureViewDesc& textureViewDesc = extension.ppTextureViews[t]->GetDesc();
											textureBarrierTemplate.StateBefore = ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;// CalculateResourceTextureState(ERenderGraphResourceType::TEXTURE, pResourceSynchronizationDesc->PrevBindingType, pResource->Texture.Format);
											textureBarrierTemplate.StateAfter = ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;
											textureBarrierTemplate.pTexture = extension.ppTextures[t];
											textureBarrierTemplate.Miplevel = textureViewDesc.Miplevel;
											textureBarrierTemplate.MiplevelCount = textureViewDesc.MiplevelCount;
											textureBarrierTemplate.ArrayIndex = textureViewDesc.ArrayIndex;
											drawTextureBarriers.PushBack(textureBarrierTemplate);
										}
									}
								}
							}
						}
					}
				}
			}

			static TArray<PipelineBufferBarrierDesc> intialBarriers;
			static TArray<PipelineTextureBarrierDesc> intialTextureBarriers;
			intialBarriers.Clear();
			intialTextureBarriers.Clear();

			//Create Initial Barriers
			for (uint32 d = 0; d < pDesc->ExternalDrawArgsUpdate.Count; d++)
			{
				DrawArg* pDrawArg = &pDesc->ExternalDrawArgsUpdate.pDrawArgs[d];

				// Vertex Buffer
				{
					VALIDATE(pDrawArg->pVertexBuffer);

					PipelineBufferBarrierDesc initialVertexBufferTransitionBarrier = drawArgsArgsIt->second.InitialTransitionBarrierTemplate;
					initialVertexBufferTransitionBarrier.pBuffer		= pDrawArg->pVertexBuffer;
					initialVertexBufferTransitionBarrier.Offset			= 0;
					initialVertexBufferTransitionBarrier.SizeInBytes	= pDrawArg->pVertexBuffer->GetDesc().SizeInBytes;
					intialBarriers.PushBack(initialVertexBufferTransitionBarrier);
				}

				// Instance Buffer
				{
					VALIDATE(pDrawArg->pInstanceBuffer);

					PipelineBufferBarrierDesc initialInstanceBufferTransitionBarrier = drawArgsArgsIt->second.InitialTransitionBarrierTemplate;
					initialInstanceBufferTransitionBarrier.pBuffer		= pDrawArg->pInstanceBuffer;
					initialInstanceBufferTransitionBarrier.Offset		= 0;
					initialInstanceBufferTransitionBarrier.SizeInBytes	= pDrawArg->pInstanceBuffer->GetDesc().SizeInBytes;
					intialBarriers.PushBack(initialInstanceBufferTransitionBarrier);
				}

				// Index Buffer
				{
					VALIDATE(pDrawArg->pIndexBuffer);

					PipelineBufferBarrierDesc initialIndexBufferTransitionBarrier = drawArgsArgsIt->second.InitialTransitionBarrierTemplate;
					initialIndexBufferTransitionBarrier.pBuffer		= pDrawArg->pIndexBuffer;
					initialIndexBufferTransitionBarrier.Offset		= 0;
					initialIndexBufferTransitionBarrier.SizeInBytes	= pDrawArg->pIndexBuffer->GetDesc().SizeInBytes;
					intialBarriers.PushBack(initialIndexBufferTransitionBarrier);
				}

				// If meshlet buffer is nullptr we assume that we are not using meshshaders
				if (pDrawArg->pMeshletBuffer)
				{
					// Meshlet Buffer
					{
						PipelineBufferBarrierDesc initialMeshletBufferTransitionBarrier = drawArgsArgsIt->second.InitialTransitionBarrierTemplate;
						initialMeshletBufferTransitionBarrier.pBuffer		= pDrawArg->pMeshletBuffer;
						initialMeshletBufferTransitionBarrier.Offset		= 0;
						initialMeshletBufferTransitionBarrier.SizeInBytes	= pDrawArg->pMeshletBuffer->GetDesc().SizeInBytes;
						intialBarriers.PushBack(initialMeshletBufferTransitionBarrier);
					}

					// Unique Indices Buffer
					{
						VALIDATE(pDrawArg->pUniqueIndicesBuffer);

						PipelineBufferBarrierDesc initialUniqueIndicesBufferTransitionBarrier = drawArgsArgsIt->second.InitialTransitionBarrierTemplate;
						initialUniqueIndicesBufferTransitionBarrier.pBuffer		= pDrawArg->pUniqueIndicesBuffer;
						initialUniqueIndicesBufferTransitionBarrier.Offset		= 0;
						initialUniqueIndicesBufferTransitionBarrier.SizeInBytes = pDrawArg->pUniqueIndicesBuffer->GetDesc().SizeInBytes;
						intialBarriers.PushBack(initialUniqueIndicesBufferTransitionBarrier);
					}

					// Primitive Indices Buffer
					{
						VALIDATE(pDrawArg->pPrimitiveIndices);

						PipelineBufferBarrierDesc initialPrimitiveIndicesBufferTransitionBarrier = drawArgsArgsIt->second.InitialTransitionBarrierTemplate;
						initialPrimitiveIndicesBufferTransitionBarrier.pBuffer		= pDrawArg->pPrimitiveIndices;
						initialPrimitiveIndicesBufferTransitionBarrier.Offset		= 0;
						initialPrimitiveIndicesBufferTransitionBarrier.SizeInBytes	= pDrawArg->pPrimitiveIndices->GetDesc().SizeInBytes;
						intialBarriers.PushBack(initialPrimitiveIndicesBufferTransitionBarrier);
					}
				}

				// Draw arg extensions
				if (pDrawArg->HasExtensions)
				{
					PipelineTextureBarrierDesc initialMaskTexturesTransitionBarrier = drawArgsArgsIt->second.InitialTextureTransitionBarrierTemplate;
					uint32 numExtensionGroups = pDrawArg->InstanceCount;
					for (uint32 i = 0; i < numExtensionGroups; i++)
					{
						DrawArgExtensionGroup* extensionGroup = pDrawArg->ppExtensionGroups[i];
						if (extensionGroup)
						{
							uint32 extensionCount = extensionGroup->ExtensionCount;
							for (uint32 e = 0; e < extensionCount; e++)
							{
								DrawArgExtensionData& extension = extensionGroup->pExtensions[e];
								uint32 numTextures = extension.TextureCount;
								for (uint32 t = 0; t < numTextures; t++)
								{
									const TextureDesc& textureDesc = extension.ppTextures[t]->GetDesc();
									initialMaskTexturesTransitionBarrier.pTexture = extension.ppTextures[t];
									initialMaskTexturesTransitionBarrier.TextureFlags = textureDesc.Flags;
									initialMaskTexturesTransitionBarrier.ArrayCount = textureDesc.ArrayCount;
									initialMaskTexturesTransitionBarrier.ArrayIndex = 0;
									initialMaskTexturesTransitionBarrier.Miplevel = 0;
									initialMaskTexturesTransitionBarrier.MiplevelCount = textureDesc.Miplevels;
									intialTextureBarriers.PushBack(initialMaskTexturesTransitionBarrier);
								}
							}
						}
					}
				}
			}

			// Transfer to Initial State for buffer barriers
			if (!intialBarriers.IsEmpty())
			{
				FPipelineStageFlags srcPipelineStage = pResource->LastPipelineStageOfFirstRenderStage;
				FPipelineStageFlags dstPipelineStage = pResource->LastPipelineStageOfFirstRenderStage;

				if (intialBarriers[0].QueueAfter == ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS)
				{
					CommandList* pCommandList = m_ppGraphicsCopyCommandLists[m_ModFrameIndex];

					if (!pCommandList->IsBegin())
					{
						m_ppGraphicsCopyCommandAllocators[m_ModFrameIndex]->Reset();
						pCommandList->Begin(nullptr);
					}

					uint32 remaining = intialBarriers.GetSize() % MAX_BUFFER_BARRIERS;
					uint32 i = 0;
					for(; i < floor(intialBarriers.GetSize()/ MAX_BUFFER_BARRIERS); i++)
						pCommandList->PipelineBufferBarriers(srcPipelineStage, dstPipelineStage, &intialBarriers[i*MAX_BUFFER_BARRIERS], MAX_BUFFER_BARRIERS);
					if(remaining != 0)
						pCommandList->PipelineBufferBarriers(srcPipelineStage, dstPipelineStage, &intialBarriers[i*MAX_BUFFER_BARRIERS], remaining);
				}
				else if (intialBarriers[0].QueueAfter == ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE)
				{
					CommandList* pCommandList = m_ppComputeCopyCommandLists[m_ModFrameIndex];

					if (!pCommandList->IsBegin())
					{
						m_ppComputeCopyCommandAllocators[m_ModFrameIndex]->Reset();
						pCommandList->Begin(nullptr);
					}

					uint32 remaining = intialBarriers.GetSize() % MAX_BUFFER_BARRIERS;
					uint32 i = 0;
					for (; i < floor(intialBarriers.GetSize() / MAX_BUFFER_BARRIERS); i++)
						pCommandList->PipelineBufferBarriers(srcPipelineStage, dstPipelineStage, &intialBarriers[i * MAX_BUFFER_BARRIERS], MAX_BUFFER_BARRIERS);
					if (remaining != 0)
						pCommandList->PipelineBufferBarriers(srcPipelineStage, dstPipelineStage, &intialBarriers[i * MAX_BUFFER_BARRIERS], remaining);
				}
			}

			// Transfer to Initial State for texture barriers
			if (!intialTextureBarriers.IsEmpty())
			{
				FPipelineStageFlags srcPipelineStage = pResource->LastPipelineStageOfFirstRenderStage;
				FPipelineStageFlags dstPipelineStage = pResource->LastPipelineStageOfFirstRenderStage;

				if (intialTextureBarriers[0].QueueAfter == ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS)
				{
					CommandList* pCommandList = m_ppGraphicsCopyCommandLists[m_ModFrameIndex];

					if (!pCommandList->IsBegin())
					{
						m_ppGraphicsCopyCommandAllocators[m_ModFrameIndex]->Reset();
						pCommandList->Begin(nullptr);
					}

					PipelineTextureBarriers(pCommandList, intialTextureBarriers, srcPipelineStage, dstPipelineStage);
				}
				else if (intialTextureBarriers[0].QueueAfter == ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE)
				{
					CommandList* pCommandList = m_ppComputeCopyCommandLists[m_ModFrameIndex];

					if (!pCommandList->IsBegin())
					{
						m_ppComputeCopyCommandAllocators[m_ModFrameIndex]->Reset();
						pCommandList->Begin(nullptr);
					}

					PipelineTextureBarriers(pCommandList, intialTextureBarriers, srcPipelineStage, dstPipelineStage);
				}
			}

			m_DirtyBoundDrawArgResources.insert(pResource);
		}
		else
		{
			LOG_WARNING("[RenderGraph]: Update DrawArgs called for unused IncludeDrawArgsMask %08x & ExcludeDrawArgsMask %08x", pDesc->ExternalDrawArgsUpdate.DrawArgsMaskDesc.IncludeMask, pDesc->ExternalDrawArgsUpdate.DrawArgsMaskDesc.ExcludeMask);
		}
	}

	void RenderGraph::UpdateResourceBuffer(Resource* pResource, const ResourceUpdateDesc* pDesc)
	{
		uint32 actualSubResourceCount = 0;
		if (pResource->BackBufferBound)
		{
			actualSubResourceCount = m_BackBufferCount;
		}
		else
		{
			actualSubResourceCount = pResource->SubResourceCount;
		}

		VALIDATE(actualSubResourceCount == pDesc->ExternalBufferUpdate.Count);

		//Update Buffer
		for (uint32 sr = 0; sr < actualSubResourceCount; sr++)
		{
			Buffer** ppBuffer		= &pResource->Buffer.Buffers[sr];
			uint64* pOffset			= &pResource->Buffer.Offsets[sr];
			uint64* pSizeInBytes	= &pResource->Buffer.SizesInBytes[sr];

			Buffer* pBuffer = nullptr;

			if (pResource->OwnershipType == EResourceOwnershipType::INTERNAL)
			{
				SAFERELEASE(*ppBuffer);
				pBuffer = m_pGraphicsDevice->CreateBuffer(pDesc->InternalBufferUpdate.pBufferDesc);
			}
			else if (pResource->OwnershipType == EResourceOwnershipType::EXTERNAL)
			{
				pBuffer = pDesc->ExternalBufferUpdate.ppBuffer[sr];
			}
			else
			{
				LOG_ERROR("[RenderGraph]: UpdateResourceBuffer called for Resource with unknown OwnershipType, \"%s\"", pResource->Name.c_str());
				return;
			}

			const BufferDesc& bufferDesc = pBuffer->GetDesc();

			//Todo: This should not be here
			uint64 offset = 0;

			(*ppBuffer)		= pBuffer;
			(*pSizeInBytes) = bufferDesc.SizeInBytes;
			(*pOffset)		= offset;

			for (uint32 b = sr; b < pResource->BarriersPerSynchronizationStage.GetSize(); b += pResource->SubResourceCount)
			{
				const ResourceBarrierInfo* pBarrierInfo = &pResource->BarriersPerSynchronizationStage[b];
				SynchronizationStage* pSynchronizationStage = &m_pSynchronizationStages[pBarrierInfo->SynchronizationStageIndex];

				PipelineBufferBarrierDesc* pBufferBarrier = &pSynchronizationStage->BufferBarriers[pBarrierInfo->SynchronizationTypeIndex][pBarrierInfo->BarrierIndex];

				pBufferBarrier->pBuffer		= pBuffer;
				pBufferBarrier->SizeInBytes = bufferDesc.SizeInBytes;
				pBufferBarrier->Offset		= offset;
			}

			//Transfer to Initial State
			if (pResource->Buffer.InitialTransitionBarrier.QueueBefore != ECommandQueueType::COMMAND_QUEUE_TYPE_UNKNOWN)
			{
				PipelineBufferBarrierDesc& initialBarrier = pResource->Buffer.InitialTransitionBarrier;

				initialBarrier.pBuffer		= pBuffer;
				initialBarrier.Offset		= offset;
				initialBarrier.SizeInBytes	= bufferDesc.SizeInBytes;

				FPipelineStageFlags srcPipelineStage = pResource->LastPipelineStageOfFirstRenderStage;
				FPipelineStageFlags dstPipelineStage = pResource->LastPipelineStageOfFirstRenderStage;

				if (initialBarrier.QueueAfter == ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS)
				{
					CommandList* pCommandList = m_ppGraphicsCopyCommandLists[m_ModFrameIndex];

					if (!pCommandList->IsBegin())
					{
						m_ppGraphicsCopyCommandAllocators[m_ModFrameIndex]->Reset();
						pCommandList->Begin(nullptr);
					}

					pCommandList->PipelineBufferBarriers(srcPipelineStage, dstPipelineStage, &initialBarrier, 1);
				}
				else if (initialBarrier.QueueAfter == ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE)
				{
					CommandList* pCommandList = m_ppComputeCopyCommandLists[m_ModFrameIndex];

					if (!pCommandList->IsBegin())
					{
						m_ppComputeCopyCommandAllocators[m_ModFrameIndex]->Reset();
						pCommandList->Begin(nullptr);
					}

					pCommandList->PipelineBufferBarriers(srcPipelineStage, dstPipelineStage, &initialBarrier, 1);
				}
			}
		}

		if (pResource->ResourceBindings.GetSize() > 0)
		{
			m_DirtyBoundBufferResources.insert(pResource);

			for (const ResourceBinding& binding : pResource->ResourceBindings)
			{
				m_DirtyRenderStageBufferSets.insert(binding.pRenderStage);
			}
		}
	}

	void RenderGraph::UpdateResourceAccelerationStructure(Resource* pResource, const ResourceUpdateDesc* pDesc)
	{
		//Update Acceleration Structure
		pResource->AccelerationStructure.pTLAS = pDesc->ExternalAccelerationStructure.pTLAS;

		m_DirtyBoundAccelerationStructureResources.insert(pResource);

		for (const ResourceBinding& binding : pResource->ResourceBindings)
		{
			m_DirtyRenderStageBufferSets.insert(binding.pRenderStage);
		}
	}

	void RenderGraph::UpdateRelativeRenderStageDimensions(RenderStage* pRenderStage)
	{
		if (pRenderStage->Parameters.XDimType == ERenderGraphDimensionType::RELATIVE_1D)
		{
			pRenderStage->Dimensions.x = uint32(pRenderStage->Parameters.XDimVariable * m_WindowWidth * m_WindowHeight);
		}
		else
		{
			if (pRenderStage->Parameters.XDimType == ERenderGraphDimensionType::RELATIVE)
			{
				pRenderStage->Dimensions.x = uint32(pRenderStage->Parameters.XDimVariable * m_WindowWidth);
			}

			if (pRenderStage->Parameters.YDimType == ERenderGraphDimensionType::RELATIVE)
			{
				pRenderStage->Dimensions.y = uint32(pRenderStage->Parameters.YDimVariable * m_WindowHeight);
			}
		}
	}

	void RenderGraph::UpdateRelativeResourceDimensions(InternalResourceUpdateDesc* pResourceUpdateDesc)
	{
		switch (pResourceUpdateDesc->Type)
		{
			case ERenderGraphResourceType::TEXTURE:
			{
				if (pResourceUpdateDesc->TextureUpdate.XDimType == ERenderGraphDimensionType::RELATIVE)
				{
					pResourceUpdateDesc->TextureUpdate.TextureDesc.Width = uint32(pResourceUpdateDesc->TextureUpdate.XDimVariable * m_WindowWidth);
					m_DirtyInternalResources.insert(pResourceUpdateDesc->ResourceName);
				}

				if (pResourceUpdateDesc->TextureUpdate.YDimType == ERenderGraphDimensionType::RELATIVE)
				{
					pResourceUpdateDesc->TextureUpdate.TextureDesc.Height = uint32(pResourceUpdateDesc->TextureUpdate.YDimVariable * m_WindowHeight);
					m_DirtyInternalResources.insert(pResourceUpdateDesc->ResourceName);
				}

				break;
			}
			default:
			{
				LOG_WARNING("[RenderGraph]: Resource \"%s\" in Render Graph has unsupported Type for relative dimensions", pResourceUpdateDesc->ResourceName.c_str());
				return;
			}
		}
	}

	void RenderGraph::ExecuteSynchronizationStage(
		SynchronizationStage*	pSynchronizationStage,
		CommandAllocator*		pGraphicsCommandAllocator,
		CommandList*			pGraphicsCommandList,
		CommandAllocator*		pComputeCommandAllocator,
		CommandList*			pComputeCommandList,
		CommandList**			ppFirstExecutionStage,
		CommandList**			ppSecondExecutionStage)
	{
		Profiler::GetGPUProfiler()->GetTimestamp(pGraphicsCommandList);
		pGraphicsCommandAllocator->Reset();
		pGraphicsCommandList->Begin(nullptr);
		Profiler::GetGPUProfiler()->ResetTimestamp(pGraphicsCommandList);
		Profiler::GetGPUProfiler()->StartTimestamp(pGraphicsCommandList);

		Profiler::GetGPUProfiler()->GetTimestamp(pComputeCommandList);
		pComputeCommandAllocator->Reset();
		pComputeCommandList->Begin(nullptr);
		Profiler::GetGPUProfiler()->ResetTimestamp(pComputeCommandList);
		Profiler::GetGPUProfiler()->StartTimestamp(pComputeCommandList);

		CommandList* pFirstExecutionCommandList = nullptr;
		CommandList* pSecondExecutionCommandList = nullptr;

		if (pSynchronizationStage->ExecutionQueue == ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS)
		{
			(*ppFirstExecutionStage) = pGraphicsCommandList;
			pFirstExecutionCommandList = pGraphicsCommandList;
			pSecondExecutionCommandList = pComputeCommandList;
		}
		else if (pSynchronizationStage->ExecutionQueue == ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE)
		{
			(*ppFirstExecutionStage) = pComputeCommandList;
			pFirstExecutionCommandList = pComputeCommandList;
			pSecondExecutionCommandList = pGraphicsCommandList;
		}

		//Texture Synchronizations
		{
			const TArray<PipelineTextureBarrierDesc>& sameQueueBackBufferBarriers	= pSynchronizationStage->TextureBarriers[SAME_QUEUE_BACK_BUFFER_BOUND_SYNCHRONIZATION_INDEX];
			const TArray<PipelineTextureBarrierDesc>& sameQueueTextureBarriers		= pSynchronizationStage->TextureBarriers[SAME_QUEUE_TEXTURE_SYNCHRONIZATION_INDEX];
			const TArray<PipelineTextureBarrierDesc>& otherQueueBackBufferBarriers	= pSynchronizationStage->TextureBarriers[OTHER_QUEUE_BACK_BUFFER_BOUND_SYNCHRONIZATION_INDEX];
			const TArray<PipelineTextureBarrierDesc>& otherQueueTextureBarriers		= pSynchronizationStage->TextureBarriers[OTHER_QUEUE_TEXTURE_SYNCHRONIZATION_INDEX];

			if (sameQueueBackBufferBarriers.GetSize() > 0)
			{
				pFirstExecutionCommandList->PipelineTextureBarriers(pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->SameQueueDstPipelineStage, &otherQueueBackBufferBarriers[m_BackBufferIndex], 1);
			}

			if (sameQueueTextureBarriers.GetSize() > 0)
			{
				pFirstExecutionCommandList->PipelineTextureBarriers(pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->SameQueueDstPipelineStage, sameQueueTextureBarriers.GetData(), sameQueueTextureBarriers.GetSize());
			}

			if (otherQueueBackBufferBarriers.GetSize() > 0)
			{
				const PipelineTextureBarrierDesc* pTextureBarrier = &otherQueueBackBufferBarriers[m_BackBufferIndex];
				pFirstExecutionCommandList->PipelineTextureBarriers(pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->OtherQueueDstPipelineStage, pTextureBarrier, 1);
				pSecondExecutionCommandList->PipelineTextureBarriers(pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->OtherQueueDstPipelineStage, pTextureBarrier, 1);
				(*ppSecondExecutionStage) = pSecondExecutionCommandList;
			}

			if (otherQueueTextureBarriers.GetSize() > 0)
			{
				pFirstExecutionCommandList->PipelineTextureBarriers(pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->OtherQueueDstPipelineStage, otherQueueTextureBarriers.GetData(), otherQueueTextureBarriers.GetSize());
				pSecondExecutionCommandList->PipelineTextureBarriers(pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->OtherQueueDstPipelineStage, otherQueueTextureBarriers.GetData(), otherQueueTextureBarriers.GetSize());
				(*ppSecondExecutionStage) = pSecondExecutionCommandList;
			}

			const TArray<TArray<PipelineTextureBarrierDesc>>& sameQueueUnboundedTextureBarrierArrays	= pSynchronizationStage->UnboundedTextureBarriers[SAME_QUEUE_UNBOUNDED_TEXTURE_SYNCHRONIZATION_INDEX];
			const TArray<TArray<PipelineTextureBarrierDesc>>& otherQueueUnboundedTextureBarrierArrays	= pSynchronizationStage->UnboundedTextureBarriers[OTHER_QUEUE_UNBOUNDED_TEXTURE_SYNCHRONIZATION_INDEX];

			for (const TArray<PipelineTextureBarrierDesc>& sameQueueUnboundedTextureBarriers : sameQueueUnboundedTextureBarrierArrays)
			{
				UNREFERENCED_VARIABLE(sameQueueUnboundedTextureBarriers);

				if (sameQueueTextureBarriers.GetSize() > 0)
				{
					pFirstExecutionCommandList->PipelineTextureBarriers(pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->SameQueueDstPipelineStage, sameQueueTextureBarriers.GetData(), sameQueueTextureBarriers.GetSize());
				}
			}

			for (const TArray<PipelineTextureBarrierDesc>& otherQueueUnboundedTextureBarriers : otherQueueUnboundedTextureBarrierArrays)
			{
				UNREFERENCED_VARIABLE(otherQueueUnboundedTextureBarriers);

				if (otherQueueTextureBarriers.GetSize() > 0)
				{
					pFirstExecutionCommandList->PipelineTextureBarriers(pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->SameQueueDstPipelineStage, otherQueueTextureBarriers.GetData(), otherQueueTextureBarriers.GetSize());
					pSecondExecutionCommandList->PipelineTextureBarriers(pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->OtherQueueDstPipelineStage, otherQueueTextureBarriers.GetData(), otherQueueTextureBarriers.GetSize());
				}

				(*ppSecondExecutionStage) = pSecondExecutionCommandList;
			}
		}

		// Draw Texture Synchronizations
		{
			const TArray<PipelineTextureBarrierDesc>& sameQueueDrawTextureBarriers = pSynchronizationStage->DrawTextureBarriers[0]; // SAME_QUEUE_TEXTURE_SYNCHRONIZATION_INDEX
			const TArray<PipelineTextureBarrierDesc>& otherQueueDrawTextureBarriers = pSynchronizationStage->DrawTextureBarriers[1]; // OTHER_QUEUE_TEXTURE_SYNCHRONIZATION_INDEX

			if (sameQueueDrawTextureBarriers.GetSize() > 0 && sameQueueDrawTextureBarriers[0].pTexture != nullptr)
			{
				PipelineTextureBarriers(pFirstExecutionCommandList, sameQueueDrawTextureBarriers, pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->SameQueueDstPipelineStage);
			}

			if (otherQueueDrawTextureBarriers.GetSize() > 0 && otherQueueDrawTextureBarriers[0].pTexture != nullptr)
			{
				PipelineTextureBarriers(pFirstExecutionCommandList, otherQueueDrawTextureBarriers, pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->OtherQueueDstPipelineStage);
				PipelineTextureBarriers(pSecondExecutionCommandList, otherQueueDrawTextureBarriers, pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->OtherQueueDstPipelineStage);
				(*ppSecondExecutionStage) = pSecondExecutionCommandList; // Can I do this like normal textures?
			}
		}

		//Draw Buffer Synchronization
		{
			const TArray<PipelineBufferBarrierDesc>& sameQueueDrawBufferBarriers		= pSynchronizationStage->DrawBufferBarriers[SAME_QUEUE_BUFFER_SYNCHRONIZATION_INDEX];
			const TArray<PipelineBufferBarrierDesc>& otherQueueDrawBufferBarriers		= pSynchronizationStage->DrawBufferBarriers[OTHER_QUEUE_BUFFER_SYNCHRONIZATION_INDEX];

			if (sameQueueDrawBufferBarriers.GetSize() > 0 && sameQueueDrawBufferBarriers[0].pBuffer != nullptr)
			{
				PipelineBufferBarriers(pFirstExecutionCommandList, sameQueueDrawBufferBarriers, pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->SameQueueDstPipelineStage);
			}

			if (otherQueueDrawBufferBarriers.GetSize() > 0 && otherQueueDrawBufferBarriers[0].pBuffer != nullptr)
			{
				PipelineBufferBarriers(pFirstExecutionCommandList, otherQueueDrawBufferBarriers, pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->OtherQueueDstPipelineStage);
				PipelineBufferBarriers(pSecondExecutionCommandList, otherQueueDrawBufferBarriers, pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->OtherQueueDstPipelineStage);
				(*ppSecondExecutionStage) = pSecondExecutionCommandList;
			}
		}

		//Buffer Synchronization
		{
			const TArray<PipelineBufferBarrierDesc>& sameQueueBufferBarriers		= pSynchronizationStage->BufferBarriers[SAME_QUEUE_BUFFER_SYNCHRONIZATION_INDEX];
			const TArray<PipelineBufferBarrierDesc>& otherQueueBufferBarriers		= pSynchronizationStage->BufferBarriers[OTHER_QUEUE_BUFFER_SYNCHRONIZATION_INDEX];

			if (sameQueueBufferBarriers.GetSize() > 0)
			{
				PipelineBufferBarriers(pFirstExecutionCommandList, sameQueueBufferBarriers, pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->SameQueueDstPipelineStage);
			}

			if (otherQueueBufferBarriers.GetSize() > 0)
			{
				PipelineBufferBarriers(pFirstExecutionCommandList, otherQueueBufferBarriers, pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->OtherQueueDstPipelineStage);
				PipelineBufferBarriers(pSecondExecutionCommandList, otherQueueBufferBarriers, pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->OtherQueueDstPipelineStage);
				(*ppSecondExecutionStage) = pSecondExecutionCommandList;
			}
		}

		Profiler::GetGPUProfiler()->EndTimestamp(pGraphicsCommandList);
		pGraphicsCommandList->End();
		Profiler::GetGPUProfiler()->EndTimestamp(pComputeCommandList);
		pComputeCommandList->End();
		//if (pSynchronizationStage->ExecutionQueue == ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS)
		//{
		//}
		//else if (pSynchronizationStage->ExecutionQueue == ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE)
		//{
		//}

	}

	void RenderGraph::ExecuteGraphicsRenderStage(
		RenderStage*		pRenderStage,
		CommandAllocator*	pGraphicsCommandAllocator,
		CommandList*		pGraphicsCommandList,
		CommandList**		ppExecutionStage)
	{
		if ((pRenderStage->FrameCounter != pRenderStage->FrameOffset || pRenderStage->Sleeping) && pRenderStage->pDisabledRenderPass == nullptr )
			return;

		Profiler::GetGPUProfiler()->GetTimestamp(pGraphicsCommandList);
		Profiler::GetGPUProfiler()->GetGraphicsPipelineStat();
		pGraphicsCommandAllocator->Reset();
		pGraphicsCommandList->Begin(nullptr);
		Profiler::GetGPUProfiler()->ResetGraphicsPipelineStat(pGraphicsCommandList);
		Profiler::GetGPUProfiler()->ResetTimestamp(pGraphicsCommandList);
		Profiler::GetGPUProfiler()->StartGraphicsPipelineStat(pGraphicsCommandList);
		Profiler::GetGPUProfiler()->StartTimestamp(pGraphicsCommandList);

		Viewport viewport = { };
		viewport.MinDepth	= 0.0f;
		viewport.MaxDepth	= 1.0f;
		viewport.Width		= (float)pRenderStage->Dimensions.x;
		viewport.Height		= -(float)pRenderStage->Dimensions.y;
		viewport.x			= 0.0f;
		viewport.y			= (float)pRenderStage->Dimensions.y;

		pGraphicsCommandList->SetViewports(&viewport, 0, 1);

		ScissorRect scissorRect = { };
		scissorRect.Width	= pRenderStage->Dimensions.x;
		scissorRect.Height	= pRenderStage->Dimensions.y;
		scissorRect.x		= 0;
		scissorRect.y		= 0;

		pGraphicsCommandList->SetScissorRects(&scissorRect, 0, 1);

		pGraphicsCommandList->BindGraphicsPipeline(pRenderStage->pPipelineState);

		if (pRenderStage->ExternalPushConstants.DataSize > 0)
			pGraphicsCommandList->SetConstantRange(pRenderStage->pPipelineLayout, pRenderStage->PipelineStageMask, pRenderStage->ExternalPushConstants.pData, pRenderStage->ExternalPushConstants.DataSize, pRenderStage->ExternalPushConstants.Offset);

		if (pRenderStage->ppBufferDescriptorSets != nullptr)
			pGraphicsCommandList->BindDescriptorSetGraphics(pRenderStage->ppBufferDescriptorSets[m_BackBufferIndex], pRenderStage->pPipelineLayout, pRenderStage->BufferSetIndex);

		if (pRenderStage->ppTextureDescriptorSets != nullptr)
			pGraphicsCommandList->BindDescriptorSetGraphics(pRenderStage->ppTextureDescriptorSets[m_BackBufferIndex], pRenderStage->pPipelineLayout, pRenderStage->TextureSetIndex);

		uint32 frameBufferWidth		= 0;
		uint32 frameBufferHeight	= 0;

		DescriptorSet** ppDrawArgsDescriptorSetsPerFrame = nullptr;
		DescriptorSet** ppDrawArgsExtensionsDescriptorSetsPerFrame = nullptr;
		if (pRenderStage->DrawType == ERenderStageDrawType::SCENE_INSTANCES || pRenderStage->DrawType == ERenderStageDrawType::SCENE_INSTANCES_MESH_SHADER)
		{
			ppDrawArgsDescriptorSetsPerFrame = pRenderStage->pppDrawArgDescriptorSets[m_ModFrameIndex];

			if(pRenderStage->pppDrawArgExtensionsDescriptorSets)
				ppDrawArgsExtensionsDescriptorSetsPerFrame = pRenderStage->pppDrawArgExtensionsDescriptorSets[m_ModFrameIndex];
		}

		for (uint32 r = 0; r < pRenderStage->ExecutionCount; r++)
		{
			TextureView* ppTextureViews[MAX_COLOR_ATTACHMENTS];
			TextureView* pDepthStencilTextureView = nullptr;
			ClearColorDesc clearColorDescriptions[MAX_COLOR_ATTACHMENTS + 1];

			uint32 textureViewCount = 0;
			uint32 clearColorCount = 0;
			for (Resource* pResource : pRenderStage->RenderTargetResources)
			{
				TextureView* pRenderTarget = nullptr;

				if (pResource->BackBufferBound)
				{
					pRenderTarget = pResource->Texture.PerSubImageTextureViews[m_BackBufferIndex * (pResource->Texture.PerSubImageTextureViews.GetSize() / m_BackBufferCount)];
				}
				else
				{
					bool indexed = pResource->Texture.PerSubImageTextureViews.GetSize() > 1;

					pRenderTarget = indexed ? pResource->Texture.PerSubImageTextureViews[r] : pResource->Texture.PerSubImageTextureViews[0];
				}

				const TextureDesc& renderTargetDesc = pRenderTarget->GetTexture()->GetDesc();
				frameBufferWidth	= renderTargetDesc.Width;
				frameBufferHeight	= renderTargetDesc.Height;

				ppTextureViews[textureViewCount++] = pRenderTarget;

				ClearColorDesc* pClearColorDesc = &clearColorDescriptions[clearColorCount];
				pClearColorDesc->Color[0] = 0.0f;
				pClearColorDesc->Color[1] = 0.0f;
				pClearColorDesc->Color[2] = 0.0f;
				pClearColorDesc->Color[3] = 0.0f;

				clearColorCount++;
			}

			if (pRenderStage->pDepthStencilAttachment != nullptr)
			{
				if (pRenderStage->pDepthStencilAttachment->BackBufferBound)
				{
					pDepthStencilTextureView = pRenderStage->pDepthStencilAttachment->Texture.PerSubImageTextureViews[m_BackBufferIndex * (pRenderStage->pDepthStencilAttachment->Texture.PerSubImageTextureViews.GetSize() / m_BackBufferCount)];
				}
				else
				{
					bool indexed = pRenderStage->pDepthStencilAttachment->Texture.PerSubImageTextureViews.GetSize() > 1;

					pDepthStencilTextureView = indexed ? pRenderStage->pDepthStencilAttachment->Texture.PerSubImageTextureViews[r] : pRenderStage->pDepthStencilAttachment->Texture.PerSubImageTextureViews[0];
				}

				const TextureDesc& depthStencilDesc = pDepthStencilTextureView->GetTexture()->GetDesc();
				frameBufferWidth	= depthStencilDesc.Width;
				frameBufferHeight	= depthStencilDesc.Height;

				ClearColorDesc* pClearDepthStencilDesc = &clearColorDescriptions[clearColorCount];
				pClearDepthStencilDesc->Depth		= 1.0f;
				pClearDepthStencilDesc->Stencil		= 0;

				clearColorCount++;
			}

			if (pRenderStage->FrameCounter == pRenderStage->FrameOffset && !pRenderStage->Sleeping)
			{
				BeginRenderPassDesc beginRenderPassDesc = { };
				beginRenderPassDesc.pRenderPass			= pRenderStage->pRenderPass;
				beginRenderPassDesc.ppRenderTargets		= ppTextureViews;
				beginRenderPassDesc.RenderTargetCount	= textureViewCount;
				beginRenderPassDesc.pDepthStencil		= pDepthStencilTextureView;
				beginRenderPassDesc.Width				= frameBufferWidth;
				beginRenderPassDesc.Height				= frameBufferHeight;
				beginRenderPassDesc.Flags				= FRenderPassBeginFlag::RENDER_PASS_BEGIN_FLAG_INLINE;
				beginRenderPassDesc.pClearColors		= clearColorDescriptions;
				beginRenderPassDesc.ClearColorCount		= clearColorCount;
				beginRenderPassDesc.Offset.x			= 0;
				beginRenderPassDesc.Offset.y			= 0;

				pGraphicsCommandList->BeginRenderPass(&beginRenderPassDesc);

				PushConstants* pDrawIterationPushConstants = &pRenderStage->pInternalPushConstants[DRAW_ITERATION_PUSH_CONSTANTS_INDEX];
				if (pDrawIterationPushConstants->MaxDataSize == sizeof(uint32))
				{
					memcpy(pDrawIterationPushConstants->pData, &r, sizeof(uint32));
					pGraphicsCommandList->SetConstantRange(pRenderStage->pPipelineLayout, pRenderStage->PipelineStageMask, pDrawIterationPushConstants->pData, pDrawIterationPushConstants->DataSize, pDrawIterationPushConstants->Offset);
				}

				if (pRenderStage->DrawType == ERenderStageDrawType::SCENE_INSTANCES)
				{
					for (uint32 d = 0; d < pRenderStage->NumDrawArgsPerFrame; d++)
					{
						const DrawArg& drawArg = pRenderStage->pDrawArgs[d];
						pGraphicsCommandList->BindIndexBuffer(drawArg.pIndexBuffer, 0, EIndexType::INDEX_TYPE_UINT32);

						if (ppDrawArgsDescriptorSetsPerFrame)
						{
							pGraphicsCommandList->BindDescriptorSetGraphics(ppDrawArgsDescriptorSetsPerFrame[d], pRenderStage->pPipelineLayout, pRenderStage->DrawSetIndex);

							if (ppDrawArgsExtensionsDescriptorSetsPerFrame && ppDrawArgsExtensionsDescriptorSetsPerFrame[d])
							{
								pGraphicsCommandList->BindDescriptorSetGraphics(ppDrawArgsExtensionsDescriptorSetsPerFrame[d], pRenderStage->pPipelineLayout, pRenderStage->DrawExtensionSetIndex);
							}
						}

						pGraphicsCommandList->DrawIndexInstanced(drawArg.IndexCount, drawArg.InstanceCount, 0, 0, 0);
					}
				}
				else if (pRenderStage->DrawType == ERenderStageDrawType::SCENE_INSTANCES_MESH_SHADER)
				{
					for (uint32 d = 0; d < pRenderStage->NumDrawArgsPerFrame; d++)
					{
						const DrawArg& drawArg = pRenderStage->pDrawArgs[d];
						if (ppDrawArgsDescriptorSetsPerFrame)
						{
							pGraphicsCommandList->BindDescriptorSetGraphics(ppDrawArgsDescriptorSetsPerFrame[d], pRenderStage->pPipelineLayout, pRenderStage->DrawSetIndex);

							if (ppDrawArgsExtensionsDescriptorSetsPerFrame && ppDrawArgsExtensionsDescriptorSetsPerFrame[d])
							{
								pGraphicsCommandList->BindDescriptorSetGraphics(ppDrawArgsExtensionsDescriptorSetsPerFrame[d], pRenderStage->pPipelineLayout, pRenderStage->DrawExtensionSetIndex);
							}
						}

						const uint32 maxTaskCount = m_Features.MaxDrawMeshTasksCount;
						const uint32 totalMeshletCount = drawArg.MeshletCount * drawArg.InstanceCount;
						if (totalMeshletCount > maxTaskCount)
						{
							int32 meshletsLeft	= static_cast<int32>(totalMeshletCount);
							int32 meshletOffset = 0;
							while (meshletsLeft > 0)
							{
								int32 meshletCount = std::min<int32>(maxTaskCount, meshletsLeft);
								pGraphicsCommandList->DispatchMesh(meshletCount, meshletOffset);

								meshletOffset += meshletCount;
								meshletsLeft -= meshletCount;
							}
						}
						else
						{
							pGraphicsCommandList->DispatchMesh(totalMeshletCount, 0);
						}
					}
				}
				else if (pRenderStage->DrawType == ERenderStageDrawType::FULLSCREEN_QUAD)
				{
					pGraphicsCommandList->DrawInstanced(3, 1, 0, 0);
				}
				else if (pRenderStage->DrawType == ERenderStageDrawType::CUBE)
				{
					pGraphicsCommandList->DrawInstanced(36, 1, 0, 0);
				}
			}
			else
			{
				BeginRenderPassDesc beginRenderPassDesc = { };
				beginRenderPassDesc.pRenderPass			= pRenderStage->pDisabledRenderPass;
				beginRenderPassDesc.ppRenderTargets		= ppTextureViews;
				beginRenderPassDesc.RenderTargetCount	= textureViewCount;
				beginRenderPassDesc.pDepthStencil		= pDepthStencilTextureView;
				beginRenderPassDesc.Width				= frameBufferWidth;
				beginRenderPassDesc.Height				= frameBufferHeight;
				beginRenderPassDesc.Flags				= FRenderPassBeginFlag::RENDER_PASS_BEGIN_FLAG_INLINE;
				beginRenderPassDesc.pClearColors		= clearColorDescriptions;
				beginRenderPassDesc.ClearColorCount		= clearColorCount;
				beginRenderPassDesc.Offset.x			= 0;
				beginRenderPassDesc.Offset.y			= 0;

				pGraphicsCommandList->BeginRenderPass(&beginRenderPassDesc);
			}

			pGraphicsCommandList->EndRenderPass();
		}

		Profiler::GetGPUProfiler()->EndGraphicsPipelineStat(pGraphicsCommandList);
		Profiler::GetGPUProfiler()->EndTimestamp(pGraphicsCommandList);
		pGraphicsCommandList->End();

		(*ppExecutionStage) = pGraphicsCommandList;
	}

	void RenderGraph::ExecuteComputeRenderStage(
		RenderStage*		pRenderStage,
		CommandAllocator*	pComputeCommandAllocator,
		CommandList*		pComputeCommandList,
		CommandList**		ppExecutionStage)
	{
		if (pRenderStage->FrameCounter == pRenderStage->FrameOffset && !pRenderStage->Sleeping)
		{
			Profiler::GetGPUProfiler()->GetTimestamp(pComputeCommandList);
			pComputeCommandAllocator->Reset();
			pComputeCommandList->Begin(nullptr);
			Profiler::GetGPUProfiler()->ResetTimestamp(pComputeCommandList);
			Profiler::GetGPUProfiler()->StartTimestamp(pComputeCommandList);

			pComputeCommandList->BindComputePipeline(pRenderStage->pPipelineState);

			if (pRenderStage->ppBufferDescriptorSets != nullptr)
				pComputeCommandList->BindDescriptorSetCompute(pRenderStage->ppBufferDescriptorSets[m_BackBufferIndex], pRenderStage->pPipelineLayout, pRenderStage->BufferSetIndex);

			if (pRenderStage->ppTextureDescriptorSets != nullptr)
				pComputeCommandList->BindDescriptorSetCompute(pRenderStage->ppTextureDescriptorSets[m_BackBufferIndex], pRenderStage->pPipelineLayout, pRenderStage->TextureSetIndex);

			pComputeCommandList->Dispatch(pRenderStage->Dimensions.x, pRenderStage->Dimensions.y, pRenderStage->Dimensions.z);

			Profiler::GetGPUProfiler()->EndTimestamp(pComputeCommandList);
			pComputeCommandList->End();

			(*ppExecutionStage) = pComputeCommandList;
		}
	}

	void RenderGraph::ExecuteRayTracingRenderStage(
		RenderStage*		pRenderStage,
		CommandAllocator*	pComputeCommandAllocator,
		CommandList*		pComputeCommandList,
		CommandList**		ppExecutionStage)
	{
		if (pRenderStage->FrameCounter == pRenderStage->FrameOffset && !pRenderStage->Sleeping && pRenderStage->pSBT != nullptr)
		{
			Profiler::GetGPUProfiler()->GetTimestamp(pComputeCommandList);
			pComputeCommandAllocator->Reset();
			pComputeCommandList->Begin(nullptr);
			Profiler::GetGPUProfiler()->ResetTimestamp(pComputeCommandList);
			Profiler::GetGPUProfiler()->StartTimestamp(pComputeCommandList);

			pComputeCommandList->BindRayTracingPipeline(pRenderStage->pPipelineState);

			if (pRenderStage->ppBufferDescriptorSets != nullptr)
				pComputeCommandList->BindDescriptorSetRayTracing(pRenderStage->ppBufferDescriptorSets[m_BackBufferIndex], pRenderStage->pPipelineLayout, pRenderStage->BufferSetIndex);

			if (pRenderStage->ppTextureDescriptorSets != nullptr)
				pComputeCommandList->BindDescriptorSetRayTracing(pRenderStage->ppTextureDescriptorSets[m_BackBufferIndex], pRenderStage->pPipelineLayout, pRenderStage->TextureSetIndex);

			pComputeCommandList->TraceRays(pRenderStage->pSBT, pRenderStage->Dimensions.x, pRenderStage->Dimensions.y, pRenderStage->Dimensions.z);

			Profiler::GetGPUProfiler()->EndTimestamp(pComputeCommandList);
			pComputeCommandList->End();

			(*ppExecutionStage) = pComputeCommandList;
		}
	}

	void RenderGraph::PipelineTextureBarriers(CommandList* pCommandList, const TArray<PipelineTextureBarrierDesc>& textureBarriers, FPipelineStageFlags srcPipelineStage, FPipelineStageFlags dstPipelineStage)
	{
		uint32 remaining = textureBarriers.GetSize() % MAX_IMAGE_BARRIERS;
		uint32 i = 0;
		for (; i < floor(textureBarriers.GetSize() / MAX_IMAGE_BARRIERS); i++)
			pCommandList->PipelineTextureBarriers(srcPipelineStage, dstPipelineStage, &textureBarriers[i * MAX_IMAGE_BARRIERS], MAX_IMAGE_BARRIERS);
		if (remaining != 0)
			pCommandList->PipelineTextureBarriers(srcPipelineStage, dstPipelineStage, &textureBarriers[i * MAX_IMAGE_BARRIERS], remaining);
	}

	void RenderGraph::PipelineBufferBarriers(CommandList* pCommandList, const TArray<PipelineBufferBarrierDesc>& bufferBarriers, FPipelineStageFlags srcPipelineStage, FPipelineStageFlags dstPipelineStage)
	{
		uint32 remaining = bufferBarriers.GetSize() % MAX_BUFFER_BARRIERS;
		uint32 i = 0;
		for (; i < floor(bufferBarriers.GetSize() / MAX_BUFFER_BARRIERS); i++)
			pCommandList->PipelineBufferBarriers(srcPipelineStage, dstPipelineStage, &bufferBarriers[i * MAX_BUFFER_BARRIERS], MAX_BUFFER_BARRIERS);
		if (remaining != 0)
			pCommandList->PipelineBufferBarriers(srcPipelineStage, dstPipelineStage, &bufferBarriers[i * MAX_BUFFER_BARRIERS], remaining);
	}
}
