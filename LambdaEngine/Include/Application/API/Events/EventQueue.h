#pragma once
#include "EventHandler.h"

#include "Containers/TUniquePtr.h"

namespace LambdaEngine
{
	/*
	* TEventContainer
	*/
	struct EventContainer
	{
	public:
		virtual void Push(const Event& event) = 0;
		virtual const Event& Get(int32 index) = 0;
		virtual void Clear() = 0;

	public:
		EventType EventType;
	};

	/*
	* TEventContainer
	*/
	template<typename TEvent>
	struct TEventContainer : public EventContainer
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

	/*
	* EventQueue
	*/
	class EventQueue
	{
	public:
		template<typename TEvent>
		inline static bool RegisterEventHandler(const EventHandler& eventHandler)
		{
			static_assert(std::is_base_of<Event, TEvent>());
			return RegisterEventHandler(TEvent::GetStaticType(), eventHandler);
		}

		template<typename TEvent>
		inline static bool UnregisterEventHandler(const EventHandler& eventHandler)
		{
			static_assert(std::is_base_of<Event, TEvent>());
			return UnregisterEventHandler(TEvent::GetStaticType(), eventHandler);
		}

		template<typename TEvent>
		inline static bool RegisterEventHandler(bool(*function)(const TEvent&))
		{
			static_assert(std::is_base_of<Event, TEvent>());
			return RegisterEventHandler(TEvent::GetStaticType(), EventHandler(function));
		}

		template<typename TEvent>
		inline static bool UnregisterEventHandler(bool(*function)(const TEvent&))
		{
			static_assert(std::is_base_of<Event, TEvent>());
			return UnregisterEventHandler(TEvent::GetStaticType(), EventHandler(function));
		}

		template<typename TEvent, typename T>
		inline static bool RegisterEventHandler(T* pThis, bool(T::* memberFunc)(const TEvent&))
		{
			static_assert(std::is_base_of<Event, TEvent>());
			return RegisterEventHandler(TEvent::GetStaticType(), EventHandler(pThis, memberFunc));
		}

		template<typename TEvent, typename T>
		inline static bool UnregisterEventHandler(T* pThis, bool(T::* memberFunc)(const TEvent&))
		{
			static_assert(std::is_base_of<Event, TEvent>());
			return UnregisterEventHandler(TEvent::GetStaticType(), EventHandler(pThis, memberFunc));
		}

		static bool RegisterEventHandler(EventType eventType, const EventHandler& eventHandler);
		static bool UnregisterEventHandler(EventType eventType, const EventHandler& eventHandler);

		static void UnregisterAll();

		static void SendEvent(const Event& event);
		static bool SendEventImmediate(Event& event);

		static void Tick();

	private:
		static std::unordered_map<EventType, EventContainer*, EventTypeHasher> s_DeferredEvents;
	};
}