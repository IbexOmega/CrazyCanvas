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
		inline PreSwapChainRecreatedEvent()
			: Event()
		{
		}

		DECLARE_EVENT_TYPE(PreSwapChainRecreatedEvent);

		virtual String ToString() const override
		{
			return String("PreSwapChainRecreatedEvent");
		}

	};

	/*
	* PostSwapChainRecreatedEvent
	*/
	struct PostSwapChainRecreatedEvent : public Event
	{
	public:
		inline PostSwapChainRecreatedEvent()
			: Event()
		{
		}

		DECLARE_EVENT_TYPE(PostSwapChainRecreatedEvent);

		virtual String ToString() const override
		{
			return String("PostSwapChainRecreatedEvent");
		}

	};
}