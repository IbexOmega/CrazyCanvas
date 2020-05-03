#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Input/API/InputDeviceBase.h"

#include "Application/Win32/IWin32MessageHandler.h"

namespace LambdaEngine
{
	class Win32InputDevice : public InputDeviceBase, public IWin32MessageHandler 
	{
	public:
		Win32InputDevice() 	= default;
		~Win32InputDevice() = default;

		virtual LRESULT MessageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) override;
	};
}
#endif
