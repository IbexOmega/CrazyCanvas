#ifdef LAMBDA_PLATFORM_MACOS
#include <string.h>
#include <stdarg.h>

#include "Application/Mac/MacMisc.h"

#include <Foundation/Foundation.h>

namespace LambdaEngine
{
    void MacMisc::MessageBox(const char* pCaption, const char* pText)
    {
        CFStringRef captionRef  = CFStringCreateWithCString(0, pCaption, strlen(pCaption));
        CFStringRef textRef     = CFStringCreateWithCString(0, pText, strlen(pText));
        
        CFOptionFlags result    = 0;
        CFOptionFlags flags     = kCFUserNotificationStopAlertLevel;
        CFUserNotificationDisplayAlert(0, flags, 0, 0, 0, captionRef, textRef, 0, 0, 0, &result);
        
        CFRelease(captionRef);
        CFRelease(textRef);
    }

    void MacMisc::OutputDebugString(const char* pFormat, ...)
    {
        va_list args;
        va_start(args, pFormat);
        OutputDebugStringV(pFormat, args);
        va_end(args);
    }

    void MacMisc::OutputDebugStringV(const char* pFormat, va_list args)
    {
        NSString* format = [NSString stringWithUTF8String:pFormat];
        NSString* string = [[NSString alloc] initWithFormat:format arguments:args];

        NSLogv(format, args);

        [string release];
    }
}

#endif
