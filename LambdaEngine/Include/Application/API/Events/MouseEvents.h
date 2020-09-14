#pragma once
#include "Event.h"

#include "Input/API/InputCodes.h"

namespace LambdaEngine
{
	/*
	* MouseButtonClickedEvent
	*/
	struct MouseButtonClickedEvent : public Event
	{
	public:
		inline MouseButtonClickedEvent(EMouseButton button, ModifierKeyState modiferState)
			: Event()
			, Button(button)
			, ModiferState(modiferState)
		{
		}

		DECLARE_EVENT_TYPE(MouseButtonClickedEvent);

		virtual String ToString() const
		{
			return String("MouseButtonClickedEvent=[=") + ButtonToString(Button);
		}

	public:
		EMouseButton Button;
		ModifierKeyState ModiferState;
	};

	/*
	* MouseButtonReleasedEvent
	*/
	struct MouseButtonReleasedEvent : public Event
	{
	public:
		inline MouseButtonReleasedEvent(EMouseButton button, ModifierKeyState modiferState)
			: Event()
			, Button(button)
			, ModiferState(modiferState)
		{
		}

		DECLARE_EVENT_TYPE(MouseButtonReleasedEvent);

		virtual String ToString() const
		{
			return String("MouseButtonReleasedEvent=[=") + ButtonToString(Button);
		}

	public:
		EMouseButton Button;
		ModifierKeyState ModiferState;
	};

	/*
	* MouseScrolledEvent
	*/
	struct MouseScrolledEvent : public Event
	{
	public:
		inline MouseScrolledEvent(int32 deltaX, int32 deltaY)
			: Event()
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
			: Event()
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
			: Event()
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