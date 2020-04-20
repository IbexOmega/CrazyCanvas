#pragma once
#include "IDeviceChild.h"
#include "GraphicsTypes.h"

namespace LambdaEngine
{
	class IBuffer;
	class ISampler;
	class ITextureView;
	class IDescriptorHeap;

	class IDescriptorSet : public IDeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(IDescriptorSet);

		virtual void WriteTextureDescriptors(const ITextureView* const* ppTextures, const ISampler* const* ppSamplers, ETextureState textureState, uint32 firstBinding, uint32 descriptorCount, EDescriptorType type) = 0;
		virtual void WriteBufferDescriptors(const IBuffer* const * ppBuffers, const uint64* pOffsets, const uint64* pSizes, uint32 firstBinding, uint32 descriptorCount, EDescriptorType type)= 0;
		virtual void WriteAccelerationStructureDescriptors(uint32 firstBinding, uint32 descriptorCount) = 0;

		virtual IDescriptorHeap*	GetHeap()			= 0;
		virtual uint64				GetHandle() const	= 0;
	};
}