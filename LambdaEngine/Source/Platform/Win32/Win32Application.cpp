#ifdef LAMBDA_PLATFORM_WINDOWS
#include <stdlib.h>

#include "Platform/Win32/Win32Application.h"

namespace LambdaEngine
{
	Win32Application Win32Application::s_Application;

	void Win32Application::AddMessageHandler(IApplicationMessageHandler* pHandler)
	{
		//Check first so that this handler is not already added
		const uint32 count = uint32(m_MessageHandler.size());
		for (uint32 i = 0; i < count; i++)
		{
			if (pHandler == m_MessageHandlers[i])
			{
				return;
			}
		}

		//Add new handler
		m_MessageHandlers.emplace_back(pHandler);
	}

	void Win32Application::RemoveMessageHandler(IApplicationMessageHandler* pHandler)
	{
		const uint32 count = uint32(m_MessageHandler.size());
		for (uint32 i = 0; i < count; i++)
		{
			if (pHandler == m_MessageHandlers[i])
			{
				m_MessageHandlers.erase(m_MessageHandlers.begin() + i);
				break;
			}
		}
	}

	Window* Win32Application::GetWindow()
	{
		return &m_Window;
	}

	const Window* Win32Application::Getwindow() const
	{
		return &m_Window;
	}

	void Win32Application::Create(HINSTANCE hInstance)
	{
		m_hInstance = hInstance;
		if (!s_Window.Init(800, 600))
		{
			return false;
		}

		s_Window.Show();
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
		if (!::UnregisterClass(WINDOW_CLASS, s_hInstance))
		{
			//TODO: Log this
			return false;
		}

		return true;
	}
	
	bool Win32Application::Tick()
	{
		bool result = ProcessMessages();

		s_Application.ProcessBufferedMessages();

		return result;
	}

	void Win32Application::ProcessBufferedMessages()
	{
		for (Win32Message& message : m_BufferedMessages)
		{
			std::vector<IApplicationMessageHandler*>& handlers = s_Application.m_MessageHandlers;
			for (IApplicationMessageHandler* pHandler : handlers)
			{
				pHandler->MessageProc(hWnd, uMessage, wParam, lParam);
			}
		}
	}

	bool Win32Application::ProcessMessages()
	{
		MSG msg = { };
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

	LRESULT Win32Application::WindowProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
	{
		if (uMessage == WM_DESTROY)
		{
			Terminate();
		}

		s_Application.BufferMessage(hWnd, uMessage);
		return ::DefWindowProc(hWnd, uMessage, wParam, lParam);
	}

	void Win32Application::BufferMessage(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
	{
		m_BufferedMessages.emplace_back({ hWnd, uMessage, wParam, lParam });
	}
}

#endif
