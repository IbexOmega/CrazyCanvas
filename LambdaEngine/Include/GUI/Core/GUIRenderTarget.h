#pragma once

#include "NsRender/RenderTarget.h"

#include "Rendering/Core/API/CommandList.h"

#include "GUITexture.h"

namespace LambdaEngine
{
	class Texture;
	class TextureView;
	class RenderPass;
	class GUITexture;
	class GUIRenderer;

	struct GUIRenderTargetDesc
	{
		String		DebugName		= "";
		uint32		Width			= 0;
		uint32		Height			= 0;
		uint32		SampleCount		= 0;
	};

	class GUIRenderTarget : public Noesis::RenderTarget
	{
	public:
		GUIRenderTarget();
		~GUIRenderTarget();

		bool Init(const GUIRenderTargetDesc* pDesc);

		virtual Noesis::Texture* GetTexture() override final;

		FORCEINLINE const GUIRenderTargetDesc* GetDesc() const { return &m_Desc; }

		FORCEINLINE RenderPass* GetRenderPass() { return m_pRenderPass; }
		FORCEINLINE TextureView** GetRenderTargets() { return m_ppRenderTargets; }
		FORCEINLINE TextureView* GetDepthStencil() { return m_pDepthStencilTextureView; }
		FORCEINLINE const ClearColorDesc* GetClearColors() const { return m_pClearColorDesc; }
		FORCEINLINE uint32 GetClearColorCount() const { return ARR_SIZE(m_pClearColorDesc); }

	private:
		bool CreateColorTextures(const GUIRenderTargetDesc* pDesc);
		bool CreateDepthStencilTexture(const GUIRenderTargetDesc* pDesc);
		bool CreateRenderPass(const GUIRenderTargetDesc* pDesc);

	private:
		RenderPass*		m_pRenderPass	= nullptr;

		GUITexture		m_Texture;

		Texture*		m_pColorTexture		= nullptr;
		TextureView*	m_pColorTextureView	= nullptr;
		Texture*		m_pResolveTexture		= nullptr;
		TextureView*	m_pResolveTextureView	= nullptr;

		TextureView*	m_ppRenderTargets[2];

		Texture*		m_pDepthStencilTexture		= nullptr;
		TextureView*	m_pDepthStencilTextureView	= nullptr;

		ClearColorDesc	m_pClearColorDesc[3];

		GUIRenderTargetDesc m_Desc = { };
	};
}