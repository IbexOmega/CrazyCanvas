#pragma once

#include "LambdaEngine.h"

#include "Time/API/Timestamp.h"

namespace LambdaEngine
{
	class Window;
	class Texture;
	class SwapChain;
	class RenderGraph;
	class CommandList;
	class TextureView;
	class ImGuiRenderer;
	class GraphicsDevice;
	class CommandAllocator;
	
	struct RendererDesc
	{
		const char*		pName			= "";
		bool			Debug			= false;
		Window*			pWindow			= nullptr;
		RenderGraph*	pRenderGraph	= nullptr;
		uint32			BackBufferCount = 3;
	};
	
	class LAMBDA_API Renderer
	{
	public:
		DECL_REMOVE_COPY(Renderer);
		DECL_REMOVE_MOVE(Renderer);

		Renderer(const GraphicsDevice* pGraphicsDevice);
		~Renderer();

		bool Init(const RendererDesc* pDesc);

		void Begin(Timestamp delta);
		void End(Timestamp delta);

		void Render(Timestamp delta);

		FORCEINLINE uint32 GetFrameIndex()		{ return m_FrameIndex; }
		FORCEINLINE uint32 GetModFrameIndex()	{ return m_ModFrameIndex; }
		FORCEINLINE uint32 GetBufferIndex()		{ return m_BackBufferIndex; }

	private:
		const char*				m_pName;
		const GraphicsDevice*	m_pGraphicsDevice;

		CommandAllocator**		m_ppImGuiCommandAllocators		= nullptr;
		CommandList**			m_ppImGuiCommandLists			= nullptr;

		ImGuiRenderer*			m_pImGuiRenderer				= nullptr;

		SwapChain*				m_pSwapChain					= nullptr;
		RenderGraph*			m_pRenderGraph					= nullptr;
		Texture**				m_ppBackBuffers					= nullptr;
		TextureView**			m_ppBackBufferViews				= nullptr;

		uint32					m_BackBufferCount				= 0;

		uint32					m_FrameIndex					= 0;
		uint32					m_ModFrameIndex					= 0;
		uint32					m_BackBufferIndex				= 0;

	};
}
