#pragma once
#include "Event.h"

#include "Input/API/InputCodes.h"
#include "Input/API/InputState.h"

namespace LambdaEngine
{
	/*
	* MouseButtonClickedEvent
	* bool OnMouseButtonClicked(const MouseButtonClickedEvent& mouseButtonClickedEvent);
	*/
	struct MouseButtonClickedEvent : public Event
	{
	public:
		inline MouseButtonClickedEvent(EMouseButton button, ModifierKeyState modiferState, int32 x, int32 y)
			: Event()
			, Button(button)
			, ModiferState(modiferState)
			, Position({x, y})
		{
		}

		DECLARE_EVENT_TYPE(MouseButtonClickedEvent);

		virtual String ToString() const override
		{
			return String("MouseButtonClickedEvent=[=") + ButtonToString(Button);
		}

	public:
		EMouseButton Button;
		ModifierKeyState ModiferState;
		struct
		{
			int32 x;
			int32 y;
		} Position;
	};

	/*
	* MouseButtonReleasedEvent
	* bool OnMouseButtonReleased(const MouseButtonReleasedEvent& mouseButtonReleasedEvent);
	*/
	struct MouseButtonReleasedEvent : public Event
	{
	public:
		inline MouseButtonReleasedEvent(EMouseButton button, ModifierKeyState modiferState, int32 x, int32 y)
			: Event()
			, Button(button)
			, ModiferState(modiferState)
			, Position({ x, y })
		{
		}

		DECLARE_EVENT_TYPE(MouseButtonReleasedEvent);

		virtual String ToString() const override
		{
			return String("MouseButtonReleasedEvent=[=") + ButtonToString(Button);
		}

	public:
		EMouseButton Button;
		ModifierKeyState ModiferState;
		struct
		{
			int32 x;
			int32 y;
		} Position;
	};

	/*
	* MouseScrolledEvent
	* bool OnMouseScrolled(const MouseScrolledEvent& mouseScrolledEvent);
	*/
	struct MouseScrolledEvent : public Event
	{
	public:
		inline MouseScrolledEvent(int32 deltaX, int32 deltaY, int32 x, int32 y)
			: Event()
			, DeltaX(deltaX)
			, DeltaY(deltaY)
			, Position({ x, y })
		{
		}

		DECLARE_EVENT_TYPE(MouseScrolledEvent);

		virtual String ToString() const override
		{
			return String("MouseScrolledEvent=[Horizontal=") + std::to_string(DeltaX) + ", Vertical=" + std::to_string(DeltaY) + "]";
		}

	public:
		int32 DeltaX;
		int32 DeltaY;
		struct
		{
			int32 x;
			int32 y;
		} Position;
	};

	/*
	* MouseMovedEvent
	* bool OnMouseMoved(const MouseMovedEvent& mouseMovedEvent);
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

		virtual String ToString() const override
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
	* bool OnRawMouseMoved(const RawMouseMovedEvent& rawMouseMovedEvent);
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
		 
		virtual String ToString() const override
		{
			return String("RawMouseMovedEvent=[") + std::to_string(DeltaX) + ", " + std::to_string(DeltaY) + "]";
		}

	public:
		int32 DeltaX;
		int32 DeltaY;
	};
}