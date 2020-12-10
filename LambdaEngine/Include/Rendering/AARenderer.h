#pragma once
#include "Rendering/CustomRenderer.h"

#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/DescriptorHeap.h"
#include "Rendering/Core/API/DescriptorSet.h"
#include "Rendering/Core/API/PipelineLayout.h"

#include "Application/API/Events/WindowEvents.h"
#include "Application/API/Events/KeyEvents.h"

namespace LambdaEngine
{
	enum class EAAMode
	{
		AAMODE_NONE	= 0,
		AAMODE_FXAA	= 1,
		AAMODE_TAA	= 2,
	};

	/*
	* AARenderer
	*/

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

		virtual void UpdateBufferResource(
			const String& resourceName,
			const Buffer* const* ppBuffers,
			uint64* pOffsets,
			uint64* pSizesInBytes,
			uint32 count,
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

		FORCEINLINE void SetAAMode(EAAMode aaMode)
		{
			m_AAMode = aaMode;
		}

		FORCEINLINE EAAMode GetAAMode() const
		{
			return m_AAMode;
		}

	public:
		static FORCEINLINE AARenderer* GetInstance()
		{
			return s_pInstance;
		}

	private:
		inline static AARenderer* s_pInstance = 0;

	private:
		TSharedRef<const Buffer>		m_PerFrameBuffer;
		TSharedRef<const Texture>		m_IntermediateOutput;
		TSharedRef<const TextureView>	m_IntermediateOutputView;
		TSharedRef<const Texture>		m_Velocity;
		TSharedRef<const TextureView>	m_VelocityView;
		TSharedRef<const Texture>		m_Depth;
		TSharedRef<const TextureView>	m_DepthView;
		TArray<TSharedRef<Texture>>		m_BackBuffers;
		TArray<TSharedRef<TextureView>>	m_BackBufferViews;

		TArray<TSharedRef<const Texture>>		m_TAAHistory;
		TArray<TSharedRef<const TextureView>>	m_TAAHistoryViews;
		uint32 m_Width;
		uint32 m_Height;
		uint64 m_Tick = 0;

		bool m_NeedsUpdate = true;
		EAAMode m_AAMode;

		TArray<TSharedRef<CommandList>>			m_CommandLists;
		TArray<TSharedRef<CommandAllocator>>	m_CommandAllocators;

		uint64 m_BlitState;
		TSharedRef<RenderPass> m_RenderPass;
		TSharedRef<RenderPass> m_TAARenderPass;
		uint64 m_TAAState;
		TSharedRef<PipelineLayout> m_TAALayout;
		uint64 m_FXAAState;
		TSharedRef<PipelineLayout> m_FXAALayout;

		TSharedRef<DescriptorHeap>			m_DescriptorHeap;
		TArray<TSharedRef<DescriptorSet>>	m_TAATextureDescriptorSets;
		TArray<TSharedRef<DescriptorSet>>	m_TAABufferDescriptorSets;
		TArray<TSharedRef<DescriptorSet>>	m_FXAADescriptorSets;
	};
}