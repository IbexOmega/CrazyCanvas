#ifdef LAMBDA_PLATFORM_WINDOWS
#include <stdlib.h>

#include "Application/Win32/Win32Application.h"

#include "Input/Win32/Win32InputDevice.h"
#include "Input/Win32/Win32InputCodeTable.h"

namespace LambdaEngine
{
	Win32Application Win32Application::s_Application;

	void Win32Application::AddMessageHandler(IApplicationMessageHandler* pHandler)
	{
		//Check first so that this handler is not already added
		const uint32 count = uint32(m_MessageHandlers.size());
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
		const uint32 count = uint32(m_MessageHandlers.size());
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
		m_Window.Release();
	}

	bool Win32Application::PreInit(HINSTANCE hInstance)
	{
		ASSERT(hInstance != NULL);

		WNDCLASS wc = { };
		ZERO_MEMORY(&wc, sizeof(WNDCLASS));
		wc.hInstance		= hInstance;
		wc.lpszClassName	= WINDOW_CLASS;
		wc.hbrBackground	= (HBRUSH)::GetStockObject(BLACK_BRUSH);
		wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
		wc.lpfnWndProc		= Win32Application::WindowProc;

		ATOM classAtom = ::RegisterClass(&wc);
		if (classAtom == 0)
		{
			//TODO: Log this
			return false;
		}

		if (!Win32InputCodeTable::Init())
		{
			return false;
		}

		return s_Application.Create(hInstance);
	}
	
	bool Win32Application::PostRelease()
	{
		s_Application.Destroy();

		if (!::UnregisterClass(WINDOW_CLASS, GetInstanceHandle()))
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
				pHandler->MessageProc(message.hWnd, message.uMessage, message.wParam, message.lParam);
			}
		}

		m_BufferedMessages.clear();
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

	InputDevice* Win32Application::CreateInputDevice()
	{
		Win32InputDevice* pInputDevice = new Win32InputDevice();
		s_Application.AddMessageHandler(pInputDevice);
		
		return pInputDevice;
	}

	LRESULT Win32Application::WindowProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
	{
		if (uMessage == WM_DESTROY)
		{
			Terminate();
		}

		s_Application.BufferMessage(hWnd, uMessage, wParam, lParam);
		return ::DefWindowProc(hWnd, uMessage, wParam, lParam);
	}

	void Win32Application::BufferMessage(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
	{
		m_BufferedMessages.push_back({ hWnd, uMessage, wParam, lParam });
	}
}

#endif
