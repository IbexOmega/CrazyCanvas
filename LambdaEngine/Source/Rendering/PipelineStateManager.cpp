#include "Rendering/PipelineStateManager.h"
#include "Rendering/RenderAPI.h"

#include "Rendering/Core/API/CommandQueue.h"
#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/PipelineState.h"

#include "Log/Log.h"

#include "Application/API/Events/EventQueue.h"

namespace LambdaEngine
{
	uint64													PipelineStateManager::s_CurrentPipelineIndex = 1;
	THashTable<uint64, TSharedRef<PipelineState>>			PipelineStateManager::s_PipelineStates;
	THashTable<uint64, ManagedGraphicsPipelineStateDesc>	PipelineStateManager::s_GraphicsPipelineStateDescriptions;
	THashTable<uint64, ManagedComputePipelineStateDesc>		PipelineStateManager::s_ComputePipelineStateDescriptions;
	THashTable<uint64, ManagedRayTracingPipelineStateDesc>	PipelineStateManager::s_RayTracingPipelineStateDescriptions;

	/*
	* Managed Shader Module
	*/
	ManagedShaderModule::operator ShaderModuleDesc() const noexcept
	{ 
		ShaderModuleDesc desc = {};
		desc.pShader			= ResourceManager::GetShader(ShaderGUID);
		desc.ShaderConstants	= ShaderConstants;
		
		return desc;
	}

	/*
	* Graphics
	*/
	GraphicsPipelineStateDesc ManagedGraphicsPipelineStateDesc::GetDesc() const noexcept
	{
		GraphicsPipelineStateDesc desc = { };
		desc.DebugName			= DebugName;
		
		desc.InputAssembly		= InputAssembly;
		desc.InputLayout		= InputLayout;
		desc.DepthStencilState	= DepthStencilState;
		desc.BlendState			= BlendState;
		desc.RasterizerState	= RasterizerState;
		desc.SampleCount		= SampleCount;
		desc.SampleMask			= SampleMask;
		desc.Subpass			= Subpass;
		desc.ExtraDynamicState	= ExtraDynamicState;

		desc.pRenderPass		= RenderPass.Get();
		desc.pPipelineLayout	= PipelineLayout.Get();
		
		// Vertex-Shader Pipeline
		desc.VertexShader		= VertexShader;
		desc.HullShader			= HullShader;
		desc.DomainShader		= DomainShader;
		desc.GeometryShader		= GeometryShader;
		
		// Mesh-Shader Pipeline
		desc.MeshShader			= MeshShader;
		desc.TaskShader			= TaskShader;
		
		// Common-Shader Pipeline
		desc.PixelShader		= PixelShader;

		return desc;
	}

	/*
	* Compute
	*/
	ComputePipelineStateDesc ManagedComputePipelineStateDesc::GetDesc() const noexcept
	{
		ComputePipelineStateDesc desc = { };
		desc.DebugName			= DebugName;
		desc.pPipelineLayout	= PipelineLayout.Get();
		desc.Shader				= Shader;
		
		return desc;
	}

	/*
	* RayTracing
	*/
	RayTracingPipelineStateDesc ManagedRayTracingPipelineStateDesc::GetDesc() const noexcept
	{
		RayTracingPipelineStateDesc desc = { };
		desc.DebugName			= DebugName;
		desc.MaxRecursionDepth	= MaxRecursionDepth;
		desc.RaygenShader		= RaygenShader;
		desc.pPipelineLayout	= PipelineLayout.Get();
		
		desc.MissShaders.Resize(MissShaders.GetSize());
		for (uint32 i = 0; i < MissShaders.GetSize(); i++)
		{
			desc.MissShaders[i] = MissShaders[i];
		}

		desc.ClosestHitShaders.Resize(ClosestHitShaders.GetSize());
		for (uint32 i = 0; i < ClosestHitShaders.GetSize(); i++)
		{
			desc.ClosestHitShaders[i] = ClosestHitShaders[i];
		}

		return desc;
	}

	/*
	* PipelineStateManager
	*/

	bool PipelineStateManager::Init()
	{
		EventQueue::RegisterEventHandler<PipelineStateRecompileEvent>(&OnPipelineStateRecompileEvent);

		return true;
	}

