#pragma once
#include "LambdaEngine.h"

#include "Containers/String.h"

#include "Utilities/StringHash.h"

namespace LambdaEngine
{
	/*
	* EventType
	*/
	struct EventType
	{
	public:
		using EventTypeID = uint64;

		inline constexpr EventType(EventTypeID id, const char* pInName)
			: ID(id)
			, pName(pInName)
		{
		}

		inline EventType(const EventType& other)
			: ID(other.ID)
			, pName(other.pName)
		{
		}

		FORCEINLINE size_t GetHash() const
		{
			return std::hash<EventTypeID>()(ID);
		}

		FORCEINLINE bool operator==(const EventType& other) const
		{
			return (ID == other.ID) && (pName == other.pName);
		}

		FORCEINLINE bool operator!=(const EventType& other) const
		{
			return (ID != other.ID) && (pName != other.pName);
		}

		FORCEINLINE EventType& operator=(const EventType& other)
		{
			if (this != &other)
			{
				ID = other.ID;
				pName = other.pName;
			}

			return *this;
		}

	public:
		EventTypeID ID;
		const char* pName;
	};

	struct EventTypeHasher
	{
		size_t operator()(const EventType& type) const
		{
			return type.GetHash();
		}
	};

	/*
	* Define for declaring events
	*/
	#define DECLATE_STATIC_EVENT_TYPE(Type) \
		public: \
			FORCEINLINE static EventType GetStaticType() \
			{ \
				constexpr EventType type(HashString(#Type), #Type); \
				return type; \
			} \

	#define DECLARE_EVENT_TYPE(Type) \
		DECLATE_STATIC_EVENT_TYPE(Type); \
		public: \
			virtual EventType GetType() const override \
			{ \
				return GetStaticType(); \
			} \
			virtual const char* GetName() const override \
			{ \
				return #Type; \
			} \

	/*
	* Base for all events
	*/
	struct Event
	{
	public:
		inline Event()
			: IsConsumed(false)
		{
		}

		virtual ~Event() = default;

		DECLATE_STATIC_EVENT_TYPE(Event);

		virtual String ToString() const = 0;
		virtual EventType GetType() const = 0;
		virtual const char* GetName() const = 0;

	public:
		bool IsConsumed;
	};

	/*
	* Event cast
	*/
	template<typename TEvent>
	inline bool IsEventOfType(const Event& event)
	{
		static_assert(std::is_base_of<Event, TEvent>());
		return event.GetType() == TEvent::GetStaticType();
	}

	template<typename TEvent>
	inline TEvent& EventCast(Event& event)
	{
		static_assert(std::is_base_of<Event, TEvent>());
		return static_cast<TEvent&>(event);
	}

	template<typename TEvent>
	inline const TEvent& EventCast(const Event& event)
	{
		static_assert(std::is_base_of<Event, TEvent>());
		return static_cast<const TEvent&>(event);
	}

	template<typename TEvent>
	inline bool DispatchEvent(const Event& event, bool(*function)(const TEvent&))
	{
		if (IsEventOfType<TEvent>(event))
		{
			return function(EventCast<TEvent>(event));
		}

		return false;
	}

	template<typename TEvent, typename T>
	inline bool DispatchEvent(const Event& event, bool(T::*function)(const TEvent&), T* pObject)
	{
		if (IsEventOfType<TEvent>(event))
		{
			return ((*pObject).*(function))(EventCast<TEvent>(event));
		}

		return false;
	}
}