#include "Input/Input.h"
#include "Platform/PlatformApplication.h"

namespace LambdaEngine
{
	IInputDevice*   Input::s_pInputDevice = nullptr;
	KeyboardState   Input::s_KeyboardState;
	MouseState      Input::s_MouseState;

	bool Input::Init()
	{
		s_pInputDevice = PlatformApplication::CreateInputDevice();

        //TODO: Implement on macOS and check if nullptr
        return true;//s_pInputDevice != nullptr;
	}

	void Input::Release()
	{
		SAFEDELETE(s_pInputDevice);
	}

	void Input::Update()
	{
		KeyboardState   newKeyboardState = s_pInputDevice->GetKeyboardState();
		MouseState      newMouseState = s_pInputDevice->GetMouseState();
	}
}
