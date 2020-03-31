#pragma once
#include "LambdaEngine.h"

#define LOG(severity, ...)  LambdaEngine::Log::Print(severity, __VA_ARGS__)
#define LOG_MESSAGE(...)    LOG(LambdaEngine::ELogSeverity::LOG_MESSAGE, __VA_ARGS__)
#define LOG_WARNING(...)    LOG(LambdaEngine::ELogSeverity::LOG_WARNING, __VA_ARGS__)
#define LOG_ERROR(...)      LOG(LambdaEngine::ELogSeverity::LOG_ERROR, __VA_ARGS__)

#ifdef LAMBDA_DEBUG
    #define D_LOG(severity, ...)    LOG(severity, __VA_ARGS__)
    #define D_LOG_MESSAGE(...)      LOG_MESSAGE(__VA_ARGS__)
    #define D_LOG_WARNING(...)      LOG_WARNING(__VA_ARGS__)
    #define D_LOG_ERROR(...)        LOG_ERROR(__VA_ARGS__)
#else
    #define D_LOG(severity, ...)
    #define D_LOG_MESSAGE(...)
    #define D_LOG_WARNING(...)
    #define D_LOG_ERROR(...)
#endif

namespace LambdaEngine
{
    enum class ELogSeverity
    {
		LOG_MESSAGE = 0,
		LOG_WARNING = 1,
        LOG_ERROR   = 2
    };

    class Log
    {
    public:
        static void Print(ELogSeverity severity, const char* pFormat, ...);
    };
}
