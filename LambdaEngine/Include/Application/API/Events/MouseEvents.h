#pragma once
#include "Event.h"

#include "Input/API/InputCodes.h"

namespace LambdaEngine
{
	/*
	* Base class for mouseevents
	*/
	struct MouseEvent : public Event
	{
	public:
		inline explicit MouseEvent(EMouseButton button)
			: Event(EVENT_FLAG_MOUSE)
			, Button(button)
		{
		}

	public:
		EMouseButton Button;
	};

	/*
	* MouseClickedEvent
	*/
	struct MouseClickedEvent : public MouseEvent
	{
	public:
		inline explicit MouseClickedEvent(EMouseButton button)
			: MouseEvent(button)
		{
		}
	};

	/*
	* MouseReleasedEvent
	*/
	struct MouseReleasedEvent : public MouseEvent
	{
	public:
		inline explicit MouseReleasedEvent(EMouseButton button)
			: MouseEvent(button)
		{
		}
	};

	/*
	* MouseScrolledEvent
	*/
	struct MouseScrolledEvent : public Event
	{
	public:
		inline explicit MouseScrolledEvent(float32 horizontal, float32 vertical)
			: Event(EVENT_FLAG_MOUSE)
			, Horizontal(horizontal)
			, Vertical(vertical)
		{
		}

	public:
		float32 Horizontal;
		float32 Vertical;
	};

	/*
	* MouseMovedEvent
	*/
	struct MouseMovedEvent : public Event
	{
	public:
	};
}