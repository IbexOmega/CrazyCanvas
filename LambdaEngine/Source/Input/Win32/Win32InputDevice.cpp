#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Input/Win32/Win32InputDevice.h"
#include "Input/Win32/Win32InputCodeTable.h"

#include <Windowsx.h>

namespace LambdaEngine
{
	LRESULT Win32InputDevice::MessageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
	{
		UNREFERENCED_VARIABLE(hWnd);

		constexpr uint16 SCAN_CODE_MASK		= 0x01ff;
		constexpr uint32 REPEAT_KEY_MASK	= 0x40000000;
		constexpr uint16 BACK_BUTTON_MASK	= 0x0001;

		switch (uMessage)
		{
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
			{
				const uint32	modifierMask	= Win32InputCodeTable::GetModifierMask();
				const uint16	scancode		= HIWORD(lParam) & SCAN_CODE_MASK;
				EKey			keyCode			= Win32InputCodeTable::GetKeyFromScanCode(scancode);
				bool			isRepeat		= (lParam & REPEAT_KEY_MASK);

				OnKeyPressed(keyCode, modifierMask, isRepeat);
				break;
			}

			case WM_KEYUP:
			case WM_SYSKEYUP:
			{
				const uint16	scancode = HIWORD(lParam) & SCAN_CODE_MASK;
				EKey			keyCode = Win32InputCodeTable::GetKeyFromScanCode(scancode);

				OnKeyReleased(keyCode);
				break;
			}

			case WM_CHAR:
			case WM_SYSCHAR:
			{
				OnKeyTyped(uint32(wParam));
				break;
			}

			case WM_MOUSEMOVE:
			{
				const int32 x = GET_X_LPARAM(lParam);
				const int32 y = GET_Y_LPARAM(lParam);
			
				OnMouseMoved(x, y);
				break;
			}

			case WM_LBUTTONDOWN:
			case WM_MBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_XBUTTONDOWN:
			{
				EMouseButton button = EMouseButton::MOUSE_BUTTON_UNKNOWN;
				if (uMessage == WM_LBUTTONDOWN)
				{
					button = EMouseButton::MOUSE_BUTTON_LEFT;
				}
				else if (uMessage == WM_MBUTTONDOWN)
				{
					button = EMouseButton::MOUSE_BUTTON_MIDDLE;
				}
				else if (uMessage == WM_RBUTTONDOWN)
				{
					button = EMouseButton::MOUSE_BUTTON_RIGHT;
				}
				else if (GET_XBUTTON_WPARAM(wParam) == BACK_BUTTON_MASK)
				{
					button = EMouseButton::MOUSE_BUTTON_BACK;
				}
				else
				{
					button = EMouseButton::MOUSE_BUTTON_FORWARD;
				}

				const uint32 modifierMask = Win32InputCodeTable::GetModifierMask();
				OnMouseButtonPressed(button, modifierMask);
				break;
			}

			case WM_LBUTTONUP:
			case WM_MBUTTONUP:
			case WM_RBUTTONUP:
			case WM_XBUTTONUP:
			{
				EMouseButton button = EMouseButton::MOUSE_BUTTON_UNKNOWN;
				if (uMessage == WM_LBUTTONUP)
				{
					button = EMouseButton::MOUSE_BUTTON_LEFT;
				}
				else if (uMessage == WM_MBUTTONUP)
				{
					button = EMouseButton::MOUSE_BUTTON_MIDDLE;
				}
				else if (uMessage == WM_RBUTTONUP)
				{
					button = EMouseButton::MOUSE_BUTTON_RIGHT;
				}
				else if (GET_XBUTTON_WPARAM(wParam) == BACK_BUTTON_MASK)
				{
					button = EMouseButton::MOUSE_BUTTON_BACK;
				}
				else
				{
					button = EMouseButton::MOUSE_BUTTON_FORWARD;
				}

				OnMouseButtonReleased(button);
				break;
			}

			case WM_MOUSEWHEEL:
			{
				int32 scollDelta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
				OnMouseScrolled(0, scollDelta);
				break;
			}

			case WM_MOUSEHWHEEL:
			{
				int32 scollDelta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
				OnMouseScrolled(scollDelta, 0);
				break;
			}
		}

		return 0;
	}
}
#endif
