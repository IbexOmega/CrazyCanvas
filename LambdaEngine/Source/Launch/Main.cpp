#include "Engine/EngineLoop.h"

#include "Debug/Profiler.h"

#include "Threading/API/PlatformThread.h"

#include <argh/argh.h>

namespace LambdaEngine
{
	extern Game* CreateGame(const argh::parser& flagParser);
}

#ifdef LAMBDA_PLATFORM_WINDOWS

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	int argc = __argc;
	char** argv = __argv;

#else
int main(int argc, char** argv)
{
#endif
	using namespace LambdaEngine;

	// Set the name of the mainthread
	PlatformThread::SetThreadName(PlatformThread::GetCurrentThreadHandle(), "MainThread");

	argh::parser flagParser(argc, argv);

	LAMBDA_PROFILER_BEGIN_SESSION("Startup", "LambdaProfile-Startup.json");
	if (!EngineLoop::PreInit(flagParser))
	{
		return -1;
	}

	if (!EngineLoop::Init())
	{
		return -1;
	}

	LAMBDA_PROFILER_END_SESSION();

	LAMBDA_PROFILER_BEGIN_SESSION("Runtime", "LambdaProfile-Runtime.json");
	Game* pGame = CreateGame(flagParser);
	EngineLoop::Run();
	LAMBDA_PROFILER_END_SESSION();

	LAMBDA_PROFILER_BEGIN_SESSION("Shutdown", "LambdaProfile-Shutdown.json");
	if (!EngineLoop::PreRelease())
	{
		return -1;
	}

	SAFEDELETE(pGame);

	if (!EngineLoop::Release())
	{
		return -1;
	}

	if (!EngineLoop::PostRelease())
	{
		return -1;
	}
	LAMBDA_PROFILER_END_SESSION();

	return 0;
}
