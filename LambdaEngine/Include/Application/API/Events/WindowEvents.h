#pragma once
#include "Event.h"

#include "Application/API/Window.h"

namespace LambdaEngine
{
	/*
	* WindowEvent
	*/
	struct WindowEvent : public Event
	{
	public:
		inline explicit WindowEvent(TSharedRef<Window> window)
			: Event(0)
			, Window(window)
		{
		}

		DECLARE_EVENT_TYPE(WindowEvent);

	public:
		TSharedRef<Window> Window;
	};

	/*
	* FocusChangedEvent
	*/
	struct FocusChangedEvent : public WindowEvent
	{
	public:
		inline explicit FocusChangedEvent(TSharedRef<Window> window, bool hasFocus)
			: WindowEvent(window)
			, HasFocus(hasFocus)
		{
		}

		DECLARE_EVENT_TYPE(FocusChangedEvent);

		inline virtual String ToString() const
		{
			return String("FocusChangedEvent=") + std::to_string(HasFocus);
		}

	public:
		bool HasFocus;
	};

	/*
	* WindowMovedEvent
	*/
	struct WindowMovedEvent : public WindowEvent
	{
	public:
		inline explicit WindowMovedEvent(TSharedRef<Window> window, int32 x, int32 y)
			: WindowEvent(window)
			, Position({ x, y })
		{
		}

		DECLARE_EVENT_TYPE(WindowMovedEvent);

		inline virtual String ToString() const
		{
			return String("WindowMovedEvent=[x, ") + std::to_string(Position.x) + ", y=" + std::to_string(Position.y) + "]";
		}

	public:
		struct
		{
			int32 x;
			int32 y;
		} Position;
	};

	/*
	* WindowResizedEvent
	*/
	struct WindowResizedEvent : public WindowEvent
	{
	public:
		inline explicit WindowResizedEvent(TSharedRef<Window> window, uint32 width, uint32 height)
			: WindowEvent(window)
			, Width(width)
			, Height(height)
		{
		}

		DECLARE_EVENT_TYPE(WindowResizedEvent);

		inline virtual String ToString() const
		{
			return String("WindowResizedEvent=[Width, ") + std::to_string(Width) + ", Height=" + std::to_string(Height) + "]";
		}

	public:
		uint32 Width;
		uint32 Height;
	};

	/*
	* WindowMouseLeftEvent
	*/
	struct WindowMouseLeftEvent : public WindowEvent
	{
	public:
		inline explicit WindowMouseLeftEvent(TSharedRef<Window> window)
			: WindowEvent(window)
		{
		}

		DECLARE_EVENT_TYPE(WindowMouseLeftEvent);

		inline virtual String ToString() const
		{
			return "WindowMouseLeftEvent";
		}
	};

	/*
	* WindowMouseEnteredEvent
	*/
	struct WindowMouseEnteredEvent : public WindowEvent
	{
	public:
		inline explicit WindowMouseEnteredEvent(TSharedRef<Window> window)
			: WindowEvent(window)
		{
		}

		DECLARE_EVENT_TYPE(WindowMouseEnteredEvent);

		inline virtual String ToString() const
		{
			return "WindowMouseEnteredEvent";
		}
	};

	/*
	* WindowClosedEvent
	*/
	struct WindowClosedEvent : public WindowEvent
	{
	public:
		inline explicit WindowClosedEvent(TSharedRef<Window> window)
			: WindowEvent(window)
		{
		}

		DECLARE_EVENT_TYPE(WindowClosedEvent);

		inline virtual String ToString() const
		{
			return "WindowClosedEvent";
		}
	};
}