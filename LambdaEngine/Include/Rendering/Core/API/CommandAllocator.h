#pragma once
#include "DeviceChild.h"
#include "GraphicsTypes.h"

namespace LambdaEngine
{
	class CommandAllocator : public DeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(CommandAllocator);

		/*
		* Resets the memory used for all CommadLists using this CommandAllocator
		*	return - Returns true on success
		*/
		virtual bool Reset() = 0;

		/*
		* Returns the API-specific handle to the underlaying CommandAllocator-resource
		*	return - Returns a valid handle on success otherwise zero
		*/
		virtual uint64 GetHandle() const = 0;
		
		virtual ECommandQueueType GetType() const
		{
			return m_Type;
		}

	protected:
		ECommandQueueType	m_Type = ECommandQueueType::COMMAND_QUEUE_TYPE_UNKNOWN;
		String				m_DebugName;
	};
}