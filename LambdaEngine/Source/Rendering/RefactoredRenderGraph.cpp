#include "Rendering/RefactoredRenderGraph.h"
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

namespace LambdaEngine
{
	constexpr const char* RENDER_GRAPH_IMGUI_STAGE_NAME			= "RENDER_STAGE_IMGUI";

	constexpr const char* RENDER_GRAPH_BACK_BUFFER_ATTACHMENT   = "BACK_BUFFER_TEXTURE";

	constexpr const char* FULLSCREEN_QUAD_VERTEX_BUFFER		    = "FULLSCREEN_QUAD_VERTEX_BUFFER";

	constexpr const char* PER_FRAME_BUFFER					    = "PER_FRAME_BUFFER";
	constexpr const char* SCENE_LIGHTS_BUFFER					= "SCENE_LIGHTS_BUFFER";

	constexpr const char* SCENE_MAT_PARAM_BUFFER				= "SCENE_MAT_PARAM_BUFFER";
	constexpr const char* SCENE_VERTEX_BUFFER					= "SCENE_VERTEX_BUFFER";
	constexpr const char* SCENE_INDEX_BUFFER					= "SCENE_INDEX_BUFFER";
	constexpr const char* SCENE_INSTANCE_BUFFER				    = "SCENE_INSTANCE_BUFFER";
	constexpr const char* SCENE_MESH_INDEX_BUFFER				= "SCENE_MESH_INDEX_BUFFER";
	constexpr const char* SCENE_TLAS							= "SCENE_TLAS";

	constexpr const char* SCENE_ALBEDO_MAPS					    = "SCENE_ALBEDO_MAPS";
	constexpr const char* SCENE_NORMAL_MAPS					    = "SCENE_NORMAL_MAPS";
	constexpr const char* SCENE_AO_MAPS						    = "SCENE_AO_MAPS";
	constexpr const char* SCENE_ROUGHNESS_MAPS				    = "SCENE_ROUGHNESS_MAPS";
	constexpr const char* SCENE_METALLIC_MAPS					= "SCENE_METALLIC_MAPS";

	RefactoredRenderGraph::RefactoredRenderGraph(const IGraphicsDevice* pGraphicsDevice) :
		m_pGraphicsDevice(pGraphicsDevice)
	{
	}

	RefactoredRenderGraph::~RefactoredRenderGraph()
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

			if (pResource->Type == EResourceType::INTERNAL_BUFFER)
			{
				for (uint32 sr = 0; sr < pResource->SubResourceCount; sr++)
				{
					SAFERELEASE(pResource->Buffer.Buffers[sr]);
				}
			}
			else if (pResource->Type == EResourceType::INTERNAL_TEXTURE)
			{
				for (uint32 sr = 0; sr < pResource->SubResourceCount; sr++)
				{
					SAFERELEASE(pResource->Texture.Textures[sr]);
					SAFERELEASE(pResource->Texture.TextureViews[sr]);
					SAFERELEASE(pResource->Texture.Samplers[sr]);
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

			if (pPipelineStage->Type == ERefactoredRenderGraphPipelineStageType::RENDER)
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
			else if (pPipelineStage->Type == ERefactoredRenderGraphPipelineStageType::SYNCHRONIZATION)
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

	bool RefactoredRenderGraph::Init(const RenderGraphDesc* pDesc)
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
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Fence", pDesc->pName);
			return false;
		}

		if (!CreateDescriptorHeap())
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Descriptor Heap", pDesc->pName);
			return false;
		}

