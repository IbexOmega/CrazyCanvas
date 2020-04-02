#ifdef LAMBDA_PLATFORM_WINDOWS
#include <stdlib.h>

#include "Application/Win32/Win32Application.h"

#include "Input/Win32/Win32InputDevice.h"
#include "Input/Win32/Win32RawInputDevice.h"
#include "Input/Win32/Win32InputCodeTable.h"

namespace LambdaEngine
{
	Win32Application* Win32Application::s_pApplication = nullptr;

	Win32Application::Win32Application(HINSTANCE hInstance)
		: m_hInstance(hInstance)
	{
	}

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
		return m_pWindow;
	}

	const Window* Win32Application::GetWindow() const
	{
		return m_pWindow;
	}

	bool Win32Application::PreInit(HINSTANCE hInstance)
	{
		ASSERT(hInstance != NULL);

		s_pApplication = new Win32Application(hInstance);

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

		s_pApplication->m_pWindow = (Win32Window*)Win32Application::CreateWindow("Lambda Game Engine", 1440, 900);
		s_pApplication->m_pWindow->Show();
		return true;
	}
	
	bool Win32Application::PostRelease()
	{
		if (!::UnregisterClass(WINDOW_CLASS, s_pApplication->GetInstanceHandle()))
		{
			//TODO: Log this
			return false;
		}

		return true;
	}
	
	bool Win32Application::Tick()
	{
		bool result = ProcessMessages();
		s_pApplication->ProcessBufferedMessages();

		return result;
	}

	void Win32Application::ProcessBufferedMessages()
	{
		for (Win32Message& message : m_BufferedMessages)
		{
			for (IApplicationMessageHandler* pHandler : s_pApplication->m_MessageHandlers)
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

	Window* Win32Application::CreateWindow(const char* pTitle, uint32 width, uint32 height)
	{
		Win32Window* pWindow = new Win32Window();
		pWindow->Init(pTitle, width, height);
		return pWindow;
	}

	InputDevice* Win32Application::CreateInputDevice(EInputMode inputType)
	{
		InputDevice* pInputDevice = nullptr;
		if (inputType == EInputMode::INPUT_STANDARD)
		{
			pInputDevice = new Win32InputDevice();
		}
		else if (inputType == EInputMode::INPUT_RAW)
		{
			pInputDevice = new Win32RawInputDevice();
		}

		if (pInputDevice)
		{
			if (pInputDevice->Init())
			{
				s_pApplication->AddMessageHandler(pInputDevice);
			}
		}

		return pInputDevice;
	}

	LRESULT Win32Application::WindowProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
	{
		if (uMessage == WM_DESTROY)
		{
			Terminate();
		}

		s_pApplication->BufferMessage(hWnd, uMessage, wParam, lParam);
		return ::DefWindowProc(hWnd, uMessage, wParam, lParam);
	}

	void Win32Application::BufferMessage(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
	{
		m_BufferedMessages.push_back({ hWnd, uMessage, wParam, lParam });
	}
}

#endif
