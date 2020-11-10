#pragma once

#include "RenderGraphTypes.h"
#include "CustomRenderer.h"

#include "Rendering/Core/API/DescriptorCache.h"

namespace LambdaEngine
{
	struct DescriptorBindingData
	{
		const Buffer* pBuffers	= nullptr;
		uint64 Offset			= 0;
		uint64 SizeInByte		= 0;
	};

	class ParticleRenderer : public CustomRenderer
	{
	public:
		ParticleRenderer();
		~ParticleRenderer();

		bool Init();
		
		void SetCurrentParticleCount(uint32 particleCount, uint32 emitterCount) { m_ParticleCount = particleCount; m_EmitterCount = emitterCount; };

		virtual bool RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc) override final;

		virtual void Update(Timestamp delta, uint32 modFrameIndex, uint32 backBufferIndex) override final;
		virtual void UpdateTextureResource(const String& resourceName, const TextureView* const* ppPerImageTextureViews, const TextureView* const* ppPerSubImageTextureViews, const Sampler* const* ppPerImageSamplers, uint32 imageCount, uint32 subImageCount, bool backBufferBound) override final;
		virtual void UpdateBufferResource(const String& resourceName, const Buffer* const* ppBuffers, uint64* pOffsets, uint64* pSizesInBytes, uint32 count, bool backBufferBound) override final;

		virtual void Render(
			uint32 modFrameIndex,
			uint32 backBufferIndex,
			CommandList** ppFirstExecutionStage,
			CommandList** ppSecondaryExecutionStage,
			bool Sleeping)	override final;

		FORCEINLINE virtual FPipelineStageFlag GetFirstPipelineStage() const override final { return FPipelineStageFlag::PIPELINE_STAGE_FLAG_VERTEX_SHADER; }
		FORCEINLINE virtual FPipelineStageFlag GetLastPipelineStage() const override final { return FPipelineStageFlag::PIPELINE_STAGE_FLAG_PIXEL_SHADER; }

		virtual const String& GetName() const override final
		{
			static String name = RENDER_GRAPH_PARTICLE_RENDER_STAGE_NAME;
			return name;
		}

	private:
		bool CreatePipelineLayout();
		bool CreateDescriptorSets();
		bool CreateShaders();
		bool CreateCommandLists();
		bool CreateRenderPass(RenderPassAttachmentDesc* pColorAttachmentDesc, RenderPassAttachmentDesc* pDepthStencilAttachmentDesc);
		bool CreatePipelineState();

	private:
		bool									m_Initilized = false;
		bool									m_MeshShaders = false;
		uint32									m_ParticleCount;
		uint32									m_EmitterCount;


		GUID_Lambda								m_MeshShaderGUID		= 0;
		GUID_Lambda								m_PixelShaderGUID		= 0;
		GUID_Lambda								m_VertexShaderGUID		= 0;

		const Buffer*							m_pIndirectBuffer		= nullptr;
		const Buffer*							m_pIndexBuffer			= nullptr;

		TSharedRef<const TextureView>			m_RenderTarget			= nullptr;
		TSharedRef<const TextureView>			m_DepthStencil			= nullptr;
		TSharedRef<RenderPass>					m_RenderPass			= nullptr;

		uint64									m_PipelineStateID		= 0;
		TSharedRef<PipelineLayout>				m_PipelineLayout		= nullptr;
		TSharedRef<DescriptorHeap>				m_DescriptorHeap		= nullptr;

		// Descriptor sets
		TSharedRef<DescriptorSet>				m_VertexInstanceDescriptorSet	= nullptr;
		TSharedRef<DescriptorSet>				m_AtlasTexturesDescriptorSet	= nullptr;
		TSharedRef<DescriptorSet>				m_AtlasInfoBufferDescriptorSet	= nullptr;
		TSharedRef<DescriptorSet>				m_PerFrameBufferDescriptorSet;
		DescriptorCache							m_DescriptorCache;

		uint32									m_BackBufferCount = 0;

		CommandAllocator**				m_ppGraphicCommandAllocators = nullptr;
		CommandList**					m_ppGraphicCommandLists = nullptr;

	private:
		static ParticleRenderer* s_pInstance;
	};
}

