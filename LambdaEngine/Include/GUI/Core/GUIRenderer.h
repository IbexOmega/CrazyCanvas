#pragma once
#include "Rendering/CustomRenderer.h"
#include "Rendering/RenderGraphTypes.h"

#include "Rendering/Core/API/CommandList.h"

#include "NsRender/RenderDevice.h"
#include "NsCore/Ptr.h"
#include "NsGui/IView.h"

#define FORCED_SAMPLE_COUNT 1

namespace LambdaEngine
{
	class GUIRenderTarget;
	class GUITexture;

	class GUIRenderer : public Noesis::RenderDevice, public CustomRenderer
	{
		struct GUIParamsData
		{
			glm::mat4	ProjMatrix		= glm::mat4(1.0f);
			glm::vec4	RGBA			= glm::vec4(1.0f);
			glm::vec4	RadialGrad0		= glm::vec4(0.0f);
			glm::vec4	RadialGrad1		= glm::vec4(0.0f);
			glm::vec2	TextSize		= glm::vec2(1.0f);
			glm::vec2	TextPixelSize	= glm::vec2(1.0f);
			float32		Opacity			= 1.0f;
			glm::vec3	Padding;
			float32		pEffectParams[32];
		};

	public:
		GUIRenderer();
		~GUIRenderer();

		bool Init();
		virtual bool RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc) override final;

		//Noesis::RenderDevice
		/// Creates render target surface with given dimensions and number of samples
		virtual Noesis::Ptr<Noesis::RenderTarget> CreateRenderTarget(const char* pLabel, uint32_t width, uint32_t height, uint32_t sampleCount) override final;

		/// Creates render target sharing transient (stencil, colorAA) buffers with the given surface
		virtual Noesis::Ptr<Noesis::RenderTarget> CloneRenderTarget(const char* pLabel, Noesis::RenderTarget* pSurface) override final;

		/// Creates texture with given dimensions and format. For immutable textures, the content of
		/// each mipmap is given in 'data'. The passed data is tightly packed (no extra pitch). When 
		/// 'data' is null the texture is considered dynamic and will be updated using UpdateTexture()
		virtual Noesis::Ptr<Noesis::Texture> CreateTexture(const char* pLabel, uint32_t width, uint32_t height,
			uint32_t numLevels, Noesis::TextureFormat::Enum format, const void** ppData) override final;

		/// Updates texture mipmap copying the given data to desired position. The passed data is
		/// tightly packed (no extra pitch). Origin is located at the left of the first scanline
		virtual void UpdateTexture(Noesis::Texture* pTexture, uint32_t level, uint32_t x, uint32_t y,
			uint32_t width, uint32_t height, const void* pData) override final;

		/// Begins rendering offscreen or onscreen commands
		virtual void BeginRender(bool offscreen) override final;

		/// Binds render target and sets viewport to cover the entire surface
		virtual void SetRenderTarget(Noesis::RenderTarget* pSurface) override final;

		/// Clears the given region to transparent (#000000) and sets the scissor rectangle to fit it.
		/// Until next call to EndTile() all rendering commands will only update the extents of the tile
		virtual void BeginTile(const Noesis::Tile& tile, uint32_t surfaceWidth, uint32_t surfaceHeight) override final;

		/// Completes rendering to the tile specified by BeginTile()
		virtual void EndTile() override final;

		/// Resolves multisample render target
		virtual void ResolveRenderTarget(Noesis::RenderTarget* pSurface, const Noesis::Tile* pTiles, uint32_t numTiles) override final;

		/// Ends rendering
		virtual void EndRender() override final;

		/// Gets a pointer to stream vertices
		virtual void* MapVertices(uint32_t bytes) override final;

		/// Invalidates the pointer previously mapped
		virtual void UnmapVertices() override final;

		/// Gets a pointer to stream 16-bit indices
		virtual void* MapIndices(uint32_t bytes) override final;

		/// Invalidates the pointer previously mapped
		virtual void UnmapIndices() override final;

