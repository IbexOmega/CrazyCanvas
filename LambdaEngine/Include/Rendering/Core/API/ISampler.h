#pragma once
#include "IDeviceChild.h"
#include "GraphicsTypes.h"

namespace LambdaEngine
{
	struct SamplerDesc
	{
		const char*     pName			    = "Sampler";
		EFilter         MinFilter		    = EFilter::NONE;
		EFilter         MagFilter		    = EFilter::NONE;
		EMipmapMode     MipmapMode		    = EMipmapMode::NONE;
		EAddressMode    AddressModeU	    = EAddressMode::NONE;
		EAddressMode    AddressModeV	    = EAddressMode::NONE;
		EAddressMode    AddressModeW	    = EAddressMode::NONE;
		float           MipLODBias		    = 0.0f;
		bool            AnisotropyEnabled	= false;
		float           MaxAnisotropy		= 1.0f;
		float           MinLOD				= 0.0f;
		float           MaxLOD				= FLT32_MAX;
	};

	class ISampler : public IDeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(ISampler);

		/*
	   * Returns the API-specific handle to the underlaying resource
	   *
	   * return - Returns a valid handle on success otherwise zero
	   */
		virtual uint64      GetHandle() const = 0;
        virtual SamplerDesc GetDesc()   const = 0;
	};
}
