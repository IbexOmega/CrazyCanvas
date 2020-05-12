#include "Input/API/Input.h"

#include "Application/API/PlatformApplication.h"

namespace LambdaEngine
{
	Input* Input::s_pInstance = nullptr;

	/*
	* Instance
	*/

	void Input::ButtonPressed(EMouseButton button, uint32 modifierMask)
	{
		m_MouseState.ButtonStates[button] = true;
	}

	void Input::ButtonReleased(EMouseButton button)
	{
		m_MouseState.ButtonStates[button] = false;
	}

	void Input::MouseMoved(int32 x, int32 y)
	{
		m_MouseState.x = x;
		m_MouseState.y = y;
	}

	void Input::MouseScrolled(int32 deltaX, int32 deltaY)
	{
		m_MouseState.ScrollX = deltaX;
		m_MouseState.ScrollY = deltaY;
	}

	void Input::KeyPressed(EKey key, uint32 modifierMask, bool isRepeat)
	{
		if (!isRepeat)
		{
			m_KeyboardState.KeyStates[key] = true;
		}
	}

	void Input::KeyReleased(EKey key)
	{
		m_KeyboardState.KeyStates[key] = false;
	}

	/*
	* Static
	*/

	bool Input::Init()
	{
		s_pInstance = DBG_NEW Input();
		PlatformApplication::Get()->AddEventHandler(s_pInstance);

        return (s_pInstance != nullptr);
	}

	void Input::Release()
	{
		PlatformApplication::Get()->RemoveEventHandler(s_pInstance);
		SAFEDELETE(s_pInstance);
	}

	void Input::Tick()
	{
	}
}
