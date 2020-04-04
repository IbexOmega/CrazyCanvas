#pragma once
#include "IDeviceChild.h"

namespace LambdaEngine
{
	class IFence;
	class ICommandList;

	class ICommandQueue : public IDeviceChild
	{
	public:
		DECL_INTERFACE(ICommandQueue);

		/*
		* Dispatches a commandlist to the device for execution
		* 
		* ppCommandLists 	- An array ICommandList* to be executed
		* numCommandLists	- Number of CommandLists in ppCommandLists
		* pWaitFence		- Fence to wait for, before executing the commandlists  
		*/
		virtual bool ExecuteCommandLists(const ICommandList* const* ppCommandLists, uint32 numCommandLists, const IFence* pWaitFence) = 0;
		
		/*
		* Waits for the queue to finish all work that has been submited to it
		*/
		virtual void Wait() = 0;

        /*
        * Returns the API-specific handle to the underlaying CommandQueue
        * 
        * return - Returns a valid handle on success otherwise zero
        */
		virtual uint64 GetHandle() const = 0;
	};
}