#include "Input/API/Input.h"

#include "Application/API/CommonApplication.h"

namespace LambdaEngine
{
	Input* Input::s_pInstance = nullptr;

	/*
	* Instance
	*/
	void Input::OnButtonPressed(EMouseButton button, ModifierKeyState modifierState)
	{
		UNREFERENCED_VARIABLE(modifierState);
		m_MouseState.ButtonStates[button] = m_InputEnabled;
	}

	void Input::OnButtonReleased(EMouseButton button)
	{
		m_MouseState.ButtonStates[button] = false;
	}

	void Input::OnMouseMoved(int32 x, int32 y)
	{
		if (m_InputEnabled)
		{
			m_MouseState.x = x;
			m_MouseState.y = y;
		}
	}

	void Input::OnMouseScrolled(int32 deltaX, int32 deltaY)
	{
		if (m_InputEnabled)
		{
			m_MouseState.ScrollX = deltaX;
			m_MouseState.ScrollY = deltaY;
		}
	}

	void Input::OnKeyPressed(EKey key, ModifierKeyState modifierState, bool isRepeat)
	{
		UNREFERENCED_VARIABLE(modifierState);
		if (!isRepeat)
		{
			m_KeyboardState.KeyStates[key] = m_InputEnabled;
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

	void Input::Disable()
	{
		s_pInstance->m_InputEnabled = false;
		std::fill_n(s_pInstance->m_KeyboardState.KeyStates, EKey::KEY_LAST, false);
		std::fill_n(s_pInstance->m_MouseState.ButtonStates, EMouseButton::MOUSE_BUTTON_COUNT, false);
	}
}
