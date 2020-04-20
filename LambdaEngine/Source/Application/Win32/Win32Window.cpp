#ifdef LAMBDA_PLATFORM_WINDOWS
#include <stdlib.h>

#include "Application/Win32/Win32Window.h"
#include "Application/Win32/Win32Application.h"

namespace LambdaEngine
{
	Win32Window::~Win32Window()
	{
		::DestroyWindow(m_hWnd);
	}

	bool Win32Window::Init(const char* pTitle, uint32 width, uint32 height)
	{
		DWORD	dwStyle		= WS_OVERLAPPEDWINDOW;
		RECT	clientRect 	= { 0, 0, LONG(width), LONG(height) };
		::AdjustWindowRect(&clientRect, dwStyle, FALSE);

		INT nWidth	= clientRect.right - clientRect.left;
		INT nHeight = clientRect.bottom - clientRect.top;

		constexpr uint32 MAX_CHARS = 256;
		static wchar_t title[MAX_CHARS];
		ZERO_MEMORY(title, sizeof(title));

		mbstowcs(title, pTitle, MAX_CHARS);

		m_hWnd = ::CreateWindowEx(0, WINDOW_CLASS, title, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, nWidth, nHeight, NULL, NULL, PlatformApplication::Get()->GetInstanceHandle(), NULL);
		if (m_hWnd == NULL)
		{
			//TODO: Log this
			return false;
		}

		return true;
	}

	void Win32Window::Show()
	{
		::ShowWindow(m_hWnd, SW_NORMAL);
	}

	uint16 Win32Window::GetWidth() const
	{
		RECT rect = {};
		GetClientRect(m_hWnd, &rect);
		return uint16(rect.right - rect.left);
	}

	uint16 Win32Window::GetHeight() const
	{
		RECT rect = {};
		GetClientRect(m_hWnd, &rect);
		return uint16(rect.bottom - rect.top);
	}

	void Win32Window::SetTitle(const char* pTitle)
	{
		::SetWindowTextA(m_hWnd, pTitle);
	}
}

#endif