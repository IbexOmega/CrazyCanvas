#include "Input/API/Input.h"

#include "Application/API/CommonApplication.h"

namespace LambdaEngine
{
	Input* Input::s_pInstance = nullptr;

	/*
	* Instance
	*/

	void Input::OnButtonPressed(EMouseButton button, uint32 modifierMask)
	{
		UNREFERENCED_VARIABLE(modifierMask);
		m_MouseState.ButtonStates[button] = true;
	}

	void Input::OnButtonReleased(EMouseButton button)
	{
		m_MouseState.ButtonStates[button] = false;
	}

	void Input::OnMouseMoved(int32 x, int32 y)
	{
		m_MouseState.x = x;
		m_MouseState.y = y;
	}

	void Input::OnMouseScrolled(int32 deltaX, int32 deltaY)
	{
		m_MouseState.ScrollX = deltaX;
		m_MouseState.ScrollY = deltaY;
	}

	void Input::OnKeyPressed(EKey key, uint32 modifierMask, bool isRepeat)
	{
		UNREFERENCED_VARIABLE(modifierMask);

		if (!isRepeat)
		{
			m_KeyboardState.KeyStates[key] = true;
		}
	}

	void Input::OnKeyReleased(EKey key)
	{
		m_KeyboardState.KeyStates[key] = false;
	}

	/*
	* Static
	*/

	bool Input::Init()
	{
		s_pInstance = DBG_NEW Input();
		CommonApplication::Get()->AddEventHandler(s_pInstance);

        return (s_pInstance != nullptr);
	}

	void Input::Release()
	{
		CommonApplication::Get()->RemoveEventHandler(s_pInstance);
		SAFEDELETE(s_pInstance);
	}

	void Input::Tick()
	{
	}
}
