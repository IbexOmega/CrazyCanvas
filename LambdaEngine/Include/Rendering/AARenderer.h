#pragma once
#include "Rendering/CustomRenderer.h"

#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/DescriptorHeap.h"
#include "Rendering/Core/API/DescriptorSet.h"
#include "Rendering/Core/API/PipelineLayout.h"

namespace LambdaEngine
{
	enum class EAAMode
	{
		AAMODE_NONE	= 0,
		AAMODE_FXAA	= 1,
		AAMODE_TAA	= 2,
	};

	class AARenderer : public CustomRenderer
	{
	public:
		AARenderer();
		~AARenderer();

		virtual bool Init() override final;

		virtual bool RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc) override final;

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
			return FPipelineStageFlag::PIPELINE_STAGE_FLAG_VERTEX_SHADER;
		}

		FORCEINLINE virtual FPipelineStageFlag GetLastPipelineStage() const override final 
		{ 
			return FPipelineStageFlag::PIPELINE_STAGE_FLAG_PIXEL_SHADER; 
		}

		FORCEINLINE virtual const String& GetName() const override final
		{
			static String name = "AA_RENDERER";
			return name;
		}

	public:
		static FORCEINLINE AARenderer* GetInstance()
		{
			return s_pInstance;
		}

	private:
		inline static AARenderer* s_pInstance = 0;

	private:
		const Texture*		m_pIntermediateOutput		= nullptr;
		const TextureView*	m_pIntermediateOutputView	= nullptr;

		TArray<TSharedRef<const TextureView>> m_BackBuffers;

		TArray<TSharedRef<Texture>>		m_TAAHistory;
		TArray<TSharedRef<TextureView>>	m_TAAHistoryViews;

		TSharedRef<RenderPass> m_RenderPass;

		bool m_NeedsUpdate = true;
		EAAMode m_AAMode;

		TArray<TSharedRef<CommandList>>			m_CommandLists;
		TArray<TSharedRef<CommandAllocator>>	m_CommandAllocators;

		uint64 m_TAAState;
		TSharedRef<PipelineLayout>	m_TAALayout;
		uint64 m_FXAAState;
		TSharedRef<PipelineLayout>	m_FXAALayout;

		TSharedRef<DescriptorHeap>			m_DescriptorHeap;
		TArray<TSharedRef<DescriptorSet>>	m_TAADescriptorSets;
		TArray<TSharedRef<DescriptorSet>>	m_FXAADescriptorSets;
	};
}