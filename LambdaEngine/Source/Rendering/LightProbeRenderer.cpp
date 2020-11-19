#include "Rendering/LightProbeRenderer.h"
#include "Rendering/PipelineStateManager.h"
#include "Rendering/RenderAPI.h"
#include "Rendering/Core/API/PipelineState.h"
#include "Rendering/Core/API/PipelineLayout.h"

namespace LambdaEngine
{
	LightProbeRenderer::LightProbeRenderer()
		: CustomRenderer()
		, m_pRenderGraph(nullptr)
		, m_ModFrameIndex()
		, m_BackBufferCount()
		, m_ComputeCommandLists()
		, m_ComputeCommandAllocators()
		, m_SpecularFilterState(0)
		, m_SpecularFilterLayout(nullptr)
		, m_DiffuseFilterState(0)
		, m_DiffuseFilterLayout(nullptr)
	{
	}

	LightProbeRenderer::~LightProbeRenderer()
	{
	}

	bool LightProbeRenderer::Init()
	{
		{
			PipelineLayoutDesc specularLayoutDesc;
			specularLayoutDesc.DebugName = "Specular Cubemap Filter Layout";

			m_SpecularFilterLayout = RenderAPI::GetDevice()->CreatePipelineLayout(&specularLayoutDesc);
			if (!m_SpecularFilterLayout)
			{
				LOG_ERROR("[LightProbeRenderer]: Failed to create Specular Cubemap Filter Layout");
				return false;
			}
		}

		{
			ManagedComputePipelineStateDesc specularStateDesc;
			specularStateDesc.DebugName			= "Specular Cubemap Filter State";
			specularStateDesc.PipelineLayout	= m_SpecularFilterLayout;

			m_SpecularFilterState = PipelineStateManager::CreateComputePipelineState(&specularStateDesc);
			if (m_SpecularFilterState == 0)
			{
				return false;
			}
		}

		{
			PipelineLayoutDesc diffuseLayoutDesc;
			diffuseLayoutDesc.DebugName = "Diffuse Cubemap Filter Layout";

			m_DiffuseFilterLayout = RenderAPI::GetDevice()->CreatePipelineLayout(&diffuseLayoutDesc);
			if (!m_DiffuseFilterLayout)
			{
				LOG_ERROR("[LightProbeRenderer]: Failed to create Diffuse Cubemap Filter Layout");
				return false;
			}
		}

		{
			ManagedComputePipelineStateDesc diffuseStateDesc;
			diffuseStateDesc.DebugName		= "Diffuse Cubemap Filter State";
			diffuseStateDesc.PipelineLayout	= m_DiffuseFilterLayout;

			m_DiffuseFilterState = PipelineStateManager::CreateComputePipelineState(&diffuseStateDesc);
			if (m_DiffuseFilterState == 0)
			{
				return false;
			}
		}

		return true;
	}

	bool LightProbeRenderer::RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc)
	{
		return true;
	}

	bool LightProbeRenderer::RenderGraphPostInit()
	{
		return true;
	}

	void LightProbeRenderer::Update(Timestamp delta, uint32 modFrameIndex, uint32 backBufferIndex)
	{
	}

	void LightProbeRenderer::Render(uint32 modFrameIndex, uint32 backBufferIndex, CommandList** ppFirstExecutionStage, CommandList** ppSecondaryExecutionStage, bool sleeping)
	{
	}
}