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
			ProcessStoredMessage(message.hWnd, message.uMessage, message.wParam, message.lParam, message.RawInput.MouseDeltaX, message.RawInput.MouseDeltaY);
		}
	}

	void Win32Application::MakeMainWindow(IWindow* pMainWindow)
	{
		m_pMainWindow = reinterpret_cast<Win32Window*>(pMainWindow);

		if (m_InputMode == EInputMode::INPUT_MODE_RAW)
		{
			HWND hwnd = (HWND)m_pMainWindow->GetHandle();
			RegisterRawInputDevices(hwnd);
		}
	}

	void Win32Application::SetInputMode(EInputMode inputMode)
	{
		if (inputMode == m_InputMode || inputMode == EInputMode::INPUT_MODE_NONE)
		{
			return;
		}

		if (inputMode == EInputMode::INPUT_MODE_RAW)
		{
			// Get the cursor position
			POINT cursorPos = { };
			::GetCursorPos(&cursorPos);

			m_RawInput.MouseX = cursorPos.x;
			m_RawInput.MouseY = cursorPos.y;

			// Register input devices for the main window
			HWND hWnd = (HWND)m_pMainWindow->GetHandle();
			RegisterRawInputDevices(hWnd);
		}
		else if (inputMode == EInputMode::INPUT_MODE_STANDARD)
		{
			if (m_InputMode == EInputMode::INPUT_MODE_RAW)
			{
				UnregisterRawInputDevices();
			}
		}
		
		m_InputMode = inputMode;
	}

	EInputMode Win32Application::GetInputMode() const
	{
		return m_InputMode;
	}

	void Win32Application::ProcessStoredMessage(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam, int32 mouseDeltaX, int32 mouseDeltaY)
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
				const uint32	modifierMask	= Win32InputCodeTable::GetModifierMask();
				const uint16	scancode		= HIWORD(lParam) & SCAN_CODE_MASK;
				EKey			keyCode			= Win32InputCodeTable::GetKeyFromScanCode(scancode);
				bool			isRepeat		= (lParam & REPEAT_KEY_MASK);

				for (IEventHandler* pEventHandler : m_EventHandlers)
				{
					pEventHandler->KeyPressed(keyCode, modifierMask, isRepeat);
				}

				break;
			}

			case WM_KEYUP:
			case WM_SYSKEYUP:
			{
				const uint16	scancode	= HIWORD(lParam) & SCAN_CODE_MASK;
				EKey			keyCode		= Win32InputCodeTable::GetKeyFromScanCode(scancode);

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
				// Prevent double messages of mouse moved for main windows
				if (pMessageWindow == m_pMainWindow)
				{
					if (m_InputMode == EInputMode::INPUT_MODE_STANDARD)
					{
						const int32 x = GET_X_LPARAM(lParam);
						const int32 y = GET_Y_LPARAM(lParam);

						for (IEventHandler* pEventHandler : m_EventHandlers)
						{
							pEventHandler->MouseMoved(x, y);
						}
					}
				}

				// Mouse must have entered the window
				if (!m_IsTrackingMouse)
				{
					m_IsTrackingMouse = true;

					TRACKMOUSEEVENT tme = { sizeof(tme) };
					tme.dwFlags		= TME_LEAVE;
					tme.hwndTrack	= hWnd;
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
				m_RawInput.MouseX += mouseDeltaX;
				m_RawInput.MouseY += mouseDeltaY;

				for (IEventHandler* pEventHandler : m_EventHandlers)
				{
					pEventHandler->MouseMoved(m_RawInput.MouseX, m_RawInput.MouseY);
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
	}

	LRESULT Win32Application::ProcessRawInput(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
	{
		UINT dwSize = 0;
		::GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
		if (dwSize > m_RawInput.BufferSize)
		{
			m_RawInput.BufferSize	= dwSize;
			m_RawInput.pInputBuffer = DBG_NEW byte[dwSize];
		}

		if (!m_RawInput.pInputBuffer)
		{
			LOG_ERROR("[Win32Application]: Failed to allocate Raw Input buffer");
			return 0;
		}

		if (::GetRawInputData((HRAWINPUT)lParam, RID_INPUT, m_RawInput.pInputBuffer, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
		{
			LOG_ERROR("[Win32Application]: GetRawInputData did not return correct size");
			return 0;
		}

		RAWINPUT* pRaw = (RAWINPUT*)m_RawInput.pInputBuffer;
		if (pRaw->header.dwType == RIM_TYPEMOUSE)
		{
			const int32 deltaX = pRaw->data.mouse.lLastX;
			const int32 deltaY = pRaw->data.mouse.lLastY;

			if (deltaX != 0 && deltaY != 0)
			{
				StoreMessage(hWnd, uMessage, wParam, lParam, deltaX, deltaY);
			}

			return 0;
		}
		else
		{
			return ::DefRawInputProc(&pRaw, 1, sizeof(RAWINPUTHEADER));
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
		return m_hInstance;
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

	bool Win32Application::PreInit(HINSTANCE hInstance)
	{
		VALIDATE(hInstance != NULL);

		Win32Application* pApplication = DBG_NEW Win32Application(hInstance);
		pApplication->SetInputMode(EInputMode::INPUT_MODE_STANDARD);

		if (!RegisterWindowClass())
		{
			return false;
		}

		if (!Win32InputCodeTable::Init())
		{
			return false;
		}

		Win32Window* pWindow = (Win32Window*)Win32Application::CreateWindow("Lambda Game Engine", 1920, 1080);
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

	bool Win32Application::RegisterRawInputDevices(HWND hWnd)
	{
		constexpr uint32 DEVICE_COUNT = 1;
		RAWINPUTDEVICE devices[DEVICE_COUNT];
		ZERO_MEMORY(devices, sizeof(RAWINPUTDEVICE) * DEVICE_COUNT);

		// Mouse
		devices[0].dwFlags		= 0;
		devices[0].hwndTarget	= hWnd;
		devices[0].usUsage		= 0x02;
		devices[0].usUsagePage	= 0x01;

		BOOL bResult = ::RegisterRawInputDevices(devices, DEVICE_COUNT, sizeof(RAWINPUTDEVICE));
		if (bResult == FALSE)
		{
			LOG_ERROR("[Win32Application]: Failed to register Raw Input devices");
			return false;
		}
		else
		{
			D_LOG_MESSAGE("[Win32Application]: Registered Raw Input devices");
			return true;
		}
	}

	bool Win32Application::UnregisterRawInputDevices()
	{
		constexpr uint32 DEVICE_COUNT = 1;
		RAWINPUTDEVICE devices[DEVICE_COUNT];
		ZERO_MEMORY(devices, sizeof(RAWINPUTDEVICE) * DEVICE_COUNT);

		// Mouse
		devices[0].dwFlags		= RIDEV_REMOVE;
		devices[0].hwndTarget	= 0;
		devices[0].usUsage		= 0x02;
		devices[0].usUsagePage	= 0x01;

		BOOL bResult = ::RegisterRawInputDevices(devices, DEVICE_COUNT, sizeof(RAWINPUTDEVICE));
		if (bResult == FALSE)
		{
			LOG_ERROR("[Win32Application]: Failed to unregister Raw Input devices");
			return false;
		}
		else
		{
			D_LOG_MESSAGE("[Win32Application]: Unregistered Raw Input devices");
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

	Win32Application* Win32Application::Get()
	{
		return s_pApplication;
	}

	LRESULT Win32Application::WindowProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
	{
		VALIDATE(Win32Application::Get() != nullptr);
		return Win32Application::Get()->ProcessMessage(hWnd, uMessage, wParam, lParam);
	}

	LRESULT Win32Application::ProcessMessage(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
	{
		// Let other modules handle messages
		bool	messageHandled	= false;
		LRESULT result			= 0;
		for (IWin32MessageHandler* pMessageHandler : m_MessageHandlers)
		{
			result = pMessageHandler->ProcessMessage(hWnd, uMessage, wParam, lParam);
			if (result == 0)
			{
				messageHandled = true;
			}
		}

		// Process messages
		switch (uMessage)
		{
			case WM_INPUT:
			{
				return ProcessRawInput(hWnd, uMessage, wParam, lParam);
			}

			case WM_MOUSEMOVE:
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
			case WM_KEYUP:
			case WM_SYSKEYUP:
			case WM_CHAR:
			case WM_SYSCHAR:
			case WM_LBUTTONDOWN:
			case WM_MBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_XBUTTONDOWN:
			case WM_LBUTTONUP:
			case WM_MBUTTONUP:
			case WM_RBUTTONUP:
			case WM_XBUTTONUP:
			case WM_MOUSEWHEEL:
			case WM_MOUSEHWHEEL:
			case WM_MOVE:
			case WM_SIZE:
			case WM_MOUSELEAVE:
			case WM_DESTROY:
			case WM_SETFOCUS:
			case WM_KILLFOCUS:
			{
				StoreMessage(hWnd, uMessage, wParam, lParam, 0, 0);
				return 0;
			}
		}

		// Return the default or result from MessageHandler
		if (!messageHandled)
		{
			return ::DefWindowProc(hWnd, uMessage, wParam, lParam);
		}
		else
		{
			return result;
		}
	}

	void Win32Application::StoreMessage(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam, int32 mouseDeltaX, int32 mouseDeltaY)
	{
		m_StoredMessages.push_back({ hWnd, uMessage, wParam, lParam, mouseDeltaX, mouseDeltaY });
	}
}

#endif
