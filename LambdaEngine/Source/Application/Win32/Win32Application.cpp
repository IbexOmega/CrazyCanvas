#ifdef LAMBDA_PLATFORM_WINDOWS
#include <stdlib.h>

#include "Log/Log.h"

#include "Application/API/IWindowHandler.h"

#include "Application/Win32/Win32Application.h"
#include "Application/Win32/IWin32MessageHandler.h"

#include "Input/Win32/Win32InputDevice.h"
#include "Input/Win32/Win32RawInputDevice.h"
#include "Input/Win32/Win32InputCodeTable.h"

namespace LambdaEngine
{
	Win32Application* Win32Application::s_pApplication = nullptr;

	/*
	* Instance
	*/

	Win32Application::Win32Application(HINSTANCE hInstance)
		: m_hInstance(hInstance)
	{
		VALIDATE_MSG(s_pApplication == nullptr, "[Win32Application]: An instance of application already exists");
		s_pApplication = this;
	}

	Win32Application::~Win32Application()
	{
		VALIDATE_MSG(s_pApplication != nullptr, "[Win32Application]: Instance of application has already been deleted");
		s_pApplication = nullptr;

		for (Win32Window* pWindow : m_Windows)
		{
			SAFERELEASE(pWindow);
		}

		m_hInstance = 0;
	}

	void Win32Application::AddWin32MessageHandler(IWin32MessageHandler* pHandler)
	{
		// Check first so that this handler is not already added
		const uint32 count = uint32(m_MessageHandlers.size());
		for (uint32 i = 0; i < count; i++)
		{
			if (pHandler == m_MessageHandlers[i])
			{
				return;
			}
		}

		// Add new handler
		m_MessageHandlers.emplace_back(pHandler);
	}

	void Win32Application::RemoveWin32MessageHandler(IWin32MessageHandler* pHandler)
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

	void Win32Application::AddWindowHandler(IWindowHandler* pHandler)
	{
		// Check first so that this handler is not already added
		const uint32 count = uint32(m_WindowHandlers.size());
		for (uint32 i = 0; i < count; i++)
		{
			if (pHandler == m_WindowHandlers[i])
			{
				return;
			}
		}

		// Add new handler
		m_WindowHandlers.emplace_back(pHandler);
	}

	void Win32Application::RemoveWindowHandler(IWindowHandler* pHandler)
	{
		const uint32 count = uint32(m_WindowHandlers.size());
		for (uint32 i = 0; i < count; i++)
		{
			if (pHandler == m_WindowHandlers[i])
			{
				m_WindowHandlers.erase(m_WindowHandlers.begin() + i);
				break;
			}
		}
	}

	void Win32Application::ProcessStoredEvents()
	{
		TArray<Win32Message> messagesToProcess = TArray<Win32Message>(m_StoredMessages);
		m_StoredMessages.clear();

		for (Win32Message& message : messagesToProcess)
		{
			ProcessMessage(message.hWnd, message.uMessage, message.wParam, message.lParam);
		}
	}

	void Win32Application::MakeMainWindow(IWindow* pMainWindow)
	{
		m_pMainWindow = reinterpret_cast<Win32Window*>(pMainWindow);
	}

