#pragma once
#include "LambdaEngine.h"

#include <stdarg.h>

#ifdef LAMBDA_VISUAL_STUDIO
	#define FUNCTION_SIG __FUNCTION__
#else
	#define FUNCTION_SIG __PRETTY_FUNCTION__
#endif

#define LOG(fileName, lineNr, severity, ...)  LambdaEngine::Log::Print(fileName, lineNr, severity, __VA_ARGS__)
#define LOG_MESSAGE(...)    LOG(__FILE__, __LINE__, LambdaEngine::ELogSeverity::LOG_MESSAGE, __VA_ARGS__)
#define LOG_INFO(...)		LOG(__FILE__, __LINE__, LambdaEngine::ELogSeverity::LOG_INFO, __VA_ARGS__)
#define LOG_WARNING(...)    LOG(__FILE__, __LINE__, LambdaEngine::ELogSeverity::LOG_WARNING, __VA_ARGS__)
#define LOG_ERROR(...)      LOG(__FILE__, __LINE__, LambdaEngine::ELogSeverity::LOG_ERROR, __VA_ARGS__)
#define LOG_ERROR_CRIT(...) LambdaEngine::Log::PrintTraceError(FUNCTION_SIG, __FILE__, __LINE__, __VA_ARGS__)

#ifndef LAMBDA_ENABLE_LOGS
	#ifdef LAMBDA_DEVELOPMENT
		#define LAMBDA_ENABLE_LOGS 1
	#else
		#define LAMBDA_ENABLE_LOGS 0
	#endif
#endif

#if LAMBDA_ENABLE_LOGS
	#define LOG_DEBUG(...) LOG_INFO(__VA_ARGS__)
#else
	#define LOG_DEBUG(...)
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
		/**
		* Prints a message to the log
		*
		* @param severity	The message severity, determines how important the message is
		* @param pFileName	Name of the file where Print is called from
		* @param lineNr		Line number inside the code file in which Print is called from
		* @param pFormat	Formatted string to print to the log
		* @param args		Arguments for the formatted string
		*/
		static void Print(const char* pFileName, uint32 lineNr, ELogSeverity severity, const char* pFormat, ...);
		static void PrintV(const char* pFileName, uint32 lineNr, ELogSeverity severity, const char* pFormat, va_list vaArgs);

		/**
		* Prints an error message with the function-signature where the error occured
		*
		* @param pFunction The function- signature as a string
		* @param pFileName	Name of the file where Print is called from
		* @param lineNr		Line number inside the code file in which Print is called from
		* @param pFormat   Formatted string to print to the log
		* @param args      The arguments to the formatted string
		*/
		static void PrintTraceError(const char* pFunction, const char* pFileName, uint32 lineNr, const char* pFormat, ...);
		static void PrintTraceErrorV(const char* pFunction, const char* pFileName, uint32 lineNr, const char* pFormat, va_list vaArgs);

		/**
		* Enables the log to print to the debugger using PlatformMisc::OutputDebugString
		*
		* @param enable True if the debugger logging should be enabled
		*/
		FORCEINLINE static void SetDebuggerOutputEnabled(bool enable)
		{
			s_DebuggerOutputEnabled = enable;
		}

	private:
		static bool s_DebuggerOutputEnabled;
	};
}
