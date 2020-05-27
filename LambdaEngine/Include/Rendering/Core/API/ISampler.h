#pragma once
#include "IDeviceChild.h"
#include "GraphicsTypes.h"

namespace LambdaEngine
{
	struct SamplerDesc
	{
		const char*				pName				= "Sampler";
		EFilterType				MinFilter			= EFilterType::FILTER_TYPE_NONE;
		EFilterType				MagFilter			= EFilterType::FILTER_TYPE_NONE;
		EMipmapMode				MipmapMode			= EMipmapMode::MIPMAP_MODE_NONE;
		ESamplerAddressMode		AddressModeU		= ESamplerAddressMode::SAMPLER_ADDRESS_MODE_NONE;
		ESamplerAddressMode		AddressModeV		= ESamplerAddressMode::SAMPLER_ADDRESS_MODE_NONE;
		ESamplerAddressMode		AddressModeW		= ESamplerAddressMode::SAMPLER_ADDRESS_MODE_NONE;
		float					MipLODBias			= 0.0f;
		bool					AnisotropyEnabled	= false;
		float					MaxAnisotropy		= 1.0f;
		float					MinLOD				= 0.0f;
		float					MaxLOD				= FLT32_MAX;
	};

	class ISampler : public IDeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(ISampler);

		/*
		* Returns the API-specific handle to the underlying resource
		*	return - Returns a valid handle on success otherwise zero
		*/
		virtual uint64		GetHandle()	const = 0;
		virtual SamplerDesc	GetDesc()	const = 0;
	};
}
