#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Containers/TArray.h"

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

		Win32Window* 	GetWindowFromHandle(HWND hWnd);
		HINSTANCE 		GetInstanceHandle();

		// Application
		virtual void AddMessageHandler(IApplicationMessageHandler* pHandler) 	override final;
		virtual void RemoveMessageHandler(IApplicationMessageHandler* pHandler) override final;
        
		virtual void ProcessBufferedMessages() override final;

        virtual Window* GetForegroundWindow()   const override final;
        virtual Window* GetMainWindow()         const override final;

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
		Win32Window*	m_pMainWindow	= nullptr;
		HINSTANCE		m_hInstance 	= 0;
		
		TArray<Win32Window*>		m_Windows;
		TArray<Win32Message> 		m_BufferedMessages;
		TArray<IMessageHandler*> 	m_MessageHandlers;

	private:
		static Win32Application* s_pApplication;
	};

	typedef Win32Application PlatformApplication;
}

#endif