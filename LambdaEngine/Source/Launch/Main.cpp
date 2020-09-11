#include "Engine/EngineLoop.h"
#include "Debug/Profiler.h"

namespace LambdaEngine
{
	extern Game* CreateGame();
}

#ifdef LAMBDA_PLATFORM_WINDOWS
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#else
int main(int, const char*[])
#endif 
{
	using namespace LambdaEngine;

	LAMBDA_PROFILER_BEGIN_SESSION("Startup", "LambdaProfile-Startup.json");
	if (!EngineLoop::PreInit())
	{
		return -1;
	}

	if (!EngineLoop::Init())
	{
		return -1;
	}
	LAMBDA_PROFILER_END_SESSION();

	LAMBDA_PROFILER_BEGIN_SESSION("Runtime", "LambdaProfile-Runtime.json");
	Game* pGame = CreateGame();	
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
