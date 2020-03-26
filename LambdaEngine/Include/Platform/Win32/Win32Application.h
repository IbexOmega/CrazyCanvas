#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
#include <vector>

#include "Platform/Common/Application.h"

#include "Win32Window.h"

namespace LambdaEngine
{
	struct Win32Message
	{
		HWND 	hWnd 		= 0;
		UINT 	uMessage 	= 0;
		WPARAM 	wParam 		= 0;
		LPARAM 	lParam 		= 0;
	};

	class LAMBDA_API Win32Application : public Application
	{
	public:
		Win32Application() = default;
		~Win32Application() = default;

		virtual void AddMessageHandler(IApplicationMessageHandler* pHandler) 	override;
		virtual void RemoveMessageHandler(IApplicationMessageHandler* pHandler) override;
        
		virtual void ProcessBufferedMessages() override;

        virtual Window*         GetWindow()         override;
        virtual const Window*   GetWindow() const   override;

		void BufferMessage(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

		bool Create(HINSTANCE hInstance);
		void Destroy();

		static bool PreInit(HINSTANCE hInstance);
		static bool PostRelease();

		static bool Tick();
		static bool ProcessMessages();

		static InputDevice* CreateInputDevice();

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
		
		std::vector<Win32Message> 					m_BufferedMessages;
		std::vector<IApplicationMessageHandler*> 	m_MessageHandlers;

		static Win32Application	s_Application;
	};

	typedef Win32Application PlatformApplication;
}

#endif