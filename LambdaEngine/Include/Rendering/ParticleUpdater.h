#pragma once

#include "RenderGraphTypes.h"
#include "CustomRenderer.h"

#include "Rendering/Core/API/PipelineContext.h"

namespace LambdaEngine
{
	class ParticleUpdater : public CustomRenderer
	{
		struct PushConstantData
		{
			float delta;
			uint32 particleCount;
		};

	public:
		ParticleUpdater();
		~ParticleUpdater();

		bool Init();

		void SetCurrentParticleCount(uint32 particleCount, uint32 emitterCount) { m_ParticleCount = particleCount; m_EmitterCount = emitterCount; m_PushConstant.particleCount = m_ParticleCount; };

		virtual bool RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc) override final;

		virtual void Update(Timestamp delta, uint32 modFrameIndex, uint32 backBufferIndex) override final;
		virtual void UpdateTextureResource(const String& resourceName, const TextureView* const* ppPerImageTextureViews, const TextureView* const* ppPerSubImageTextureViews, uint32 imageCount, uint32 subImageCount, bool backBufferBound) override final;
		virtual void UpdateBufferResource(const String& resourceName, const Buffer* const* ppBuffers, uint64* pOffsets, uint64* pSizesInBytes, uint32 count, bool backBufferBound) override final;

		virtual void Render(
			uint32 modFrameIndex,
			uint32 backBufferIndex,
			CommandList** ppFirstExecutionStage,
			CommandList** ppSecondaryExecutionStage,
			bool Sleeping)	override final;

		FORCEINLINE virtual FPipelineStageFlag GetFirstPipelineStage() const override final { return FPipelineStageFlag::PIPELINE_STAGE_FLAG_COMPUTE_SHADER; }
		FORCEINLINE virtual FPipelineStageFlag GetLastPipelineStage() const override final { return FPipelineStageFlag::PIPELINE_STAGE_FLAG_COMPUTE_SHADER; }

		virtual const String& GetName() const override final
		{
			static String name = RENDER_GRAPH_PARTICLE_UPDATE_STAGE_NAME;
			return name;
		}

	private:
		bool CreatePipelineLayout();
		bool CreateDescriptorSets();
		bool CreateShaders();
		bool CreateCommandLists();

	private:
		bool								m_Initilized = false;
		uint32								m_ParticleCount;
		uint32								m_EmitterCount;

		PushConstantData					m_PushConstant;

		TSharedRef<Sampler>					m_Sampler = nullptr;
		TSharedRef<DescriptorHeap>			m_DescriptorHeap			= nullptr;
		PipelineContext						m_UpdatePipeline;

		uint32								m_BackBufferCount = 0;

		CommandAllocator**					m_ppComputeCommandAllocators = nullptr;
		CommandList**						m_ppComputeCommandLists = nullptr;

	private:
		static ParticleUpdater* s_pInstance;
	};
}

