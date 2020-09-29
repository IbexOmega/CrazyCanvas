#pragma once
#include "LambdaEngine.h"

#include "Containers/THashTable.h"
#include "Containers/String.h"

#include "RenderGraphTypes.h"
#include "ICustomRenderer.h"

#include "Application/API/Events/EventQueue.h"

#include "Core/API/DescriptorHeap.h"
#include "Core/API/DescriptorSet.h"
#include "Core/API/CommandList.h"

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

	class LAMBDA_API ImGuiRenderer : public ICustomRenderer
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
		virtual void UpdateDrawArgsResource(const String& resourceName, const DrawArg* pDrawArgs, uint32 count) override final;

		virtual void Render(
			uint32 modFrameIndex,
			uint32 backBufferIndex,
			CommandList** ppFirstExecutionStage,
			CommandList** ppSecondaryExecutionStage)	override final;

		FORCEINLINE virtual FPipelineStageFlag GetFirstPipelineStage()	override final { return FPipelineStageFlag::PIPELINE_STAGE_FLAG_VERTEX_INPUT; }
		FORCEINLINE virtual FPipelineStageFlag GetLastPipelineStage()	override final { return FPipelineStageFlag::PIPELINE_STAGE_FLAG_PIXEL_SHADER; }

		bool OnEvent(const Event& event);
		
		FORCEINLINE virtual const String& GetName() const  override
		{
			static String name = RENDER_GRAPH_IMGUI_STAGE_NAME;
			return name;
		}

	public:
		static ImGuiContext* GetImguiContext();
		static ImGuiRenderer& Get();

	private:
		bool InitImGui();

		bool CreateCopyCommandList();
		bool CreateBuffers(uint32 vertexBufferSize, uint32 indexBufferSize);
		bool CreateTextures();
		bool CreateSamplers();
		bool CreatePipelineLayout();
		bool CreateDescriptorSet();
		bool CreateShaders();
		bool CreateCommandLists();
		bool CreateRenderPass(RenderPassAttachmentDesc* pBackBufferAttachmentDesc);
		bool CreatePipelineState();

		uint64 InternalCreatePipelineState(GUID_Lambda vertexShader, GUID_Lambda pixelShader);

	private:
		const GraphicsDevice*	m_pGraphicsDevice			= nullptr;

		TArray<TSharedRef<const TextureView>>	m_BackBuffers;
		uint32 m_BackBufferCount = 0;

		TSharedRef<CommandAllocator>	m_CopyCommandAllocator		= nullptr;
		TSharedRef<CommandList>			m_CopyCommandList			= nullptr;

		CommandAllocator**	m_ppRenderCommandAllocators	= nullptr;
		CommandList**		m_ppRenderCommandLists		= nullptr;

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
		SpinLock m_DrawCallsLock;

		THashTable<String, TArray<TSharedRef<DescriptorSet>>>		m_TextureResourceNameDescriptorSetsMap;
		THashTable<GUID_Lambda, THashTable<GUID_Lambda, uint64>>	m_ShadersIDToPipelineStateIDMap;

	private:
		static ImGuiRenderer* s_pRendererInstance;
	};
}
