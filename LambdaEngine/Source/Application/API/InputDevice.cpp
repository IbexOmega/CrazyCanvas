#include "Input/IKeyboardHandler.h"
#include "Input/IMouseHandler.h"

#include "Application/API/InputDevice.h"

namespace LambdaEngine
{
	void InputDevice::OnKeyDown(EKey key)
	{
		m_KeyboardState.KeyStates[key] = true;

		for (IKeyboardHandler* pHandler : m_KeyboardHandlers)
		{
			pHandler->OnKeyDown(key);
		}
	}

	void InputDevice::OnKeyHeldDown(EKey key)
	{
		for (IKeyboardHandler* pHandler : m_KeyboardHandlers)
		{
			pHandler->OnKeyHeldDown(key);
		}
	}

	void InputDevice::OnKeyUp(EKey key)
	{
		m_KeyboardState.KeyStates[key] = false;

		for (IKeyboardHandler* pHandler : m_KeyboardHandlers)
		{
			pHandler->OnKeyUp(key);
		}
	}

	void InputDevice::OnMouseMove(int32 x, int32 y)
	{
		m_MouseState.x = x;
		m_MouseState.y = y;

		for (IMouseHandler* pHandler : m_MouseHandlers)
		{
			pHandler->OnMouseMove(x, y);
		}
	}

	void InputDevice::OnMouseButtonPressed(EMouseButton button)
	{
		m_MouseState.ButtonStates[button] = true;

		for (IMouseHandler* pHandler : m_MouseHandlers)
		{
			pHandler->OnButtonPressed(button);
		}
	}

	void InputDevice::OnMouseButtonReleased(EMouseButton button)
	{
		m_MouseState.ButtonStates[button] = false;

		for (IMouseHandler* pHandler : m_MouseHandlers)
		{
			pHandler->OnButtonReleased(button);
		}
	}

	void InputDevice::OnMouseScrolled(int32 delta)
	{
		m_MouseState.Scroll = delta;

		for (IMouseHandler* pHandler : m_MouseHandlers)
		{
			pHandler->OnScroll(delta);
		}
	}

	void InputDevice::AddKeyboardHandler(IKeyboardHandler* pHandler)
	{
		m_KeyboardHandlers.emplace_back(pHandler);
	}

	void InputDevice::AddMouseHandler(IMouseHandler* pHandler)
	{
		m_MouseHandlers.emplace_back(pHandler);
	}

	void InputDevice::RemoveKeyboardHandler(IKeyboardHandler* pHandler)
	{
		for (auto it = m_KeyboardHandlers.begin(); it != m_KeyboardHandlers.end(); it++)
		{
			if (*it == pHandler)
			{
				m_KeyboardHandlers.erase(it);
				return;
			}
		}
	}

	void InputDevice::RemoveMouseHandler(IMouseHandler* pHandler)
	{
		for (auto it = m_MouseHandlers.begin(); it != m_MouseHandlers.end(); it++)
		{
			if (*it == pHandler)
			{
				m_MouseHandlers.erase(it);
				return;
			}
		}
	}
}
