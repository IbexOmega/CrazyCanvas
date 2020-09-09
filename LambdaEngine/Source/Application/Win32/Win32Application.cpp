#ifdef LAMBDA_PLATFORM_WINDOWS
#include <stdlib.h>

#include "Log/Log.h"

#include "Application/API/EventHandler.h"

#include "Application/Win32/Win32Application.h"
#include "Application/Win32/Win32Window.h"
#include "Application/Win32/IWin32MessageHandler.h"

#include "Input/Win32/Win32InputCodeTable.h"

#include <windowsx.h>

namespace LambdaEngine
{
	Win32Application* Win32Application::s_pApplication = nullptr;

	/*
	* Win32Application
	*/
	Win32Application::Win32Application(HINSTANCE hInstance)
		: m_hInstance(hInstance)
		, m_Windows()
		, m_StoredMessages()
		, m_MessageHandlers()
	{
		VALIDATE_MSG(s_pApplication == nullptr, "[Win32Application]: An instance of application already exists");
		s_pApplication = this;

		VALIDATE(hInstance != NULL);
	}

	Win32Application::~Win32Application()
	{
		// Unregister window class after destroying all windows
		if (!::UnregisterClass(WINDOW_CLASS, m_hInstance))
		{
			LOG_ERROR("[Win32Application]: Failed to unregister windowclass");
		}

		// Destroy application
		VALIDATE_MSG(s_pApplication != nullptr, "[Win32Application]: Instance of application has already been deleted");
		s_pApplication = nullptr;

		m_hInstance = 0;
	}

	void Win32Application::AddWin32MessageHandler(IWin32MessageHandler* pHandler)
	{
		// Check first so that this handler is not already added
		const uint32 count = uint32(m_MessageHandlers.GetSize());
		for (uint32 i = 0; i < count; i++)
		{
			if (pHandler == m_MessageHandlers[i])
			{
				return;
			}
		}

		// Add new handler
		m_MessageHandlers.EmplaceBack(pHandler);
	}

	void Win32Application::RemoveWin32MessageHandler(IWin32MessageHandler* pHandler)
	{
		const uint32 count = uint32(m_MessageHandlers.GetSize());
		for (uint32 i = 0; i < count; i++)
		{
			if (pHandler == m_MessageHandlers[i])
			{
				m_MessageHandlers.Erase(m_MessageHandlers.begin() + i);
				break;
			}
		}
	}

	bool Win32Application::Create()
	{
		if (!RegisterWindowClass())
		{
			return false;
		}

		if (!Win32InputCodeTable::Init())
		{
			return false;
		}

		return true;
	}

	bool Win32Application::ProcessStoredEvents()
	{
		TArray<Win32Message> messagesToProcess = m_StoredMessages;
		m_StoredMessages.Clear();

		bool shouldRun = true;
		for (Win32Message& message : messagesToProcess)
		{
			if (message.uMessage == WM_QUIT)
			{
				shouldRun = false;
			}
			else
			{
				ProcessStoredMessage(message.hWnd, message.uMessage, message.wParam, message.lParam, message.RawInput.MouseDeltaX, message.RawInput.MouseDeltaY);
			}
		}

		return shouldRun;
	}

	bool Win32Application::SupportsRawInput() const
	{
		return true;
	}

	void Win32Application::SetMouseVisibility(bool visible)
	{
		int level = ShowCursor(visible);
		if (visible && level > 1)
		{
			ShowCursor(FALSE);
		}
		else if (!visible && level < -1)
		{
			ShowCursor(TRUE);
		}
	}

	void Win32Application::SetMousePosition(int32 x, int32 y)
	{
		// Sets mouse position relative to the active window, if there are no active window this function does nothing
		TSharedRef<Window> window = GetActiveWindow();
		if (window)
		{
			HWND hWnd = static_cast<HWND>(window->GetHandle());
			if (::IsWindow(hWnd))
			{
				POINT point = { };
				point.x = x;
				point.y = y;

				ClientToScreen(hWnd, &point);
				
				BOOL bResult = SetCursorPos(point.x, point.y);
				if (!bResult)
				{
					LOG_ERROR("[Win32Application]: Failed to set mouse position!");
				}
			}
		}
	}

