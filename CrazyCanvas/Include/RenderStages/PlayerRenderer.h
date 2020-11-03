#pragma once
#include "Rendering/RenderGraphTypes.h"
#include "Rendering/CustomRenderer.h"
#include "Rendering/Core/API/DescriptorCache.h"

#define MAX_PLAYERS_IN_MATCH 10

namespace LambdaEngine
{
	struct TeamsPushConstant
	{
		byte* pData = nullptr;
		uint32	DataSize = 0;
		uint32	Offset = 0;
		uint32	MaxDataSize = 0;
	};

	struct TeamsPushConstantData
	{
		uint32 viewerTeamId;
		uint32 ToBeDrawnTeamId;
	};

	struct UpdateData
	{
		uint32 viewerTeamId;
		uint32 ToBeDrawnTeamId;
	};

	using ReleaseFrame = uint32;
	using DescriptorSetIndex = uint32;

	class PlayerRenderer : public CustomRenderer
	{
	public:
		DECL_REMOVE_COPY(PlayerRenderer);
		DECL_REMOVE_MOVE(PlayerRenderer);

		PlayerRenderer();
		~PlayerRenderer();

		bool Init();

		virtual bool RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc) override final;

		virtual void Update(Timestamp delta, uint32 modFrameIndex, uint32 backBufferIndex) override final;
		virtual void UpdateTextureResource(const String& resourceName, const TextureView* const* ppPerImageTextureViews, const TextureView* const* ppPerSubImageTextureViews, uint32 imageCount, uint32 subImageCount, bool backBufferBound) override final;
		virtual void UpdateBufferResource(const String& resourceName, const Buffer* const* ppBuffers, uint64* pOffsets, uint64* pSizesInBytes, uint32 count, bool backBufferBound) override final;
		virtual void UpdateDrawArgsResource(const String& resourceName, const DrawArg* pDrawArgs, uint32 count)  override final;

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
			static String name = "PLAYER_PASS";
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

		uint32									m_CurrModFrameIndex = 0;

		const DrawArg*							m_pDrawArgs = nullptr;
		uint32									m_DrawCount = 0;
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
		TSharedRef<DescriptorSet>				m_DescriptorSet0; // always one buffer with different offset
		TSharedRef<DescriptorSet>				m_DescriptorSet1; // always one buffer with different offset
		TArray<TSharedRef<DescriptorSet>>		m_DescriptorSetList2; // Needs to switch buffer
		TArray<TSharedRef<DescriptorSet>>		m_DescriptorSetList3; // Needs to switch buffer

		DescriptorCache							m_DescriptorCache;

		uint32									m_BackBufferCount = 0;

		TSharedRef<const TextureView>			m_DepthStencil;
		TSharedRef<const TextureView>			m_IntermediateOutputImage;
		uint32									m_ViewerTeamId;
		uint32									m_ViewerDrawArgIndex;
		TArray<uint32>							m_TeamIds;
		TArray<TSharedRef<const TextureView>>	m_PointLFaceViews;

		bool									m_DirtyUniformBuffers = true;

	private:
		static PlayerRenderer* s_pInstance;

	};
}