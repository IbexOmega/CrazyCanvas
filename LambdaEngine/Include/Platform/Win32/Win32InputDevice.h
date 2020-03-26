#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Platform/Common/InputDevice.h"

namespace LambdaEngine
{
	class Win32InputDevice : public InputDevice
	{
	public:
		Win32InputDevice() = default;
		~Win32InputDevice() = default;

		virtual LRESULT MessageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) override;
	};
}
#endif
