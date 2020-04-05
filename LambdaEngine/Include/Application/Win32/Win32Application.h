#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
#include <vector>

#include "Application/API/Application.h"

#include "Win32Window.h"

namespace LambdaEngine
{
	/*
	* Struct used to buffer events from the OS
	*/
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
		Win32Application(HINSTANCE hInstance);
		~Win32Application();

		void BufferMessage(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

		virtual void AddMessageHandler(IApplicationMessageHandler* pHandler) 	override;
		virtual void RemoveMessageHandler(IApplicationMessageHandler* pHandler) override;
        
		virtual void ProcessBufferedMessages() override;

        virtual Window*         GetWindow()         override;
        virtual const Window*   GetWindow() const   override;

		HINSTANCE GetInstanceHandle();

	public:
		static bool PreInit(HINSTANCE hInstance);
		static bool PostRelease();

		static bool Tick();
		static bool ProcessMessages();

		static Window*			CreateWindow(const char* pTitle, uint32 width, uint32 height);
		static IInputDevice* 	CreateInputDevice(EInputMode inputType);

		static void Terminate();

		static FORCEINLINE Win32Application* Get()
		{
			return s_pApplication;
		}

	private:
		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	private:
		Win32Window*	m_pWindow	= nullptr;
		HINSTANCE		m_hInstance = 0;
		
		std::vector<Win32Message> 					m_BufferedMessages;
		std::vector<IApplicationMessageHandler*> 	m_MessageHandlers;

	private:
		static Win32Application* s_pApplication;
	};

	typedef Win32Application PlatformApplication;
}

#endif