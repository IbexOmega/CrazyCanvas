#pragma once
#include "Texture.h"

#include "Core/TSharedRef.h"

namespace LambdaEngine
{
	/*
	* ETextureViewType
	*/
	enum class ETextureViewType : uint8
	{
		TEXTURE_VIEW_TYPE_NONE		= 0,
		TEXTURE_VIEW_TYPE_1D		= 1,
		TEXTURE_VIEW_TYPE_2D		= 2,
		TEXTURE_VIEW_TYPE_3D		= 3,
		TEXTURE_VIEW_TYPE_CUBE		= 4,
		TEXTURE_VIEW_TYPE_CUBE_ARRAY= 5,
		TEXTURE_VIEW_TYPE_2D_ARRAY	= 6,
	};

	/*
	* TextureViewDesc
	*/
	struct TextureViewDesc
	{
		String				DebugName		= "";
		Texture*			pTexture		= nullptr;
		uint32				Flags			= FTextureViewFlags::TEXTURE_VIEW_FLAG_NONE;
		EFormat				Format			= EFormat::FORMAT_NONE;
		ETextureViewType	Type			= ETextureViewType::TEXTURE_VIEW_TYPE_NONE;
		uint32				MiplevelCount	= 0;
		uint32				ArrayCount		= 0;
		uint32				Miplevel		= 0;
		uint32				ArrayIndex		= 0;
	};

	/*
	* TextureView
	*/
	class TextureView : public DeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(TextureView);
		
		virtual Texture* GetTexture() = 0;
		
		/*
		* Returns the API-specific handle to the underlaying CommandQueue
		*	return - Returns a valid handle on success otherwise zero
		*/
		virtual uint64 GetHandle() const = 0;
		
		FORCEINLINE const TextureViewDesc& GetDesc() const
		{
			return m_Desc;
		}

	protected:
		TextureViewDesc m_Desc;
	};
}
