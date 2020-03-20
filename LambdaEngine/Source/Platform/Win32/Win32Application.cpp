#ifdef LAMBDA_PLATFORM_WINDOWS
#include <stdlib.h>

#include "Platform/Win32/Win32Application.h"

#define WINDOW_CLASS L"MainWindowClass"

namespace LambdaEngine
{
	HINSTANCE Win32Application::s_hInstance = 0;

	bool Win32Application::PreInit(HINSTANCE hInstance)
	{
		s_hInstance = hInstance;

		WNDCLASS wc = {};
		ZERO_MEMORY(&wc, sizeof(WNDCLASS));
		wc.hInstance		= s_hInstance;
		wc.lpszClassName	= WINDOW_CLASS;
		wc.lpfnWndProc		= Win32Application::WindowProc;

		ATOM classAtom = RegisterClass(&wc);
		if (classAtom == 0)
		{
			//TODO: Log this
			return false;
		}

		HWND hWnd = CreateWindowEx(0, WINDOW_CLASS, L"Learn to Program Windows", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
		if (hWnd == NULL)
		{
			//TODO: Log this
			return false;
		}

		ShowWindow(hWnd, SW_NORMAL);

		return true;
	}
	
	bool Win32Application::PostRelease()
	{
		if (!UnregisterClass(WINDOW_CLASS, s_hInstance))
		{
			//TODO: Log this
			return false;
		}

		return false;
	}
	
	void Win32Application::Tick()
	{
		MSG msg = {};
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	LRESULT Win32Application::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if (uMsg == WM_DESTROY)
		{
			PostQuitMessage(0);
		}

		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
}

#endif