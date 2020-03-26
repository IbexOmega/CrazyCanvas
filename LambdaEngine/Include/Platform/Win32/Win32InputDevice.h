#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Platform/Common/IInputDevice.h"

namespace LambdaEngine
{
	class Win32InputDevice : public IInputDevice
	{
	public:
		Win32InputDevice();
		~Win32InputDevice();

		virtual KeyboardState GetKeyboardState() override;
		virtual MouseState GetMouseState() override;

		virtual LRESULT MessageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) override;

	private:
		KeyboardState m_KeyboardState;
		MouseState m_MouseState;
	};
}
#endif
