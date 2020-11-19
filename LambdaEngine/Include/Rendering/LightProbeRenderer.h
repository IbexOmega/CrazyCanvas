#pragma once
#include "Rendering/CustomRenderer.h"

#include "Rendering/Core/API/CommandList.h"

namespace LambdaEngine
{
	class LightProbeRenderer : public CustomRenderer
	{
	public:
		LightProbeRenderer();
		~LightProbeRenderer();

		virtual bool Init() override final;
		virtual bool RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc) override final;
		virtual bool RenderGraphPostInit() override final;

		virtual void Update(Timestamp delta, uint32 modFrameIndex, uint32 backBufferIndex) override final;

		virtual void Render(
			uint32 modFrameIndex,
			uint32 backBufferIndex,
			CommandList** ppFirstExecutionStage,
			CommandList** ppSecondaryExecutionStage,
			bool sleeping) override final;

		FORCEINLINE virtual FPipelineStageFlag GetFirstPipelineStage() const override final 
		{ 
			return FPipelineStageFlag::PIPELINE_STAGE_FLAG_COMPUTE_SHADER; 
		}

		FORCEINLINE virtual FPipelineStageFlag GetLastPipelineStage() const override final 
		{ 
			return FPipelineStageFlag::PIPELINE_STAGE_FLAG_COMPUTE_SHADER; 
		}

		FORCEINLINE virtual const String& GetName() const override final
		{
			static String name = "AS_BUILDER";
			return name;
		}

	private:
		RenderGraph* m_pRenderGraph = nullptr;
		uint32 m_ModFrameIndex		= 0;
		uint32 m_BackBufferCount	= 0;

		TArray<TSharedRef<CommandList>>			m_ComputeCommandLists;
		TArray<TSharedRef<CommandAllocator>>	m_ComputeCommandAllocators;

		//Utility
		TArray<DeviceChild*>* m_pResourcesToRemove = nullptr;

		//Threading
		mutable SpinLock m_Lock;
	};
}