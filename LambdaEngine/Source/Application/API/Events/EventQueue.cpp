#include "Application/API/Events/EventQueue.h"
#include <unordered_set>

namespace LambdaEngine
{
	SpinLock				EventQueue::s_EventLock;
	EventQueue::EventTable	EventQueue::s_DeferredEvents;

	static std::unordered_map<EventType, TArray<EventHandler>, EventTypeHasher> g_EventHandlers;
	static SpinLock g_EventHandlersSpinlock;

	/*
	* EventQueue
	*/
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

	void EventQueue::UnregisterAll()
	{
		std::scoped_lock<SpinLock> lock(g_EventHandlersSpinlock);
		g_EventHandlers.clear();
	}
	
	bool EventQueue::SendEventImmediate(Event& event)
	{
		InternalSendEvent(event);
		return event.IsConsumed;
	}
	
	void EventQueue::Tick()
	{
		// Copy eventcontainers
		EventTable containersToProcess;
		{
			std::scoped_lock<SpinLock> lock(s_EventLock);
			containersToProcess = s_DeferredEvents;
			for (auto& containerPair : s_DeferredEvents)
			{
				EventContainerProxy& container = containerPair.second;
				container.Clear();
			}
		}

		// Process events
		for (auto& containerPair : containersToProcess)
		{
			EventContainerProxy& container = containerPair.second;
			for (uint32 i = 0; i < container.Size(); i++)
			{
				InternalSendEvent(container[i]);
			}
			container.Clear();
		}
	}
	
	void EventQueue::InternalSendEvent(Event& event)
	{
		EventType eventType = event.GetType();

		std::scoped_lock<SpinLock> lock(g_EventHandlersSpinlock);

		auto handlerPair = g_EventHandlers.find(eventType);
		if (handlerPair != g_EventHandlers.end())
		{
			TArray<EventHandler> eventHandlers = handlerPair->second;
			for (EventHandler& handler : eventHandlers)
			{
				// If true then set that this element is consumed
				if (handler(event))
				{
					event.IsConsumed = true;
				}
			}
		}
	}
}