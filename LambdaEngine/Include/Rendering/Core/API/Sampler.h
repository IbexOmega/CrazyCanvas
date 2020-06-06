#pragma once
#include "DeviceChild.h"
#include "GraphicsTypes.h"

namespace LambdaEngine
{
	struct SamplerDesc
	{
		String					DebugName			= "";
		EFilterType				MinFilter			= EFilterType::FILTER_TYPE_NONE;
		EFilterType				MagFilter			= EFilterType::FILTER_TYPE_NONE;
		EMipmapMode				MipmapMode			= EMipmapMode::MIPMAP_MODE_NONE;
		ESamplerAddressMode		AddressModeU		= ESamplerAddressMode::SAMPLER_ADDRESS_MODE_NONE;
		ESamplerAddressMode		AddressModeV		= ESamplerAddressMode::SAMPLER_ADDRESS_MODE_NONE;
		ESamplerAddressMode		AddressModeW		= ESamplerAddressMode::SAMPLER_ADDRESS_MODE_NONE;
		float32					MipLODBias			= 0.0f;
		bool					AnisotropyEnabled	= false;
		float32					MaxAnisotropy		= 1.0f;
		float32					MinLOD				= 0.0f;
		float32					MaxLOD				= FLT32_MAX;
	};

	class Sampler : public DeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(Sampler);

		/*
		* Returns the API-specific handle to the underlying resource
		*	return - Returns a valid handle on success otherwise zero
		*/
		virtual uint64 GetHandle()	const = 0;

		virtual SamplerDesc GetDesc() const
		{
			return m_Desc;
		}

	protected:
		SamplerDesc m_Desc;
	};
}
