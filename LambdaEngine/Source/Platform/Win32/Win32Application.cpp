#ifdef LAMBDA_PLATFORM_WINDOWS
#include <stdlib.h>

#include "Platform/Win32/Win32Application.h"

namespace LambdaEngine
{
	HINSTANCE	Win32Application::s_hInstance	= 0;
	Win32Window	Win32Application::s_Window		= Win32Window();

	bool Win32Application::PreInit(HINSTANCE hInstance)
	{
		ASSERT(hInstance != NULL);

		s_hInstance = hInstance;

		WNDCLASS wc = {};
		ZERO_MEMORY(&wc, sizeof(WNDCLASS));
		wc.hInstance		= s_hInstance;
		wc.lpszClassName	= WINDOW_CLASS;
		wc.hbrBackground	= (HBRUSH)GetStockObject(BLACK_BRUSH);
		wc.lpfnWndProc		= Win32Application::WindowProc;

		ATOM classAtom = RegisterClass(&wc);
		if (classAtom == 0)
		{
			//TODO: Log this
			return false;
		}

		if (!s_Window.Init(800, 600))
		{
			return false;
		}

		s_Window.Show();
		return true;
	}
	
	bool Win32Application::PostRelease()
	{
		s_Window.Release();

		if (!UnregisterClass(WINDOW_CLASS, s_hInstance))
		{
			//TODO: Log this
			return false;
		}

		return true;
	}
	
	bool Win32Application::Tick()
	{
		MSG msg = {};
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
			{
				return false;
			}
		}

		return true;
	}

	LRESULT Win32Application::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if (uMsg == WM_DESTROY)
		{
			Terminate();
		}

		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
}

#endif
