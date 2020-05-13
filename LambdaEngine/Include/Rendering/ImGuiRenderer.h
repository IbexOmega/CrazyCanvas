#pragma once

#include "LambdaEngine.h"
#include "Application/API/EventHandler.h"

#include "Time/API/Timestamp.h"

#include "Containers/THashTable.h"

struct ImGuiContext;

namespace LambdaEngine
{
	class ICommandAllocator;
	class IDeviceAllocator;
	class IGraphicsDevice;
	class IPipelineLayout;
	class IDescriptorHeap;
	class IDescriptorSet;
	class IPipelineState;
	class ICommandList;
	class ITextureView;
	class ICommandList;
	class IRenderPass;
	class ITexture;
	class ISampler;
	class IShader;
	class IBuffer;
	class IWindow;

	struct ImGuiRendererDesc
	{
		IWindow*	pWindow				= nullptr;
		uint32		BackBufferCount		= 0;
		uint32		VertexBufferSize	= 0;
		uint32		IndexBufferSize		= 0;
	};

	struct ImGuiTexture
	{
		ITextureView*	pTextureView		= nullptr;
		float32			ChannelMult[4]		= { 1.0f, 1.0f, 1.0f, 1.0f };
		float32			ChannelAdd[4]		= { 0.0f, 0.0f, 0.0f, 0.0f };
		uint32			ReservedIncludeMask = 0x00008421; //0000 0000 0000 0000 1000 0100 0010 0001
		GUID_Lambda		VertexShaderGUID	= GUID_NONE;
		GUID_Lambda		PixelShaderGUID		= GUID_NONE;
	};

	class LAMBDA_API ImGuiRenderer : public EventHandler
	{
	public:
		DECL_REMOVE_COPY(ImGuiRenderer);
		DECL_REMOVE_MOVE(ImGuiRenderer);

		ImGuiRenderer(const IGraphicsDevice* pGraphicsDevice);
		~ImGuiRenderer();

		bool Init(const ImGuiRendererDesc* pDesc);

		void Begin(Timestamp delta, uint32 windowWidth, uint32 windowHeight, float32 scaleX, float32 scaleY);
		void End();

		void Render(ICommandList* pCommandList, ITextureView* pRenderTarget, uint32 modFrameIndex, uint32 backBufferIndex);

		//virtual void FocusChanged(IWindow* pWindow, bool hasFocus)									override final;
		//virtual void WindowMoved(IWindow* pWindow, int16 x, int16 y)									override final;
		//virtual void WindowResized(IWindow* pWindow, uint16 width, uint16 height, EResizeType type)	override final;
		//virtual void WindowClosed(IWindow* pWindow)													override final;
		//virtual void MouseEntered(IWindow* pWindow)													override final;
		//virtual void MouseLeft(IWindow* pWindow)														override final;
		virtual void MouseMoved(int32 x, int32 y)														override final;
		virtual void ButtonPressed(EMouseButton button, uint32 modifierMask)							override final;
		virtual void ButtonReleased(EMouseButton button)												override final;
		virtual void MouseScrolled(int32 deltaX, int32 deltaY)											override final;
		virtual void KeyPressed(EKey key, uint32 modifierMask, bool isRepeat)							override final;
		virtual void KeyReleased(EKey key)																override final;
		virtual void KeyTyped(uint32 character)															override final;

	public:
		static ImGuiContext* GetImguiContext();

	private:
		bool InitImGui(IWindow* pWindow);
		bool CreateCopyCommandList();
		bool CreateAllocator(uint32 pageSize);
		bool CreateBuffers(uint32 vertexBufferSize, uint32 indexBufferSize);
		bool CreateTextures();
		bool CreateSamplers();
		bool CreateRenderPass();
		bool CreatePipelineLayout();
		bool CreateDescriptorSet();
		bool CreateShaders();
		bool CreatePipelineState();

		uint64 InternalCreatePipelineState(GUID_Lambda vertexShader, GUID_Lambda pixelShader);

	private:
		const IGraphicsDevice*	m_pGraphicsDevice			= nullptr;

		uint32					m_BackBufferCount			= 0;

		ICommandAllocator*		m_pCopyCommandAllocator		= nullptr;
		ICommandList*			m_pCopyCommandList			= nullptr;

		IDeviceAllocator*		m_pAllocator				= nullptr;

		uint64					m_PipelineStateID			= 0;
		IPipelineLayout*		m_pPipelineLayout			= nullptr;
		IDescriptorHeap*		m_pDescriptorHeap			= nullptr;
		IDescriptorSet*			m_pDescriptorSet			= nullptr;

		GUID_Lambda				m_VertexShaderGUID			= 0;
		GUID_Lambda				m_PixelShaderGUID			= 0;

		IRenderPass*			m_pRenderPass				= nullptr;

		IBuffer*				m_pVertexCopyBuffer			= nullptr;
		IBuffer*				m_pIndexCopyBuffer			= nullptr;
		IBuffer**				m_ppVertexBuffers			= nullptr;
		IBuffer**				m_ppIndexBuffers			= nullptr;

		ITexture*				m_pFontTexture				= nullptr;
		ITextureView*			m_pFontTextureView			= nullptr;

		ISampler*				m_pSampler					= nullptr;

		THashTable<ITextureView*, IDescriptorSet*>					m_TextureDescriptorSetMap;
		THashTable<GUID_Lambda, THashTable<GUID_Lambda, uint64>>	m_ShadersIDToPipelineStateIDMap;
	};
}
