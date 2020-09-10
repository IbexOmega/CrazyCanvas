#include "Application/API/Events/EventQueue.h"
#include <unordered_set>

namespace LambdaEngine
{
	static std::unordered_map<FEventFlags, TArray<EventHandler>> g_EventHandlers;
	static SpinLock g_EventHandlersSpinlock;

	/*
	* EventQueue
	*/
	bool EventQueue::RegisterEventHandler(const EventHandler& eventHandler)
	{
		std::scoped_lock<SpinLock> lock(g_EventHandlersSpinlock);
		FEventFlags flags = eventHandler.GetEventFlags();
		
		auto handlerList = g_EventHandlers.find(flags);
		if (handlerList != g_EventHandlers.end())
		{
			for (const EventHandler& handler : handlerList->second)
			{
				if (handler == eventHandler)
				{
					return false;
				}
			}

			handlerList->second.EmplaceBack(eventHandler);
			return true;
		}
		else
		{
			return true;
		}
	}
	
	bool EventQueue::UnregisterEventHandler(const EventHandler& eventHandler)
	{
		std::scoped_lock<SpinLock> lock(g_EventHandlersSpinlock);
		for (auto it = g_EventHandlers.Begin(); it != g_EventHandlers.End(); it++)
		{
			if ((*it) == eventHandler)
			{
				g_EventHandlers.Erase(it);
				return true;
			}
		}

		return false;
	}
	
	bool EventQueue::SendEvent(Event& event)
	{
		std::scoped_lock<SpinLock> lock(g_EventHandlersSpinlock);
		for (EventHandler& handler : g_EventHandlers)
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