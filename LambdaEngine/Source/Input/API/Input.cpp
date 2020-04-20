#include "Input/API/Input.h"

namespace LambdaEngine
{
	IInputDevice*   Input::s_pInputDevice = nullptr;
	KeyboardState   Input::s_KeyboardState;
	MouseState      Input::s_MouseState;

	bool Input::Init()
	{
		s_pInputDevice = PlatformApplication::CreateInputDevice(EInputMode::INPUT_STANDARD);
        return (s_pInputDevice != nullptr);
	}

	void Input::Release()
	{
		SAFEDELETE(s_pInputDevice);
	}

	void Input::Tick()
	{
		KeyboardState   newKeyboardState = s_pInputDevice->GetKeyboardState();
		MouseState      newMouseState = s_pInputDevice->GetMouseState();

		s_KeyboardState = newKeyboardState;
		s_MouseState	= newMouseState;
	}

	void Input::AddKeyboardHandler(IKeyboardHandler* pHandler)
	{
        ASSERT(s_pInputDevice != nullptr);
        s_pInputDevice->AddKeyboardHandler(pHandler);
	}

	void Input::AddMouseHandler(IMouseHandler* pHandler)
	{
        ASSERT(s_pInputDevice != nullptr);
        s_pInputDevice->AddMouseHandler(pHandler);
	}
	
	void Input::SetInputMode(EInputMode inputMode)
	{
		SAFEDELETE(s_pInputDevice);
		s_pInputDevice = PlatformApplication::CreateInputDevice(inputMode);
	}
}
