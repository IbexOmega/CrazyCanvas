#pragma once
#include "Rendering/RenderGraphTypes.h"
#include "Rendering/ICustomRenderer.h"

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
		TArray<uint32> othersTeamId;
	};

	struct UpdateData
	{
		uint32 viewerTeamId;
		TArray<uint32> othersTeamId;
	};

	using ReleaseFrame = uint32;
	using DescriptorSetIndex = uint32;

	class PlayerRenderer : public ICustomRenderer
	{
	public:
		DECL_REMOVE_COPY(PlayerRenderer);
		DECL_REMOVE_MOVE(PlayerRenderer);

		PlayerRenderer();
		~PlayerRenderer();

		bool Init();

		virtual bool RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc) override final;

		void PrepareTextureUpdates(const TArray<UpdateData>& textureIndices);

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
			static String name = "RENDER_STAGE_PLAYERS";
			return name;
		}

	private:
		void HandleUnavailableDescriptors(uint32 modFrameIndex);

		bool CreatePipelineLayout();
		bool CreateDescriptorSets();
		bool CreateShaders();
		bool CreateCommandLists();
		bool CreateRenderPass(RenderPassAttachmentDesc* pDepthStencilAttachmentDesc);
		bool CreatePipelineState();

		TSharedRef<DescriptorSet> GetDescriptorSet(const String& debugname, uint32 descriptorLayoutIndex);

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
		TSharedRef<DescriptorSet>				m_PlayerDescriptorSet;
		TArray<TSharedRef<DescriptorSet>>		m_DrawArgsDescriptorSets;

		THashTable<DescriptorSetIndex, TArray<std::pair<TSharedRef<DescriptorSet>, ReleaseFrame>>>	m_UnavailableDescriptorSets;
		THashTable<DescriptorSetIndex, TArray<TSharedRef<DescriptorSet>>>							m_AvailableDescriptorSets;

		uint32									m_BackBufferCount = 0;
		TArray<UpdateData>						m_TextureUpdateQueue;
		TArray<TSharedRef<const TextureView>>	m_PointLFaceViews;

		TeamsPushConstant						m_PushConstant;
	private:
		static PlayerRenderer* s_pInstance;

	};
}