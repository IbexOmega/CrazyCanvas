#include "..\Include\Sandbox.h"

#include "Platform/PlatformConsole.h"
#include "Input/Input.h"

Sandbox::Sandbox()
{
}

Sandbox::~Sandbox()
{
}

void Sandbox::OnKeyDown(LambdaEngine::EKey key)
{
	using namespace LambdaEngine;

	PlatformConsole::PrintLine("Key Pressed: %i", key);
}

void Sandbox::OnKeyUp(LambdaEngine::EKey key)
{
	using namespace LambdaEngine;

	PlatformConsole::PrintLine("Key Released: %i", key);
}

void Sandbox::OnMouseMove(int32 x, int32 y)
{
	using namespace LambdaEngine;

	//Console::PrintLine();
}

void Sandbox::OnButtonPressed(LambdaEngine::EMouseButton button)
{
	using namespace LambdaEngine;

	PlatformConsole::PrintLine("Mouse Button Pressed: %i", button);
}

void Sandbox::OnButtonReleased(LambdaEngine::EMouseButton button)
{
	using namespace LambdaEngine;

	PlatformConsole::PrintLine("Mouse Button Released: %i", button);
}

void Sandbox::OnScroll(int32 delta)
{
	using namespace LambdaEngine;

	PlatformConsole::PrintLine("Mouse Scrolled: %i", delta);
}

void Sandbox::Tick()
{

}

LambdaEngine::Game* CreateGame()
{
	Sandbox* pSandbox = new Sandbox();
	LambdaEngine::Input::AddKeyboardHandler(pSandbox);
	LambdaEngine::Input::AddMouseHandler(pSandbox);
	return pSandbox;
}
