#pragma once
#include "IDeviceChild.h"

namespace LambdaEngine
{
	class IBuffer;
	class ITexture;
	class IDescriptorHeap;

	enum class ETextureDescriptorType : uint8
	{
		NONE						= 0,
		TEXTURE_RENDER_TARGET		= 1,
		TEXTURE_SHADER_RESOURCE		= 2,
		TEXTURE_UNORDERED_ACCESS	= 3
	};

	class IDescriptorSet : public IDeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(IDescriptorSet);

		virtual void WriteTextureDescriptors(const ITexture* const * ppTextures)					= 0;
		virtual void WriteBufferDescriptors()														= 0;
		virtual void WriteSamplerDescriptors()														= 0;
		virtual void WriteAccelerationStructureDescriptors()										= 0;

		virtual IDescriptorHeap*	GetHeap()			= 0;
		virtual uint64				GetHandle() const	= 0;
	};
}