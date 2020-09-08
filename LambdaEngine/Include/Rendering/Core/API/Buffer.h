#pragma once
#include "DeviceChild.h"
#include "GraphicsTypes.h"

namespace LambdaEngine
{
	struct BufferDesc
	{
		String		DebugName	= "";
		EMemoryType MemoryType	= EMemoryType::MEMORY_TYPE_NONE;
		uint32		Flags		= FBufferFlags::BUFFER_FLAG_NONE;
		uint64		SizeInBytes = 0;
	};

	class Buffer : public DeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(Buffer);

		/*
		* Maps GPU memory to CPU memory
		*   return - Returns a pointer for CPU memory on success otherwise nullpre
		*/
		virtual void* Map() = 0;

		/*
		* Unmaps the GPU memory from CPU memory. The pointer returned from IBuffer::Map becomes invalid
		*/
		virtual void Unmap() = 0;

		/*
		* Returns the API-specific handle to the underlaying buffer-resource
		*	return - Returns a valid handle on success otherwise zero
		*/
		virtual uint64 GetHandle() const = 0;

		/*
		* Returns this resource's address on the device
		*	return -  Returns a valid 64-bit address on success, otherwise zero. Returns zero on systems that does not support deviceaddresses.
		*/
		virtual uint64 GetDeviceAdress() const
		{
			return 0;
		}

		/*
		* Returns the alignment needed for the buffer when using a buffer offset
		*	return - Returns the needed alignement on success otherwise zero
		*/
		virtual uint64 GetAlignmentRequirement() const = 0;
		
		FORCEINLINE const BufferDesc& GetDesc() const
		{
			return m_Desc;
		}

	protected:
		BufferDesc m_Desc;
	};
}
