#pragma once
#include "DeviceChild.h"
#include "GraphicsTypes.h"

namespace LambdaEngine
{
	class Buffer;
	class Sampler;
	class TextureView;
	class DescriptorHeap;
    class AccelerationStructure;

	class DescriptorSet : public DeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(DescriptorSet);

		virtual void WriteTextureDescriptors(const TextureView* const* ppTextures, const Sampler* const* ppSamplers, ETextureState textureState, uint32 firstBinding, uint32 descriptorCount, EDescriptorType type)	= 0;
		virtual void WriteBufferDescriptors(const Buffer* const * ppBuffers, const uint64* pOffsets, const uint64* pSizes, uint32 firstBinding, uint32 descriptorCount, EDescriptorType type)						= 0;
		virtual void WriteAccelerationStructureDescriptors(const AccelerationStructure* const * ppAccelerationStructures, uint32 firstBinding, uint32 descriptorCount)												= 0;

		virtual DescriptorHeap*	GetHeap()			= 0;
		virtual uint64			GetHandle() const	= 0;

		FORCEINLINE const String& GetName() const
		{
			return m_DebugName;
		}

	protected:
		String m_DebugName;
	};
}
