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
		MouseEvent(EMouseButton button)
			: Event(EVENT_FLAG_MOUSE)
			, Button(button)
		{
		}

		DECLARE_EVENT_TYPE(MouseEvent);

	public:
		EMouseButton Button;
	};

	/*
	* MouseClickedEvent
	*/
	struct MouseClickedEvent : public MouseEvent
	{
	public:
		MouseClickedEvent(EMouseButton button, ModifierKeyState modiferState)
			: MouseEvent(button)
			, ModiferState(modiferState)
		{
		}

		DECLARE_EVENT_TYPE(MouseClickedEvent);

		virtual String ToString() const
		{
			return String("MouseClickedEvent=[=") + ButtonToString(Button);
		}

	public:
		ModifierKeyState ModiferState;
	};

	/*
	* MouseReleasedEvent
	*/
	struct MouseReleasedEvent : public MouseEvent
	{
	public:
		MouseReleasedEvent(EMouseButton button)
			: MouseEvent(button)
		{
		}

		DECLARE_EVENT_TYPE(MouseReleasedEvent);

		virtual String ToString() const
		{
			return String("MouseReleasedEvent=[=") + ButtonToString(Button);
		}
	};

	/*
	* MouseScrolledEvent
	*/
	struct MouseScrolledEvent : public Event
	{
	public:
		MouseScrolledEvent(float32 deltaX, float32 deltaY)
			: Event(EVENT_FLAG_MOUSE)
			, DeltaX(deltaX)
			, DeltaY(deltaY)
		{
		}

		DECLARE_EVENT_TYPE(MouseScrolledEvent);

		virtual String ToString() const
		{
			return String("MouseScrolledEvent=[Horizontal=") + std::to_string(DeltaX) + ", Vertical=" + std::to_string(DeltaY) + "]";
		}

	public:
		float32 DeltaX;
		float32 DeltaY;
	};

	/*
	* MouseMovedEvent
	*/
	struct MouseMovedEvent : public Event
	{
	public:
		MouseMovedEvent(int32 x, int32 y)
			: Event(EVENT_FLAG_MOUSE)
			, Position({ x, y })
		{
		}

		DECLARE_EVENT_TYPE(MouseMovedEvent);

		virtual String ToString() const
		{
			return String("MouseMovedEvent=[") + std::to_string(Position.x) + ", " + std::to_string(Position.y) + "]";
		}

	public:
		struct
		{
			int32 x;
			int32 y;
		} Position;
	};

	/*
	* RawMouseMovedEvent
	*/
	struct RawMouseMovedEvent : public Event
	{
	public:
		RawMouseMovedEvent(int32 deltaX, int32 deltaY)
			: Event(EVENT_FLAG_MOUSE)
			, DeltaX(deltaX)
			, DeltaY(deltaY)
		{
		}

		DECLARE_EVENT_TYPE(RawMouseMovedEvent);

		virtual String ToString() const
		{
			return String("RawMouseMovedEvent=[") + std::to_string(DeltaX) + ", " + std::to_string(DeltaY) + "]";
		}

	public:
		int32 DeltaX;
		int32 DeltaY;
	};
}