#pragma once

#include "RenderGraphTypes.h"
#include "ICustomRenderer.h"
#include "Core/API/DescriptorCache.h"

namespace LambdaEngine
{
	struct PushConstant
	{
		byte*	pData = nullptr;
		uint32	DataSize = 0;
		uint32	Offset = 0;
		uint32	MaxDataSize = 0;
	};

	struct PushConstantData
	{
		uint32 Iteration;
		uint32 PointLightIndex;
	};

	struct LightUpdateData
	{
		uint32 PointLightIndex;
		uint32 TextureIndex;
	};

	using ReleaseFrame = uint32;
	using DescriptorSetIndex = uint32;

	class LightRenderer : public ICustomRenderer
	{
	public:
		DECL_REMOVE_COPY(LightRenderer);
		DECL_REMOVE_MOVE(LightRenderer);

		LightRenderer();
		~LightRenderer();

		bool Init();

		virtual bool RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc) override final;

		void PrepareTextureUpdates(const TArray<LightUpdateData>& textureIndices);

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
		bool CreateRenderPass(RenderPassAttachmentDesc* pDepthStencilAttachmentDesc);
		bool CreatePipelineState();

	private:
		bool									m_Initilized = false;

		const DrawArg*							m_pDrawArgs = nullptr;
		uint32									m_DrawCount	= 0;
		bool									m_UsingMeshShader = false;

		GUID_Lambda								m_VertexShaderPointGUID = 0;
		GUID_Lambda								m_PixelShaderPointGUID = 0;

		TSharedRef<CommandAllocator>			m_CopyCommandAllocator = nullptr;
		TSharedRef<CommandList>					m_CopyCommandList = nullptr;

		CommandAllocator**						m_ppGraphicCommandAllocators = nullptr;
		CommandList**							m_ppGraphicCommandLists = nullptr;

		TSharedRef<RenderPass>					m_RenderPass = nullptr;

		uint64									m_PipelineStateID = 0;
		TSharedRef<PipelineLayout>				m_PipelineLayout = nullptr;
		TSharedRef<DescriptorHeap>				m_DescriptorHeap = nullptr;
		TSharedRef<DescriptorSet>				m_LightDescriptorSet;
		TArray<TSharedRef<DescriptorSet>>		m_DrawArgsDescriptorSets;

		DescriptorCache							m_DescriptorCache;

		uint32									m_BackBufferCount = 0;
		TArray<LightUpdateData>					m_TextureUpdateQueue;
		TArray<TSharedRef<const TextureView>>	m_PointLFaceViews;

		PushConstant							m_PushConstant;
	private:
		static LightRenderer* s_pInstance;

	};
}