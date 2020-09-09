#include "Application/API/Events/EventQueue.h"

namespace LambdaEngine
{
	static TArray<EventHandlerProxy> g_EventHandlers;
	static SpinLock g_EventHandlersSpinlock;

	/*
	* EventQueue
	*/
	bool EventQueue::RegisterEventHandler(const EventHandlerProxy& eventHandler)
	{
		// TODO: Make sure that the eventhandler is not already added
		std::scoped_lock<SpinLock> lock(g_EventHandlersSpinlock);
		g_EventHandlers.EmplaceBack(eventHandler);
		return true;
	}
	
	bool EventQueue::UnregisterEventHandler(const EventHandlerProxy& eventHandler)
	{
		//TODO: Implement this
		return false;
	}
	
	bool EventQueue::SendEvent(Event& event)
	{
		std::scoped_lock<SpinLock> lock(g_EventHandlersSpinlock);
		for (EventHandlerProxy& handler : g_EventHandlers)
		{
			// If true then set that this element is consumed
			if (handler(event))
			{
				event.IsConsumed = true;
			}
		}

		return event.IsConsumed;
	}
}