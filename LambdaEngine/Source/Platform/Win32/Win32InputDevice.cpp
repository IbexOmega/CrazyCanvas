#include "Platform/Win32/Win32InputDevice.h"
#include <windowsx.h>

#include "Platform/PlatformConsole.h"

namespace LambdaEngine
{
	Win32InputDevice::Win32InputDevice() :
		m_KeyboardState(),
		m_MouseState()
	{
	}

	Win32InputDevice::~Win32InputDevice()
	{
	}

	KeyboardState Win32InputDevice::GetKeyboardState()
	{
		return m_KeyboardState;
	}

	MouseState Win32InputDevice::GetMouseState()
	{
		return m_MouseState;
	}

	LRESULT Win32InputDevice::MessageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
	{
		switch (uMessage)
		{
		case WM_KEYDOWN:
			m_KeyboardState.KeyStates[wParam] = true;
			break;

		case WM_KEYUP:
			m_KeyboardState.KeyStates[wParam] = false;
			break;

		case WM_MOUSEMOVE:
			m_MouseState.X = GET_X_LPARAM(lParam);
			m_MouseState.Y = GET_Y_LPARAM(lParam);
			break;

		case WM_LBUTTONDOWN:
			m_MouseState.ButtonStates[0] = true;
			break;

		case WM_LBUTTONUP:
			m_MouseState.ButtonStates[0] = false;
			break;

		case WM_MBUTTONDOWN:
			m_MouseState.ButtonStates[1] = true;
			break;

		case WM_MBUTTONUP:
			m_MouseState.ButtonStates[1] = false;
			break;

		case WM_RBUTTONDOWN:
			m_MouseState.ButtonStates[2] = true;
			break;

		case WM_RBUTTONUP:
			m_MouseState.ButtonStates[2] = false;
			break;

		case WM_XBUTTONDOWN:
			m_MouseState.ButtonStates[2 + GET_XBUTTON_WPARAM(wParam)] = true;
			break;

		case WM_XBUTTONUP:
			m_MouseState.ButtonStates[2 + GET_XBUTTON_WPARAM(wParam)] = false;
			break;

		case WM_MOUSEWHEEL:
			m_MouseState.Scroll = GET_WHEEL_DELTA_WPARAM(wParam);
			break;
		}

		PlatformConsole::PrintLine("Input!");
		return 0;
	}
}
