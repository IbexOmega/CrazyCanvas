#include "Sandbox.h"

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

	PlatformConsole::PrintLine("Key Pressed: %d", key);
}

void Sandbox::OnKeyHeldDown(LambdaEngine::EKey key)
{
	using namespace LambdaEngine;

	PlatformConsole::PrintLine("Key Held Down: %d", key);
}

void Sandbox::OnKeyUp(LambdaEngine::EKey key)
{
	using namespace LambdaEngine;

	PlatformConsole::PrintLine("Key Released: %d", key);
}

void Sandbox::OnMouseMove(int32 x, int32 y)
{
	using namespace LambdaEngine;
    
    PlatformConsole::PrintLine("Mouse Moved: x=%d, y=%d", x, y);
}

void Sandbox::OnButtonPressed(LambdaEngine::EMouseButton button)
{
	using namespace LambdaEngine;

	PlatformConsole::PrintLine("Mouse Button Pressed: %d", button);
}

void Sandbox::OnButtonReleased(LambdaEngine::EMouseButton button)
{
	using namespace LambdaEngine;

	PlatformConsole::PrintLine("Mouse Button Released: %d", button);
}

void Sandbox::OnScroll(int32 delta)
{
	using namespace LambdaEngine;

	PlatformConsole::PrintLine("Mouse Scrolled: %d", delta);
}

void Sandbox::Tick()
{
}

namespace LambdaEngine
{
    Game* CreateGame()
    {
        Sandbox* pSandbox = new Sandbox();
        Input::AddKeyboardHandler(pSandbox);
        Input::AddMouseHandler(pSandbox);
        
        return pSandbox;
    }
}
