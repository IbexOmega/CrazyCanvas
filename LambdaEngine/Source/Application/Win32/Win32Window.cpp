#ifdef LAMBDA_PLATFORM_WINDOWS
#include <stdlib.h>

#include "Log/Log.h"

#include "Application/Win32/Win32Window.h"
#include "Application/Win32/Win32Application.h"

namespace LambdaEngine
{
	Win32Window::~Win32Window()
	{
		::DestroyWindow(m_hWnd);
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

		size_t charsWritten = mbstowcs(title, pDesc->pTitle, MAX_CHARS);
		if (charsWritten != static_cast<size_t>(-1))
		{
			title[charsWritten] = L'\0';
		}

		HINSTANCE hInstance = PlatformApplication::Get()->GetInstanceHandle();
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

			memcpy(&m_Desc, pDesc, sizeof(WindowDesc));
			::UpdateWindow(m_hWnd);
			return true;
		}
	}

	void Win32Window::Show()
	{
		VALIDATE(m_hWnd != 0);
		::ShowWindow(m_hWnd, SW_NORMAL);
	}

	void Win32Window::Close()
	{
		if (m_Desc.Style & WINDOW_STYLE_FLAG_CLOSABLE)
		{
			VALIDATE(m_hWnd != 0);
			::DestroyWindow(m_hWnd);
		}
	}

	void Win32Window::Minimize()
	{
		if (m_Desc.Style & WINDOW_STYLE_FLAG_MINIMIZABLE)
		{
			VALIDATE(m_hWnd != 0);
			::ShowWindow(m_hWnd, SW_MINIMIZE);
		}
	}

	void Win32Window::Maximize()
	{
		if (m_Desc.Style & WINDOW_STYLE_FLAG_MAXIMIZABLE)
		{
			VALIDATE(m_hWnd != 0);
			::ShowWindow(m_hWnd, SW_MAXIMIZE);
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

		BOOL result = ::IsIconic(m_hWnd);
		if (result)
		{
			::ShowWindow(m_hWnd, SW_RESTORE);
		}
	}

	void Win32Window::ToggleFullscreen()
	{
		// TODO: Implement when rendering resize works
	}

	uint16 Win32Window::GetWidth() const
	{
		VALIDATE(m_hWnd != 0);

		RECT rect = { };
		::GetClientRect(m_hWnd, &rect);
		return uint16(rect.right - rect.left);
	}

	uint16 Win32Window::GetHeight() const
	{
		VALIDATE(m_hWnd != 0);

		RECT rect = { };
		::GetClientRect(m_hWnd, &rect);
		return uint16(rect.bottom - rect.top);
	}

	void* Win32Window::GetHandle() const
	{
		return (void*)m_hWnd;
	}

	void Win32Window::SetTitle(const char* pTitle)
	{
		VALIDATE(m_hWnd != 0);
		::SetWindowTextA(m_hWnd, pTitle);
	}
	
	void Win32Window::SetPosition(int32 x, int32 y)
	{
		VALIDATE(m_hWnd != 0);
		::SetWindowPos(m_hWnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	}
	
	void Win32Window::GetPosition(int32* pPosX, int32* pPosY) const
	{
		VALIDATE(m_hWnd != 0);
		VALIDATE(pPosX	!= nullptr);
		VALIDATE(pPosY	!= nullptr);

		WINDOWPLACEMENT placement = { };
		placement.length = sizeof(WINDOWPLACEMENT);

		if (::GetWindowPlacement(m_hWnd, &placement))
		{
			(*pPosX) = placement.rcNormalPosition.left;
			(*pPosY) = placement.rcNormalPosition.bottom;
		}
	}
	
	void Win32Window::SetSize(uint16 width, uint16 height)
	{
		VALIDATE(m_hWnd != 0);
		::SetWindowPos(m_hWnd, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	}
}

#endif