#include "Application/API/Events/EventQueue.h"
#include <unordered_set>

namespace LambdaEngine
{
	struct EventContainer
	{
	public:
		virtual void Push(const Event& event) = 0;
		virtual void Clear() = 0;

	public:
		EventType EventType;
	};

	template<typename TEvent>
	struct TEventContainer : EventContainer
	{
	public:
		virtual void Push(const Event& event) override final
		{
			VALIDATE(TEvent::GetStaticType() == event.GetType());
			Events.EmplaceBack(static_cast<TEvent>(event));
		}

		virtual void Clear() override final
		{
			Events.Clear();
		}

	public:
		TArray<TEvent> Events;
	};

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
	
	void EventQueue::SendEvent(const Event& event)
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

		return event.IsConsumed;
	}

	bool EventQueue::SendEventImmediate(Event& event)
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

		return event.IsConsumed;
	}
}