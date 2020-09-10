#pragma once
#include "EventHandler.h"

namespace LambdaEngine
{
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
		inline static bool RegisterEventHandler(T* pThis, bool(T::*memberFunc)(const TEvent&))
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

		static bool SendEvent(Event& event);
	};
}