#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Input/Win32/Win32InputDevice.h"
#include "Input/Win32/Win32InputCodeTable.h"

#include <Windowsx.h>

namespace LambdaEngine
{
	LRESULT Win32InputDevice::MessageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
	{
		UNREFERENCED_PARAMETER(hWnd);

		switch (uMessage)
		{
		case WM_KEYDOWN:
		{
			if (lParam & 0x40000000)
			{
				OnKeyHeldDown(Win32InputCodeTable::GetKey(int32(wParam)));
			}
			else
			{
				OnKeyDown(Win32InputCodeTable::GetKey(int32(wParam)));
			}

			break;
		}

		case WM_KEYUP:
			OnKeyUp(Win32InputCodeTable::GetKey(int32(wParam)));
			break;

		case WM_MOUSEMOVE:
			OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			break;

		case WM_LBUTTONDOWN:
			OnMouseButtonPressed(EMouseButton::MOUSE_BUTTON_LEFT);
			break;

		case WM_LBUTTONUP:
			OnMouseButtonReleased(EMouseButton::MOUSE_BUTTON_LEFT);
			break;

		case WM_MBUTTONDOWN:
			OnMouseButtonPressed(EMouseButton::MOUSE_BUTTON_MIDDLE);
			break;

		case WM_MBUTTONUP:
			OnMouseButtonReleased(EMouseButton::MOUSE_BUTTON_MIDDLE);
			break;

		case WM_RBUTTONDOWN:
			OnMouseButtonPressed(EMouseButton::MOUSE_BUTTON_RIGHT);
			break;

		case WM_RBUTTONUP:
			OnMouseButtonReleased(EMouseButton::MOUSE_BUTTON_RIGHT);
			break;

		case WM_XBUTTONDOWN:
			OnMouseButtonPressed(GET_XBUTTON_WPARAM(wParam) == 0x0001 ? EMouseButton::MOUSE_BUTTON_BACK : EMouseButton::MOUSE_BUTTON_FORWARD);
			break;

		case WM_XBUTTONUP:
			OnMouseButtonReleased(GET_XBUTTON_WPARAM(wParam) == 0x0001 ? EMouseButton::MOUSE_BUTTON_BACK : EMouseButton::MOUSE_BUTTON_FORWARD);
			break;

		case WM_MOUSEWHEEL:
			OnMouseScrolled(GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA);
			break;
		}

		return 0;
	}
}
#endif