	void Win32Application::ProcessMessage(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
	{
		Win32Window* pMessageWindow = GetWindowFromHandle(hWnd);

		switch (uMessage)
		{
			case WM_MOVE:
			{
				const int16 x = (int16)LOWORD(lParam);
				const int16 y = (int16)HIWORD(lParam);

				for (IWindowHandler* pWindowHandler : m_WindowHandlers)
				{
					pWindowHandler->WindowMoved(pMessageWindow, x, y);
				}

				break;
			}

			case WM_SIZE:
			{
				EResizeType resizeType = EResizeType::RESIZE_TYPE_NONE;
				if (wParam == SW_MINIMIZE)
				{
					resizeType = EResizeType::RESIZE_TYPE_MINIMIZE;
				}
				else if (wParam == SW_MAXIMIZE)
				{
					resizeType = EResizeType::RESIZE_TYPE_MAXIMIZE;
				}

				const uint16 width	= (uint16)LOWORD(lParam);
				const uint16 height = (uint16)HIWORD(lParam);

				for (IWindowHandler* pWindowHandler : m_WindowHandlers)
				{
					pWindowHandler->WindowResized(pMessageWindow, width, height, resizeType);
				}

				break;
			}

			case WM_MOUSEMOVE:
			{
				if (!m_IsTrackingMouse)
				{
					m_IsTrackingMouse = true;

					TRACKMOUSEEVENT tme = { sizeof(tme) };
					tme.dwFlags		= TME_LEAVE;
					tme.hwndTrack	= hWnd;
					TrackMouseEvent(&tme);

					for (IWindowHandler* pWindowHandler : m_WindowHandlers)
					{
						pWindowHandler->MouseEntered(pMessageWindow);
					}
				}
				break;
			}

			case WM_MOUSELEAVE:
			{
				for (IWindowHandler* pWindowHandler : m_WindowHandlers)
				{
					pWindowHandler->MouseLeft(pMessageWindow);
				}

				m_IsTrackingMouse = false;
				break;
			}

			case WM_DESTROY:
			{
				if (pMessageWindow == m_pMainWindow)
				{
					Terminate();
				}

				for (IWindowHandler* pWindowHandler : m_WindowHandlers)
				{
					pWindowHandler->WindowClosed(pMessageWindow);
				}

				break;
			}

			case WM_SETFOCUS:
			case WM_KILLFOCUS:
			{
				bool hasFocus = (uMessage == WM_SETFOCUS);

				for (IWindowHandler* pWindowHandler : m_WindowHandlers)
				{
					pWindowHandler->FocusChanged(pMessageWindow, hasFocus);
				}

				break;
			}
		}

		for (IWin32MessageHandler* pHandler : m_MessageHandlers)
		{
			pHandler->MessageProc(hWnd, uMessage, wParam, lParam);
		}
	}

	Win32Window* Win32Application::GetWindowFromHandle(HWND hWnd) const
	{
		for (Win32Window* pWindow : m_Windows)
		{
			HWND handle = (HWND)pWindow->GetHandle();
			if (handle == hWnd)
			{
				return pWindow;
			}
		}

		return nullptr;
	}

	HINSTANCE Win32Application::GetInstanceHandle()
	{
		return Win32Application::Get()->m_hInstance;
	}

	IWindow* Win32Application::GetForegroundWindow() const
	{
		HWND hForegroundWindow = ::GetForegroundWindow();
		return GetWindowFromHandle(hForegroundWindow);
	}

	IWindow* Win32Application::GetMainWindow() const
	{
		return m_pMainWindow;
	}

	void Win32Application::AddWindow(Win32Window* pWindow)
	{
		m_Windows.emplace_back(pWindow);
	}

	/*
	* Static
	*/

	bool Win32Application::PreInit(HINSTANCE hInstance)
	{
		VALIDATE(hInstance != NULL);

		Win32Application* pApplication = DBG_NEW Win32Application(hInstance);

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
			LOG_ERROR("[Win32Application]: Failed to register windowclass");
			return false;
		}

		if (!Win32InputCodeTable::Init())
		{
			return false;
		}

		Win32Window* pWindow = (Win32Window*)Win32Application::CreateWindow("Lambda Game Engine", 1440, 900);
		if (pWindow)
		{
			pApplication->MakeMainWindow(pWindow);
			pWindow->Show();
			
			return true;
		}
		else
		{
			return false;
		}
	}
	
	bool Win32Application::PostRelease()
	{
		if (!::UnregisterClass(WINDOW_CLASS, Win32Application::Get()->GetInstanceHandle()))
		{
			LOG_ERROR("[Win32Application]: Failed to unregister windowclass");
			return false;
		}

		SAFEDELETE(s_pApplication);
		return true;
	}
	
	bool Win32Application::Tick()
	{
		bool result = ProcessMessages();
		Win32Application::Get()->ProcessStoredEvents();

		return result;
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

	IWindow* Win32Application::CreateWindow(const char* pTitle, uint32 width, uint32 height)
	{
		Win32Window* pWindow = DBG_NEW Win32Window();
		if (!pWindow->Init(pTitle, width, height))
		{
			SAFEDELETE(pWindow);
		}
		else
		{
			Win32Application::Get()->AddWindow(pWindow);
		}
		
		return pWindow;
	}

	IInputDevice* Win32Application::CreateInputDevice(EInputMode inputType)
	{
		if (inputType == EInputMode::INPUT_STANDARD)
		{
			Win32InputDevice* pInputDevice = DBG_NEW Win32InputDevice();
			Win32Application::Get()->AddWin32MessageHandler(pInputDevice);

			return pInputDevice;
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

		return nullptr;
	}

	void Win32Application::Terminate()
	{
		//TODO: Maybe take in the exitcode
		PostQuitMessage(0);
	}

	LRESULT Win32Application::WindowProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
	{
		VALIDATE(Win32Application::Get() != nullptr);

		Win32Application::Get()->StoreMessage(hWnd, uMessage, wParam, lParam);
		return ::DefWindowProc(hWnd, uMessage, wParam, lParam);
	}

	void Win32Application::StoreMessage(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
	{
		m_StoredMessages.push_back({ hWnd, uMessage, wParam, lParam });
	}
}

#endif
