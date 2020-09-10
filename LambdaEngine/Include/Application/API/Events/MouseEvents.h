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
		inline MouseEvent(EMouseButton button, ModifierKeyState modiferState)
			: Event(EVENT_FLAG_MOUSE)
			, Button(button)
			, ModiferState(modiferState)
		{
		}

		DECLARE_EVENT_TYPE(MouseEvent);

	public:
		EMouseButton Button;
		ModifierKeyState ModiferState;
	};

	/*
	* MouseClickedEvent
	*/
	struct MouseClickedEvent : public MouseEvent
	{
	public:
		inline MouseClickedEvent(EMouseButton button, ModifierKeyState modiferState)
			: MouseEvent(button, modiferState)
		{
		}

		DECLARE_EVENT_TYPE(MouseClickedEvent);

		virtual String ToString() const
		{
			return String("MouseClickedEvent=[=") + ButtonToString(Button);
		}
	};

	/*
	* MouseReleasedEvent
	*/
	struct MouseReleasedEvent : public MouseEvent
	{
	public:
		inline MouseReleasedEvent(EMouseButton button, ModifierKeyState modiferState)
			: MouseEvent(button, modiferState)
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
		inline MouseScrolledEvent(int32 deltaX, int32 deltaY)
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
		int32 DeltaX;
		int32 DeltaY;
	};

	/*
	* MouseMovedEvent
	*/
	struct MouseMovedEvent : public Event
	{
	public:
		inline MouseMovedEvent(int32 x, int32 y)
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
		inline RawMouseMovedEvent(int32 deltaX, int32 deltaY)
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