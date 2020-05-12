#pragma once

#include "LambdaEngine.h"

#include "Time/API/Timestamp.h"

namespace LambdaEngine
{
	class IWindow;
	class ITexture;
	class ISwapChain;
	class RenderGraph;
	class ICommandList;
	class ITextureView;
	class ImGuiRenderer;
	class IGraphicsDevice;
	class ICommandAllocator;
	
	struct RendererDesc
	{
		const char*		pName			= "";
		bool			Debug			= false;
		IWindow*		pWindow			= nullptr;
		RenderGraph*	pRenderGraph	= nullptr;
		uint32			BackBufferCount = 3;
	};
	
	class LAMBDA_API Renderer
	{
	public:
		DECL_REMOVE_COPY(Renderer);
		DECL_REMOVE_MOVE(Renderer);

		Renderer(const IGraphicsDevice* pGraphicsDevice);
		~Renderer();

		bool Init(const RendererDesc* pDesc);

		void Render(Timestamp delta);

	private:
		const char*				m_pName;
		const IGraphicsDevice*	m_pGraphicsDevice;

		ICommandAllocator**		m_ppImGuiCommandAllocators		= nullptr;
		ICommandList**			m_ppImGuiCommandLists			= nullptr;

		ImGuiRenderer*			m_pImGuiRenderer				= nullptr;

		ISwapChain*				m_pSwapChain					= nullptr;
		RenderGraph*			m_pRenderGraph					= nullptr;
		ITexture**				m_ppBackBuffers					= nullptr;
		ITextureView**			m_ppBackBufferViews				= nullptr;

		uint32					m_FrameIndex = 0;

	};
}
