#include "Assert/Assert.h"

#include "Platform/PlatformMisc.h"
#include "Platform/PlatformConsole.h"

void Assert()
{
    using namespace LambdaEngine;
    
    PlatformConsole::PrintLine("ERROR: Assertion Failed");
	PlatformMisc::MessageBox("ERROR", "Assertion Failed");
}
