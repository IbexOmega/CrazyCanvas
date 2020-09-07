#pragma once
#include "LambdaEngine.h"

#include <stdarg.h>

#ifdef LAMBDA_VISUAL_STUDIO
	#define FUNCTION_SIG __FUNCTION__
#else
	#define FUNCTION_SIG __PRETTY_FUNCTION__
#endif

#define LOG(severity, ...)  LambdaEngine::Log::Print(severity, __VA_ARGS__)
#define LOG_MESSAGE(...)    LOG(LambdaEngine::ELogSeverity::LOG_MESSAGE, __VA_ARGS__)
#define LOG_INFO(...)		LOG(LambdaEngine::ELogSeverity::LOG_INFO, __VA_ARGS__)
#define LOG_WARNING(...)    LOG(LambdaEngine::ELogSeverity::LOG_WARNING, __VA_ARGS__)
#define LOG_ERROR(...)      LOG(LambdaEngine::ELogSeverity::LOG_ERROR, __VA_ARGS__)
#define LOG_ERROR_CRIT(...) LambdaEngine::Log::PrintTraceError(FUNCTION_SIG, __VA_ARGS__)

#ifndef LAMBDA_ENABLE_LOGS
	#ifdef LAMBDA_DEVELOPMENT
		#define LAMBDA_ENABLE_LOGS 1
	#else
		#define LAMBDA_ENABLE_LOGS 0
	#endif
#endif

#if LAMBDA_ENABLE_LOGS
	#define D_LOG(severity, ...)    LOG(severity, __VA_ARGS__)
	#define D_LOG_MESSAGE(...)      LOG_MESSAGE(__VA_ARGS__)
	#define D_LOG_INFO(...)			LOG_INFO(__VA_ARGS__)
	#define D_LOG_WARNING(...)      LOG_WARNING(__VA_ARGS__)
	#define D_LOG_ERROR(...)        LOG_ERROR(__VA_ARGS__)
#else
	#define D_LOG(severity, ...)
	#define D_LOG_MESSAGE(...)
	#define D_LOG_INFO(...)
	#define D_LOG_WARNING(...)
	#define D_LOG_ERROR(...)
#endif

namespace LambdaEngine
{
	/*
	* ELogSeverity
	*/
	enum class ELogSeverity
	{
		LOG_MESSAGE = 0, //White
		LOG_INFO	= 1, //Green
		LOG_WARNING = 2, //Yellow
		LOG_ERROR   = 3  //Red
	};

	/*
	* Log
	*/
	class LAMBDA_API Log
	{
	public:
		/*
		* Prints a message to the log
		* 
		* severity  - The message severity, determines how important the message is
		* pFormat   - Formatted string to print to the log
		* args      - Arguments for the formatted string
		*/
		static void Print(ELogSeverity severity, const char* pFormat, ...);
		static void PrintV(ELogSeverity severity, const char* pFormat, va_list args);
		
		/*
		* Prints an error message with the function-signature where the error occured
		* 
		* pFunction - The function- signature as a string
		* pFormat   - Formatted string to print to the log
		* args      - The arguments to the formatted string
		*/
		static void PrintTraceError(const char* pFunction, const char* pFormat, ...);
		static void PrintTraceErrorV(const char* pFunction, const char* pFormat, va_list args);
		
		/*
		* Enables the log to print to the debugger using PlatformMisc::OutputDebugString
		* 
		* enable - True if the debugger logging should be enabled
		*/
		FORCEINLINE static void SetDebuggerOutputEnabled(bool enable)
		{
			s_DebuggerOutputEnabled = enable;
		}
		
	private:
		static bool s_DebuggerOutputEnabled;
	};
}
