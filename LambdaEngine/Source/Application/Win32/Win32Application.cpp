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

	Win32Application::~Win32Application()
	{
		SAFEDELETE(m_pWindow);
		m_hInstance = 0;
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

		s_pApplication = DBG_NEW Win32Application(hInstance);

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

		SAFEDELETE(s_pApplication);
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

	HINSTANCE Win32Application::GetInstanceHandle()
	{
		return s_pApplication->m_hInstance;
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
		Win32Window* pWindow = DBG_NEW Win32Window();
		if (!pWindow->Init(pTitle, width, height))
		{
			SAFEDELETE(pWindow);
		}

		return pWindow;
	}

	IInputDevice* Win32Application::CreateInputDevice(EInputMode inputType)
	{
		InputDevice* pInputDevice = nullptr;
		if (inputType == EInputMode::INPUT_STANDARD)
		{
			pInputDevice = DBG_NEW Win32InputDevice();
		}
		else if (inputType == EInputMode::INPUT_RAW)
		{
			Win32RawInputDevice* pRawInputDevice = DBG_NEW Win32RawInputDevice();
			if (!pRawInputDevice->Init())
			{
				SAFEDELETE(pRawInputDevice);
				return nullptr;
			}
		}

		s_pApplication->AddMessageHandler(pInputDevice);
		return pInputDevice;
	}

	void Win32Application::Terminate()
	{
		//TODO: Maybe take in the exitcode
		PostQuitMessage(0);
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
