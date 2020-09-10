#pragma once
#include "Event.h"

#include "Core/TSharedRef.h"

#include "Application/API/Window.h"

namespace LambdaEngine
{
	/*
	* FocusChangedEvent
	*/
	struct FocusChangedEvent : public Event
	{
	public:
		inline FocusChangedEvent(TSharedRef<Window> window, bool hasFocus)
			: Event()
			, EventWindow(window)
			, HasFocus(hasFocus)
		{
		}

		DECLARE_EVENT_TYPE(FocusChangedEvent);

		virtual String ToString() const
		{
			return String("FocusChangedEvent=") + std::to_string(HasFocus);
		}

	public:
		TSharedRef<Window> EventWindow;
		bool HasFocus;
	};

	/*
	* WindowMovedEvent
	*/
	struct WindowMovedEvent : public Event
	{
	public:
		inline WindowMovedEvent(TSharedRef<Window> window, int32 x, int32 y)
			: Event()
			, EventWindow(window)
			, Position({ x, y })
		{
		}

		DECLARE_EVENT_TYPE(WindowMovedEvent);

		virtual String ToString() const
		{
			return String("WindowMovedEvent=[x, ") + std::to_string(Position.x) + ", y=" + std::to_string(Position.y) + "]";
		}

	public:
		TSharedRef<Window> EventWindow;
		struct
		{
			int32 x;
			int32 y;
		} Position;
	};

	/*
	* WindowResizedEvent
	*/
	struct WindowResizedEvent : public Event
	{
	public:
		inline WindowResizedEvent(TSharedRef<Window> window, uint32 width, uint32 height, EResizeType resizeType)
			: Event()
			, EventWindow(window)
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
		TSharedRef<Window> EventWindow;
		uint32 Width;
		uint32 Height;
		EResizeType ResizeType;
	};

	/*
	* WindowMouseLeftEvent
	*/
	struct WindowMouseLeftEvent : public Event
	{
	public:
		inline WindowMouseLeftEvent(TSharedRef<Window> window)
			: Event()
			, EventWindow(window)
		{
		}

		DECLARE_EVENT_TYPE(WindowMouseLeftEvent);

		virtual String ToString() const
		{
			return "WindowMouseLeftEvent";
		}

	public:
		TSharedRef<Window> EventWindow;
	};

	/*
	* WindowMouseEnteredEvent
	*/
	struct WindowMouseEnteredEvent : public Event
	{
	public:
		inline WindowMouseEnteredEvent(TSharedRef<Window> window)
			: Event()
			, EventWindow(window)
		{
		}

		DECLARE_EVENT_TYPE(WindowMouseEnteredEvent);

		virtual String ToString() const
		{
			return "WindowMouseEnteredEvent";
		}

	public:
		TSharedRef<Window> EventWindow;
	};

	/*
	* WindowClosedEvent
	*/
	struct WindowClosedEvent : public Event
	{
	public:
		inline WindowClosedEvent(TSharedRef<Window> window)
			: Event()
			, EventWindow(window)
		{
		}

		DECLARE_EVENT_TYPE(WindowClosedEvent);

		virtual String ToString() const
		{
			return "WindowClosedEvent";
		}

	public:
		TSharedRef<Window> EventWindow;
	};
}