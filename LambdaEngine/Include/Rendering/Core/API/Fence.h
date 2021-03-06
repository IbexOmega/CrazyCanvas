#pragma once
#include "DeviceChild.h"

namespace LambdaEngine
{
	struct FenceDesc
	{
		String DebugName	= "";
		uint64 InitalValue	= 0;
	};

	class Fence : public DeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(Fence);

		/*
		* Wait for a certain value to be reached by the GPU
		*	waitValue 	- 	Value to wait for
		*	timeOut 	- 	Number of nanoseconds to wait for. However, since different implementations
		* 					have different accuracy in their timers, the timeout can take significant longer time.
		*/
		virtual void Wait(uint64 waitValue, uint64 timeOut) const	= 0;

		/*
		* Reset the fence from the host with a non-blocking operation
		*	resetValue - Value to set the fence to
		*/
		virtual void Reset(uint64 resetValue)	= 0;

		/*
		* Retrives the current value of the fence
		*	return - Returns a valid value if successful otherwise returns zero.
		*/
		virtual uint64 GetValue() const = 0;

		FORCEINLINE const FenceDesc& GetDesc() const
		{
			return m_Desc;
		}

	protected:
		FenceDesc m_Desc;
	};
}
