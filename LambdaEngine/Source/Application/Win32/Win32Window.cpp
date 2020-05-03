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

	bool Win32Window::Init(const char* pTitle, uint32 width, uint32 height)
	{
		DWORD	dwStyle		= WS_OVERLAPPEDWINDOW | WS_MINIMIZE;
		RECT	clientRect 	= { 0, 0, LONG(width), LONG(height) };
		::AdjustWindowRect(&clientRect, dwStyle, FALSE);

		INT nWidth	= clientRect.right - clientRect.left;
		INT nHeight = clientRect.bottom - clientRect.top;

		constexpr uint32 MAX_CHARS = 256;
		static wchar_t title[MAX_CHARS];

		size_t charsWritten = mbstowcs(title, pTitle, MAX_CHARS);
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
			UpdateWindow(m_hWnd);
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
		VALIDATE(m_hWnd != 0);
		::DestroyWindow(m_hWnd);
	}

	void Win32Window::Minimize()
	{
		VALIDATE(m_hWnd != 0);
		::ShowWindow(m_hWnd, SW_MINIMIZE);
	}

	void Win32Window::Maximize()
	{
		VALIDATE(m_hWnd != 0);
		::ShowWindow(m_hWnd, SW_MAXIMIZE);
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

		RECT rect = {};
		GetClientRect(m_hWnd, &rect);
		return uint16(rect.right - rect.left);
	}

	uint16 Win32Window::GetHeight() const
	{
		VALIDATE(m_hWnd != 0);

		RECT rect = {};
		GetClientRect(m_hWnd, &rect);
		return uint16(rect.bottom - rect.top);
	}

	void* Win32Window::GetHandle() const
	{
		return (void*)m_hWnd;
	}

	const void* Win32Window::GetView() const
	{
		return nullptr;
	}

	void Win32Window::SetTitle(const char* pTitle)
	{
		VALIDATE(m_hWnd != 0);
		::SetWindowTextA(m_hWnd, pTitle);
	}
}

#endif