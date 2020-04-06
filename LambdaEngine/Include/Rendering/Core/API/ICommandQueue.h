#pragma once
#include "IDeviceChild.h"
#include "GraphicsTypes.h"

namespace LambdaEngine
{
	class IFence;
	class ICommandList;

	class ICommandQueue : public IDeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(ICommandQueue);

		/*
		* Dispatches a commandlist to the device for execution
		* 
		* ppCommandLists 	- An array ICommandList* to be executed
		* numCommandLists	- Number of CommandLists in ppCommandLists
		* waitStage			- The stage were the wait should happend
		* pWaitFence		- Fence to wait for, before executing the commandlists
		* pSignalFence		- A fence that should be signaled when the execution is completed
		* signalValue		- Value to signal the fence with
		*
		* return - Returns true if submition of commandlists are successful
		*/
		virtual bool ExecuteCommandLists(const ICommandList* const* ppCommandLists, uint32 numCommandLists, FPipelineStageFlags waitStage, const IFence* pWaitFence, uint64 waitValue, const IFence* pSignalFence, uint64 signalValue) = 0;
		
		/*
		* Waits for the queue to finish all work that has been submited to it
		*/
		virtual void Flush() = 0;

        /*
        * Returns the API-specific handle to the underlaying CommandQueue
        * 
        * return - Returns a valid handle on success otherwise zero
        */
		virtual uint64 GetHandle() const = 0;
	};
}