		/// Draws primitives for the given batch
		virtual void DrawBatch(const Noesis::Batch& batch) override final;

		//ICustomRenderer
		virtual void Update(Timestamp delta, uint32 modFrameIndex, uint32 backBufferIndex) override final;

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

		void SetView(Noesis::Ptr<Noesis::IView> view);

		/// Retrieves device render capabilities
		FORCEINLINE virtual const Noesis::DeviceCaps& GetCaps() const override final 
		{
			static  Noesis::DeviceCaps deviceCaps;
			deviceCaps.centerPixelOffset	= 0.5f;
			deviceCaps.linearRendering		= false;
			deviceCaps.subpixelRendering	= false;
			return deviceCaps;
		}

		FORCEINLINE virtual FPipelineStageFlag GetFirstPipelineStage() const override final { return FPipelineStageFlag::PIPELINE_STAGE_FLAG_VERTEX_SHADER; }
		FORCEINLINE virtual FPipelineStageFlag GetLastPipelineStage() const override final { return FPipelineStageFlag::PIPELINE_STAGE_FLAG_PIXEL_SHADER; }

		FORCEINLINE virtual const String& GetName() const override final 
		{ 
			static String name = RENDER_GRAPH_NOESIS_GUI_STAGE_NAME;
			return name;
		}

	private:
		CommandList* BeginOrGetUtilityCommandList();
		CommandList* BeginOrGetRenderCommandList();
		
		void BeginMainRenderPass(CommandList* pCommandList);
		void BeginTileRenderPass(CommandList* pCommandList);

		Buffer* CreateOrGetParamsBuffer();
		DescriptorSet* CreateOrGetDescriptorSet();

		void ResumeRenderPass();
		void EndCurrentRenderPass();

		bool CreateCommandLists();
		bool CreateDescriptorHeap();
		bool CreateSampler();
		bool CreateRenderPass(RenderPassAttachmentDesc* pBackBufferAttachmentDesc);


	private:
		TSharedRef<const TextureView>	m_pBackBuffers[BACK_BUFFER_COUNT];
		TSharedRef<const TextureView>	m_DepthStencilTextureView;
		uint32 m_BackBufferIndex	= 0;
		uint32 m_ModFrameIndex		= 0;

		CommandAllocator*	m_ppUtilityCommandAllocators[BACK_BUFFER_COUNT];
		CommandList*		m_ppUtilityCommandLists[BACK_BUFFER_COUNT];

		CommandAllocator*	m_ppRenderCommandAllocators[BACK_BUFFER_COUNT];
		CommandList*		m_ppRenderCommandLists[BACK_BUFFER_COUNT];

		GUIRenderTarget* m_pCurrentRenderTarget = nullptr;
		Sampler* m_pGUISampler = nullptr;

		Buffer* m_pVertexStagingBuffer		= nullptr;
		Buffer* m_pIndexStagingBuffer		= nullptr;
		uint64	m_RequiredVertexBufferSize	= 0;
		uint64	m_RequiredIndexBufferSize	= 0;

		Buffer* m_pVertexBuffer	= nullptr;
		Buffer* m_pIndexBuffer	= nullptr;
		TArray<Buffer*> m_AvailableParamsBuffers;
		TArray<Buffer*> m_pUsedParamsBuffers[BACK_BUFFER_COUNT];
		TArray<DeviceChild*> m_pGraphicsResourcesToRemove[BACK_BUFFER_COUNT];

		DescriptorHeap*			m_pDescriptorHeap = nullptr;
		TArray<DescriptorSet*>	m_AvailableDescriptorSets;
		TArray<DescriptorSet*>	m_pUsedDescriptorSets[BACK_BUFFER_COUNT];

		RenderPass* m_pMainRenderPass = nullptr;
		ClearColorDesc m_pMainRenderPassClearColors[2];

		Noesis::Ptr<Noesis::IView> m_View;

		bool m_IsInRenderPass			= false;
		bool m_TileBegun				= false;
		bool m_RenderPassBegun			= false;
		bool m_Initialized				= false;
	};
}