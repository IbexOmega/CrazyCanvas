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
		static bool RegisterEventHandler(const EventHandler& eventHandler);
		static bool UnregisterEventHandler(const EventHandler& eventHandler);

		static bool SendEvent(Event& event);
	};
}