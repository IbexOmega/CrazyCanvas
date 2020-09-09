#pragma once
#include "LambdaEngine.h"

#include "Containers/String.h"

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
	};

	typedef uint32 FEventFlags;

	/*
	* Define for declaring events
	*/
#define DECLARE_EVENT_TYPE(Type) \
	public: \
		inline static uint32 GetStaticType() \
		{ \
			return 1; \
		} \
	public: \
		inline virtual uint32 GetType() const override \
		{ \
			return GetStaticType(); \
		} \
		inline virtual const char* GetName() const override \
		{ \
			return #Type; \
		} \

	/*
	* Base for all events
	*/
	struct Event
	{
	public:
		inline explicit Event(FEventFlags eventFlags)
			: EventFlags(eventFlags)
		{
		}

		virtual const char* GetName() const = 0;

		inline virtual uint32 GetType() const
		{
			return GetStaticType();
		}

		inline virtual String ToString() const
		{
			return GetName();
		}

		inline bool HasEventFlags(FEventFlags flags) const
		{
			return ((EventFlags & flags) != 0);
		}

		inline FEventFlags GetEventFlags() const
		{
			return EventFlags;
		}

		

	public:
		inline static uint32 GetStaticType()
		{
			return 0;
		}

	public:
		FEventFlags EventFlags;
	};
}