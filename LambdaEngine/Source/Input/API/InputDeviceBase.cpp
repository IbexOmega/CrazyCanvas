#include "Input/API/IKeyboardHandler.h"
#include "Input/API/IMouseHandler.h"
#include "Input/API/InputDeviceBase.h"

namespace LambdaEngine
{
    KeyboardState InputDeviceBase::GetKeyboardState() const
    {
        return m_KeyboardState;
    }

    MouseState InputDeviceBase::GetMouseState() const
    {
        return m_MouseState;
    }

	void InputDeviceBase::OnKeyPressed(EKey key, uint32 modifierMask, bool isRepeat)
	{
        if (!isRepeat)
        {
            m_KeyboardState.KeyStates[key] = true;
        }
        
		for (IKeyboardHandler* pHandler : m_KeyboardHandlers)
		{
			pHandler->KeyPressed(key, modifierMask, isRepeat);
		}
	}

	void InputDeviceBase::OnKeyReleased(EKey key)
	{
		m_KeyboardState.KeyStates[key] = false;

		for (IKeyboardHandler* pHandler : m_KeyboardHandlers)
		{
			pHandler->KeyReleased(key);
		}
	}

    void InputDeviceBase::OnKeyTyped(uint32 character)
    {
        for (IKeyboardHandler* pHandler : m_KeyboardHandlers)
        {
            pHandler->KeyTyped(character);
        }
    }

	void InputDeviceBase::OnMouseMoved(int32 x, int32 y)
	{
		m_MouseState.x = x;
		m_MouseState.y = y;

		for (IMouseHandler* pHandler : m_MouseHandlers)
		{
			pHandler->MouseMoved(x, y);
		}
	}

	void InputDeviceBase::OnMouseButtonPressed(EMouseButton button, uint32 modifierMask)
	{
		m_MouseState.ButtonStates[button] = true;

		for (IMouseHandler* pHandler : m_MouseHandlers)
		{
			pHandler->ButtonPressed(button, modifierMask);
		}
	}

	void InputDeviceBase::OnMouseButtonReleased(EMouseButton button)
	{
		m_MouseState.ButtonStates[button] = false;

		for (IMouseHandler* pHandler : m_MouseHandlers)
		{
			pHandler->ButtonReleased(button);
		}
	}

	void InputDeviceBase::OnMouseScrolled(int32 deltaX, int32 deltaY)
	{
        m_MouseState.ScrollX = deltaX;
		m_MouseState.ScrollY = deltaY;

		for (IMouseHandler* pHandler : m_MouseHandlers)
		{
			pHandler->MouseScrolled(deltaX, deltaY);
		}
	}

	void InputDeviceBase::AddKeyboardHandler(IKeyboardHandler* pHandler)
	{
		m_KeyboardHandlers.emplace_back(pHandler);
	}

	void InputDeviceBase::AddMouseHandler(IMouseHandler* pHandler)
	{
		m_MouseHandlers.emplace_back(pHandler);
	}

	void InputDeviceBase::RemoveKeyboardHandler(IKeyboardHandler* pHandler)
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

	void InputDeviceBase::RemoveMouseHandler(IMouseHandler* pHandler)
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