	void Win32Application::SetInputMode(TSharedRef<Window> window, EInputMode inputMode)
	{
		VALIDATE(window != nullptr);

		if (inputMode == m_InputMode || inputMode == EInputMode::INPUT_MODE_NONE)
		{
			return;
		}

		if (inputMode == EInputMode::INPUT_MODE_RAW)
		{
			HWND hWnd = (HWND)window->GetHandle();
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

	EInputMode Win32Application::GetInputMode(TSharedRef<Window> window) const
	{
		UNREFERENCED_VARIABLE(window);

		// TODO: Return proper inputmode based on window
		return m_InputMode;
	}

	void Win32Application::ProcessStoredMessage(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam, int32 mouseDeltaX, int32 mouseDeltaY)
	{
		constexpr uint16 SCAN_CODE_MASK		= 0x01ff;
		constexpr uint32 REPEAT_KEY_MASK	= 0x40000000;
		constexpr uint16 BACK_BUTTON_MASK	= 0x0001;

		TSharedRef<Win32Window> messageWindow = GetWindowFromHandle(hWnd);
		switch (uMessage)
		{
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
			{
				const ModifierKeyState modifierState = ModifierKeyState(Win32InputCodeTable::GetModifierMask());
				const uint16 scancode = HIWORD(lParam) & SCAN_CODE_MASK;
				EKey keyCode = Win32InputCodeTable::GetKeyFromScanCode(scancode);
				bool isRepeat = (lParam & REPEAT_KEY_MASK);

				m_EventHandler->OnKeyPressed(keyCode, modifierState, isRepeat);
				break;
			}

			case WM_KEYUP:
			case WM_SYSKEYUP:
			{
				const uint16	scancode	= HIWORD(lParam) & SCAN_CODE_MASK;
				EKey			keyCode		= Win32InputCodeTable::GetKeyFromScanCode(scancode);

				m_EventHandler->OnKeyReleased(keyCode);
				break;
			}

			case WM_CHAR:
			case WM_SYSCHAR:
			{
				const uint32 character = uint32(wParam);
				m_EventHandler->OnKeyTyped(character);
				break;
			}

			case WM_MOUSEMOVE:
			{
				// Prevent double messages of mouse moved for main windows
				const int32 x = GET_X_LPARAM(lParam);
				const int32 y = GET_Y_LPARAM(lParam);

				m_EventHandler->OnMouseMoved(x, y);

				// Mouse must have entered the window
				if (!m_IsTrackingMouse)
				{
					m_IsTrackingMouse = true;

					TRACKMOUSEEVENT tme = { sizeof(tme) };
					tme.dwFlags		= TME_LEAVE;
					tme.hwndTrack	= hWnd;
					TrackMouseEvent(&tme);

					m_EventHandler->OnMouseEntered(messageWindow);
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

				const ModifierKeyState modifierState = ModifierKeyState(Win32InputCodeTable::GetModifierMask());
				m_EventHandler->OnButtonPressed(button, modifierState);
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

				m_EventHandler->OnButtonReleased(button);
				break;
			}

			case WM_MOUSEWHEEL:
			{
				const int32 scrollDelta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
				m_EventHandler->OnMouseScrolled(0, scrollDelta);

				break;
			}

			case WM_MOUSEHWHEEL:
			{
				const int32 scrollDelta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
				m_EventHandler->OnMouseScrolled(scrollDelta, 0);
				break;
			}

			case WM_MOVE:
			{
				const int16 x = (int16)LOWORD(lParam);
				const int16 y = (int16)HIWORD(lParam);

				m_EventHandler->OnWindowMoved(messageWindow, x, y);
				break;
			}

			case WM_INPUT:
			{
				m_EventHandler->OnMouseMovedRaw(mouseDeltaX, mouseDeltaY);
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

				m_EventHandler->OnWindowResized(messageWindow, width, height, resizeType);
				break;
			}

			case WM_MOUSELEAVE:
			{
				m_EventHandler->OnMouseLeft(messageWindow);
				m_IsTrackingMouse = false;
				break;
			}

			case WM_DESTROY:
			{
				m_EventHandler->OnWindowClosed(messageWindow);
				break;
			}

			case WM_SETFOCUS:
			case WM_KILLFOCUS:
			{
				bool hasFocus = (uMessage == WM_SETFOCUS);
				m_EventHandler->OnFocusChanged(messageWindow, hasFocus);
				break;
			}
		}
	}

	LRESULT Win32Application::ProcessRawInput(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
	{
		UINT dwSize = 0;
		::GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
		if (dwSize > m_RawInputBuffer.GetSize())
		{
			m_RawInputBuffer.Resize(dwSize);
		}

		if (::GetRawInputData((HRAWINPUT)lParam, RID_INPUT, m_RawInputBuffer.GetData(), &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
		{
			LOG_ERROR("[Win32Application]: GetRawInputData did not return correct size");
			return 0;
		}

		RAWINPUT* pRaw = reinterpret_cast<RAWINPUT*>(m_RawInputBuffer.GetData());
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

	TSharedRef<Win32Window> Win32Application::GetWindowFromHandle(HWND hWnd) const
	{
		for (const TSharedRef<Win32Window>& window : m_Windows)
		{
			HWND handle = (HWND)window->GetHandle();
			if (handle == hWnd)
			{
				return window;
			}
		}

		return nullptr;
	}

	HINSTANCE Win32Application::GetInstanceHandle()
	{
		return m_hInstance;
	}

	void Win32Application::SetActiveWindow(TSharedRef<Window> window)
	{
		::HWND hActiveWindow = static_cast<HWND>(window->GetHandle());
		::BOOL bResult = ::SetForegroundWindow(hActiveWindow);
		if (!bResult)
		{
			LOG_ERROR("[Win32Application]: Failed to set active window");
		}
	}

	TSharedRef<Window> Win32Application::GetActiveWindow() const
	{
		::HWND hForegroundWindow = ::GetActiveWindow();
		if (hForegroundWindow)
		{
			return GetWindowFromHandle(hForegroundWindow);
		}
		else
		{
			return TSharedRef<Window>();
		}
	}

	void Win32Application::SetCapture(TSharedRef<Window> window)
	{
		if (window)
		{
			VALIDATE(window != nullptr);

			::HWND hCaptureWindow = static_cast<HWND>(window->GetHandle());
			if (::IsWindow(hCaptureWindow))
			{
				::SetCapture(hCaptureWindow);
			}
		}
		else
		{
			::ReleaseCapture();
		}
	}

	TSharedRef<Window> Win32Application::GetCapture() const
	{
		::HWND hCaptureWindow = ::GetCapture();
		if (hCaptureWindow)
		{
			TSharedRef<Window> window = GetWindowFromHandle(hCaptureWindow);
			VALIDATE(window != nullptr);
			return window;
		}
		else
		{
			return TSharedRef<Window>();
		}
	}

	ModifierKeyState Win32Application::GetModiferKeyState() const
	{
		return ModifierKeyState();
	}

	void Win32Application::AddWindow(TSharedRef<Win32Window> window)
	{
		m_Windows.EmplaceBack(window);
	}

	void Win32Application::PeekEvents()
	{
		::MSG msg = { };
		while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}

	bool Win32Application::RegisterWindowClass()
	{
		::WNDCLASS wc = { };
		ZERO_MEMORY(&wc, sizeof(::WNDCLASS));

		wc.hInstance		= Win32Application::Get().GetInstanceHandle();
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
		::RAWINPUTDEVICE devices[DEVICE_COUNT];
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
		::RAWINPUTDEVICE devices[DEVICE_COUNT];
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

	TSharedRef<Window> Win32Application::CreateWindow(const WindowDesc* pDesc)
	{
		TSharedRef<Win32Window> window = DBG_NEW Win32Window();
		if (window->Init(pDesc))
		{
			AddWindow(window);
			return window;
		}
		else
		{
			return nullptr;
		}
	}

	bool Win32Application::Tick()
	{
		return ProcessStoredEvents();
	}

	Application* Win32Application::CreateApplication()
	{
		HINSTANCE hInstance = static_cast<::HINSTANCE>(GetModuleHandle(0));
		return DBG_NEW Win32Application(hInstance);
	}

	void Win32Application::Terminate()
	{
		// TODO: Maybe take in the exitcode

		// HACK: Do this for now
		StoreMessage(0, WM_QUIT, 0, 0, 0, 0);
	}

	Win32Application& Win32Application::Get()
	{
		VALIDATE(s_pApplication != nullptr);
		return *s_pApplication;
	}

	LRESULT Win32Application::WindowProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
	{
		return Win32Application::Get().ProcessMessage(hWnd, uMessage, wParam, lParam);
	}

	LRESULT Win32Application::ProcessMessage(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
	{
		// Let other modules handle messages
		bool	messageHandled	= false;
		::LRESULT result			= 0;
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
		m_StoredMessages.PushBack({ hWnd, uMessage, wParam, lParam, mouseDeltaX, mouseDeltaY });
	}
}

#endif
