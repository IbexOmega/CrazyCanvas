#include "Assert/Assert.h"

#include "Application/API/PlatformMisc.h"
#include "Application/API/PlatformConsole.h"

void Assert(const char* pFile, int line)
{
    using namespace LambdaEngine;
    
    PlatformConsole::PrintLine("ERROR: Assertion Failed in File %s on line %d", pFile, line);
	PlatformMisc::MessageBox("ERROR", "Assertion Failed");
}
