#pragma once
#include "DeviceChild.h"
#include "GraphicsTypes.h"

namespace LambdaEngine
{
	enum class ETextureType : uint8
	{
		TEXTURE_TYPE_NONE	= 0,
		TEXTURE_TYPE_1D		= 1,
		TEXTURE_TYPE_2D		= 2,
		TEXTURE_TYPE_3D		= 3,
	};

	struct TextureDesc
	{
		String			DebugName	= "";
		EMemoryType		MemoryType	= EMemoryType::MEMORY_TYPE_NONE;
		EFormat			Format		= EFormat::FORMAT_NONE;
		ETextureType	Type		= ETextureType::TEXTURE_TYPE_NONE;
		FTextureFlags	Flags		= FTextureFlag::TEXTURE_FLAG_NONE;
		uint32			Width		= 0;
		uint32			Height		= 0;
		uint32			Depth		= 0;
		uint32			ArrayCount	= 0;
		uint32			Miplevels	= 0;
		uint32			SampleCount	= 0;
	};

	class Texture : public DeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(Texture);

		/*
		* Returns the API-specific handle to the underlaying texture-resource
		*	return - Returns a valid handle on success otherwise zero
		*/
		virtual uint64 GetHandle() const = 0;
		
		FORCEINLINE const TextureDesc& GetDesc() const
		{
			return m_Desc;
		}

	protected:
		TextureDesc m_Desc;
	};
}
