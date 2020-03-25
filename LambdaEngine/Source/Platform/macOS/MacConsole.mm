#ifdef LAMBDA_PLATFORM_MACOS
#include "Platform/macOS/MacConsole.h"

namespace LambdaEngine
{
    MacConsole MacConsole::s_Console = MacConsole();

    void MacConsole::Init()
    {
        const CGFloat width     = 640.0f;
        const CGFloat height    = 480.0f;
        
        NSUInteger  styleMask   = NSWindowStyleMaskTitled  | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;
        NSRect      contentRect = NSMakeRect(0, 0, width, height);
        
        m_pWindow = [[MacConsoleWindow alloc] initWithContentRect:contentRect styleMask:styleMask backing:NSBackingStoreBuffered defer:NO];
        ASSERT(m_pWindow != nil);
        
        NSRect contentFrame = [[m_pWindow contentView] frame];
        m_pScrollView = [[NSScrollView alloc] initWithFrame:contentFrame];
        [m_pScrollView setBorderType:NSNoBorder];
        [m_pScrollView setHasVerticalScroller:YES];
        [m_pScrollView setHasHorizontalScroller:NO];
        [m_pScrollView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
        
        m_pTextView = [[NSTextView alloc] initWithFrame:contentFrame];
        [m_pTextView setEditable:NO];
        [m_pTextView setMinSize:NSMakeSize(0.0f, height)];
        [m_pTextView setMaxSize:NSMakeSize(FLT_MAX, FLT_MAX)];
        [m_pTextView setVerticallyResizable:YES];
        [m_pTextView setHorizontallyResizable:NO];
        [m_pTextView setAutoresizingMask:NSViewWidthSizable];

        NSTextContainer* container = [m_pTextView textContainer];
        [container setContainerSize:NSMakeSize( width, FLT_MAX )];
        [container setWidthTracksTextView:YES];
        
        [m_pScrollView setDocumentView:m_pTextView];
        
        [m_pWindow setTitle:@"Lambda Engine Debug Console"];
        [m_pWindow setContentView:m_pScrollView];
        [m_pWindow setInitialFirstResponder:m_pTextView];
        [m_pWindow setOpaque:YES];
        
        
        [m_pWindow makeKeyAndOrderFront:m_pWindow];
        
        //Colors
        NSMutableArray* colors       = [[NSMutableArray alloc] init];
        NSMutableArray* attributes   = [[NSMutableArray alloc] init];
        //Add attributes for creating a color
        [attributes addObject:NSForegroundColorAttributeName];
        [attributes addObject:NSBackgroundColorAttributeName];
        
        //Add foreground color
        [colors addObject:[NSColor colorWithSRGBRed:1.0f green:1.0f blue:1.0f alpha:1.0f]];
        //Add background color
        [colors addObject:[NSColor colorWithSRGBRed:0.1f green:0.1f blue:0.1f alpha:0.1f]];
        m_ppColors[0] = [[NSDictionary alloc] initWithObjects:colors forKeys:attributes];
        
        [colors replaceObjectAtIndex:0 withObject:[NSColor colorWithSRGBRed:1.0f green:0.0f blue:0.0f alpha:1.0f]];
        m_ppColors[1] = [[NSDictionary alloc] initWithObjects:colors forKeys:attributes];
        
        [colors replaceObjectAtIndex:0 withObject:[NSColor colorWithSRGBRed:0.0f green:1.0f blue:0.0f alpha:1.0f]];
        m_ppColors[2] = [[NSDictionary alloc] initWithObjects:colors forKeys:attributes];
        
        [colors replaceObjectAtIndex:0 withObject:[NSColor colorWithSRGBRed:1.0f green:1.0f blue:0.0f alpha:1.0f]];
        m_ppColors[3] = [[NSDictionary alloc] initWithObjects:colors forKeys:attributes];
        
        [colors release];
        [attributes release];
        
    }

    void MacConsole::Release()
    {
        [m_pWindow close];
        [m_pTextView release];
        [m_pScrollView release];
        
        m_pCurrentColor = nullptr;
        for (uint32_t i = 0; i < 4; i++)
        {
            [m_ppColors[i] release];
        }
    }

    void MacConsole::PrintV(const char* pFormat, va_list args)
    {
        NSString* format = [NSString stringWithUTF8String:pFormat];
        NSString* string = [[NSString alloc] initWithFormat:format arguments:args];
        
        AppendTextAndScroll(string);
        [string release];
    }

    void MacConsole::AppendTextAndScroll(NSString* pString)
    {
        NSAttributedString* attributedString = [[NSAttributedString alloc] initWithString:pString attributes:m_pCurrentColor];
        
        NSTextStorage* storage = [m_pTextView textStorage];
        [storage beginEditing];
        [storage appendAttributedString:attributedString];
        [storage setFont:[NSFont fontWithName:@"Courier" size:12.0f]];
        [storage endEditing];
        
        [m_pTextView scrollRangeToVisible:NSMakeRange([[m_pTextView string] length], 0)];
        
        [attributedString release];
    }

    void MacConsole::NewLine()
    {
        AppendTextAndScroll(@"\n");
    }

    void MacConsole::Show()
    {
        if (s_Console.m_pWindow == nil)
        {
            s_Console.Init();
            SetColor(EConsoleColor::COLOR_WHITE);
        }
    }
    
    void MacConsole::Close()
    {
        if (s_Console.m_pWindow != nil)
        {
            s_Console.Release();
        }
    }
    
    void MacConsole::Print(const char* pFormat, ...)
    {
        if (s_Console.m_pTextView != nil)
        {
            va_list args;
            va_start(args, pFormat);
            
            s_Console.PrintV(pFormat, args);
            
            va_end(args);
        }
    }
    
    void MacConsole::PrintLine(const char* pFormat, ...)
    {
        if (s_Console.m_pTextView != nil)
        {
            va_list args;
            va_start(args, pFormat);
            
            s_Console.PrintV(pFormat, args);
            s_Console.NewLine();
            
            va_end(args);
        }
    }
    
    void MacConsole::Clear()
    {
        if (s_Console.m_pTextView != nil)
        {
            [s_Console.m_pTextView setString:@""];
        }
    }
    
    void MacConsole::SetColor(EConsoleColor color)
    {
        if (s_Console.m_pTextView)
        {
            if (color == EConsoleColor::COLOR_WHITE)
            {
                s_Console.m_pCurrentColor = s_Console.m_ppColors[0];
            }
            else if (color == EConsoleColor::COLOR_RED)
            {
                s_Console.m_pCurrentColor = s_Console.m_ppColors[1];
            }
            else if (color == EConsoleColor::COLOR_GREEN)
            {
                s_Console.m_pCurrentColor = s_Console.m_ppColors[2];
            }
            else if (color == EConsoleColor::COLOR_YELLOW)
            {
                s_Console.m_pCurrentColor = s_Console.m_ppColors[3];
            }
        }
    }
}

#endif
