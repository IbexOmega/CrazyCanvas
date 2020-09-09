#pragma once
#include "LambdaEngine.h"

namespace LambdaEngine
{
	/*
	* EventQueue
	*/
	class EventQueue
	{
	public:
		static void RegisterEventHandler();
		static void UnregisterEventHandler();

		static bool SendEvent(const Event& event);
	};
}