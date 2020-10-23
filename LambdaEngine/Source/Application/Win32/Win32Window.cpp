#ifdef LAMBDA_PLATFORM_WINDOWS
#include <stdlib.h>

#include "Log/Log.h"

#include "Application/Win32/Win32Window.h"
#include "Application/Win32/Win32Application.h"

namespace LambdaEngine
{
	/*
	* Win32Window
	*/

	Win32Window::~Win32Window()
	{
		if (::IsWindow(m_hWnd))
		{
			::DestroyWindow(m_hWnd);
		}
	}

	bool Win32Window::Init(const WindowDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		DWORD dwStyle = 0; 
		if (pDesc->Style != 0)
		{
			dwStyle = WS_OVERLAPPED;
			if (pDesc->Style & WINDOW_STYLE_FLAG_TITLED)
			{
				dwStyle |= WS_CAPTION;
			}
			if (pDesc->Style & WINDOW_STYLE_FLAG_CLOSABLE)
			{
				dwStyle |= WS_SYSMENU;
			}
			if (pDesc->Style & WINDOW_STYLE_FLAG_MINIMIZABLE)
			{
				dwStyle |= WS_SYSMENU | WS_MINIMIZEBOX;
			}
			if (pDesc->Style & WINDOW_STYLE_FLAG_MAXIMIZABLE)
			{
				dwStyle |= WS_SYSMENU | WS_MAXIMIZEBOX;
			}
			if (pDesc->Style & WINDOW_STYLE_FLAG_RESIZEABLE)
			{
				dwStyle |= WS_THICKFRAME;
			}
		}
		else
		{
			dwStyle = WS_POPUP;
		}

		RECT clientRect = { 0, 0, LONG(pDesc->Width), LONG(pDesc->Height) };
		::AdjustWindowRect(&clientRect, dwStyle, FALSE);

		INT nWidth	= clientRect.right - clientRect.left;
		INT nHeight = clientRect.bottom - clientRect.top;

		constexpr uint32 MAX_CHARS = 256;
		static wchar_t title[MAX_CHARS];

		size_t charsWritten = mbstowcs(title, pDesc->Title.c_str(), MAX_CHARS);
		if (charsWritten != static_cast<size_t>(-1))
		{
			title[charsWritten] = L'\0';
		}

		HINSTANCE hInstance = PlatformApplication::Get().GetInstanceHandle();
		m_hWnd = ::CreateWindowEx(0, WINDOW_CLASS, title, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, nWidth, nHeight, NULL, NULL, hInstance, NULL);
		if (m_hWnd == NULL)
		{
			LOG_ERROR("[Win32Window]: Failed to create window");
			return false;
		}
		else
		{
			// If the window has a sysmenu we check if the closebutton should be active
			if (dwStyle & WS_SYSMENU)
			{
				if (!(pDesc->Style & WINDOW_STYLE_FLAG_CLOSABLE))
				{
					::EnableMenuItem(::GetSystemMenu(m_hWnd, FALSE), SC_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
				}
			}

			// Set descripton
			m_Desc = (*pDesc);

			m_dwWindowStyle = dwStyle;

			::UpdateWindow(m_hWnd);
			return true;
		}
	}

	void Win32Window::Show()
	{
		VALIDATE(m_hWnd != 0);

		if (::IsWindow(m_hWnd))
		{
			::ShowWindow(m_hWnd, SW_NORMAL);
		}
	}

	void Win32Window::Close()
	{
		if (m_Desc.Style & WINDOW_STYLE_FLAG_CLOSABLE)
		{
			VALIDATE(m_hWnd != 0);

			if (::IsWindow(m_hWnd))
			{
				::DestroyWindow(m_hWnd);
			}
		}
	}

	void Win32Window::Minimize()
	{
		if (m_Desc.Style & WINDOW_STYLE_FLAG_MINIMIZABLE)
		{
			VALIDATE(m_hWnd != 0);

			if (::IsWindow(m_hWnd))
			{
				::ShowWindow(m_hWnd, SW_MINIMIZE);
			}
		}
	}

	void Win32Window::Maximize()
	{
		if (m_Desc.Style & WINDOW_STYLE_FLAG_MAXIMIZABLE)
		{
			VALIDATE(m_hWnd != 0);

			if (::IsWindow(m_hWnd))
			{
				::ShowWindow(m_hWnd, SW_MAXIMIZE);
			}
		}
	}

	bool Win32Window::IsActiveWindow() const
	{
		HWND hActiveWindow = ::GetActiveWindow();
		return (m_hWnd == hActiveWindow);
	}

	void Win32Window::Restore()
	{
		VALIDATE(m_hWnd != 0);

		if (::IsWindow(m_hWnd))
		{
			BOOL result = ::IsIconic(m_hWnd);
			if (result)
			{
				::ShowWindow(m_hWnd, SW_RESTORE);
			}
		}
	}

	void Win32Window::ToggleFullscreen()
	{
		VALIDATE(m_hWnd != 0);

		if (::IsWindow(m_hWnd))
		{
			if (!m_IsFullscreen)
			{
				m_IsFullscreen = true;

				::GetWindowPlacement(m_hWnd, &m_WindowPlaceMent);
				if (m_dwWindowStyle == 0)
				{
					m_dwWindowStyle = ::GetWindowLong(m_hWnd, GWL_STYLE);
				}
				if (m_dwWindowStyleEx == 0)
				{
					m_dwWindowStyleEx = ::GetWindowLong(m_hWnd, GWL_EXSTYLE);
				}

				LONG newStyle = m_dwWindowStyle;
				newStyle &= ~WS_BORDER;
				newStyle &= ~WS_DLGFRAME;
				newStyle &= ~WS_THICKFRAME;

				LONG newStyleEx = m_dwWindowStyleEx;
				newStyleEx &= ~WS_EX_WINDOWEDGE;

				SetWindowLong(m_hWnd, GWL_STYLE, newStyle | WS_POPUP);
				SetWindowLong(m_hWnd, GWL_EXSTYLE, newStyleEx | WS_EX_TOPMOST);
				::ShowWindow(m_hWnd, SW_SHOWMAXIMIZED);
			}
			else
			{
				m_IsFullscreen = false;

				::SetWindowLong(m_hWnd, GWL_STYLE, m_dwWindowStyle);
				::SetWindowLong(m_hWnd, GWL_EXSTYLE, m_dwWindowStyleEx);
				::ShowWindow(m_hWnd, SW_SHOWNORMAL);
				::SetWindowPlacement(m_hWnd, &m_WindowPlaceMent);
			}
		}
	}

	uint16 Win32Window::GetWidth() const
	{
		VALIDATE(m_hWnd != 0);

		if (::IsWindow(m_hWnd))
		{
			RECT rect = { };
			::GetClientRect(m_hWnd, &rect);
			return uint16(rect.right - rect.left);
		}
		else
		{
			return 0;
		}
	}

	uint16 Win32Window::GetHeight() const
	{
		VALIDATE(m_hWnd != 0);

		if (::IsWindow(m_hWnd))
		{
			RECT rect = { };
			::GetClientRect(m_hWnd, &rect);
			return uint16(rect.bottom - rect.top);
		}
		else
		{
			return 0;
		}
	}

	void* Win32Window::GetHandle() const
	{
		return reinterpret_cast<void*>(m_hWnd);
	}

	bool Win32Window::IsValid() const
	{
		return ::IsWindow(m_hWnd);
	}

	void Win32Window::SetTitle(const String& title)
	{
		VALIDATE(m_hWnd != 0);

		if (::IsWindow(m_hWnd))
		{
			::SetWindowTextA(m_hWnd, title.c_str());
		}
	}
	
	void Win32Window::SetPosition(int32 x, int32 y)
	{
		VALIDATE(m_hWnd != 0);

		if (::IsWindow(m_hWnd))
		{
			::SetWindowPos(m_hWnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
		}
	}
	
	void Win32Window::GetPosition(int32* pPosX, int32* pPosY) const
	{
		VALIDATE(m_hWnd != 0);
		VALIDATE(pPosX	!= nullptr);
		VALIDATE(pPosY	!= nullptr);

		if (::IsWindow(m_hWnd))
		{
			WINDOWPLACEMENT placement = { };
			placement.length = sizeof(WINDOWPLACEMENT);

			if (::GetWindowPlacement(m_hWnd, &placement))
			{
				(*pPosX) = placement.rcNormalPosition.left;
				(*pPosY) = placement.rcNormalPosition.bottom;
			}
		}
		else
		{
			(*pPosX) = 0;
			(*pPosY) = 0;
		}
	}
	
	void Win32Window::SetSize(uint16 width, uint16 height)
	{
		VALIDATE(m_hWnd != 0);

		if (::IsWindow(m_hWnd))
		{
			::SetWindowPos(m_hWnd, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
		}
	}
}

#endif