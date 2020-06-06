#pragma once
#include "DeviceChild.h"

namespace LambdaEngine
{
	struct DeviceAllocatorDesc
	{
		String DebugName		= "";
		uint64 PageSizeInBytes	= 0;
	};

	struct DeviceAllocatorStatistics
	{
		uint64 TotalBytesReserved	= 0;
		uint64 TotalBytesAllocated	= 0;
	};

	class DeviceAllocator : public DeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(DeviceAllocator);

		virtual DeviceAllocatorStatistics GetStatistics() const
		{
			return m_Statistics;
		}

		virtual DeviceAllocatorDesc GetDesc() const
		{
			return m_Desc;
		}

	protected:
		DeviceAllocatorStatistics	m_Statistics;
		DeviceAllocatorDesc			m_Desc;
	};
}
