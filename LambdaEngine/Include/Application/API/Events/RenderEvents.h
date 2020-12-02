#pragma once

#include "Event.h"

namespace LambdaEngine
{
	/*
	* PreSwapChainRecreatedEvent
	*/
	struct PreSwapChainRecreatedEvent : public Event
	{
	public:
		inline PreSwapChainRecreatedEvent(uint32 width, uint32 height)
			: Event()
			, PreviousWidth(width)
			, PreviousHeight(height)
		{
		}

		DECLARE_EVENT_TYPE(PreSwapChainRecreatedEvent);

		virtual String ToString() const override
		{
			return String("PreSwapChainRecreatedEvent");
		}

	public:
		uint32 PreviousWidth;
		uint32 PreviousHeight;
	};

	/*
	* PostSwapChainRecreatedEvent
	*/
	struct PostSwapChainRecreatedEvent : public Event
	{
	public:
		inline PostSwapChainRecreatedEvent(uint32 width, uint32 height)
			: Event()
			, NewWidth(width)
			, NewHeight(height)
		{
		}

		DECLARE_EVENT_TYPE(PostSwapChainRecreatedEvent);

		virtual String ToString() const override
		{
			return String("PostSwapChainRecreatedEvent");
		}

	public:
		uint32 NewWidth;
		uint32 NewHeight;
	};
}