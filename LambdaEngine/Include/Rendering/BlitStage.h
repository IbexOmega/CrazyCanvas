#pragma once

#include "RenderGraphTypes.h"
#include "CustomRenderer.h"

namespace LambdaEngine
{
	class BlitStage : public CustomRenderer
	{
		struct BlitDescription
		{
			String SrcName = "";
			String DstName = "";
			const Texture* pSrcTexture = nullptr;
			const Texture* pDstTexture = nullptr;
		};

	public:
		DECL_REMOVE_COPY(BlitStage);
		DECL_REMOVE_MOVE(BlitStage);

		BlitStage() = default;
		~BlitStage();

		bool Init();

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
			bool sleeping)	override final;

		FORCEINLINE virtual FPipelineStageFlag GetFirstPipelineStage() const override final { return FPipelineStageFlag::PIPELINE_STAGE_FLAG_VERTEX_SHADER; }
		FORCEINLINE virtual FPipelineStageFlag GetLastPipelineStage() const override final { return FPipelineStageFlag::PIPELINE_STAGE_FLAG_VERTEX_SHADER; }

		virtual const String& GetName() const override final
		{
			static String name = BLIT_STAGE;
			return name;
		}

	private:
		bool CreateCommandLists();

	private:
		uint32 m_BackBufferCount = 0;
		
		TArray<BlitDescription> m_BlitDescriptions;

		CommandAllocator** m_ppGraphicCommandAllocators = nullptr;
		CommandList** m_ppGraphicCommandLists = nullptr;
	};
}