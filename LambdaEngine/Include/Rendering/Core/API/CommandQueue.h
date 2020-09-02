#pragma once
#include "DeviceChild.h"
#include "GraphicsTypes.h"

namespace LambdaEngine
{
	class Fence;
	class CommandList;

	class CommandQueue : public DeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(CommandQueue);

		/*
		* Dispatches a commandlist to the device for execution
		*	ppCommandLists 	- An array CommandList* to be executed
		*	numCommandLists	- Number of CommandLists in ppCommandLists
		*	waitStage		- The stage were the wait should happend
		*	pWaitFence		- Fence to wait for, before executing the commandlists
		*	pSignalFence	- A fence that should be signaled when the execution is completed
		*	signalValue		- Value to signal the fence with
		*	return - Returns true if submition of commandlists are successful
		*/
		virtual bool ExecuteCommandLists(const CommandList* const* ppCommandLists, uint32 numCommandLists, FPipelineStageFlags waitStage, const Fence* pWaitFence, uint64 waitValue, Fence* pSignalFence, uint64 signalValue) = 0;
		
		/*
		* Waits for the queue to finish all work that has been submited to it
		*/
		virtual void Flush() = 0;

		/*
		* Returns the API-specific handle to the underlaying CommandQueue
		*	return - Returns a valid handle on success otherwise zero
		*/
		virtual uint64 GetHandle() const = 0;

		virtual ECommandQueueType GetType()	const
		{
			return m_Type;
		}

	protected:
		ECommandQueueType	m_Type;
		String				m_DebugName;
	};
}
