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
		inline explicit MouseClickedEvent(EMouseButton button, ModifierKeyState modiferState)
			: MouseEvent(button)
			, ModiferState(modiferState)
		{
		}

		DECLARE_EVENT_TYPE(MouseClickedEvent);

		inline virtual String ToString() const
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
		inline explicit MouseReleasedEvent(EMouseButton button)
			: MouseEvent(button)
		{
		}

		DECLARE_EVENT_TYPE(MouseReleasedEvent);

		inline virtual String ToString() const
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
		inline explicit MouseScrolledEvent(float32 deltaX, float32 deltaY)
			: Event(EVENT_FLAG_MOUSE)
			, DeltaX(deltaX)
			, DeltaY(deltaY)
		{
		}

		DECLARE_EVENT_TYPE(MouseScrolledEvent);

		inline virtual String ToString() const
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
		inline explicit MouseMovedEvent(int32 x, int32 y)
			: Event(EVENT_FLAG_MOUSE)
			, X(x)
			, Y(y)
		{
		}

		DECLARE_EVENT_TYPE(MouseMovedEvent);

		inline virtual String ToString() const
		{
			return String("MouseMovedEvent=[") + std::to_string(X) + ", " + std::to_string(Y) + "]";
		}

	public:
		int32 X;
		int32 Y;
	};

	/*
	* RawMouseMovedEvent
	*/
	struct RawMouseMovedEvent : public Event
	{
	public:
		inline explicit RawMouseMovedEvent(int32 deltaX, int32 deltaY)
			: Event(EVENT_FLAG_MOUSE)
			, DeltaX(deltaX)
			, DeltaY(deltaY)
		{
		}

		DECLARE_EVENT_TYPE(RawMouseMovedEvent);

		inline virtual String ToString() const
		{
			return String("RawMouseMovedEvent=[") + std::to_string(DeltaX) + ", " + std::to_string(DeltaY) + "]";
		}

	public:
		int32 DeltaX;
		int32 DeltaY;
	};
}