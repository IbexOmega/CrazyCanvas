#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
#include <vector>

#include "Platform/Common/Application.h"

#include "Win32Window.h"

namespace LambdaEngine
{
	class LAMBDA_API Win32Application : public Application
	{
	public:
		Win32Application() = default;
		~Win32Application() = default;

		virtual void AddMessageHandler(IApplicationMessageHandler* pListener) override;
        
        virtual Window*         GetWindow()         override;
        virtual const Window*   GetWindow() const   override;

		bool Create(HINSTANCE hInstance);
		void Destroy();

		static bool PreInit(HINSTANCE hInstance);
		static bool PostRelease();

		static bool Tick();

		static IInputDevice* CreateInputDevice();

		static FORCEINLINE void Terminate()
		{
			//TODO: Maybe take in the exitcode
			PostQuitMessage(0);
		}

		static FORCEINLINE HINSTANCE GetInstanceHandle()
		{
			return s_Application.m_hInstance;
		}

		static FORCEINLINE Application* Get()
		{
			return &s_Application;
		}

	private:
		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		Win32Window m_Window;
		HINSTANCE	m_hInstance = 0;
		
		std::vector<IApplicationMessageHandler*> m_MessageHandlers;

		static Win32Application	s_Application;
	};

	typedef Win32Application PlatformApplication;
}

#endif