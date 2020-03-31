#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Input/API/InputDevice.h"

namespace LambdaEngine
{
	class Win32RawInputDevice : public InputDevice
	{
	public:
		Win32RawInputDevice()	= default;
		~Win32RawInputDevice()	= default;

		virtual bool Init()		override;
		virtual void Release()	override;

		virtual LRESULT MessageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) override;
	};
}
#endif