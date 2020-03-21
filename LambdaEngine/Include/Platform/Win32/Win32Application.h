#pragma once
#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Platform/Common/Application.h"

#include "Win32Window.h"

namespace LambdaEngine
{
	class LAMBDA_API Win32Application : public Application
	{
	public:
		DECL_STATIC_CLASS(Win32Application); 

		static bool PreInit(HINSTANCE hInstance);
		static bool PostRelease();

		static bool Tick();

		static FORCEINLINE void Terminate()
		{
			//TODO: Maybe take in the exitcode
			PostQuitMessage(0);
		}

		static FORCEINLINE HINSTANCE GetInstanceHandle()
		{
			return s_hInstance;
		}

	private:
		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		static Win32Window	s_Window;
		static HINSTANCE	s_hInstance;
	};

	typedef Win32Application PlatformApplication;
}

#endif