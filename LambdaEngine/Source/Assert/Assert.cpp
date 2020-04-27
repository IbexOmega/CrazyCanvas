#include "Assert/Assert.h"

#include "Application/API/PlatformMisc.h"
#include "Application/API/PlatformConsole.h"

void HandleAssert(const char* pFile, int line)
{
    using namespace LambdaEngine;
    
    PlatformConsole::PrintLine("ERROR: Assertion Failed in 'File %s' on line %d", pFile, line);

    constexpr uint32 BUFFER_SIZE = 2048;
    static char buffer[BUFFER_SIZE];

    int written = sprintf_s(buffer, BUFFER_SIZE - 2, "Assertion Failed\nFile: '%s'\nLine: %d", pFile, line);
    if (written > 0)
    {
        buffer[written] = '\n';
        buffer[written + 1] = 0;

	    PlatformMisc::MessageBox("ERROR", buffer);
    }
}

void HandleAssertWithMessage(const char* pFile, int line, const char* pMessageFormat, ...)
{
    using namespace LambdaEngine;

    constexpr uint32 BUFFER_SIZE = 2048;
    static char buffer[BUFFER_SIZE];
    static char messagebuffer[BUFFER_SIZE];

    // Format Message
    va_list args;
    va_start(args, pMessageFormat);

    int written = vsprintf_s(messagebuffer, BUFFER_SIZE - 1, pMessageFormat, args);
    if (written > 0)
    {
        buffer[written + 1] = 0;
    }
    else
    {
        buffer[0] = 0;
    }

    va_end(args);

    // Print to console
    PlatformConsole::PrintLine("ERROR: Assertion Failed in File '%s' on line '%d' with message '%s'", pFile, line, messagebuffer);

    // Print to messagebox
    written = sprintf_s(buffer, BUFFER_SIZE - 2, "Assertion Failed\nFile: '%s'\nLine: %d\nMessage: %s", pFile, line, messagebuffer);
    if (written > 0)
    {
        buffer[written] = '\n';
        buffer[written + 1] = 0;

        PlatformMisc::MessageBox("ERROR", buffer);
    }
}
