#pragma once

#include "RenderGraphTypes.h"
#include "ICustomRenderer.h"

#include "Rendering/Core/API/DescriptorCache.h"

namespace LambdaEngine
{

	

	class ParticleUpdater : public ICustomRenderer
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

		virtual bool RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc) override final;

		virtual void PreBuffersDescriptorSetWrite()		override final;
		virtual void PreTexturesDescriptorSetWrite()	override final;

		virtual void Update(Timestamp delta, uint32 modFrameIndex, uint32 backBufferIndex) override final;
		virtual void UpdateTextureResource(const String& resourceName, const TextureView* const* ppPerImageTextureViews, const TextureView* const* ppPerSubImageTextureViews, uint32 imageCount, uint32 subImageCount, bool backBufferBound) override final;
		virtual void UpdateBufferResource(const String& resourceName, const Buffer* const* ppBuffers, uint64* pOffsets, uint64* pSizesInBytes, uint32 count, bool backBufferBound) override final;
		virtual void UpdateAccelerationStructureResource(const String& resourceName, const AccelerationStructure* pAccelerationStructure) override final;
		virtual void UpdateDrawArgsResource(const String& resourceName, const DrawArg* pDrawArgs, uint32 count)  override final;

		virtual void Render(
			uint32 modFrameIndex,
			uint32 backBufferIndex,
			CommandList** ppFirstExecutionStage,
			CommandList** ppSecondaryExecutionStage,
			bool Sleeping)	override final;

		FORCEINLINE virtual FPipelineStageFlag GetFirstPipelineStage()	override final { return FPipelineStageFlag::PIPELINE_STAGE_FLAG_COMPUTE_SHADER; }
		FORCEINLINE virtual FPipelineStageFlag GetLastPipelineStage()	override final { return FPipelineStageFlag::PIPELINE_STAGE_FLAG_COMPUTE_SHADER; }

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
		bool CreatePipelineState();

	private:
		bool								m_Initilized = false;

		GUID_Lambda							m_ComputeShaderGUID = 0;

		uint64								m_PipelineStateID = 0;
		TSharedRef<PipelineLayout>			m_PipelineLayout = nullptr;
		TSharedRef<DescriptorHeap>			m_DescriptorHeap = nullptr;

		// Descriptor sets
		TSharedRef<DescriptorSet>			m_InstanceDescriptorSet;
		DescriptorCache						m_DescriptorCache;

		uint32								m_BackBufferCount = 0;

		CommandAllocator**					m_ppComputeCommandAllocators = nullptr;
		CommandList**						m_ppComputeCommandLists = nullptr;

	private:
		static ParticleUpdater* s_pInstance;
	};
}

