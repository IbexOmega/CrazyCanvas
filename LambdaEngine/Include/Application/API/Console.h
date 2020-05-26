#pragma once
#include "LambdaEngine.h"

#include <stdarg.h>

namespace LambdaEngine
{
	enum class EConsoleColor : uint8
	{
		COLOR_RED		= 0,
		COLOR_GREEN		= 1,
		COLOR_YELLOW	= 2,
		COLOR_WHITE		= 3
	};

	class LAMBDA_API Console
	{
	public:
		DECL_ABSTRACT_CLASS(Console);
		
		static void Show()	{ }
		static void Close()	{ }
		
		static void Print(const char*, ...)				{ }
		static void PrintLine(const char*, ...)			{ }
		static void PrintV(const char*, va_list)		{ }
		static void PrintLineV(const char*, va_list)	{ }
		
		/*
		* Clears the console screen
		*/
		static void Clear()	{ }
		
		static void SetTitle(const char*)	{ }
		static void SetColor(EConsoleColor)	{ }
	};
}
