#pragma once
#include "LambdaEngine.h"

#include "Application/API/EventHandler.h"

#include "Containers/THashTable.h"
#include "Containers/String.h"

#include "ICustomRenderer.h"

struct ImGuiContext;

namespace LambdaEngine
{
	class CommandAllocator;
	class DeviceAllocator;
	class GraphicsDevice;
	class PipelineLayout;
	class DescriptorHeap;
	class DescriptorSet;
	class PipelineState;
	class CommandList;
	class TextureView;
	class CommandList;
	class RenderPass;
	class Texture;
	class Sampler;
	class Shader;
	class Buffer;
	class Window;

	/*
	* ImGuiRendererDesc
	*/
	struct ImGuiRendererDesc
	{
		uint32 BackBufferCount	= 0;
		uint32 VertexBufferSize	= 0;
		uint32 IndexBufferSize	= 0;
	};

	/*
	* ImGuiTexture
	*/
	struct ImGuiTexture
	{
		String		ResourceName		= "No Name";
		float32		ChannelMul[4]		= { 1.0f, 1.0f, 1.0f, 1.0f };
		float32		ChannelAdd[4]		= { 0.0f, 0.0f, 0.0f, 0.0f };
		uint32		ReservedIncludeMask = 0x00008421; //0000 0000 0000 0000 1000 0100 0010 0001
		GUID_Lambda	VertexShaderGUID	= GUID_NONE;
		GUID_Lambda	PixelShaderGUID		= GUID_NONE;
	};

	/*
	* ImGuiRenderer
	*/
	class LAMBDA_API ImGuiRenderer : public ICustomRenderer, EventHandler
	{
		using ImGuiDrawFunc = std::function<void()>;

	public:
		DECL_REMOVE_COPY(ImGuiRenderer);
		DECL_REMOVE_MOVE(ImGuiRenderer);

		ImGuiRenderer(const GraphicsDevice* pGraphicsDevice);
		~ImGuiRenderer();

		bool Init(const ImGuiRendererDesc* pDesc);

		// Be careful to not specify to many params in lambda function since that may result in a heap allocation
		void DrawUI(ImGuiDrawFunc drawFunc);

		virtual bool RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc) override final;

		virtual void PreBuffersDescriptorSetWrite()		override final;
		virtual void PreTexturesDescriptorSetWrite()	override final;

		virtual void UpdateTextureResource(const String& resourceName, const TextureView* const* ppTextureViews, uint32 count, bool backBufferBound) override final;
		virtual void UpdateBufferResource(const String& resourceName, const Buffer* const* ppBuffers, uint64* pOffsets, uint64* pSizesInBytes, uint32 count, bool backBufferBound) override final;
		virtual void UpdateAccelerationStructureResource(const String& resourceName, const AccelerationStructure* pAccelerationStructure) override final;

		virtual void NewFrame(Timestamp delta) override final;
		virtual void PrepareRender(Timestamp delta) override final;

		virtual void Render(CommandAllocator* pCommandAllocator, CommandList* pCommandList, uint32 modFrameIndex, uint32 backBufferIndex, CommandList** ppExecutionStage) override final;

		FORCEINLINE virtual FPipelineStageFlags GetFirstPipelineStage()	override final { return FPipelineStageFlags::PIPELINE_STAGE_FLAG_VERTEX_INPUT; }
		FORCEINLINE virtual FPipelineStageFlags GetLastPipelineStage()	override final { return FPipelineStageFlags::PIPELINE_STAGE_FLAG_PIXEL_SHADER; }

		virtual void OnMouseMoved(int32 x, int32 y)								override final;
		virtual void OnButtonPressed(EMouseButton button, uint32 modifierMask)	override final;
		virtual void OnButtonReleased(EMouseButton button)						override final;
		virtual void OnMouseScrolled(int32 deltaX, int32 deltaY)				override final;
		virtual void OnKeyPressed(EKey key, uint32 modifierMask, bool isRepeat)	override final;
		virtual void OnKeyReleased(EKey key)									override final;
		virtual void OnKeyTyped(uint32 character)								override final;

	public:
		static ImGuiContext* GetImguiContext();
		static ImGuiRenderer& Get();

	private:
		bool InitImGui();
		bool CreateCopyCommandList();
		bool CreateAllocator(uint32 pageSize);
		bool CreateBuffers(uint32 vertexBufferSize, uint32 indexBufferSize);
		bool CreateTextures();
		bool CreateSamplers();
		bool CreatePipelineLayout();
		bool CreateDescriptorSet();
		bool CreateShaders();
		bool CreateRenderPass(RenderPassAttachmentDesc* pBackBufferAttachmentDesc);
		bool CreatePipelineState();

		uint64 InternalCreatePipelineState(GUID_Lambda vertexShader, GUID_Lambda pixelShader);

	private:
		const GraphicsDevice*	m_pGraphicsDevice			= nullptr;

		TArray<TSharedRef<const TextureView>>	m_BackBuffers;

		TSharedRef<CommandAllocator>	m_CopyCommandAllocator	= nullptr;
		TSharedRef<CommandList>			m_CopyCommandList		= nullptr;

		TSharedRef<DeviceAllocator> m_Allocator = nullptr;

		uint64						m_PipelineStateID	= 0;
		TSharedRef<PipelineLayout>	m_PipelineLayout	= nullptr;
		TSharedRef<DescriptorHeap>	m_DescriptorHeap	= nullptr;
		TSharedRef<DescriptorSet>	m_DescriptorSet		= nullptr;

		GUID_Lambda m_VertexShaderGUID	= 0;
		GUID_Lambda m_PixelShaderGUID	= 0;

		TSharedRef<RenderPass> m_RenderPass = nullptr;

		TArray<TSharedRef<Buffer>> m_VertexCopyBuffers;
		TArray<TSharedRef<Buffer>> m_IndexCopyBuffers;
		TSharedRef<Buffer> m_VertexBuffer	= nullptr;
		TSharedRef<Buffer> m_IndexBuffer	= nullptr;

		TSharedRef<Texture>		m_FontTexture		= nullptr;
		TSharedRef<TextureView>	m_FontTextureView	= nullptr;

		TSharedRef<Sampler> m_Sampler = nullptr;

		TArray<ImGuiDrawFunc> m_DeferredDrawCalls;

		THashTable<String, TArray<TSharedRef<DescriptorSet>>>		m_TextureResourceNameDescriptorSetsMap;
		THashTable<GUID_Lambda, THashTable<GUID_Lambda, uint64>>	m_ShadersIDToPipelineStateIDMap;

	private:
		static ImGuiRenderer* s_pRendererInstance;
	};
}
