#pragma once
#include "EventHandler.h"
#include "KeyEvents.h"

#include "Containers/TUniquePtr.h"

namespace LambdaEngine
{
	/*
	* EventContainer
	*/
	struct EventContainer
	{
	public:
		virtual ~EventContainer() = default;

		inline EventContainer(EventType eventType)
			: ContainerEventType(eventType)
		{
		}

		virtual void Push(const Event& event) = 0;
		virtual void Clear() = 0;
		
		virtual uint32 Size() const = 0;

		virtual EventContainer* Copy(void* pMemory) = 0;
		virtual EventContainer* Move(void* pMemory) = 0;
		
		virtual Event& GetAt(uint32 index) = 0;
		virtual const Event& GetAt(uint32 index) const = 0;

	public:
		EventType ContainerEventType;
	};

	/*
	* TEventContainer
	*/
	template<typename TEvent>
	struct TEventContainer : public EventContainer
	{
	public:
		inline TEventContainer(uint32 size = 1)
			: EventContainer(TEvent::GetStaticType())
			, Events()
		{
			Events.Reserve(size);
		}

		inline TEventContainer(const TEventContainer& other)
			: EventContainer(TEvent::GetStaticType())
			, Events(other.Events)
		{
		}

		inline TEventContainer(TEventContainer&& other)
			: EventContainer(TEvent::GetStaticType())
			, Events(std::move(other.Events))
		{
		}

		virtual void Push(const Event& event) override final
		{
			VALIDATE(ContainerEventType == event.GetType());
			Events.EmplaceBack(EventCast<TEvent>(event));
		}

		virtual void Clear() override final
		{
			Events.Clear();
		}

		virtual uint32 Size() const override final
		{
			return static_cast<uint32>(Events.GetSize());
		}

		virtual EventContainer* Copy(void* pMemory)
		{
			new(pMemory) TEventContainer(*this);
			return reinterpret_cast<EventContainer*>(pMemory);
		}

		virtual EventContainer* Move(void* pMemory)
		{
			new(pMemory) TEventContainer(std::move(*this));
			return reinterpret_cast<EventContainer*>(pMemory);
		}

		virtual Event& GetAt(uint32 index) override final
		{
			VALIDATE(index < Events.GetSize());
			return Events[index];
		}

		virtual const Event& GetAt(uint32 index) const override final
		{
			VALIDATE(index < Events.GetSize());
			return Events[index];
		}

	public:
		TArray<TEvent> Events;
	};

	/*
	* EventContainerProxy
	*/
	struct EventContainerProxy
	{
	public:
		inline EventContainerProxy()
			: m_StackBuffer()
			, m_pContainer(nullptr)
		{
			ZERO_MEMORY(m_StackBuffer, sizeof(m_StackBuffer));
			m_pContainer = nullptr;
		}

		inline EventContainerProxy(const EventContainerProxy& other)
			: m_StackBuffer()
			, m_pContainer(nullptr)
		{
			if (!other.m_pContainer)
			{
				ZERO_MEMORY(m_StackBuffer, sizeof(m_StackBuffer));
				m_pContainer = nullptr;
			}
			else
			{
				m_pContainer = other.m_pContainer->Copy(reinterpret_cast<void*>(m_StackBuffer));
			}
		}

		inline EventContainerProxy(EventContainerProxy&& other)
			: m_StackBuffer()
			, m_pContainer(nullptr)
		{
			if (!other.m_pContainer)
			{
				ZERO_MEMORY(m_StackBuffer, sizeof(m_StackBuffer));
				m_pContainer = nullptr;
			}
			else
			{
				m_pContainer = other.m_pContainer->Move(reinterpret_cast<void*>(m_StackBuffer));
			}
		}

		inline ~EventContainerProxy()
		{
			if (m_pContainer)
			{
				m_pContainer->~EventContainer();
			}
		}

		FORCEINLINE void Push(const Event& event)
		{
			m_pContainer->Push(event);
		}

		FORCEINLINE void Clear()
		{
			m_pContainer->Clear();
		}

		FORCEINLINE Event& GetAt(uint32 index)
		{
			return m_pContainer->GetAt(index);
		}

		FORCEINLINE const Event& GetAt(uint32 index) const
		{
			return m_pContainer->GetAt(index);
		}

		FORCEINLINE uint32 Size() const
		{
			return m_pContainer->Size();
		}

		FORCEINLINE Event& operator[](uint32 index)
		{
			return GetAt(index);
		}

		FORCEINLINE const Event& operator[](uint32 index) const
		{
			return GetAt(index);
		}

		FORCEINLINE EventContainerProxy& operator=(const EventContainerProxy& other)
		{
			if (this != &other)
			{
				if (!other.m_pContainer)
				{
					ZERO_MEMORY(m_StackBuffer, sizeof(m_StackBuffer));
					m_pContainer = nullptr;
				}
				else
				{
					m_pContainer = other.m_pContainer->Copy(reinterpret_cast<void*>(m_StackBuffer));
				}
			}

			return *this;
		}

		FORCEINLINE EventContainerProxy& operator=(EventContainerProxy&& other)
		{
			if (this != &other)
			{
				if (!other.m_pContainer)
				{
					ZERO_MEMORY(m_StackBuffer, sizeof(m_StackBuffer));
					m_pContainer = nullptr;
				}
				else
				{
					m_pContainer = other.m_pContainer->Move(reinterpret_cast<void*>(m_StackBuffer));
				}
			}

			return *this;
		}

		FORCEINLINE bool operator==(const EventContainerProxy& other) const
		{
			return (memcmp(m_StackBuffer, other.m_StackBuffer, sizeof(m_StackBuffer)) == 0);
		}

	public:
		template<typename TEvent>
		inline static EventContainerProxy Create(uint32 count)
		{
			EventContainerProxy container;
			static_assert(sizeof(TEventContainer<TEvent>) <= sizeof(container.m_StackBuffer));

			new(reinterpret_cast<void*>(container.m_StackBuffer)) TEventContainer<TEvent>(count);
			container.m_pContainer = reinterpret_cast<EventContainer*>(container.m_StackBuffer);

			return std::move(container);
		}

	private:
		// No reason to use KeyPressedEvent other than the fact that we need some 
		byte m_StackBuffer[sizeof(TEventContainer<KeyPressedEvent>)];
		EventContainer* m_pContainer;
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

			EventType eventType = event.GetType();
			VALIDATE(eventType == TEvent::GetStaticType());

			auto deferredEvents = s_DeferredEvents.find(eventType);
			if (deferredEvents == s_DeferredEvents.end())
			{
				s_DeferredEvents[eventType] = std::move(EventContainerProxy::Create<TEvent>(5));
			}

			s_DeferredEvents[eventType].Push(event);
		}
		
		static bool SendEventImmediate(Event& event);

		static void Tick();

	private:
		static void InternalSendEvent(Event& event);

	private:
		using EventTable = std::unordered_map<EventType, EventContainerProxy, EventTypeHasher>;

		static EventTable s_DeferredEvents;
		static SpinLock s_EventLock;
	};
}