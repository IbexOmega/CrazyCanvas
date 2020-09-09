#pragma once
#include "LambdaEngine.h"

#include "Containers/String.h"

#include "Utilities/StringHash.h"

namespace LambdaEngine
{
	/*
	* EventFlags
	*/
	enum FEventFlag : uint32
	{
		EVENT_FLAG_NONE		= 0,

		EVENT_FLAG_MOUSE	= FLAG(1),
		EVENT_FLAG_KEYBOARD	= FLAG(2),
		EVENT_FLAG_INPUT	= EVENT_FLAG_MOUSE | EVENT_FLAG_KEYBOARD,

		EVENT_FLAG_WINDOW	= FLAG(3),
		EVENT_FLAG_OTHER	= FLAG(4),
	};

	typedef uint32 FEventFlags;

	/*
	* Define for declaring events
	*/
#define DECLARE_EVENT_TYPE(Type) \
	public: \
		inline static constexpr uint64 GetStaticType() \
		{ \
			constexpr uint64 TYPE_HASH = HashString(#Type); \
			return TYPE_HASH; \
		} \
	public: \
		virtual uint64 GetType() const override \
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
		Event(FEventFlags eventFlags)
			: EventFlags(eventFlags)
			, IsConsumed(false)
		{
		}

		virtual String ToString() const = 0;
		virtual uint64 GetType() const = 0;
		virtual const char* GetName() const = 0;

		FORCEINLINE bool HasEventFlags(FEventFlags flags) const
		{
			return ((EventFlags & flags) != 0);
		}

	public:
		FEventFlags EventFlags;
		bool IsConsumed;

	public:
		FORCEINLINE static constexpr uint64 GetStaticType()
		{
			return 0;
		}
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