#ifdef LAMBDA_PLATFORM_WINDOWS
#include <stdlib.h>

#include "Log/Log.h"

#include "Application/API/IEventHandler.h"

#include "Application/Win32/Win32Application.h"
#include "Application/Win32/IWin32MessageHandler.h"

#include "Input/Win32/Win32InputCodeTable.h"

#include <windowsx.h>

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

	void Win32Application::AddEventHandler(IEventHandler* pEventHandler)
	{
		// Check first so that this handler is not already added
		const uint32 count = uint32(m_EventHandlers.size());
		for (uint32 i = 0; i < count; i++)
		{
			if (pEventHandler == m_EventHandlers[i])
			{
				return;
			}
		}

		// Add new handler
		m_EventHandlers.emplace_back(pEventHandler);
	}

	void Win32Application::RemoveEventHandler(IEventHandler* pEventHandler)
	{
		const uint32 count = uint32(m_EventHandlers.size());
		for (uint32 i = 0; i < count; i++)
		{
			if (pEventHandler == m_EventHandlers[i])
			{
				m_EventHandlers.erase(m_EventHandlers.begin() + i);
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

	void Win32Application::SetInputMode(EInputMode inputMode)
	{
		if (inputMode == m_InputMode)
		{
			return;
		}

		if (inputMode == EInputMode::INPUT_RAW)
		{
			RegisterRawInputDevices();
		}
		else if (inputMode == EInputMode::INPUT_STANDARD)
		{
			if (m_InputMode == EInputMode::INPUT_RAW)
			{
				UnregisterRawInputDevices();
			}
		}

		m_InputMode = inputMode;
	}

	void Win32Application::ProcessMessage(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
	{
		constexpr uint16 SCAN_CODE_MASK		= 0x01ff;
		constexpr uint32 REPEAT_KEY_MASK	= 0x40000000;
		constexpr uint16 BACK_BUTTON_MASK	= 0x0001;

		Win32Window* pMessageWindow = GetWindowFromHandle(hWnd);
		switch (uMessage)
		{
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
			{
				const uint32	modifierMask = Win32InputCodeTable::GetModifierMask();
				const uint16	scancode = HIWORD(lParam) & SCAN_CODE_MASK;
				EKey			keyCode = Win32InputCodeTable::GetKeyFromScanCode(scancode);
				bool			isRepeat = (lParam & REPEAT_KEY_MASK);

				for (IEventHandler* pEventHandler : m_EventHandlers)
				{
					pEventHandler->KeyPressed(keyCode, modifierMask, isRepeat);
				}

				break;
			}

			case WM_KEYUP:
			case WM_SYSKEYUP:
			{
				const uint16	scancode = HIWORD(lParam) & SCAN_CODE_MASK;
				EKey			keyCode = Win32InputCodeTable::GetKeyFromScanCode(scancode);

				for (IEventHandler* pEventHandler : m_EventHandlers)
				{
					pEventHandler->KeyReleased(keyCode);
				}

				break;
			}

			case WM_CHAR:
			case WM_SYSCHAR:
			{
				const uint32 character = uint32(wParam);
				for (IEventHandler* pEventHandler : m_EventHandlers)
				{
					pEventHandler->KeyTyped(character);
				}

				break;
			}

			case WM_MOUSEMOVE:
			{
				const int32 x = GET_X_LPARAM(lParam);
				const int32 y = GET_Y_LPARAM(lParam);

				for (IEventHandler* pEventHandler : m_EventHandlers)
				{
					pEventHandler->MouseMoved(x, y);
				}

				if (!m_IsTrackingMouse)
				{
					m_IsTrackingMouse = true;

					TRACKMOUSEEVENT tme = { sizeof(tme) };
					tme.dwFlags = TME_LEAVE;
					tme.hwndTrack = hWnd;
					TrackMouseEvent(&tme);

					for (IEventHandler* pEventHandler : m_EventHandlers)
					{
						pEventHandler->MouseEntered(pMessageWindow);
					}
				}

				break;
			}

			case WM_LBUTTONDOWN:
			case WM_MBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_XBUTTONDOWN:
			{
				EMouseButton button = EMouseButton::MOUSE_BUTTON_UNKNOWN;
				if (uMessage == WM_LBUTTONDOWN)
				{
					button = EMouseButton::MOUSE_BUTTON_LEFT;
				}
				else if (uMessage == WM_MBUTTONDOWN)
				{
					button = EMouseButton::MOUSE_BUTTON_MIDDLE;
				}
				else if (uMessage == WM_RBUTTONDOWN)
				{
					button = EMouseButton::MOUSE_BUTTON_RIGHT;
				}
				else if (GET_XBUTTON_WPARAM(wParam) == BACK_BUTTON_MASK)
				{
					button = EMouseButton::MOUSE_BUTTON_BACK;
				}
				else
				{
					button = EMouseButton::MOUSE_BUTTON_FORWARD;
				}

				const uint32 modifierMask = Win32InputCodeTable::GetModifierMask();
				for (IEventHandler* pEventHandler : m_EventHandlers)
				{
					pEventHandler->ButtonPressed(button, modifierMask);
				}

				break;
			}

			case WM_LBUTTONUP:
			case WM_MBUTTONUP:
			case WM_RBUTTONUP:
			case WM_XBUTTONUP:
			{
				EMouseButton button = EMouseButton::MOUSE_BUTTON_UNKNOWN;
				if (uMessage == WM_LBUTTONUP)
				{
					button = EMouseButton::MOUSE_BUTTON_LEFT;
				}
				else if (uMessage == WM_MBUTTONUP)
				{
					button = EMouseButton::MOUSE_BUTTON_MIDDLE;
				}
				else if (uMessage == WM_RBUTTONUP)
				{
					button = EMouseButton::MOUSE_BUTTON_RIGHT;
				}
				else if (GET_XBUTTON_WPARAM(wParam) == BACK_BUTTON_MASK)
				{
					button = EMouseButton::MOUSE_BUTTON_BACK;
				}
				else
				{
					button = EMouseButton::MOUSE_BUTTON_FORWARD;
				}

				for (IEventHandler* pEventHandler : m_EventHandlers)
				{
					pEventHandler->ButtonReleased(button);
				}

				break;
			}

			case WM_MOUSEWHEEL:
			{
				const int32 scrollDelta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
				for (IEventHandler* pEventHandler : m_EventHandlers)
				{
					pEventHandler->MouseScrolled(0, scrollDelta);
				}

				break;
			}

			case WM_MOUSEHWHEEL:
			{
				const int32 scrollDelta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
				for (IEventHandler* pEventHandler : m_EventHandlers)
				{
					pEventHandler->MouseScrolled(scrollDelta, 0);
				}

				break;
			}

			case WM_MOVE:
			{
				const int16 x = (int16)LOWORD(lParam);
				const int16 y = (int16)HIWORD(lParam);

				for (IEventHandler* pEventHandler : m_EventHandlers)
				{
					pEventHandler->WindowMoved(pMessageWindow, x, y);
				}

				break;
			}

			case WM_INPUT:
			{
				ProcessRawInput(wParam, lParam);
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

				for (IEventHandler* pEventHandler : m_EventHandlers)
				{
					pEventHandler->WindowResized(pMessageWindow, width, height, resizeType);
				}

				break;
			}

			case WM_MOUSELEAVE:
			{
				for (IEventHandler* pEventHandler : m_EventHandlers)
				{
					pEventHandler->MouseLeft(pMessageWindow);
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

				for (IEventHandler* pEventHandler : m_EventHandlers)
				{
					pEventHandler->WindowClosed(pMessageWindow);
				}

				break;
			}

			case WM_SETFOCUS:
			case WM_KILLFOCUS:
			{
				bool hasFocus = (uMessage == WM_SETFOCUS);

				for (IEventHandler* pEventHandler : m_EventHandlers)
				{
					pEventHandler->FocusChanged(pMessageWindow, hasFocus);
				}

				break;
			}
		}

		for (IWin32MessageHandler* pHandler : m_MessageHandlers)
		{
			pHandler->MessageProc(hWnd, uMessage, wParam, lParam);
		}
	}

	void Win32Application::ProcessRawInput(WPARAM wParam, LPARAM lParam)
	{
		UINT	dwSize	= 0;
		LPBYTE	pBytes = nullptr;
		::GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
		if (pBytes == NULL)
		{
			LOG_ERROR("[Win32Application]: Failed to read raw input data");
			return;
		}
		else
		{
			pBytes = DBG_NEW BYTE[dwSize];
		}

		if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, pBytes, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
		{
			LOG_ERROR("[Win32Application]: GetRawInputData does not return correct size");
			return;
		}

		RAWINPUT* pRaw = (RAWINPUT*)pBytes;
		if (pRaw->header.dwType == RIM_TYPEKEYBOARD)
		{
			pRaw->data.keyboard.VKey;

			const uint32	modifierMask = Win32InputCodeTable::GetModifierMask();
			EKey			keyCode = Win32InputCodeTable::GetKeyFromScanCode(pRaw->data.keyboard.MakeCode);
			//bool			isRepeat = (lParam & REPEAT_KEY_MASK);

			for (IEventHandler* pEventHandler : m_EventHandlers)
			{
				pEventHandler->KeyPressed(keyCode, modifierMask, false);
			}
		}
		else if (pRaw->header.dwType == RIM_TYPEMOUSE)
		{

		}

		SAFEDELETE_ARRAY(pBytes);
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

		if (!RegisterWindowClass())
		{
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

	bool Win32Application::RegisterWindowClass()
	{
		WNDCLASS wc = { };
		ZERO_MEMORY(&wc, sizeof(WNDCLASS));
		wc.hInstance		= Win32Application::Get()->GetInstanceHandle();
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
		else
		{
			return true;
		}
	}

	bool Win32Application::RegisterRawInputDevices()
	{
		RAWINPUTDEVICE devices[2];
		ZERO_MEMORY(devices, sizeof(RAWINPUTDEVICE) * 2);

		// Keyboard
		devices[0].dwFlags		= RIDEV_NOLEGACY;
		devices[0].hwndTarget	= 0;
		devices[0].usUsage		= 0x06;
		devices[0].usUsagePage	= 0x01;

		// Mouse
		devices[1].dwFlags		= 0;
		devices[1].hwndTarget	= 0;
		devices[1].usUsage		= 0x02;
		devices[1].usUsagePage	= 0x01;

		BOOL bResult = ::RegisterRawInputDevices(devices, 2, sizeof(RAWINPUTDEVICE));
		if (bResult == FALSE)
		{
			LOG_ERROR("[Win32Application]: Failed to register raw input devices");
			return false;
		}
		else
		{
			D_LOG_MESSAGE("[Win32Application]: Registered keyboard and mouse devices");
			return true;
		}
	}

	bool Win32Application::UnregisterRawInputDevices()
	{
		RAWINPUTDEVICE devices[2];
		ZERO_MEMORY(devices, sizeof(RAWINPUTDEVICE) * 2);

		// Keyboard
		devices[0].dwFlags		= RIDEV_REMOVE;
		devices[0].hwndTarget	= 0;
		devices[0].usUsage		= 0x06;
		devices[0].usUsagePage	= 0x01;

		// Mouse
		devices[1].dwFlags		= RIDEV_REMOVE;
		devices[1].hwndTarget	= 0;
		devices[1].usUsage		= 0x02;
		devices[1].usUsagePage	= 0x01;

		BOOL bResult = ::RegisterRawInputDevices(devices, 2, sizeof(RAWINPUTDEVICE));
		if (bResult == FALSE)
		{
			LOG_ERROR("[Win32Application]: Failed to unregister raw input devices");
			return false;
		}
		else
		{
			D_LOG_MESSAGE("[Win32Application]: Unregistered keyboard and mouse devices");
			return true;
		}
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
