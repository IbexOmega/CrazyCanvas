#pragma once
#include "Rendering/CustomRenderer.h"
#include "Rendering/RenderGraphTypes.h"
#include "Rendering/Core/API/DescriptorCache.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/DescriptorSet.h"

namespace LambdaEngine {

	using DescriptorSetIndex = uint32;

	struct FrameBuffer {
		glm::mat4 Projection;
		glm::mat4 View;
		glm::mat4 PrevProjection;
		glm::mat4 PrevView;
		glm::mat4 ViewInv;
		glm::mat4 ProjectionInv;
		glm::vec4 CameraPosition;
		glm::vec4 CameraRight;
		glm::vec4 CameraUp;
		glm::vec2 Jitter;

		uint32 FrameIndex;
		uint32 RandomSeed;
	};

	struct SWeaponBuffer {
		glm::mat4 Model;
		glm::vec3 PlayerPos;
	};

	class FirstPersonWeaponRenderer : public CustomRenderer
	{
	public:
		DECL_REMOVE_COPY(FirstPersonWeaponRenderer);
		DECL_REMOVE_MOVE(FirstPersonWeaponRenderer);

		FirstPersonWeaponRenderer();
		~FirstPersonWeaponRenderer();
		virtual bool Init() override final;

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
			static String name = RENDER_GRAPH_FIRST_PERSON_WEAPON_STAGE_NAME;
			return name;
		}

	private:
		struct SPushConstantData
		{
			glm::mat4 DefaultTransform;
			uint32 TeamIndex = 0;
			float WaveX = 0.0f;
			float WaveZ = 0.0f;
		};

	private:
		bool PrepareResources(CommandList* pCommandList);
		bool CreatePipelineLayout();
		bool CreatePipelineLayoutLiquid();
		bool CreateDescriptorSets();
		bool CreateShaders();
		bool CreateCommandLists();
		bool CreateRenderPass(RenderPassAttachmentDesc* pColorAttachmentDesc);
		bool CreatePipelineState();
		bool CreateBuffers();
		void RenderCull(bool applyDefaultTransform, uint32 drawArgIndex, CommandList* pCommandList, uint64& pipelineId);
		void RenderLiquid(CommandList* pCommandList);

		void UpdateWeaponBuffer(CommandList* pCommandList, uint32 modFrameIndex);

	private:
		bool									m_Initilized = false;
		const DrawArg*							m_pDrawArgs = nullptr;
		uint32									m_DrawCount = 0;
		bool									m_UsingMeshShader = false;
		bool									m_InitilizedWeaponBuffer = false;
		RenderGraph*							m_pRenderGraph = nullptr;

		GUID_Lambda								m_VertexShaderPointGUID = 0;
		GUID_Lambda								m_PixelShaderPointGUID = 0;

		TSharedRef<CommandAllocator>			m_CopyCommandAllocator = nullptr;
		TSharedRef<CommandList>					m_CopyCommandList = nullptr;

		CommandAllocator**						m_ppGraphicCommandAllocators = nullptr;
		CommandList**							m_ppGraphicCommandLists = nullptr;

		TSharedRef<RenderPass>					m_RenderPass = nullptr;

		TSharedRef<Buffer>						m_FrameCopyBuffer = nullptr;
		TSharedRef<Buffer>						m_FrameBuffer = nullptr;
		// First Person Weapon Vertex and Index Buffer
		TSharedRef<Buffer>						m_VertexBuffer = nullptr;
		TSharedRef<Buffer>						m_VertexStagingBuffer = nullptr;

		uint32									m_IndicesCount = 0;
		TSharedRef<Buffer>						m_IndexBuffer = nullptr;
		TSharedRef<Buffer>						m_IndexStagingBuffer = nullptr;

		TSharedRef<Buffer>						m_WeaponBuffer = nullptr;
		TArray<TSharedRef<Buffer>>				m_WeaponStagingBuffers;

		// Needed for transparent rendering
		uint64									m_PipelineStateIDFrontCull = 0;
		uint64									m_PipelineStateIDBackCull = 0;

		TSharedRef<PipelineLayout>				m_PipelineLayout = nullptr;
		TSharedRef<DescriptorHeap>				m_DescriptorHeap = nullptr;

		// start: Not sure about this part
		TSharedRef<DescriptorSet>				m_DescriptorSet0 = nullptr;	// always one buffer with different offset
		TSharedRef<DescriptorSet>				m_DescriptorSet1 = nullptr;	// always one buffer with different offset
		TArray<TSharedRef<DescriptorSet>>		m_DescriptorSetList2; // Needs to switch buffer
		TArray<TSharedRef<DescriptorSet>>		m_DescriptorSetList3; // Needs to switch buffer
		// end

		// Weapon liquid variables
		DescriptorSet*							m_LiquidWaterDrawArgsDescriptorSet;
		DescriptorSet*							m_LiquidPaintDrawArgsDescriptorSet;
		uint64									m_PipelineStateIDNoCull = 0;
		TSharedRef<PipelineLayout>				m_LiquidPipelineLayout = nullptr;
		GUID_Lambda								m_LiquidVertexShaderGUID = 0;
		GUID_Lambda								m_LiquidPixelShaderGUID = 0;
		Entity									m_LiquidEntity = UINT32_MAX;
		SPushConstantData						m_LiquidPushConstantData;

		// Player variables
		Entity									m_PlayerEntity = UINT32_MAX;

		uint32									m_WeaponIndex	= 0;
		uint32									m_LiquidIndex	= 0;
		uint32									m_ArmsIndex		= 0;

		DescriptorCache							m_DescriptorCache;
		uint32									m_BackBufferCount = 0;


		TSharedRef<const TextureView>			m_DepthStencil = nullptr;
		TSharedRef<const TextureView>			m_IntermediateOutputImage = nullptr;

		bool									m_DirtyUniformBuffers = true;

	private:
		static FirstPersonWeaponRenderer* s_pInstance;
	};

}