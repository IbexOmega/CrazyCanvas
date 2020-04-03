#pragma once
#include "Defines.h"
#include "Types.h"

#include <stdarg.h>

namespace LambdaEngine
{
	class Misc
	{
	public:
		DECL_STATIC_CLASS(Misc);

		/*
		* Displays a message window, this is a blocking call
		*
		* pCaption 	- Title of the window
		* pText		- Text to display
		*/
		static void MessageBox(const char*, const char*)		{ }

		/*
		* Outputs a message to the debugger
		* 
		* pFormat 	- A formated string to display
		* args		- Arguments for the string
		*/
        static void OutputDebugString(const char*, ...)			{ }
        static void OutputDebugStringV(const char*, va_list)	{ }
	};
}
