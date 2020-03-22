#ifdef LAMBDA_PLATFORM_MACOS
#include "Platform/macOS/MacConsole.h"

namespace LambdaEngine
{
    MacConsoleWindow*   MacConsole::s_pConsoleWindow    = nullptr;
    NSTextView*         MacConsole::s_pTextView         = nullptr;
    
    void MacConsole::Show()
    {
        if (s_pConsoleWindow == nil)
        {
            NSUInteger  styleMask   = NSWindowStyleMaskTitled  | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;
            NSRect      contentRect = NSMakeRect(0, 0, 640.0f, 480.0f);
            
            s_pConsoleWindow = [[MacConsoleWindow alloc] initWithContentRect:contentRect styleMask:styleMask backing:NSBackingStoreBuffered defer:NO];
            ASSERT(s_pConsoleWindow != nil);
            
            NSRect contentFrame = [[s_pConsoleWindow contentView] frame];
            s_pTextView = [[NSTextView alloc] initWithFrame:contentFrame];
            
            [s_pTextView setEditable:NO];

            [s_pConsoleWindow setContentView:s_pTextView];
            [s_pConsoleWindow setInitialFirstResponder:s_pTextView];
            
            [s_pConsoleWindow makeKeyAndOrderFront:s_pConsoleWindow];
        }
    }
    
    void MacConsole::Close()
    {
        if (s_pConsoleWindow != nil)
        {
            [s_pConsoleWindow close];
            [s_pTextView release];
        }
    }
    
    void MacConsole::Print(const char* pFormat, ...)
    {
        if (s_pTextView != nil)
        {
            va_list args;
            va_start(args, pFormat);
            
            PrintV(pFormat, args);
            
            va_end(args);
        }
    }
    
    void MacConsole::PrintLine(const char* pFormat, ...)
    {
        if (s_pTextView != nil)
        {
            va_list args;
            va_start(args, pFormat);
            
            PrintV(pFormat, args);
            
            va_end(args);
        
            [s_pTextView setString:@"\n"];
        }
    }
    
    void MacConsole::PrintV(const char* pFormat, va_list args)
    {
        NSString* format = [NSString stringWithUTF8String:pFormat];
        NSString* string = [[NSString alloc] initWithFormat:format arguments:args];
               
        [s_pTextView setString:string];
        [string release];
    }
    
    void MacConsole::Clear()
    {
        if (s_pTextView != nil)
        {
            [s_pTextView setString:@""];
        }
    }
    
    void MacConsole::SetColor(EConsoleColor color)
    {
        
    }
}

#endif
