#include "Sandbox.h"

#include "Input/Input.h"

#include "Log/Log.h"

Sandbox::Sandbox()
{
}

Sandbox::~Sandbox()
{
}

void Sandbox::OnKeyDown(LambdaEngine::EKey key)
{
	LOG_MESSAGE("Key Pressed: %d", key);
}

void Sandbox::OnKeyHeldDown(LambdaEngine::EKey key)
{
	LOG_MESSAGE("Key Held Down: %d", key);
}

void Sandbox::OnKeyUp(LambdaEngine::EKey key)
{
	LOG_MESSAGE("Key Released: %d", key);
}

void Sandbox::OnMouseMove(int32 x, int32 y)
{
	LOG_MESSAGE("Mouse Moved: x=%d, y=%d", x, y);
}

void Sandbox::OnButtonPressed(LambdaEngine::EMouseButton button)
{
	LOG_MESSAGE("Mouse Button Pressed: %d", button);
}

void Sandbox::OnButtonReleased(LambdaEngine::EMouseButton button)
{
	LOG_MESSAGE("Mouse Button Released: %d", button);
}

void Sandbox::OnScroll(int32 delta)
{
	LOG_MESSAGE("Mouse Scrolled: %d", delta);
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
