#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Log/Log.h"

#include "Input/Win32/Win32RawInputDevice.h"

namespace LambdaEngine
{
	Win32RawInputDevice::~Win32RawInputDevice()
	{
		RAWINPUTDEVICE devices[2];
		ZERO_MEMORY(devices, sizeof(RAWINPUTDEVICE) * 2);

		//Keyboard
		devices[0].dwFlags = RIDEV_REMOVE;
		devices[0].hwndTarget = 0;
		devices[0].usUsage = 0x06;
		devices[0].usUsagePage = 0x01;

		//Mouse
		devices[1].dwFlags = RIDEV_REMOVE;
		devices[1].hwndTarget = 0;
		devices[1].usUsage = 0x02;
		devices[1].usUsagePage = 0x01;

		BOOL bResult = RegisterRawInputDevices(devices, 2, sizeof(RAWINPUTDEVICE));
		if (bResult == FALSE)
		{
			LOG_ERROR("[Win32RawInputDevice]: Failed to unregister raw input devices");
		}
		else
		{
			D_LOG_MESSAGE("[Win32RawInputDevice]: Unregistered keyboard and mouse devices");
		}
	}

	bool Win32RawInputDevice::Init()
	{
		RAWINPUTDEVICE devices[2];
		ZERO_MEMORY(devices, sizeof(RAWINPUTDEVICE) * 2);

		//Keyboard
		devices[0].dwFlags		= RIDEV_NOLEGACY;
		devices[0].hwndTarget	= 0;
		devices[0].usUsage		= 0x06;
		devices[0].usUsagePage	= 0x01;

		//Mouse
		devices[1].dwFlags		= 0;
		devices[1].hwndTarget	= 0;
		devices[1].usUsage		= 0x02;
		devices[1].usUsagePage	= 0x01;

		BOOL bResult = RegisterRawInputDevices(devices, 2, sizeof(RAWINPUTDEVICE));
		if (bResult == FALSE)
		{
			LOG_ERROR("[Win32RawInputDevice]: Failed to register raw input devices");
			return false;
		}

		D_LOG_MESSAGE("[Win32RawInputDevice]: Registered keyboard and mouse devices");
		return true;
	}

	LRESULT Win32RawInputDevice::MessageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
	{
		UNREFERENCED_VARIABLE(hWnd);
		UNREFERENCED_VARIABLE(wParam);

		switch (uMessage)
		{
			case WM_INPUT:
			{
				UINT uSize = 0;
				GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &uSize, sizeof(RAWINPUTHEADER));
				
				LPBYTE lpBytes = DBG_NEW BYTE[uSize];
				if (lpBytes == NULL)
				{
					return 0;
				}

				if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpBytes, &uSize, sizeof(RAWINPUTHEADER)) != uSize)
				{
					LOG_ERROR("[Win32RawInputDevice]: GetRawInputData does not return correct size");

					SAFEDELETE_ARRAY(lpBytes);
					return 0;
				}

				RAWINPUT* pRawInput = (RAWINPUT*)lpBytes;
				if (pRawInput->header.dwType == RIM_TYPEKEYBOARD)
				{
					//hResult = StringCchPrintf(szTempOutput, STRSAFE_MAX_CCH, TEXT(" Kbd: make=%04x Flags:%04x Reserved:%04x ExtraInformation:%08x, msg=%04x VK=%04x \n"),
					//pRawInput->data.keyboard.MakeCode,
					//pRawInput->data.keyboard.Flags,
					//pRawInput->data.keyboard.Reserved,
					//pRawInput->data.keyboard.ExtraInformation,
					//pRawInput->data.keyboard.Message,
					//pRawInput->data.keyboard.VKey);
					//
					//if (FAILED(hResult))
					//{
					//	// TODO: write error handler
					//}
				}
				else if (pRawInput->header.dwType == RIM_TYPEMOUSE)
				{
					//hResult = StringCchPrintf(szTempOutput, STRSAFE_MAX_CCH, TEXT("Mouse: usFlags=%04x ulButtons=%04x usButtonFlags=%04x usButtonData=%04x ulRawButtons=%04x lLastX=%04x lLastY=%04x ulExtraInformation=%04x\r\n"),
					//	pRawInput->data.mouse.usFlags,
					//	pRawInput->data.mouse.ulButtons,
					//	pRawInput->data.mouse.usButtonFlags,
					//	pRawInput->data.mouse.usButtonData,
					//	pRawInput->data.mouse.ulRawButtons,
					//	pRawInput->data.mouse.lLastX,
					//	pRawInput->data.mouse.lLastY,
					//	pRawInput->data.mouse.ulExtraInformation);

					//if (FAILED(hResult))
					//{
					//	// TODO: write error handler
					//}
				}

				SAFEDELETE_ARRAY(lpBytes);
				return 0;
			}
		}

		return 0;
	}
}

#endif