		if (!CreateCopyCommandLists())
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Copy Command Lists", pDesc->pName);
			return false;
		}

		if (!CreateResources(pDesc->pParsedRenderGraphStructure->ResourceDescriptions))
		{ 
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Resources", pDesc->pName);
			return false;
		}

		if (!CreateRenderStages(renderStageDescriptions))
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Render Stages", pDesc->pName);
			return false;
		}

		if (!CreateSynchronizationStages(synchronizationStageDescriptions))
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Synchronization Stages", pDesc->pName);
			return false;
		}

		if (!CreatePipelineStages(pipelineStageDescriptions))
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Pipeline Stages", pDesc->pName);
			return false;
		}

		return true;
	}

	void RefactoredRenderGraph::UpdateResource(const ResourceUpdateDesc& desc)
	{
		auto it = m_ResourceMap.find(desc.pResourceName);

		if (it != m_ResourceMap.end())
		{
			Resource* pResource = &it->second;

			switch (pResource->Type)
			{
			case EResourceType::INTERNAL_TEXTURE:					UpdateResourceInternalTexture(pResource, desc);					break;
			case EResourceType::INTERNAL_BUFFER:					UpdateResourceInternalBuffer(pResource, desc);					break;
			case EResourceType::EXTERNAL_TEXTURE:					UpdateResourceExternalTexture(pResource, desc);					break;
			case EResourceType::EXTERNAL_BUFFER:					UpdateResourceExternalBuffer(pResource, desc);					break;
			case EResourceType::EXTERNAL_ACCELERATION_STRUCTURE:	UpdateResourceExternalAccelerationStructure(pResource, desc);	break;
			default:
				{
					LOG_WARNING("[RenderGraph]: Resource \"%s\" in Render Graph has unsupported Type", desc.pResourceName);
					return;
				}
			}
		}
		else
		{
			LOG_WARNING("[RenderGraph]: Resource \"%s\" in Render Graph could not be found in Resource Map", desc.pResourceName);
			return;
		}
	}

	void RefactoredRenderGraph::UpdateRenderStageParameters(const RenderStageParameters& desc)
	{
		auto it = m_RenderStageMap.find(desc.pRenderStageName);

		if (it != m_RenderStageMap.end())
		{
			RenderStage* pRenderStage = &m_pRenderStages[it->second];
			pRenderStage->Parameters = desc;
		}
		else
		{
			LOG_WARNING("[RenderGraph]: UpdateRenderStageParameters failed, render stage with name \"%s\" could not be found", desc.pRenderStageName);
			return;
		}
	}

	void RefactoredRenderGraph::GetAndIncrementFence(IFence** ppFence, uint64* pSignalValue)
	{
		(*pSignalValue) = m_SignalValue++;
		(*ppFence) = m_pFence;
	}

	void RefactoredRenderGraph::Update()
	{
		if (m_DirtyDescriptorSetInternalBuffers.size()			> 0 || 
			m_DirtyDescriptorSetExternalBuffers.size()			> 0 || 
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

			//Internal Buffers
			if (m_DirtyDescriptorSetInternalBuffers.size() > 0)
			{
				for (Resource* pResource : m_DirtyDescriptorSetInternalBuffers)
				{
					for (uint32 rb = 0; rb < pResource->ResourceBindings.size(); rb++)
					{
						ResourceBinding* pResourceBinding = &pResource->ResourceBindings[rb];
						RenderStage* pRenderStage = pResourceBinding->pRenderStage;

						if (pRenderStage->UsesCustomRenderer)
						{
							pRenderStage->pCustomRenderer->UpdatePerBackBufferBuffers(
								pResource->Name,
								pResource->Buffer.Buffers.data(), 
								pResource->Buffer.Offsets.data(), 
								pResource->Buffer.SizesInBytes.data());
						}
						else if (pResourceBinding->DescriptorType != EDescriptorType::DESCRIPTOR_UNKNOWN)
						{
							for (uint32 b = 0; b < m_BackBufferCount; b++)
							{
								pRenderStage->ppBufferDescriptorSets[b]->WriteBufferDescriptors(
									&pResource->Buffer.Buffers[b],
									&pResource->Buffer.Offsets[b],
									&pResource->Buffer.SizesInBytes[b],
									pResourceBinding->Binding,
									1,
									pResourceBinding->DescriptorType);
							}
						}
					}
				}

				m_DirtyDescriptorSetInternalBuffers.clear();
			}

			//External Buffers
			if (m_DirtyDescriptorSetExternalBuffers.size() > 0)
			{
				for (Resource* pResource : m_DirtyDescriptorSetExternalBuffers)
				{
					for (uint32 rb = 0; rb < pResource->ResourceBindings.size(); rb++)
					{
						ResourceBinding* pResourceBinding = &pResource->ResourceBindings[rb];
						RenderStage* pRenderStage = pResourceBinding->pRenderStage;

						if (pRenderStage->UsesCustomRenderer)
						{
							pRenderStage->pCustomRenderer->UpdateBufferArray(
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

				m_DirtyDescriptorSetExternalBuffers.clear();
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
						pRenderStage->pCustomRenderer->UpdateAccelerationStructure(
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

		if (m_DirtyDescriptorSetInternalTextures.size() > 0 ||
			m_DirtyDescriptorSetExternalTextures.size() > 0)
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

			//Internal Textures
			if (m_DirtyDescriptorSetInternalTextures.size() > 0)
			{
				for (Resource* pResource : m_DirtyDescriptorSetInternalTextures)
				{
					for (uint32 rb = 0; rb < pResource->ResourceBindings.size(); rb++)
					{
						ResourceBinding* pResourceBinding = &pResource->ResourceBindings[rb];
						RenderStage* pRenderStage = pResourceBinding->pRenderStage;

						if (pRenderStage->UsesCustomRenderer)
						{
							pRenderStage->pCustomRenderer->UpdatePerBackBufferTextures(
								pResource->Name,
								pResource->Texture.TextureViews.data());
						}
						else if (pResourceBinding->DescriptorType != EDescriptorType::DESCRIPTOR_UNKNOWN)
						{
							for (uint32 b = 0; b < m_BackBufferCount; b++)
							{
								for (uint32 s = 0; s < pRenderStage->TextureSubDescriptorSetCount; s++)
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
						}
					}
				}

				m_DirtyDescriptorSetInternalTextures.clear();
			}

			//External Textures
			if (m_DirtyDescriptorSetExternalTextures.size() > 0)
			{
				for (Resource* pResource : m_DirtyDescriptorSetExternalTextures)
				{
					for (uint32 rb = 0; rb < pResource->ResourceBindings.size(); rb++)
					{
						ResourceBinding* pResourceBinding = &pResource->ResourceBindings[rb];
						RenderStage* pRenderStage = pResourceBinding->pRenderStage;
						uint32 actualSubResourceCount = pResource->SubResourceCount / pRenderStage->TextureSubDescriptorSetCount;

						if (pRenderStage->UsesCustomRenderer)
						{
							pRenderStage->pCustomRenderer->UpdateTextureArray(
								pResource->Name,
								pResource->Texture.TextureViews.data(),
								pResource->SubResourceCount);
						}
						else if (pResourceBinding->DescriptorType != EDescriptorType::DESCRIPTOR_UNKNOWN)
						{
							for (uint32 b = 0; b < m_BackBufferCount; b++)
							{
								for (uint32 s = 0; s < pRenderStage->TextureSubDescriptorSetCount; s++)
								{
									uint32 index = b * pRenderStage->TextureSubDescriptorSetCount + s;

									pRenderStage->ppTextureDescriptorSets[b * pRenderStage->TextureSubDescriptorSetCount + s]->WriteTextureDescriptors(
										&pResource->Texture.TextureViews[s * actualSubResourceCount],
										&pResource->Texture.Samplers[s * actualSubResourceCount],
										pResourceBinding->TextureState,
										pResourceBinding->Binding,
										actualSubResourceCount,
										pResourceBinding->DescriptorType);
								}
							}
						}
					}
				}

				m_DirtyDescriptorSetExternalTextures.clear();
			}
		}
	}

	void RefactoredRenderGraph::NewFrame(Timestamp delta)
	{
		for (uint32 r = 0; r < (uint32)m_CustomRenderers.size(); r++)
		{
			ICustomRenderer* pCustomRenderer = m_CustomRenderers[r];
			pCustomRenderer->NewFrame(delta);
		}
	}

	void RefactoredRenderGraph::PrepareRender(Timestamp delta)
	{
		for (uint32 r = 0; r < (uint32)m_CustomRenderers.size(); r++)
		{
			ICustomRenderer* pCustomRenderer = m_CustomRenderers[r];
			pCustomRenderer->PrepareRender(delta);
		}
	}

	void RefactoredRenderGraph::Render(uint64 modFrameIndex, uint32 backBufferIndex)
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

				if (pPipelineStage->Type == ERefactoredRenderGraphPipelineStageType::RENDER)
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
				else if (pPipelineStage->Type == ERefactoredRenderGraphPipelineStageType::SYNCHRONIZATION)
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

		for (uint32 e = 0; e < m_ExecutionStageCount; e++)
		{
			ICommandList* pCommandList = m_ppExecutionStages[e];

			if (pCommandList != nullptr)
			{
				if (pCommandList->GetType() == ECommandQueueType::COMMAND_QUEUE_GRAPHICS)
				{
					RenderSystem::GetGraphicsQueue()->ExecuteCommandLists(&pCommandList, 1, FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, m_pFence, m_SignalValue - 1, m_pFence, m_SignalValue);
				}
				else if (pCommandList->GetType() == ECommandQueueType::COMMAND_QUEUE_COMPUTE)
				{
					RenderSystem::GetComputeQueue()->ExecuteCommandLists(&pCommandList, 1, FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, m_pFence, m_SignalValue - 1, m_pFence, m_SignalValue);
				}

				m_SignalValue++;
			}
		}
	}

	bool RefactoredRenderGraph::GetResourceTextures(const char* pResourceName, ITexture* const ** pppTexture, uint32* pTextureView) const
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

	bool RefactoredRenderGraph::GetResourceTextureViews(const char* pResourceName, ITextureView* const ** pppTextureViews, uint32* pTextureViewCount) const
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

	bool RefactoredRenderGraph::GetResourceBuffers(const char* pResourceName, IBuffer* const ** pppBuffers, uint32* pBufferCount) const
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

	bool RefactoredRenderGraph::GetResourceAccelerationStructure(const char* pResourceName, IAccelerationStructure const ** ppAccelerationStructure) const
	{
		auto it = m_ResourceMap.find(pResourceName);

		if (it != m_ResourceMap.end())
		{
			(*ppAccelerationStructure) = it->second.AccelerationStructure.pTLAS;
			return true;
		}

		return false;
	}

	bool RefactoredRenderGraph::CreateFence()
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

	bool RefactoredRenderGraph::CreateDescriptorHeap()
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

	bool RefactoredRenderGraph::CreateCopyCommandLists()
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

	bool RefactoredRenderGraph::CreateResources(const TArray<RefactoredResourceDesc>& resources)
	{
		m_ResourceMap.reserve(resources.size());

		for (uint32 i = 0; i < resources.size(); i++)
		{
			const RefactoredResourceDesc* pResourceDesc = &resources[i];

			Resource* pResource = &m_ResourceMap[pResourceDesc->Name];

			pResource->Name				= pResourceDesc->Name;

			if (pResourceDesc->SubResourceType == ERefactoredRenderGraphSubResourceType::ARRAY)
				pResource->SubResourceCount = pResourceDesc->SubResourceArrayCount;
			else if (pResourceDesc->SubResourceType == ERefactoredRenderGraphSubResourceType::PER_FRAME)
				pResource->SubResourceCount = m_BackBufferCount;
					
			if (!pResourceDesc->External)
			{
				//Internal
				if (pResourceDesc->Type == ERefactoredRenderGraphResourceType::TEXTURE)
				{
					pResource->Type				= ERefactoredRenderGraphResourceType::TEXTURE;
					pResource->OwnershipType	= pResourceDesc->Name == RENDER_GRAPH_BACK_BUFFER_ATTACHMENT ? EResourceOwnershipType::EXTERNAL : EResourceOwnershipType::INTERNAL;
					pResource->Texture.Format	= pResourceDesc->TextureFormat; 
					pResource->Texture.Textures.resize(pResource->SubResourceCount);
					pResource->Texture.TextureViews.resize(pResource->SubResourceCount);
					pResource->Texture.Samplers.resize(pResource->SubResourceCount);
				}
				else if (pResourceDesc->Type == ERefactoredRenderGraphResourceType::BUFFER)
				{
					pResource->Type				= ERefactoredRenderGraphResourceType::BUFFER;
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
				if (pResourceDesc->Type == ERefactoredRenderGraphResourceType::TEXTURE)
				{
					pResource->Type				= ERefactoredRenderGraphResourceType::TEXTURE;
					pResource->OwnershipType	= EResourceOwnershipType::EXTERNAL;
					pResource->Texture.Format	= pResourceDesc->TextureFormat; 
					pResource->Texture.Textures.resize(pResource->SubResourceCount);
					pResource->Texture.TextureViews.resize(pResource->SubResourceCount);
					pResource->Texture.Samplers.resize(pResource->SubResourceCount);
				}
				else if (pResourceDesc->Type == ERefactoredRenderGraphResourceType::BUFFER)
				{
					pResource->Type				= ERefactoredRenderGraphResourceType::BUFFER;
					pResource->OwnershipType	= EResourceOwnershipType::EXTERNAL;
					pResource->Buffer.Buffers.resize(pResource->SubResourceCount);
					pResource->Buffer.Offsets.resize(pResource->SubResourceCount);
					pResource->Buffer.SizesInBytes.resize(pResource->SubResourceCount);
				}
				else if (pResourceDesc->Type == ERefactoredRenderGraphResourceType::ACCELERATION_STRUCTURE)
				{ 
					pResource->Type				= ERefactoredRenderGraphResourceType::ACCELERATION_STRUCTURE;
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

	bool RefactoredRenderGraph::CreateRenderStages(const TArray<RefactoredRenderStageDesc>& renderStages)
	{
		m_RenderStageCount = (uint32)renderStages.size();
		m_RenderStageMap.reserve(m_RenderStageCount);
		m_pRenderStages = DBG_NEW RenderStage[m_RenderStageCount];

		for (uint32 renderStageIndex = 0; renderStageIndex < m_RenderStageCount; renderStageIndex++)
		{
			const RefactoredRenderStageDesc* pRenderStageDesc = &renderStages[renderStageIndex];

			RenderStage* pRenderStage = &m_pRenderStages[renderStageIndex];
			m_RenderStageMap[pRenderStageDesc->Name] = renderStageIndex;

			//Calculate the total number of textures we want to bind
			uint32 textureSlots = 0;
			uint32 totalNumberOfTextures = 0;
			uint32 totalNumberOfNonMaterialTextures = 0;
			uint32 textureSubresourceCount = 0;
			bool textureSubResourceCountSame = true;
			for (uint32 rs = 0; rs < pRenderStageDesc->ResourceStates.size(); rs++)
			{
				const RefactoredResourceState* pResourceStateDesc = &pRenderStageDesc->ResourceStates[rs];

				auto resourceIt = m_ResourceMap.find(pResourceStateDesc->ResourceName);

				if (resourceIt == m_ResourceMap.end())
				{
					LOG_ERROR("[RenderGraph]: Resource State with name \"%s\" has no accompanying Resource", pResourceStateDesc->ResourceName.c_str());
					return false;
				}

				const Resource* pResource = &resourceIt->second;

				if (ResourceStateNeedsDescriptor(pResourceStateDesc->BindingType) && pResource->Type == ERefactoredRenderGraphResourceType::TEXTURE)
				{
					textureSlots++;

					if (pResourceStateDesc->BindingType == ERefactoredRenderGraphResourceBindingType::READ_ONLY) //READ_ONLY texture -> assumes combined sampler
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
				const RefactoredResourceState* pResourceStateDesc = &pRenderStageDesc->ResourceStates[rs];

				auto resourceIt = m_ResourceMap.find(pResourceStateDesc->ResourceName);

				if (resourceIt == m_ResourceMap.end())
				{
					LOG_ERROR("[RenderGraph]: Resource State with name \"%s\" has no accompanying Resource", pResourceStateDesc->ResourceName.c_str());
					return false;
				}

				const Resource* pResource = &resourceIt->second;

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


					if (pResource->Type == ERefactoredRenderGraphResourceType::TEXTURE)
					{
						ETextureState textureState = CalculateResourceTextureState(pResource->Type, pResourceStateDesc->BindingType, pResource->Texture.Format);

						descriptorBinding.DescriptorCount	= pResource->SubResourceType == ERefactoredRenderGraphSubResourceType::ARRAY ? (pResource->SubResourceCount / pRenderStage->TextureSubDescriptorSetCount) : 1;
						descriptorBinding.Binding			= textureDescriptorBindingIndex++;

						textureDescriptorSetDescriptions.push_back(descriptorBinding);
						renderStageTextureResources.push_back(std::make_tuple(pResource, textureState, descriptorType));
					}
					else
					{
						descriptorBinding.DescriptorCount	= pResource->SubResourceType == ERefactoredRenderGraphSubResourceType::ARRAY ? pResource->SubResourceCount : 1;
						descriptorBinding.Binding			= bufferDescriptorBindingIndex++;

						bufferDescriptorSetDescriptions.push_back(descriptorBinding);
						renderStageBufferResources.push_back(std::make_tuple(pResource, ETextureState::TEXTURE_STATE_UNKNOWN, descriptorType));
					}
				}
				//RenderPass Attachments
				else if (pResourceStateDesc->BindingType == ERefactoredRenderGraphResourceBindingType::ATTACHMENT)
				{
					----------------Find previous and next synchronization stage using this resource, but also, what if its a queue change? Borde lösas i editorn istället

					
					bool isColorAttachment = pResource->Texture.Format != EFormat::FORMAT_D24_UNORM_S8_UINT;

					if (isColorAttachment)
					{
						ETextureState finalState	= isBackBufferAttachment ? ETextureState::TEXTURE_STATE_PRESENT : ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;
						ETextureState initialState	= ETextureState::TEXTURE_STATE_UNKNOWN;

						if (renderStageIndex > 0 && pResource->ResourceBindings.size() > 0)
						{
							initialState = pResource->ResourceBindings.back().TextureState;
						}
						else
						{
							initialState = ETextureState::TEXTURE_STATE_DONT_CARE;
						}

						RenderPassAttachmentDesc renderPassAttachmentDesc = {};
						renderPassAttachmentDesc.Format			= pResource->Texture.Format;
						renderPassAttachmentDesc.SampleCount	= 1;
						renderPassAttachmentDesc.LoadOp			= initialState != ETextureState::TEXTURE_STATE_UNKNOWN ? ELoadOp::LOAD : ELoadOp::CLEAR;
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
						ETextureState initialState = ETextureState::TEXTURE_STATE_UNKNOWN;

						if (renderStageIndex > 0 && pResource->ResourceBindings.size() > 0)
						{
							initialState = pResource->ResourceBindings.back().TextureState;
						}
						else
						{
							initialState = ETextureState::TEXTURE_STATE_DONT_CARE;
						}

						RenderPassAttachmentDesc renderPassAttachmentDesc = {};
						renderPassAttachmentDesc.Format			= pAttachment->TextureFormat;
						renderPassAttachmentDesc.SampleCount	= 1;
						renderPassAttachmentDesc.LoadOp			= ELoadOp::CLEAR;
						renderPassAttachmentDesc.StoreOp		= EStoreOp::STORE;
						renderPassAttachmentDesc.StencilLoadOp	= ELoadOp::CLEAR;
						renderPassAttachmentDesc.StencilStoreOp = EStoreOp::STORE;
						renderPassAttachmentDesc.InitialState	= initialState;
						renderPassAttachmentDesc.FinalState		= ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;

						renderPassDepthStencilDescription = renderPassAttachmentDesc;
						pDepthStencilResource = pResource;
					}
				}
				//Graphics Draw Resources (because Rasterization is a special little boy)
				else if (pResourceStateDesc->BindingType == ERefactoredRenderGraphResourceBindingType::DRAW_RESOURCE)
				{

				}
			}

			if (pRenderStageDesc->UsesCustomRenderer)
			{
				ICustomRenderer* pCustomRenderer = nullptr;

				if (strcmp(pRenderStageDesc->pName, RENDER_GRAPH_IMGUI_STAGE_NAME) == 0)
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
					pCustomRenderer = pRenderStageDesc->CustomRenderer.pCustomRenderer;
					m_CustomRenderers.push_back(pRenderStageDesc->CustomRenderer.pCustomRenderer);
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

				ConstantRangeDesc constantRangeDesc = {};
				constantRangeDesc.OffsetInBytes			= 0;
				constantRangeDesc.ShaderStageFlags		= CreateShaderStageMask(pRenderStageDesc);
				constantRangeDesc.SizeInBytes			= pRenderStageDesc->PushConstants.DataSize;

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
					pipelineLayoutDesc.pConstantRanges			= &constantRangeDesc;
					pipelineLayoutDesc.ConstantRangeCount		= constantRangeDesc.SizeInBytes > 0 ? 1 : 0;

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
				if (pRenderStageDesc->PipelineType == EPipelineStateType::GRAPHICS)
				{
					GraphicsManagedPipelineStateDesc pipelineDesc		= *pRenderStageDesc->GraphicsPipeline.pGraphicsDesc;

					pipelineDesc.pPipelineLayout				= pRenderStage->pPipelineLayout;
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
						pRenderStage->DrawType = pRenderStageDesc->GraphicsPipeline.DrawType;

						if (pRenderStageDesc->GraphicsPipeline.pIndexBufferName != nullptr)
						{
							auto indexBufferIt = m_ResourceMap.find(pRenderStageDesc->GraphicsPipeline.pIndexBufferName);

							if (indexBufferIt == m_ResourceMap.end())
							{
								LOG_ERROR("[RenderGraph]: Resource \"%s\" is referenced as index buffer resource by render stage, but it cannot be found in Resource Map", pRenderStageDesc->GraphicsPipeline.pIndexBufferName);
								return false;
							}

							pRenderStage->pIndexBufferResource = &indexBufferIt->second;
						}

						if (pRenderStageDesc->GraphicsPipeline.pMeshIndexBufferName != nullptr)
						{
							auto meshIndexBufferIt = m_ResourceMap.find(pRenderStageDesc->GraphicsPipeline.pMeshIndexBufferName);

							if (meshIndexBufferIt == m_ResourceMap.end())
							{
								LOG_ERROR("[RenderGraph]: Resource \"%s\" is referenced as mesh index buffer resource by render stage, but it cannot be found in Resource Map", pRenderStageDesc->GraphicsPipeline.pMeshIndexBufferName);
								return false;
							}

							pRenderStage->pMeshIndexBufferResource = &meshIndexBufferIt->second;
						}
					}

					pRenderStage->PipelineStateID = PipelineStateManager::CreateGraphicsPipelineState(&pipelineDesc);
				}
				else if (pRenderStageDesc->PipelineType == EPipelineStateType::COMPUTE)
				{
					ComputeManagedPipelineStateDesc pipelineDesc = *pRenderStageDesc->ComputePipeline.pComputeDesc;

					pipelineDesc.pPipelineLayout = pRenderStage->pPipelineLayout;

					pRenderStage->PipelineStateID = PipelineStateManager::CreateComputePipelineState(&pipelineDesc);
				}
				else if (pRenderStageDesc->PipelineType == EPipelineStateType::RAY_TRACING)
				{
					RayTracingManagedPipelineStateDesc pipelineDesc = *pRenderStageDesc->RayTracingPipeline.pRayTracingDesc;

					pipelineDesc.pPipelineLayout = pRenderStage->pPipelineLayout;

					pRenderStage->PipelineStateID = PipelineStateManager::CreateRayTracingPipelineState(&pipelineDesc);
				}
			}

			//Link Attachment Resources to Render Stage
			{
				if (renderStageRenderTargets.size() > 0)
				{
					if (pRenderStageDesc->PipelineType != EPipelineStateType::GRAPHICS)
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

						pResource->ResourceBindings.push_back(resourceBinding);
						pRenderStage->RenderTargetResources.push_back(pResource);
					}
				}

				if (pDepthStencilResource != nullptr)
				{
					if (pRenderStageDesc->PipelineType != EPipelineStateType::GRAPHICS)
					{
						LOG_ERROR("[RenderGraph]: There are resources that a RenderPass should be linked to, but Render Stage %u is not a Graphics Pipeline State", renderStageIndex);
						return false;
					}

					ResourceBinding resourceBinding = {};
					resourceBinding.pRenderStage	= pRenderStage;
					resourceBinding.DescriptorType	= EDescriptorType::DESCRIPTOR_UNKNOWN;
					resourceBinding.Binding			= UINT32_MAX;
					resourceBinding.TextureState	= ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;

					pDepthStencilResource->ResourceBindings.push_back(resourceBinding);
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
		}

		return true;
	}

	bool RefactoredRenderGraph::CreateSynchronizationStages(const TArray<SynchronizationStageDesc>& synchronizationStageDescriptions)
	{
		m_pSynchronizationStages = DBG_NEW SynchronizationStage[synchronizationStageDescriptions.size()];

		bool firstTimeEnvounteringBackBuffer = false;

		for (uint32 i = 0; i < synchronizationStageDescriptions.size(); i++)
		{
			const SynchronizationStageDesc* pSynchronizationStageDesc = &synchronizationStageDescriptions[i];

			SynchronizationStage* pSynchronizationStage = &m_pSynchronizationStages[i];

			for (const AttachmentSynchronizationDesc& attachmentSynchronizationDesc : pSynchronizationStageDesc->Synchronizations)
			{
				bool isBackBuffer = strcmp(attachmentSynchronizationDesc.FromAttachment.pName, RENDER_GRAPH_BACK_BUFFER_ATTACHMENT) == 0;

				ESimpleResourceType barrierType = !isBackBuffer ? GetSimpleType(attachmentSynchronizationDesc.FromAttachment.Type) : ESimpleResourceType::TEXTURE;
				EAttachmentAccessType accessType = GetAttachmentAccessType(attachmentSynchronizationDesc.FromAttachment.Type);

				auto it = m_ResourceMap.find(attachmentSynchronizationDesc.FromAttachment.pName);

				if (it == m_ResourceMap.end())
				{
					LOG_ERROR("[RenderGraph]: Resource found in Synchronization Stage but not in Resource Map \"%s\"", attachmentSynchronizationDesc.FromAttachment.pName);
					return false;
				}

				Resource* pResource = &it->second;

				if (barrierType == ESimpleResourceType::TEXTURE)
				{
					//Special Case for Back Buffer
					if (!isBackBuffer)
					{
						PipelineTextureBarrierDesc textureBarrier = {};
						textureBarrier.QueueBefore			= ConvertPipelineStateTypeToQueue(attachmentSynchronizationDesc.FromQueueOwner);
						textureBarrier.QueueAfter			= ConvertPipelineStateTypeToQueue(attachmentSynchronizationDesc.ToQueueOwner);
						textureBarrier.StateBefore			= ConvertAttachmentTypeToTextureState(attachmentSynchronizationDesc.FromAttachment.Type);
						textureBarrier.SrcMemoryAccessFlags = ConvertAttachmentTypeToMemoryAccessFlags(attachmentSynchronizationDesc.FromAttachment.Type);

						if (attachmentSynchronizationDesc.Type == EAttachmentSynchronizationType::OWNERSHIP_CHANGE_READ || attachmentSynchronizationDesc.Type == EAttachmentSynchronizationType::OWNERSHIP_CHANGE_WRITE)
						{
							textureBarrier.StateAfter			= textureBarrier.StateBefore;
							textureBarrier.DstMemoryAccessFlags = textureBarrier.SrcMemoryAccessFlags;
						}
						else
						{
							textureBarrier.StateAfter			= ConvertAttachmentTypeToTextureState(attachmentSynchronizationDesc.ToAttachment.Type);
							textureBarrier.DstMemoryAccessFlags = ConvertAttachmentTypeToMemoryAccessFlags(attachmentSynchronizationDesc.ToAttachment.Type);
						}

						TextureSynchronization textureSynchronization = {};
						textureSynchronization.SrcShaderStage			= GetLastShaderStageInMask(attachmentSynchronizationDesc.FromAttachment.ShaderStages);
						textureSynchronization.DstShaderStage			= GetFirstShaderStageInMask(attachmentSynchronizationDesc.ToAttachment.ShaderStages);
						
						if (accessType == EAttachmentAccessType::EXTERNAL_INPUT)
						{
							textureSynchronization.BarrierUseFrameIndex		= 0;
							textureSynchronization.SameFrameBarrierOffset	= 1;
						}
						else
						{
							textureSynchronization.BarrierUseFrameIndex		= 1;
							textureSynchronization.SameFrameBarrierOffset	= pResource->SubResourceCount;
						}

						textureSynchronization.Barriers.reserve(pResource->SubResourceCount);

						for (uint32 sr = 0; sr < pResource->SubResourceCount; sr++)
						{
							m_TextureBarriers.push_back(textureBarrier);
							uint32 barrierIndex = m_TextureBarriers.size() - 1;

							textureSynchronization.Barriers.push_back(barrierIndex);
							pResource->Texture.Barriers.push_back(barrierIndex);
						}

						pSynchronizationStage->TextureSynchronizations[attachmentSynchronizationDesc.FromAttachment.pName] = textureSynchronization;
					}
					else
					{
						//Barrier is for Back Buffer, this means that the Back Buffer is not synchronized by a Render Pass

						PipelineTextureBarrierDesc textureBarrier = {};
						textureBarrier.QueueBefore			= ConvertPipelineStateTypeToQueue(attachmentSynchronizationDesc.FromQueueOwner);
						textureBarrier.QueueAfter			= ConvertPipelineStateTypeToQueue(attachmentSynchronizationDesc.ToQueueOwner);

						TextureSynchronization textureSynchronization = {};

						//From "GetNextImage" to Write State
						if (attachmentSynchronizationDesc.FromAttachment.Type == EAttachmentType::NONE) 
						{
							textureBarrier.StateBefore			= ETextureState::TEXTURE_STATE_DONT_CARE;
							textureBarrier.SrcMemoryAccessFlags = FMemoryAccessFlags::MEMORY_ACCESS_FLAG_UNKNOWN;

							textureSynchronization.SrcShaderStage = FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
						}
						else if (attachmentSynchronizationDesc.FromAttachment.Type == EAttachmentType::OUTPUT_COLOR) //RenderPasses hardcoded to output in PRESENT state
						{
							textureBarrier.StateBefore			= ETextureState::TEXTURE_STATE_PRESENT;
							textureBarrier.SrcMemoryAccessFlags = FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_WRITE;

							textureSynchronization.SrcShaderStage = GetLastShaderStageInMask(attachmentSynchronizationDesc.FromAttachment.ShaderStages);
						}
						else
						{
							textureBarrier.StateBefore			= ConvertAttachmentTypeToTextureState(attachmentSynchronizationDesc.FromAttachment.Type);
							textureBarrier.SrcMemoryAccessFlags = ConvertAttachmentTypeToMemoryAccessFlags(attachmentSynchronizationDesc.FromAttachment.Type);

							textureSynchronization.SrcShaderStage = GetLastShaderStageInMask(attachmentSynchronizationDesc.FromAttachment.ShaderStages);
						}

						//To Present State
						if (attachmentSynchronizationDesc.ToAttachment.Type == EAttachmentType::NONE) 
						{
							textureBarrier.StateAfter				= ETextureState::TEXTURE_STATE_PRESENT;
							textureBarrier.DstMemoryAccessFlags		= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_READ;

							textureSynchronization.DstShaderStage	= FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
						}
						else if (attachmentSynchronizationDesc.Type == EAttachmentSynchronizationType::OWNERSHIP_CHANGE_READ || attachmentSynchronizationDesc.Type == EAttachmentSynchronizationType::OWNERSHIP_CHANGE_WRITE)
						{
							textureBarrier.StateAfter				= textureBarrier.StateBefore;
							textureBarrier.DstMemoryAccessFlags		= textureBarrier.SrcMemoryAccessFlags;

							textureSynchronization.DstShaderStage	= GetFirstShaderStageInMask(attachmentSynchronizationDesc.ToAttachment.ShaderStages);
						}
						else
						{
							textureBarrier.StateAfter				= ConvertAttachmentTypeToTextureState(attachmentSynchronizationDesc.ToAttachment.Type);
							textureBarrier.DstMemoryAccessFlags		= ConvertAttachmentTypeToMemoryAccessFlags(attachmentSynchronizationDesc.ToAttachment.Type);

							textureSynchronization.DstShaderStage	= GetFirstShaderStageInMask(attachmentSynchronizationDesc.ToAttachment.ShaderStages);
						}
						
						textureSynchronization.BarrierUseFrameIndex		= 1;
						textureSynchronization.SameFrameBarrierOffset	= pResource->SubResourceCount;
												
						textureSynchronization.Barriers.reserve(pResource->SubResourceCount);

						for (uint32 sr = 0; sr < pResource->SubResourceCount; sr++)
						{
							m_TextureBarriers.push_back(textureBarrier);
							uint32 barrierIndex = m_TextureBarriers.size() - 1;

							textureSynchronization.Barriers.push_back(barrierIndex);
							pResource->Texture.Barriers.push_back(barrierIndex);
						}

						pSynchronizationStage->TextureSynchronizations[attachmentSynchronizationDesc.FromAttachment.pName] = textureSynchronization;
					}
				}
				else if (barrierType == ESimpleResourceType::BUFFER)
				{
					PipelineBufferBarrierDesc bufferBarrier = {};
					bufferBarrier.QueueBefore			= ConvertPipelineStateTypeToQueue(attachmentSynchronizationDesc.FromQueueOwner);
					bufferBarrier.QueueAfter			= ConvertPipelineStateTypeToQueue(attachmentSynchronizationDesc.ToQueueOwner);
					bufferBarrier.SrcMemoryAccessFlags	= ConvertAttachmentTypeToMemoryAccessFlags(attachmentSynchronizationDesc.FromAttachment.Type);

					if (attachmentSynchronizationDesc.Type == EAttachmentSynchronizationType::OWNERSHIP_CHANGE_READ || attachmentSynchronizationDesc.Type == EAttachmentSynchronizationType::OWNERSHIP_CHANGE_WRITE)
					{
						bufferBarrier.DstMemoryAccessFlags = bufferBarrier.SrcMemoryAccessFlags;
					}
					else
					{
						bufferBarrier.DstMemoryAccessFlags = ConvertAttachmentTypeToMemoryAccessFlags(attachmentSynchronizationDesc.ToAttachment.Type);
					}

					BufferSynchronization bufferSynchronization = {};
					bufferSynchronization.SrcShaderStage			= GetLastShaderStageInMask(attachmentSynchronizationDesc.FromAttachment.ShaderStages);
					bufferSynchronization.DstShaderStage			= GetFirstShaderStageInMask(attachmentSynchronizationDesc.ToAttachment.ShaderStages);

					if (accessType == EAttachmentAccessType::EXTERNAL_INPUT)
					{
						bufferSynchronization.BarrierUseFrameIndex		= 0;
						bufferSynchronization.SameFrameBarrierOffset	= 1;
					}
					else
					{
						bufferSynchronization.BarrierUseFrameIndex		= 1;
						bufferSynchronization.SameFrameBarrierOffset	= pResource->SubResourceCount;
					}

					bufferSynchronization.Barriers.reserve(pResource->SubResourceCount);

					for (uint32 sr = 0; sr < pResource->SubResourceCount; sr++)
					{
						m_BufferBarriers.push_back(bufferBarrier);
						uint32 barrierIndex = m_BufferBarriers.size() - 1;

						bufferSynchronization.Barriers.push_back(barrierIndex);
						pResource->Buffer.Barriers.push_back(barrierIndex);
					}

					pSynchronizationStage->BufferSynchronizations[attachmentSynchronizationDesc.FromAttachment.pName] = bufferSynchronization;
				}
			}
		}

		return true;
	}

	bool RefactoredRenderGraph::CreatePipelineStages(const TArray<PipelineStageDesc>& pipelineStageDescriptions)
	{
		m_PipelineStageCount = (uint32)pipelineStageDescriptions.size();
		m_pPipelineStages = DBG_NEW PipelineStage[m_PipelineStageCount];

		for (uint32 i = 0; i < m_PipelineStageCount; i++)
		{
			const PipelineStageDesc* pPipelineStageDesc = &pipelineStageDescriptions[i];

			PipelineStage* pPipelineStage = &m_pPipelineStages[i];

			m_ExecutionStageCount += pPipelineStageDesc->Type == EPipelineStageType::SYNCHRONIZATION ? 2 : 1;

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

	void RefactoredRenderGraph::UpdateResourceInternalTexture(Resource* pResource, const ResourceUpdateDesc& desc)
	{
		for (uint32 sr = 0; sr < pResource->SubResourceCount; sr++)
		{
			ITexture** ppTexture = &pResource->Texture.Textures[sr];
			ITextureView** ppTextureView = &pResource->Texture.TextureViews[sr];
			ISampler** ppSampler = &pResource->Texture.Samplers[sr];

			//Update Texture
			{
				const TextureDesc& textureDesc = *desc.InternalTextureUpdate.ppTextureDesc[sr];
				TextureViewDesc textureViewDesc = *desc.InternalTextureUpdate.ppTextureViewDesc[sr];
				
				SAFERELEASE(*ppTexture);
				SAFERELEASE(*ppTextureView);

				ITexture* pTexture			= m_pGraphicsDevice->CreateTexture(&textureDesc, nullptr);

				textureViewDesc.pTexture = pTexture;
				ITextureView* pTextureView	= m_pGraphicsDevice->CreateTextureView(&textureViewDesc);

				(*ppTexture)		= pTexture;
				(*ppTextureView)	= pTextureView;

				if (pResource->Texture.Barriers.size() > 0)
				{
					for (uint32 b = sr; b < pResource->Texture.Barriers.size(); b += pResource->SubResourceCount)
					{
						PipelineTextureBarrierDesc* pTextureBarrier = &m_TextureBarriers[pResource->Texture.Barriers[b]];

						pTextureBarrier->pTexture		= pTexture;
						pTextureBarrier->Miplevel		= 0;
						pTextureBarrier->MiplevelCount	= textureDesc.Miplevels;
						pTextureBarrier->ArrayIndex		= 0;
						pTextureBarrier->ArrayCount		= textureDesc.ArrayCount;
					}

					//Transfer to Initial State			
					{
						PipelineTextureBarrierDesc* pFirstBarrier	= &m_TextureBarriers[pResource->Texture.Barriers[sr]];
						PipelineTextureBarrierDesc  initialBarrier = {};

						initialBarrier.pTexture						= pTexture;
						initialBarrier.StateBefore					= ETextureState::TEXTURE_STATE_DONT_CARE;
						initialBarrier.StateAfter					= pFirstBarrier->StateBefore;
						initialBarrier.QueueBefore					= pFirstBarrier->QueueBefore;
						initialBarrier.QueueAfter					= pFirstBarrier->QueueBefore;
						initialBarrier.SrcMemoryAccessFlags			= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_UNKNOWN;
						initialBarrier.DstMemoryAccessFlags			= pFirstBarrier->SrcMemoryAccessFlags;
						initialBarrier.TextureFlags					= FTextureFlags::TEXTURE_FLAG_NONE;
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

			//Update Sampler
			if (desc.InternalTextureUpdate.ppSamplerDesc != nullptr)
			{
				const SamplerDesc& samplerDesc = *desc.InternalTextureUpdate.ppSamplerDesc[sr];

				SAFERELEASE(*ppSampler);
				ISampler* pSampler = m_pGraphicsDevice->CreateSampler(&samplerDesc);
				(*ppSampler) = pSampler;
			}
		}

		if (pResource->ResourceBindings.size() > 0)
			m_DirtyDescriptorSetInternalTextures.insert(pResource);
	}

	void RefactoredRenderGraph::UpdateResourceInternalBuffer(Resource* pResource, const ResourceUpdateDesc& desc)
	{
		//Update Buffer
		for (uint32 sr = 0; sr < pResource->SubResourceCount; sr++)
		{
			const BufferDesc& bufferDesc = *desc.InternalBufferUpdate.ppBufferDesc[sr];

			IBuffer** ppBuffer		= &pResource->Buffer.Buffers[sr];
			uint64* pOffset			= &pResource->Buffer.Offsets[sr];
			uint64* pSizeInBytes	= &pResource->Buffer.SizesInBytes[sr];

			SAFERELEASE(*ppBuffer);
			IBuffer* pBuffer = m_pGraphicsDevice->CreateBuffer(desc.InternalBufferUpdate.ppBufferDesc[sr], nullptr);
			
			(*ppBuffer)		= pBuffer;
			(*pSizeInBytes) = bufferDesc.SizeInBytes;
			(*pOffset)		= 0;

			for (uint32 b = sr; b < pResource->Buffer.Barriers.size(); b += pResource->SubResourceCount)
			{
				PipelineBufferBarrierDesc* pBufferBarrier = &m_BufferBarriers[pResource->Buffer.Barriers[b]];

				pBufferBarrier->pBuffer		= pBuffer;
				pBufferBarrier->SizeInBytes = bufferDesc.SizeInBytes;
				pBufferBarrier->Offset		= 0;
			}
		}

		if (pResource->ResourceBindings.size() > 0)
			m_DirtyDescriptorSetInternalBuffers.insert(pResource);
	}

	void RefactoredRenderGraph::UpdateResourceExternalTexture(Resource* pResource, const ResourceUpdateDesc& desc)
	{
		//Update Texture
		for (uint32 sr = 0; sr < pResource->SubResourceCount; sr++)
		{
			ITexture** ppTexture = &pResource->Texture.Textures[sr];
			ITextureView** ppTextureView = &pResource->Texture.TextureViews[sr];
			ISampler** ppSampler = &pResource->Texture.Samplers[sr];

			//Update Texture
			{
				ITexture* pTexture			= desc.ExternalTextureUpdate.ppTextures[sr];
				ITextureView* pTextureView	= desc.ExternalTextureUpdate.ppTextureViews[sr];

				(*ppTexture) = pTexture;
				(*ppTextureView) = pTextureView;

				for (uint32 b = sr; b < pResource->Texture.Barriers.size(); b += pResource->SubResourceCount)
				{
					PipelineTextureBarrierDesc* pTextureBarrier = &m_TextureBarriers[pResource->Texture.Barriers[b]];

					pTextureBarrier->pTexture		= pTexture;
					pTextureBarrier->Miplevel		= 0;
					pTextureBarrier->MiplevelCount	= pTexture->GetDesc().Miplevels;
					pTextureBarrier->ArrayIndex		= 0;
					pTextureBarrier->ArrayCount		= pTexture->GetDesc().ArrayCount;
				}
			}

			//Update Sampler
			if (desc.ExternalTextureUpdate.ppSamplers != nullptr)
			{
				ISampler* pSampler	= desc.ExternalTextureUpdate.ppSamplers[sr];
				(*ppSampler)		= pSampler;
			}
		}

		if (strcmp(desc.pResourceName, RENDER_GRAPH_BACK_BUFFER_ATTACHMENT) == 0)
			m_DirtyDescriptorSetInternalTextures.insert(pResource);
		else
			m_DirtyDescriptorSetExternalTextures.insert(pResource);
	}

	void RefactoredRenderGraph::UpdateResourceExternalBuffer(Resource* pResource, const ResourceUpdateDesc& desc)
	{
		//Update Buffer
		for (uint32 sr = 0; sr < pResource->SubResourceCount; sr++)
		{
			IBuffer** ppBuffer		= &pResource->Buffer.Buffers[sr];
			uint64* pOffset			= &pResource->Buffer.Offsets[sr];
			uint64* pSizeInBytes	= &pResource->Buffer.SizesInBytes[sr];


			IBuffer* pBuffer = desc.ExternalBufferUpdate.ppBuffer[sr];

			(*ppBuffer)		= pBuffer;
			(*pSizeInBytes) = pBuffer->GetDesc().SizeInBytes;
			(*pOffset)		= 0;

			for (uint32 b = sr; b < pResource->Buffer.Barriers.size(); b += pResource->SubResourceCount)
			{
				PipelineBufferBarrierDesc* pBufferBarrier = &m_BufferBarriers[pResource->Buffer.Barriers[b]];

				pBufferBarrier->pBuffer		= pBuffer;
				pBufferBarrier->SizeInBytes = pBuffer->GetDesc().SizeInBytes;
				pBufferBarrier->Offset		= 0;
			}
		}

		m_DirtyDescriptorSetExternalBuffers.insert(pResource);
	}

	void RefactoredRenderGraph::UpdateResourceExternalAccelerationStructure(Resource* pResource, const ResourceUpdateDesc& desc)
	{
		//Update Acceleration Structure
		pResource->AccelerationStructure.pTLAS = desc.ExternalAccelerationStructure.pTLAS;

		m_DirtyDescriptorSetAccelerationStructures.insert(pResource);
	}

	void RefactoredRenderGraph::ExecuteSynchronizationStage(
		SynchronizationStage*	pSynchronizationStage, 
		ICommandAllocator*		pGraphicsCommandAllocator, 
		ICommandList*			pGraphicsCommandList, 
		ICommandAllocator*		pComputeCommandAllocator, 
		ICommandList*			pComputeCommandList,
		ICommandList**			ppFirstExecutionStage,
		ICommandList**			ppSecondExecutionStage)
	{
		static TSet<uint64> synchronizedTextureHandles;
		static TSet<uint64> synchronizedBufferHandles;

		synchronizedTextureHandles.clear();
		synchronizedBufferHandles.clear();

		pGraphicsCommandAllocator->Reset();
		pGraphicsCommandList->Begin(nullptr);

		pComputeCommandAllocator->Reset();
		pComputeCommandList->Begin(nullptr);

		//Texture Synchronizations
		for (auto it = pSynchronizationStage->TextureSynchronizations.begin(); it != pSynchronizationStage->TextureSynchronizations.end(); it++)
		{
			const TextureSynchronization* pTextureSynchronization = &it->second;

			FPipelineStageFlags srcPipelineStage = ConvertShaderStageToPipelineStage(pTextureSynchronization->SrcShaderStage);
			FPipelineStageFlags dstPipelineStage = ConvertShaderStageToPipelineStage(pTextureSynchronization->DstShaderStage);

			for (uint32 b = pTextureSynchronization->BarrierUseFrameIndex * m_BackBufferIndex; b < pTextureSynchronization->Barriers.size(); b += pTextureSynchronization->SameFrameBarrierOffset)
			{
				const PipelineTextureBarrierDesc* pBarrier = &m_TextureBarriers[pTextureSynchronization->Barriers[b]];

				uint64 textureHandle = pBarrier->pTexture->GetHandle();

				if (synchronizedTextureHandles.count(textureHandle) > 0)
					continue;

				synchronizedTextureHandles.insert(textureHandle);

				if (pBarrier->QueueBefore == ECommandQueueType::COMMAND_QUEUE_GRAPHICS)
				{
					if (pBarrier->QueueAfter == ECommandQueueType::COMMAND_QUEUE_COMPUTE)
					{
						//Graphics -> Compute
						pGraphicsCommandList->PipelineTextureBarriers(srcPipelineStage, dstPipelineStage, pBarrier, 1);
						pComputeCommandList->PipelineTextureBarriers(srcPipelineStage, dstPipelineStage, pBarrier, 1);

						(*ppFirstExecutionStage) = pGraphicsCommandList;
						(*ppSecondExecutionStage) = pComputeCommandList;
					}
					else if (pBarrier->QueueAfter == ECommandQueueType::COMMAND_QUEUE_GRAPHICS)
					{
						//Graphics -> Graphics
						pGraphicsCommandList->PipelineTextureBarriers(srcPipelineStage, dstPipelineStage, pBarrier, 1);

						(*ppSecondExecutionStage) = pGraphicsCommandList;
					}
				}
				else if (pBarrier->QueueBefore == ECommandQueueType::COMMAND_QUEUE_COMPUTE)
				{
					if (pBarrier->QueueAfter == ECommandQueueType::COMMAND_QUEUE_GRAPHICS)
					{
						//Compute -> Graphics
						pComputeCommandList->PipelineTextureBarriers(srcPipelineStage, dstPipelineStage, pBarrier, 1);
						pGraphicsCommandList->PipelineTextureBarriers(srcPipelineStage, dstPipelineStage, pBarrier, 1);

						(*ppFirstExecutionStage) = pComputeCommandList;
						(*ppSecondExecutionStage) = pGraphicsCommandList;
					}
					else if (pBarrier->QueueAfter == ECommandQueueType::COMMAND_QUEUE_COMPUTE)
					{
						//Compute -> Compute
						pComputeCommandList->PipelineTextureBarriers(srcPipelineStage, dstPipelineStage, pBarrier, 1);

						(*ppSecondExecutionStage) = pComputeCommandList;
					}
				}
			}
		}

		//Buffer Synchronization
		for (auto it = pSynchronizationStage->BufferSynchronizations.begin(); it != pSynchronizationStage->BufferSynchronizations.end(); it++)
		{
			const BufferSynchronization* pBufferSynchronization = &it->second;

			FPipelineStageFlags srcPipelineStage = ConvertShaderStageToPipelineStage(pBufferSynchronization->SrcShaderStage);
			FPipelineStageFlags dstPipelineStage = ConvertShaderStageToPipelineStage(pBufferSynchronization->DstShaderStage);

			for (uint32 b = pBufferSynchronization->BarrierUseFrameIndex * m_BackBufferIndex; b < pBufferSynchronization->Barriers.size(); b += pBufferSynchronization->SameFrameBarrierOffset)
			{
				const PipelineBufferBarrierDesc* pBarrier = &m_BufferBarriers[pBufferSynchronization->Barriers[b]];

				uint64 bufferHandle = pBarrier->pBuffer->GetHandle();

				if (synchronizedBufferHandles.count(bufferHandle) > 0)
					continue;

				synchronizedBufferHandles.insert(bufferHandle);

				if (pBarrier->QueueBefore == ECommandQueueType::COMMAND_QUEUE_GRAPHICS)
				{
					if (pBarrier->QueueAfter == ECommandQueueType::COMMAND_QUEUE_COMPUTE)
					{
						//Graphics -> Compute
						pGraphicsCommandList->PipelineBufferBarriers(srcPipelineStage, dstPipelineStage, pBarrier, 1);
						pComputeCommandList->PipelineBufferBarriers(srcPipelineStage, dstPipelineStage, pBarrier, 1);

						(*ppFirstExecutionStage) = pGraphicsCommandList;
						(*ppSecondExecutionStage) = pComputeCommandList;
					}
					else if (pBarrier->QueueAfter == ECommandQueueType::COMMAND_QUEUE_GRAPHICS)
					{
						//Graphics -> Graphics
						pGraphicsCommandList->PipelineBufferBarriers(srcPipelineStage, dstPipelineStage, pBarrier, 1);

						(*ppSecondExecutionStage) = pGraphicsCommandList;
					}
				}
				else if (pBarrier->QueueBefore == ECommandQueueType::COMMAND_QUEUE_COMPUTE)
				{
					if (pBarrier->QueueAfter == ECommandQueueType::COMMAND_QUEUE_GRAPHICS)
					{
						//Compute -> Graphics
						pComputeCommandList->PipelineBufferBarriers(srcPipelineStage, dstPipelineStage, pBarrier, 1);
						pGraphicsCommandList->PipelineBufferBarriers(srcPipelineStage, dstPipelineStage, pBarrier, 1);

						(*ppFirstExecutionStage) = pComputeCommandList;
						(*ppSecondExecutionStage) = pGraphicsCommandList;
					}
					else if (pBarrier->QueueAfter == ECommandQueueType::COMMAND_QUEUE_COMPUTE)
					{
						//Compute -> Compute
						pComputeCommandList->PipelineBufferBarriers(srcPipelineStage, dstPipelineStage, pBarrier, 1);

						(*ppSecondExecutionStage) = pComputeCommandList;
					}
				}
			}
		}

		pGraphicsCommandList->End();
		pComputeCommandList->End();
	}

	void RefactoredRenderGraph::ExecuteGraphicsRenderStage(
		RenderStage*		pRenderStage, 
		IPipelineState*		pPipelineState,
		ICommandAllocator*	pGraphicsCommandAllocator, 
		ICommandList*		pGraphicsCommandList, 
		ICommandList**		ppExecutionStage)
	{
		RenderStageParameters* pParameters = &pRenderStage->Parameters;

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
		beginRenderPassDesc.Width				= pParameters->Graphics.Width;
		beginRenderPassDesc.Height				= pParameters->Graphics.Height;
		beginRenderPassDesc.Flags				= flags;
		beginRenderPassDesc.pClearColors		= clearColorDescriptions;
		beginRenderPassDesc.ClearColorCount		= clearColorCount;
		beginRenderPassDesc.Offset.x			= 0;
		beginRenderPassDesc.Offset.y			= 0;

		pGraphicsCommandList->BeginRenderPass(&beginRenderPassDesc);

		Viewport viewport = {};
		viewport.MinDepth	= 0.0f;
		viewport.MaxDepth	= 1.0f;
		viewport.Width		= (float)pParameters->Graphics.Width;
		viewport.Height		= (float)pParameters->Graphics.Height;
		viewport.x			= 0.0f;
		viewport.y			= 0.0f;

		pGraphicsCommandList->SetViewports(&viewport, 0, 1);

		ScissorRect scissorRect = {};
		scissorRect.Width	= pParameters->Graphics.Width;
		scissorRect.Height	= pParameters->Graphics.Height;
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

			IBuffer* pDrawBuffer		= pRenderStage->pMeshIndexBufferResource->Buffer.Buffers[0];
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

	void RefactoredRenderGraph::ExecuteComputeRenderStage(
		RenderStage*		pRenderStage, 
		IPipelineState*		pPipelineState,
		ICommandAllocator*	pComputeCommandAllocator,
		ICommandList*		pComputeCommandList,
		ICommandList**		ppExecutionStage)
	{
		RenderStageParameters* pParameters = &pRenderStage->Parameters;

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

		pComputeCommandList->Dispatch(pParameters->Compute.WorkGroupCountX, pParameters->Compute.WorkGroupCountY, pParameters->Compute.WorkGroupCountZ);

		pComputeCommandList->End();

		(*ppExecutionStage) = pComputeCommandList;
	}

	void RefactoredRenderGraph::ExecuteRayTracingRenderStage(
		RenderStage*		pRenderStage, 
		IPipelineState*		pPipelineState,
		ICommandAllocator*	pComputeCommandAllocator,
		ICommandList*		pComputeCommandList,
		ICommandList**		ppExecutionStage)
	{
		RenderStageParameters* pParameters = &pRenderStage->Parameters;

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

		pComputeCommandList->TraceRays(pParameters->RayTracing.RayTraceWidth, pParameters->RayTracing.RayTraceHeight, pParameters->RayTracing.RayTraceDepth);

		pComputeCommandList->End();

		(*ppExecutionStage) = pComputeCommandList;
	}
}
