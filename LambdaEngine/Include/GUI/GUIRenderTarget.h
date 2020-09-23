#pragma once

#include "NsRender/RenderTarget.h"

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

		Texture*		m_pDepthStencilTexture		= nullptr;
		TextureView*	m_pDepthStencilTextureView	= nullptr;

		GUIRenderTargetDesc m_Desc = { };
	};
}