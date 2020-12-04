#pragma once
#include "Rendering/CustomRenderer.h"

#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/DescriptorHeap.h"
#include "Rendering/Core/API/DescriptorSet.h"
#include "Rendering/Core/API/PipelineLayout.h"

namespace LambdaEngine
{
	class AARenderer : public CustomRenderer
	{
	public:
		AARenderer();
		~AARenderer() = default;

		virtual bool Init() override final;

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
			static String name = "AA_RENDERER";
			return name;
		}

	private:
		const Texture*		m_pIntermediateOutput		= nullptr;
		const TextureView*	m_pIntermediateOutputView	= nullptr;
		const Texture*		m_pBackBuffer				= nullptr;
		const TextureView*	m_pBackBufferView			= nullptr;

		TArray<TSharedRef<Texture>>		TAAHistory;
		TArray<TSharedRef<TextureView>>	TAAHistoryViews;

		bool m_NeedsUpdate = true;

		TArray<TSharedRef<CommandList>>			m_GraphicsCommandLists;
		TArray<TSharedRef<CommandAllocator>>	m_GraphicsCommandAllocators;

		uint64 m_TAAState;
		TSharedRef<PipelineLayout>	m_TAALayout;
		uint64 m_FXAAState;
		TSharedRef<PipelineLayout>	m_FXAALayout;

		TSharedRef<DescriptorHeap>			m_DescriptorHeap;
		TArray<TSharedRef<DescriptorSet>>	m_TAADescriptorSets;
		TArray<TSharedRef<DescriptorSet>>	m_FXAADescriptorSets;
	};
}