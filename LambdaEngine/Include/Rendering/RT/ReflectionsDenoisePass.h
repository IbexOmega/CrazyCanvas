#pragma once

#include "Rendering/CustomRenderer.h"

#include "Rendering/Core/API/CommandList.h"
#include "Rendering/RenderGraphTypes.h"

namespace LambdaEngine
{
	class CommandAllocator;
	class CommandList;
	class PipelineLayout;
	class DescriptorHeap;
	class DescriptorSet;
	class DeviceChild;

	class ReflectionsDenoisePass : public CustomRenderer
	{
		static constexpr const uint32 DENOISED_REFLECTIONS_BARRIER_INDEX = 0;
		static constexpr const uint32 INTERMEDIATE_OUTPUT_BARRIER_INDEX = 1;

		static constexpr const uint32 WORKGROUP_WIDTH_HEIGHT = 16;

	public:
		DECL_REMOVE_COPY(ReflectionsDenoisePass);
		DECL_REMOVE_MOVE(ReflectionsDenoisePass);

		ReflectionsDenoisePass() = default;
		~ReflectionsDenoisePass();

		virtual bool Init() override final;
		virtual bool RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc) override final;

		virtual void PreTexturesDescriptorSetWrite() override final;

		virtual void UpdateTextureResource(
			const String& resourceName,
			const TextureView* const* ppPerImageTextureViews,
			const TextureView* const* ppPerSubImageTextureViews,
			const Sampler* const* ppPerImageSamplers,
			uint32 imageCount,
			uint32 subImageCount,
			bool backBufferBound) override final;

		virtual void Render(
			uint32 modFrameIndex,
			uint32 backBufferIndex,
			CommandList** ppFirstExecutionStage,
			CommandList** ppSecondaryExecutionStage,
			bool sleeping) override final;

		FORCEINLINE virtual FPipelineStageFlag GetFirstPipelineStage() const override final { return FPipelineStageFlag::PIPELINE_STAGE_FLAG_COMPUTE_SHADER; }
		FORCEINLINE virtual FPipelineStageFlag GetLastPipelineStage() const override final { return FPipelineStageFlag::PIPELINE_STAGE_FLAG_COMPUTE_SHADER; }

		FORCEINLINE virtual const String& GetName() const override final
		{
			static String name = REFLECTIONS_DENOISE_PASS;
			return name;
		}

	private:
		bool Release();

		bool CreateCommandLists();
		bool CreatePipelineLayouts();
		bool CreateDescriptorSets();
		bool CreatePipelineStates();
		bool CreateBarriers();

	private:
		bool m_Initialized = false;
		uint32 m_ModFrameIndex = 0;
		uint32 m_BackBufferCount = 0;

		CommandAllocator** m_ppCommandAllocators = nullptr;
		CommandList** m_ppCommandLists = nullptr;

		PipelineLayout* m_pSpatioTemporalPassPipelineLayout = nullptr;
		PipelineLayout* m_pGaussianPassPipelineLayout = nullptr;

		DescriptorHeap* m_pDescriptorHeap = nullptr;
		DescriptorSet* m_pSpatioTemporalPassDescriptorSet = nullptr;
		DescriptorSet* m_pHorizontalGaussianPassDescriptorSet = nullptr;
		DescriptorSet* m_pVerticalGaussianPassDescriptorSet = nullptr;

		uint64 m_SpatioTemporalPassPipelineStateID = 0;
		uint64 m_HorizontalGaussianPassPipelineStateID = 0;
		uint64 m_VerticalGaussianPassPipelineStateID = 0;

		TArray<DeviceChild*>* m_pDeviceResourcesToRemove = nullptr;

		PipelineTextureBarrierDesc m_FirstStagePipelineBarriers[2];
		PipelineTextureBarrierDesc m_SecondStagePipelineBarriers[2];

		uint32 m_DispatchWidth = 0;
		uint32 m_DispatchHeight = 0;
	};
}