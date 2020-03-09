#ifdef LAMBDA_PLATFORM_WINDOWS
	#include "Platform/Win32/Windows.h"
#endif

#include "LambdaEngine.h"

#include "Engine/EngineLoop.h"

#ifdef LAMBDA_PLATFORM_WINDOWS
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmd, int nCmdShow)
#else
int main(int argc, const char* argv[])
#endif 
{
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmd);
	UNREFERENCED_PARAMETER(nCmdShow);

	using namespace LambdaEngine;

	if (!EngineLoop::PreInit())
	{
		return -1;
	}

	if (!EngineLoop::Init())
	{
		return -1;
	}

	EngineLoop::Run();

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
