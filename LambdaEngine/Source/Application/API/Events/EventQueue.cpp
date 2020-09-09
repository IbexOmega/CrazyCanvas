#include "Application/API/Events/EventQueue.h"

namespace LambdaEngine
{
	static TArray<EventHandlerProxy> g_EventHandlers;
	static SpinLock g_EventHandlersSpinlock;

	/*
	* EventQueue
	*/
	void EventQueue::RegisterEventHandler(const EventHandlerProxy& eventHandler)
	{
		// TODO: Make sure that the eventhandler is not already added
		std::scoped_lock<SpinLock> lock(g_EventHandlersSpinlock);
		g_EventHandlers.EmplaceBack(eventHandler);
	}
	
	void EventQueue::UnregisterEventHandler(const EventHandlerProxy& eventHandler)
	{
		//TODO: Implement this
	}
	
	bool EventQueue::SendEvent(const Event& event)
	{
		std::scoped_lock<SpinLock> lock(g_EventHandlersSpinlock);
		for (EventHandlerProxy& handler : g_EventHandlers)
		{
			// If true then stop handle this event
			if (handler(event))
			{
				return true;
			}
		}

		return false;
	}
}