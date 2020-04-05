#pragma once
#include "IDeviceChild.h"
#include "GraphicsTypes.h"

namespace LambdaEngine
{
	class ICommandAllocator : public IDeviceChild
	{
	public:
		DECL_INTERFACE(ICommandAllocator);

		/*
		* Resets the memory used for all CommadLists using this CommandAllocator
		*
		* return - Returns true on success
		*/
		virtual bool Reset() = 0;

        /*
        * Returns the API-specific handle to the underlaying CommandAllocator-resource
        * 
        * return - Returns a valid handle on success otherwise zero
        */
		virtual uint64				GetHandle() const	= 0;
		virtual ECommandQueueType	GetType()	const	= 0;
	};
}