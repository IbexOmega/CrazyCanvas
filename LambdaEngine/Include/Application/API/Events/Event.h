#pragma once
#include "LambdaEngine.h"

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
	* Base for all events
	*/
	struct Event
	{
	public:
		inline explicit Event(FEventFlags eventFlags)
			: EventFlags(eventFlags)
		{
		}

		inline bool HasEventFlags(FEventFlags flags) const
		{
			return ((EventFlags & flags) != 0);
		}

	public:
		FEventFlags EventFlags;
	};
}