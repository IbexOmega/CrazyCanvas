#include "Assert/Assert.h"

#include "Platform/PlatformMisc.h"

void Assert()
{
	LambdaEngine::PlatformMisc::MessageBox("ERROR", "Assertion Failed");
}