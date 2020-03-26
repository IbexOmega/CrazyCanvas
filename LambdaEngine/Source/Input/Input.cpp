#include "Input/Input.h"

namespace LambdaEngine
{
	IInputDevice* Input::s_pInputDevice = nullptr;
	KeyboardState Input::s_KeyboardState;
	MouseState Input::s_MouseState;

	void Input::Update()
	{
		KeyboardState newKeyboardState = s_pInputDevice->GetKeyboardState();
		MouseState newMouseState = s_pInputDevice->GetMouseState();
	}
}