#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Input/API/InputDevice.h"

namespace LambdaEngine
{
	class Win32RawInputDevice : public InputDevice
	{
	public:
		Win32RawInputDevice() = default;
		~Win32RawInputDevice();

		/*
		* Registers RawInput- Devices. This disables the normal WM_KEYDOWN, WM_LBUTTONDOWN etc. messages to be sent
		*
		* return - Returns true if successful
		*/
		bool Init();

		virtual LRESULT MessageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) override;
	};
}
#endif