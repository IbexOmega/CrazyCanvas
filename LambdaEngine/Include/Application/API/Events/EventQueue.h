#pragma once
#include "EventHandler.h"

namespace LambdaEngine
{
	/*
	* EventQueue
	*/
	class EventQueue
	{
	public:
		static bool RegisterEventHandler(const EventHandlerProxy& eventHandler);
		static bool UnregisterEventHandler(const EventHandlerProxy& eventHandler);

		static bool SendEvent(Event& event);
	};
}