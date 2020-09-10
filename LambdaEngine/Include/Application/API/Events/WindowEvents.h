#pragma once
#include "Event.h"

#include "Core/TSharedRef.h"

#include "Application/API/Window.h"

namespace LambdaEngine
{
	/*
	* WindowEvent
	*/
	struct WindowEvent : public Event
	{
	public:
		inline WindowEvent(TSharedRef<Window> window)
			: Event(0)
			, EventWindow(window)
		{
		}

		DECLARE_EVENT_TYPE(WindowEvent);

	public:
		TSharedRef<Window> EventWindow;
	};

	/*
	* FocusChangedEvent
	*/
	struct FocusChangedEvent : public WindowEvent
	{
	public:
		inline FocusChangedEvent(TSharedRef<Window> window, bool hasFocus)
			: WindowEvent(window)
			, HasFocus(hasFocus)
		{
		}

		DECLARE_EVENT_TYPE(FocusChangedEvent);

		virtual String ToString() const
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
		inline WindowMovedEvent(TSharedRef<Window> window, int32 x, int32 y)
			: WindowEvent(window)
			, Position({ x, y })
		{
		}

		DECLARE_EVENT_TYPE(WindowMovedEvent);

		virtual String ToString() const
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
		inline WindowResizedEvent(TSharedRef<Window> window, uint32 width, uint32 height, EResizeType resizeType)
			: WindowEvent(window)
			, Width(width)
			, Height(height)
			, ResizeType(resizeType)
		{
		}

		DECLARE_EVENT_TYPE(WindowResizedEvent);

		virtual String ToString() const
		{
			return String("WindowResizedEvent=[Width, ") + std::to_string(Width) + ", Height=" + std::to_string(Height) + "]";
		}

	public:
		uint32 Width;
		uint32 Height;
		EResizeType ResizeType;
	};

	/*
	* WindowMouseLeftEvent
	*/
	struct WindowMouseLeftEvent : public WindowEvent
	{
	public:
		inline WindowMouseLeftEvent(TSharedRef<Window> window)
			: WindowEvent(window)
		{
		}

		DECLARE_EVENT_TYPE(WindowMouseLeftEvent);

		virtual String ToString() const
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
		inline WindowMouseEnteredEvent(TSharedRef<Window> window)
			: WindowEvent(window)
		{
		}

		DECLARE_EVENT_TYPE(WindowMouseEnteredEvent);

		virtual String ToString() const
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
		inline WindowClosedEvent(TSharedRef<Window> window)
			: WindowEvent(window)
		{
		}

		DECLARE_EVENT_TYPE(WindowClosedEvent);

		virtual String ToString() const
		{
			return "WindowClosedEvent";
		}
	};
}