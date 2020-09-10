#include "Engine/EngineLoop.h"

#include "Application/API/Events/EventQueue.h"


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

	if (!EngineLoop::PreInit())
	{
		return -1;
	}

	if (!EngineLoop::Init())
	{
		return -1;
	}

	Game* pGame = CreateGame();	
	EngineLoop::Run();

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

	return 0;
}
