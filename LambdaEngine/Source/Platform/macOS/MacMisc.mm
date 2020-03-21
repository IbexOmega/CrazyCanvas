#ifdef LAMBDA_PLATFORM_MACOS
#include "Platform/macOS/MacMisc.h"

#include <string.h>
#include <stdarg.h>

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
        NSString* format = [NSString stringWithUTF8String:pFormat];
        
        va_list args;
        va_start(args, pFormat);
        NSLogv(format, args);
        va_end(args);
    }
}

#endif
