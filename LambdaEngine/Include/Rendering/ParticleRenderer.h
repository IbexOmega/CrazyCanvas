#pragma once

#include "RenderGraphTypes.h"
#include "ICustomRenderer.h"

namespace LambdaEngine
{

	class ParticleRenderer : public ICustomRenderer
	{
		ParticleRenderer();
		~ParticleRenderer();

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

		FORCEINLINE virtual FPipelineStageFlag GetFirstPipelineStage()	override final { return FPipelineStageFlag::PIPELINE_STAGE_FLAG_VERTEX_INPUT; }
		FORCEINLINE virtual FPipelineStageFlag GetLastPipelineStage()	override final { return FPipelineStageFlag::PIPELINE_STAGE_FLAG_PIXEL_SHADER; }

		virtual const String& GetName() const override final
		{
			static String name = RENDER_GRAPH_LIGHT_STAGE_NAME;
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
		bool							m_Initilized = false;
		bool							m_MeshShaders = false;

		GUID_Lambda						m_ComputeGUID = 0;
		GUID_Lambda						m_MeshShaderGUID = 0;
		GUID_Lambda						m_PixelShaderGUID = 0;
		GUID_Lambda						m_VertexShaderGUID = 0;

		TSharedRef<RenderPass>			m_RenderPass = nullptr;

		uint64							m_PipelineStateID = 0;
		TSharedRef<PipelineLayout>		m_PipelineLayout = nullptr;
		TSharedRef<DescriptorHeap>		m_DescriptorHeap = nullptr;

		// Descriptor sets
		TSharedRef<DescriptorSet>		m_PerFrameBufferDescriptorSet;

		uint32							m_BackBufferCount = 0;

		CommandAllocator** m_ppGraphicCommandAllocators = nullptr;
		CommandList** m_ppGraphicCommandLists = nullptr;

	private:
		static ParticleRenderer* s_pInstance;
	};
}

