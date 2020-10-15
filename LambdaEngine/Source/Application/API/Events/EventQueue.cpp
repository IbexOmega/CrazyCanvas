#include "Application/API/Events/EventQueue.h"
#include <unordered_set>

namespace LambdaEngine
{
	/*
	* Global data for this compilation unit
	*/

	static THashTable<EventType, TArray<EventHandler>, EventTypeHasher> g_EventHandlers;
	static SpinLock g_EventHandlersSpinlock;

	// This function returns a copy to avoid having to lock
	static TArray<EventHandler> GetEventHandlerOfType(EventType type)
	{
		std::scoped_lock<SpinLock> lock(g_EventHandlersSpinlock);
		
		auto handlers = g_EventHandlers.find(type);
		if (handlers != g_EventHandlers.end())
		{
			return handlers->second;
		}
		else
		{
			return TArray<EventHandler>();
		}
	}

	/*
	* EventQueue
	*/

	SpinLock		EventQueue::s_WriteLock;
	EventContainer	EventQueue::s_DeferredEvents[2];
	uint32			EventQueue::s_ReadIndex		= 0;
	uint32			EventQueue::s_WriteIndex	= 1;
	
	bool EventQueue::RegisterEventHandler(EventType eventType, const EventHandler& eventHandler)
	{
		std::scoped_lock<SpinLock> lock(g_EventHandlersSpinlock);
	
		TArray<EventHandler>& eventHandlers = g_EventHandlers[eventType];
		for (const EventHandler& handler : eventHandlers)
		{
			if (handler == eventHandler)
			{
				return false;
			}
		}

		eventHandlers.EmplaceBack(eventHandler);

		LOG_INFO("Register eventhandler. Eventtype=%s", eventType.pName);
		return true;
	}

	bool EventQueue::UnregisterEventHandler(EventType eventType, const EventHandler& eventHandler)
	{
		std::scoped_lock<SpinLock> lock(g_EventHandlersSpinlock);

		auto handlerPair = g_EventHandlers.find(eventType);
		if (handlerPair != g_EventHandlers.end())
		{
			TArray<EventHandler>& eventHandlers = handlerPair->second;
			for (auto it = eventHandlers.Begin(); it != eventHandlers.End(); it++)
			{
				if ((*it) == eventHandler)
				{
					eventHandlers.Erase(it);

					LOG_INFO("Unregister eventhandler. Eventtype=%s", eventType.pName);
					return true;
				}
			}
		}

		return false;
	}

	bool EventQueue::UnregisterEventHandlerForAllTypes(const EventHandler& eventHandler)
	{
		std::scoped_lock<SpinLock> lock(g_EventHandlersSpinlock);

		for (auto& handlerPair : g_EventHandlers)
		{
			TArray<EventHandler>& eventHandlers = handlerPair.second;
			for (auto it = eventHandlers.Begin(); it != eventHandlers.End(); it++)
			{
				if ((*it) == eventHandler)
				{
					eventHandlers.Erase(it);
				}
			}
		}

		LOG_INFO("Unregister eventhandler from all types");
		return true;
	}

	void EventQueue::UnregisterAll()
	{
		std::scoped_lock<SpinLock> lock(g_EventHandlersSpinlock);
		g_EventHandlers.clear();
	}
	
	bool EventQueue::SendEventImmediate(Event& event)
	{
		TArray<EventHandler> handlers = GetEventHandlerOfType(event.GetType());
		InternalSendEventToHandlers(event, handlers);
		return event.IsConsumed;
	}
	
	void EventQueue::Tick()
	{
		// Process events
		EventContainer& container = s_DeferredEvents[s_ReadIndex];
		for (uint32 i = 0; i < container.Size(); i++)
		{
			Event& event = container[i];

			TArray<EventHandler> handlers = GetEventHandlerOfType(event.GetType());
			InternalSendEventToHandlers(event, handlers);
		}

		container.Clear();

		{
			std::scoped_lock<SpinLock> lock(s_WriteLock);
			std::swap(s_WriteIndex, s_ReadIndex);
		}
	}

	void EventQueue::Release()
	{
		s_DeferredEvents[0].Clear();
		s_DeferredEvents[1].Clear();
	}
	
	void EventQueue::InternalSendEventToHandlers(Event& event, const TArray<EventHandler>& handlers)
	{
		for (const EventHandler& handler : handlers)
		{
			// If true then set that this element is consumed
			if (handler(event))
			{
				event.IsConsumed = true;
			}
		}
	}
}