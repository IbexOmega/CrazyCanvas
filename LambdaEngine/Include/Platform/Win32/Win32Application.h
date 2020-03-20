#pragma once
#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Platform/Common/Application.h"

#include "Windows.h"

namespace LambdaEngine
{
	class LAMBDA_API Win32Application : public Application
	{
	public:
		DECL_STATIC_CLASS(Win32Application);

		static bool PreInit(HINSTANCE hInstance);
		static bool PostRelease();

		static bool Tick();

	private:
		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		static HINSTANCE s_hInstance;
	};

	typedef Win32Application PlatformApplication;
}

#endif