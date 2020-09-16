#pragma once
#include "LambdaEngine.h"

#include "ECS/System.h"

#include "Core/TSharedRef.h"

#include "Rendering/Core/API/SwapChain.h"
#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/Fence.h"
#include "Rendering/Core/API/PipelineState.h"
#include "Rendering/Core/API/RenderPass.h"
#include "Rendering/Core/API/PipelineLayout.h"

#include "Containers/IDVector.h"

namespace LambdaEngine
{
	class Scene;
	class Window;
	class Texture;
	class RenderGraph;
	class CommandList;
	class TextureView;
	class ImGuiRenderer;
	class GraphicsDevice;
	class CommandAllocator;

	struct RenderGraphStructureDesc;

	class LAMBDA_API RenderSystem : public System
	{
	public:
		DECL_REMOVE_COPY(RenderSystem);
		DECL_REMOVE_MOVE(RenderSystem);
		~RenderSystem() = default;

		bool InitSystem();

		bool Release();

		void Tick(float dt);

		bool Render();

		CommandList* AcquireGraphicsCopyCommandList();
		CommandList* AcquireComputeCopyCommandList();	

		void SetRenderGraph(const String& name, RenderGraphStructureDesc* pRenderGraphStructureDesc);

		RenderGraph*	GetRenderGraph()	{ return m_pRenderGraph; }
		uint64			GetFrameIndex()		{ return m_FrameIndex; }
		uint64			GetModFrameIndex()	{ return m_ModFrameIndex; }
		uint32			GetBufferIndex()	{ return m_BackBufferIndex; }
	public:
		static RenderSystem* GetInstance() { return &s_pInstance; }

	private:
		void UpdateRenderGraphFromScene();

	private:
		IDVector				m_StaticEntities;
		IDVector				m_DynamicEntities;

		TSharedRef<SwapChain>	m_SwapChain			= nullptr;
		Texture**				m_ppBackBuffers		= nullptr;
		TextureView**			m_ppBackBufferViews	= nullptr;
		RenderGraph*			m_pRenderGraph		= nullptr;
		Scene*					m_pScene			= nullptr;
		uint64					m_FrameIndex		= 0;
		uint64					m_ModFrameIndex		= 0;
		uint32					m_BackBufferIndex	= 0;

	private:
		static RenderSystem		s_pInstance;
	};
}