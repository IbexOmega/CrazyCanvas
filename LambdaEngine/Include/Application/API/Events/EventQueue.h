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
		static void RegisterEventHandler(const EventHandlerProxy& eventHandler);
		static void UnregisterEventHandler(const EventHandlerProxy& eventHandler);

		static bool SendEvent(const Event& event);
	};
}