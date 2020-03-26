#ifdef LAMBDA_PLATFORM_WINDOWS
#include <stdlib.h>

#include "Platform/Win32/Win32Application.h"
#include "Platform/Win32/Win32InputDevice.h"

namespace LambdaEngine
{
	Win32Application Win32Application::s_Application;

	void Win32Application::AddMessageHandler(IApplicationMessageHandler* pHandler)
	{
		m_MessageHandlers.emplace_back(pHandler);
	}

	Window* Win32Application::GetWindow()
	{
		return &m_Window;
	}

	const Window* Win32Application::GetWindow() const
	{
		return &m_Window;
	}

	bool Win32Application::Create(HINSTANCE hInstance)
	{
		m_hInstance = hInstance;
		if (!m_Window.Init(800, 600))
		{
			return false;
		}

		m_Window.Show();
		return true;
	}

	void Win32Application::Destroy()
	{
		s_Application.Destroy();
	}

	bool Win32Application::PreInit(HINSTANCE hInstance)
	{
		ASSERT(hInstance != NULL);

		WNDCLASS wc = {};
		ZERO_MEMORY(&wc, sizeof(WNDCLASS));
		wc.hInstance		= hInstance;
		wc.lpszClassName	= WINDOW_CLASS;
		wc.hbrBackground	= (HBRUSH)::GetStockObject(BLACK_BRUSH);
		wc.lpfnWndProc		= Win32Application::WindowProc;

		ATOM classAtom = ::RegisterClass(&wc);
		if (classAtom == 0)
		{
			//TODO: Log this
			return false;
		}

		if (!s_Application.Create(hInstance))
		{
			return false;
		}

		return true;
	}
	
	bool Win32Application::PostRelease()
	{
		if (!::UnregisterClass(WINDOW_CLASS, GetInstanceHandle()))
		{
			//TODO: Log this
			return false;
		}

		return true;
	}
	
	bool Win32Application::Tick()
	{
		MSG msg = {};
		while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
			{
				return false;
			}
		}

		return true;
	}

	IInputDevice* Win32Application::CreateInputDevice()
	{
		Win32InputDevice* pInputDevice = new Win32InputDevice();
		s_Application.AddMessageHandler(pInputDevice);
		return pInputDevice;
	}

	LRESULT Win32Application::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if (uMsg == WM_DESTROY)
		{
			Terminate();
		}

		std::vector<IApplicationMessageHandler*>& handlers = s_Application.m_MessageHandlers;
		for (IApplicationMessageHandler* pHandler : handlers)
		{
			pHandler->MessageProc(hWnd, uMsg, wParam, lParam);
		}

		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
}

#endif
