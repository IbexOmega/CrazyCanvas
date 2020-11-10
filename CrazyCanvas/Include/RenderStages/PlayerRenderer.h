#pragma once
#include "Rendering/RenderGraphTypes.h"
#include "Rendering/CustomRenderer.h"
#include "Rendering/Core/API/DescriptorCache.h"
#include "Rendering/Core/API/CommandList.h"

#define MAX_PLAYERS_IN_MATCH 10

namespace LambdaEngine
{
	struct ViewerData
	{
		uint32 TeamId;
		uint32 EntityId;
		uint32 DrawArgIndex;
		glm::vec3 Positon;
	};

	struct PlayerData
	{
		uint32 DrawArgIndex;
		uint32 TeamId;
		glm::vec3 Position;
		float32 Distance2ToViewer;
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
		virtual void UpdateTextureResource(const String& resourceName, const TextureView* const* ppPerImageTextureViews, const TextureView* const* ppPerSubImageTextureViews, const Sampler* const* ppPerImageSamplers, uint32 imageCount, uint32 subImageCount, bool backBufferBound) override final;
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
		void RenderCull(bool renderEnemy, CommandList* pCommandList, uint64& pipelineId);

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

		uint64									m_PipelineStateIDFrontCull = 0;
		uint64									m_PipelineStateIDBackCull = 0;
		uint64									m_PipelineStateIDNoCull = 0;
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
		ViewerData								m_Viewer;
		TArray<PlayerData>						m_PlayerData;

		bool									m_DirtyUniformBuffers = true;

	private:
		static PlayerRenderer* s_pInstance;

	};
}
