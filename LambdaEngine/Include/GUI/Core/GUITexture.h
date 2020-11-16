#pragma once

#include "NsRender/Texture.h"

#include "Rendering/Core/API/GraphicsTypes.h"

namespace LambdaEngine
{
	class Buffer;
	class Texture;
	class TextureView;
	class CommandList;
	class GUIRenderer;

	struct GUITextureDesc
	{
		String			DebugName		= "";
		uint32			Width			= 0;
		uint32			Height			= 0;
		uint32			MipLevelCount	= 0;
		EFormat			Format			= EFormat::FORMAT_NONE;
		const void**	ppData			= nullptr;
		GUIRenderer*	pGUIRenderer	= nullptr;
	};

	class GUITexture : public Noesis::Texture
	{
	public:
		GUITexture();
		~GUITexture();

		bool Init(CommandList* pCommandList, const GUITextureDesc* pDesc);
		bool Init(LambdaEngine::Texture* pTexture, LambdaEngine::TextureView* pTextureView);

		/// Returns the width of the texture
		virtual uint32_t GetWidth() const override final;

		/// Returns the height of the texture
		virtual uint32_t GetHeight() const override final;

		/// True if the texture has mipmaps
		virtual bool HasMipMaps() const override final;

		/// True is the texture must be vertically inverted when mapped. This is true for render targets
		/// on platforms (OpenGL) where texture V coordinate is zero at the "bottom of the texture"
		virtual bool IsInverted() const override final;

		void UpdateTexture(CommandList* pCommandList, uint32 mipLevel, uint32 x, uint32 y, uint32 width, uint32 height, const void* pData,
			ECommandQueueType prevCommandQueue, ETextureState prevTextureState);

		FORCEINLINE LambdaEngine::Texture* GetTexture()			
		{ 
			return m_pTexture;
		}

		FORCEINLINE LambdaEngine::Texture** GetTextureToBind()	
		{ 
			return &m_pTexture;
		}

		FORCEINLINE LambdaEngine::TextureView* GetTextureView()
		{
			return m_pTextureView;
		}

		FORCEINLINE LambdaEngine::TextureView** GetTextureViewToBind()
		{
			return &m_pTextureView;
		}

	private:
		GUIRenderer* m_pGUIRenderer = nullptr;

		LambdaEngine::Texture*		m_pTexture		= nullptr;
		LambdaEngine::TextureView*	m_pTextureView	= nullptr;
	};
}