	bool PipelineStateManager::Release()
	{
		EventQueue::UnregisterEventHandler<PipelineStateRecompileEvent>(&OnPipelineStateRecompileEvent);

		s_GraphicsPipelineStateDescriptions.clear();
		s_ComputePipelineStateDescriptions.clear();
		s_RayTracingPipelineStateDescriptions.clear();
		s_PipelineStates.clear();
		return true;
	}

	uint64 PipelineStateManager::CreateGraphicsPipelineState(const ManagedGraphicsPipelineStateDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		GraphicsPipelineStateDesc pipelineDesc = pDesc->GetDesc();
		PipelineState* pPipelineState = RenderAPI::GetDevice()->CreateGraphicsPipelineState(&pipelineDesc);
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

	uint64 PipelineStateManager::CreateComputePipelineState(const ManagedComputePipelineStateDesc* pDesc)
	{
		ComputePipelineStateDesc pipelineDesc = pDesc->GetDesc();
		PipelineState* pPipelineState = RenderAPI::GetDevice()->CreateComputePipelineState(&pipelineDesc);
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

	uint64 PipelineStateManager::CreateRayTracingPipelineState(const ManagedRayTracingPipelineStateDesc* pDesc)
	{
		RayTracingPipelineStateDesc pipelineDesc = pDesc->GetDesc();
		PipelineState*	pPipelineState	= RenderAPI::GetDevice()->CreateRayTracingPipelineState(&pipelineDesc);
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
			case EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS:			s_GraphicsPipelineStateDescriptions.erase(id);		break;
			case EPipelineStateType::PIPELINE_STATE_TYPE_COMPUTE:			s_ComputePipelineStateDescriptions.erase(id);		break;
			case EPipelineStateType::PIPELINE_STATE_TYPE_RAY_TRACING:		s_RayTracingPipelineStateDescriptions.erase(id);	break;
			}

			s_PipelineStates.erase(id);
		}
	}

	PipelineState* PipelineStateManager::GetPipelineState(uint64 id)
	{
		auto it = s_PipelineStates.find(id);
		if (it != s_PipelineStates.end())
		{
			return it->second.Get();
		}

		return nullptr;
	}

	bool PipelineStateManager::OnPipelineStateRecompileEvent(const PipelineStateRecompileEvent& event)
	{
		UNREFERENCED_VARIABLE(event);

		RenderAPI::GetGraphicsQueue()->Flush();
		RenderAPI::GetComputeQueue()->Flush();
		RenderAPI::GetCopyQueue()->Flush();

		for (auto it = s_PipelineStates.begin(); it != s_PipelineStates.end(); it++)
		{
			PipelineState* pNewPipelineState = nullptr;
			
			VALIDATE(it->second != nullptr);
			
			switch (it->second->GetType())
			{
				case EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS:
				{
					const ManagedGraphicsPipelineStateDesc* pPipelineDesc = &s_GraphicsPipelineStateDescriptions[it->first];
					GraphicsPipelineStateDesc pipelineDesc = pPipelineDesc->GetDesc();
					
					pNewPipelineState = RenderAPI::GetDevice()->CreateGraphicsPipelineState(&pipelineDesc);
					break;
				}
				case EPipelineStateType::PIPELINE_STATE_TYPE_COMPUTE:
				{
					const ManagedComputePipelineStateDesc* pPipelineDesc = &s_ComputePipelineStateDescriptions[it->first];
					ComputePipelineStateDesc pipelineDesc = pPipelineDesc->GetDesc();
					
					pNewPipelineState = RenderAPI::GetDevice()->CreateComputePipelineState(&pipelineDesc);
					break;
				}
				case EPipelineStateType::PIPELINE_STATE_TYPE_RAY_TRACING:
				{
					const ManagedRayTracingPipelineStateDesc* pPipelineDesc = &s_RayTracingPipelineStateDescriptions[it->first];
					RayTracingPipelineStateDesc pipelineDesc = pPipelineDesc->GetDesc();

					pNewPipelineState			= RenderAPI::GetDevice()->CreateRayTracingPipelineState(&pipelineDesc);
					break;
				}
			}

			it->second = pNewPipelineState;
		}

		PipelineStatesRecompiledEvent recompiledEvent = {};
		EventQueue::SendEventImmediate(recompiledEvent);

		return true;
	}
}
