#pragma once

#include "LambdaEngine.h"

namespace LambdaEngine
{
	class IWindow;
	class ITexture;
	class ISwapChain;
	class RenderGraph;
	class ITextureView;
	class IGraphicsDevice;
	
	struct RendererDesc
	{
		const char*		pName			= "";
		IWindow*			pWindow			= nullptr;
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

		bool Init(const RendererDesc& desc);

		void Render();

	private:
		const char*				m_pName;
		const IGraphicsDevice*	m_pGraphicsDevice;

		ISwapChain*				m_pSwapChain		= nullptr;
		RenderGraph*			m_pRenderGraph		= nullptr;
		ITexture**				m_ppBackBuffers		= nullptr;
		ITextureView**			m_ppBackBufferViews = nullptr;

		uint32					m_FrameIndex = 0;

	};
}
