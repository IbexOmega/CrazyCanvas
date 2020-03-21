#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
#include "LambdaEngine.h"

#include "Windows.h"

#define WINDOW_CLASS L"MainWindowClass"

namespace LambdaEngine
{
	class LAMBDA_API Win32Window
	{
	public:
		Win32Window();
		~Win32Window() = default;

		DECL_REMOVE_COPY(Win32Window);
		DECL_REMOVE_MOVE(Win32Window);

		bool Init(uint32 width, uint32 height);
		void Show();
		void Release();

		HWND hWnd;
	};
}

#endif