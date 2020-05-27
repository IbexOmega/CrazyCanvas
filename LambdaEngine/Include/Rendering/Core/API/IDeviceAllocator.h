#pragma once
#include "IDeviceChild.h"

namespace LambdaEngine
{
	struct DeviceAllocatorDesc
	{
		const char* pName			= "";
		uint64		PageSizeInBytes	= 0;
	};

	struct DeviceAllocatorStatistics
	{
		uint64 TotalBytesReserved	= 0;
		uint64 TotalBytesAllocated	= 0;
	};

	class IDeviceAllocator : public IDeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(IDeviceAllocator);
		
		virtual DeviceAllocatorStatistics	GetStatistics()	const = 0;
		virtual DeviceAllocatorDesc			GetDesc()		const = 0;
	};
}
