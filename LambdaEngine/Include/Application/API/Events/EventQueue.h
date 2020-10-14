#pragma once
#include "EventHandler.h"
#include "KeyEvents.h"

#include "Containers/TUniquePtr.h"

#include "Memory/API/Malloc.h"
#include "Memory/API/StackAllocator.h"

namespace LambdaEngine
{
	/*
	* EventContainer
	*/

	class EventContainer
	{
	public:
		inline EventContainer()
			: m_Allocator()
			, m_Events()
		{
		}

		~EventContainer() = default;

		template<typename TEvent>
		FORCEINLINE void Push(const TEvent& event)
		{
			void* pMemory = m_Allocator.Push(sizeof(TEvent));
			new(pMemory) TEvent(event);
			m_Events.EmplaceBack(reinterpret_cast<TEvent*>(pMemory));
		}

		FORCEINLINE Event& At(uint32 index)
		{
			return *m_Events[index];
		}

		FORCEINLINE const Event& At(uint32 index) const
		{
			return *m_Events[index];
		}

		FORCEINLINE uint32 Size() const
		{
			return m_Events.GetSize();
		}

		FORCEINLINE void Clear()
		{
			// Call destructor for all events
			for (Event* pEvent : m_Events)
			{
				pEvent->~Event();
			}
			
			// Clear containers
			m_Events.Clear();
			m_Allocator.Reset();
		}

		FORCEINLINE Event& operator[](uint32 index)
		{
			return At(index);
		}

		FORCEINLINE const Event& operator[](uint32 index) const
		{
			return At(index);
		}

	private:
		StackAllocator m_Allocator;
		TArray<Event*> m_Events;
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

		static bool UnregisterEventHandlerForAllTypes(const EventHandler& eventHandler);

		static void UnregisterAll();

		template<typename TEvent>
		inline static void SendEvent(const TEvent& event)
		{
			std::scoped_lock<SpinLock> lock(s_EventLock);

			VALIDATE(event.GetType() == TEvent::GetStaticType());
			s_DeferredEvents.Push(event);
		}
		
		static bool SendEventImmediate(Event& event);

		static void Tick();

	private:
		static void InternalSendEventToHandlers(Event& event, const TArray<EventHandler>& handlers);

	private:
		static EventContainer s_DeferredEvents;
		static SpinLock s_EventLock;
	};
}