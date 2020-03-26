#include "Input/Input.h"
#include "Platform/PlatformApplication.h"

namespace LambdaEngine
{
	InputDevice*   Input::s_pInputDevice = nullptr;
	KeyboardState   Input::s_KeyboardState;
	MouseState      Input::s_MouseState;

	bool Input::Init()
	{
		s_pInputDevice = PlatformApplication::CreateInputDevice();

		return s_pInputDevice != nullptr;
	}

	void Input::Release()
	{
		SAFEDELETE(s_pInputDevice);
	}

	void Input::Update()
	{
		KeyboardState newKeyboardState = s_pInputDevice->GetKeyboardState();
		MouseState newMouseState = s_pInputDevice->GetMouseState();
	}

	void Input::AddKeyboardHandler(IKeyboardHandler* pHandler)
	{
		s_pInputDevice->AddKeyboardHandler(pHandler);
	}

	void Input::AddMouseHandler(IMouseHandler* pHandler)
	{
		s_pInputDevice->AddMouseHandler(pHandler);
	}
}
