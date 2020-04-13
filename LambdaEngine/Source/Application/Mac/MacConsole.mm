#ifdef LAMBDA_PLATFORM_MACOS
#include "Log/Log.h"

#include "Application/Mac/MacConsole.h"
#include "Application/Mac/CocoaConsoleWindow.h"
#include "Application/Mac/MacApplication.h"
#include "Application/Mac/MacScopedPool.h"

#include "Threading/Mac/MacMainThread.h"

namespace LambdaEngine
{
    CocoaConsoleWindow* MacConsole::s_pConsoleWindow = nullptr;

    void MacConsole::Show()
    {
        if (s_pConsoleWindow == nil)
        {
            MacMainThread::MakeCall(^
            {
                SCOPED_AUTORELEASE_POOL();
                
                const CGFloat width     = 1280.0f;
                const CGFloat height    = 720.0f;
                
                s_pConsoleWindow = [[CocoaConsoleWindow alloc] init: width height:height];
                [s_pConsoleWindow setColor:EConsoleColor::COLOR_YELLOW];
                
                MacApplication::ProcessMessages();
            });
        }
    }
    
    void MacConsole::Close()
    {
        if (s_pConsoleWindow != nil)
        {
            MacMainThread::MakeCall(^
            {
                SCOPED_AUTORELEASE_POOL();
                
                MacApplication::ProcessMessages();
                
                [s_pConsoleWindow release];
                s_pConsoleWindow = nil;
            });
        }
    }
    
    void MacConsole::Print(const char* pFormat, ...)
    {
        va_list args;
        va_start(args, pFormat);
        
        VPrint(pFormat, args);
        
        va_end(args);
    }
    
    void MacConsole::PrintLine(const char* pFormat, ...)
    {
        va_list args;
        va_start(args, pFormat);
            
        VPrintLine(pFormat, args);
            
        va_end(args);
    }

    void MacConsole::VPrint(const char* pFormat, va_list args)
    {
        if (s_pConsoleWindow)
        {
            NSString* string = [CocoaConsoleWindow convertStringWithArgs:pFormat args:args];
            
            MacMainThread::MakeCall(^
            {
                SCOPED_AUTORELEASE_POOL();
                
                [s_pConsoleWindow appendStringAndScroll:string];
                
                [string release];
                
                MacApplication::ProcessMessages();
            });
        }
    }

    void MacConsole::VPrintLine(const char* pFormat, va_list args)
    {
        if (s_pConsoleWindow)
        {
            NSString* string        = [CocoaConsoleWindow convertStringWithArgs:pFormat args:args];
            NSString* finalString   = [string stringByAppendingString:@"\n"];
            
            MacMainThread::MakeCall(^
            {
                SCOPED_AUTORELEASE_POOL();
                
                [s_pConsoleWindow appendStringAndScroll:finalString];
                
                [finalString release];
                [string release];
                
                MacApplication::ProcessMessages();
            });
        }
    }
    
    void MacConsole::Clear()
    {
        if (s_pConsoleWindow)
        {
            MacMainThread::MakeCall(^
            {
                [s_pConsoleWindow clearWindow];
            });
        }
    }
    
    void MacConsole::SetTitle(const char* pTitle)
    {
        if (s_pConsoleWindow)
        {
            MacMainThread::MakeCall(^
            {
                SCOPED_AUTORELEASE_POOL();
                
                NSString* title = [NSString stringWithUTF8String:pTitle];
                [s_pConsoleWindow setTitle:title];
            });
        }
    }

    void MacConsole::SetColor(EConsoleColor color)
    {
        if (s_pConsoleWindow)
        {
            MacMainThread::MakeCall(^
            {
                [s_pConsoleWindow setColor:color];
            });
        }
    }
}

#endif
