#include "Rendering/PipelineStateManager.h"
#include "Rendering/RenderSystem.h"

#include "Rendering/Core/API/ICommandQueue.h"
#include "Rendering/Core/API/IGraphicsDevice.h"
#include "Rendering/Core/API/IPipelineState.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	uint64															PipelineStateManager::s_CurrentPipelineIndex = 0;
	std::unordered_map<uint64, IPipelineState*>						PipelineStateManager::s_PipelineStates;
	std::unordered_map<uint64, GraphicsManagedPipelineStateDesc>	PipelineStateManager::s_GraphicsPipelineStateDescriptions;
	std::unordered_map<uint64, ComputeManagedPipelineStateDesc>		PipelineStateManager::s_ComputePipelineStateDescriptions;
	std::unordered_map<uint64, RayTracingManagedPipelineStateDesc>	PipelineStateManager::s_RayTracingPipelineStateDescriptions;

	bool PipelineStateManager::Init()
	{
		return true;
	}

	bool PipelineStateManager::Release()
	{
		for (auto it = s_PipelineStates.begin(); it != s_PipelineStates.end(); it++)
		{
			SAFERELEASE(it->second);
		}

		s_PipelineStates.clear();

		return true;
	}

	uint64 PipelineStateManager::CreateGraphicsPipelineState(const GraphicsManagedPipelineStateDesc* pDesc)
	{
		GraphicsPipelineStateDesc pipelineStateDesc = {};
		FillGraphicsPipelineStateDesc(&pipelineStateDesc, pDesc);

		IPipelineState* pPipelineState = RenderSystem::GetDevice()->CreateGraphicsPipelineState(&pipelineStateDesc);
		if (pPipelineState)
		{
			uint64 pipelineIndex = s_CurrentPipelineIndex++;
			s_GraphicsPipelineStateDescriptions[pipelineIndex] 	= *pDesc;
			s_PipelineStates[pipelineIndex] 					= pPipelineState;
			return pipelineIndex;
		}
		else
		{
			D_LOG_ERROR("[PipelineStateManager]: PipelineState is nullptr");
			return 0;
		}
	}

	uint64 PipelineStateManager::CreateComputePipelineState(const ComputeManagedPipelineStateDesc* pDesc)
	{
		ComputePipelineStateDesc pipelineStateDesc = {};
		FillComputePipelineStateDesc(&pipelineStateDesc, pDesc);

		IPipelineState* pPipelineState = RenderSystem::GetDevice()->CreateComputePipelineState(&pipelineStateDesc);
		if (pPipelineState)
		{
			uint64 pipelineIndex = s_CurrentPipelineIndex++;
			s_ComputePipelineStateDescriptions[pipelineIndex] 	= *pDesc;
			s_PipelineStates[pipelineIndex] 					= pPipelineState;
			return pipelineIndex;
		}
		else
		{
			D_LOG_ERROR("[PipelineStateManager]: PipelineState is nullptr");
			return 0;
		}
	}

	uint64 PipelineStateManager::CreateRayTracingPipelineState(const RayTracingManagedPipelineStateDesc* pDesc)
	{
		RayTracingPipelineStateDesc pipelineStateDesc = {};
		FillRayTracingPipelineStateDesc(&pipelineStateDesc, pDesc);

		IPipelineState* pPipelineState = RenderSystem::GetDevice()->CreateRayTracingPipelineState(&pipelineStateDesc);
		if (pPipelineState)
		{
			uint64 pipelineIndex = s_CurrentPipelineIndex++;
			s_RayTracingPipelineStateDescriptions[pipelineIndex] 	= *pDesc;
			s_PipelineStates[pipelineIndex] 						= pPipelineState;
			return pipelineIndex;
		}
		else
		{
			D_LOG_ERROR("[PipelineStateManager]: PipelineState is nullptr");
			return 0;
		}
	}

	void PipelineStateManager::ReleasePipelineState(uint64 id)
	{
		auto it = s_PipelineStates.find(id);

		if (it != s_PipelineStates.end())
		{
			VALIDATE(it->second != nullptr);
			
			switch (it->second->GetType())
			{
			case EPipelineStateType::PIPELINE_GRAPHICS:			s_GraphicsPipelineStateDescriptions.erase(id); break;
			case EPipelineStateType::PIPELINE_COMPUTE:			s_ComputePipelineStateDescriptions.erase(id); break;
			case EPipelineStateType::PIPELINE_RAY_TRACING:		s_RayTracingPipelineStateDescriptions.erase(id); break;
			}

			SAFERELEASE(it->second);
			s_PipelineStates.erase(id);
		}
	}

	IPipelineState* PipelineStateManager::GetPipelineState(uint64 id)
	{
		auto it = s_PipelineStates.find(id);

		if (it != s_PipelineStates.end())
		{
			return it->second;
		}

		return nullptr;
	}

	void PipelineStateManager::ReloadPipelineStates()
	{
		for (auto it = s_PipelineStates.begin(); it != s_PipelineStates.end(); it++)
		{
			IPipelineState* pNewPipelineState = nullptr;
			
			VALIDATE(it->second != nullptr);
			
			switch (it->second->GetType())
			{
				case EPipelineStateType::PIPELINE_GRAPHICS:
				{
					GraphicsPipelineStateDesc pipelineStateDesc = {};
					FillGraphicsPipelineStateDesc(&pipelineStateDesc, &s_GraphicsPipelineStateDescriptions[it->first]);
					pNewPipelineState = RenderSystem::GetDevice()->CreateGraphicsPipelineState(&pipelineStateDesc);
					break;
				}
				case EPipelineStateType::PIPELINE_COMPUTE:
				{
					ComputePipelineStateDesc pipelineStateDesc = {};
					FillComputePipelineStateDesc(&pipelineStateDesc, &s_ComputePipelineStateDescriptions[it->first]);
					pNewPipelineState = RenderSystem::GetDevice()->CreateComputePipelineState(&pipelineStateDesc);
					break;
				}
				case EPipelineStateType::PIPELINE_RAY_TRACING:
				{
					RayTracingPipelineStateDesc pipelineStateDesc = {};
					FillRayTracingPipelineStateDesc(&pipelineStateDesc, &s_RayTracingPipelineStateDescriptions[it->first]);
					pNewPipelineState = RenderSystem::GetDevice()->CreateRayTracingPipelineState(&pipelineStateDesc);
					break;
				}
			}

			SAFERELEASE(it->second);
			it->second = pNewPipelineState;
		}
	}

	void PipelineStateManager::FillGraphicsPipelineStateDesc(GraphicsPipelineStateDesc* pDstDesc, const GraphicsManagedPipelineStateDesc* pSrcDesc)
	{
		pDstDesc->pName						= pSrcDesc->pName;
		pDstDesc->pRenderPass				= pSrcDesc->pRenderPass;
		pDstDesc->pPipelineLayout			= pSrcDesc->pPipelineLayout;
		
		memcpy(pDstDesc->pVertexInputBindings, pSrcDesc->pVertexInputBindings, MAX_VERTEX_INPUT_ATTACHMENTS * sizeof(VertexInputBindingDesc));

		pDstDesc->VertexInputBindingCount	= pSrcDesc->VertexInputBindingCount;
		
		memcpy(pDstDesc->pBlendAttachmentStates, pSrcDesc->pBlendAttachmentStates, MAX_COLOR_ATTACHMENTS * sizeof(BlendAttachmentStateDesc));
		
		pDstDesc->BlendAttachmentStateCount = pSrcDesc->BlendAttachmentStateCount;
		pDstDesc->pTaskShader				= ResourceManager::GetShader(pSrcDesc->TaskShader);
		pDstDesc->pMeshShader				= ResourceManager::GetShader(pSrcDesc->MeshShader);
		pDstDesc->pVertexShader				= ResourceManager::GetShader(pSrcDesc->VertexShader);
		pDstDesc->pGeometryShader			= ResourceManager::GetShader(pSrcDesc->GeometryShader);
		pDstDesc->pHullShader				= ResourceManager::GetShader(pSrcDesc->HullShader);
		pDstDesc->pDomainShader				= ResourceManager::GetShader(pSrcDesc->DomainShader);
		pDstDesc->pPixelShader				= ResourceManager::GetShader(pSrcDesc->PixelShader);
	}

	void PipelineStateManager::FillComputePipelineStateDesc(ComputePipelineStateDesc* pDstDesc, const ComputeManagedPipelineStateDesc* pSrcDesc)
	{
		pDstDesc->pName				= pSrcDesc->pName;
		pDstDesc->pPipelineLayout	= pSrcDesc->pPipelineLayout;
		pDstDesc->pShader			= ResourceManager::GetShader(pSrcDesc->Shader);
	}

	void PipelineStateManager::FillRayTracingPipelineStateDesc(RayTracingPipelineStateDesc* pDstDesc, const RayTracingManagedPipelineStateDesc* pSrcDesc)
	{
		pDstDesc->pName							= pSrcDesc->pName;
		pDstDesc->pPipelineLayout				= pSrcDesc->pPipelineLayout;
		pDstDesc->pPipelineLayout				= pSrcDesc->pPipelineLayout;
		pDstDesc->MaxRecursionDepth				= pSrcDesc->MaxRecursionDepth;
		pDstDesc->MissShaderCount				= pSrcDesc->MissShaderCount;
		pDstDesc->ClosestHitShaderCount			= pSrcDesc->ClosestHitShaderCount;
		pDstDesc->pRaygenShader					= ResourceManager::GetShader(pSrcDesc->RaygenShader);

		for (uint32 i = 0; i < pDstDesc->MissShaderCount; i++)
		{
			pDstDesc->ppMissShaders[i]			= ResourceManager::GetShader(pSrcDesc->pMissShaders[i]);
		}

		for (uint32 i = 0; i < pDstDesc->ClosestHitShaderCount; i++)
		{
			pDstDesc->ppClosestHitShaders[i]	= ResourceManager::GetShader(pSrcDesc->pClosestHitShaders[i]);
		}
	}
}
