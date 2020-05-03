#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Containers/TArray.h"

#include "Application/API/Application.h"

#include "Win32Window.h"

namespace LambdaEngine
{
	class IWin32MessageHandler;

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

	/*
	 * Class that represents the OS- application. Handles windows and the event loop.
	 */
	class LAMBDA_API Win32Application : public Application
	{
	public:
		Win32Application(HINSTANCE hInstance);
		~Win32Application();

		void AddWin32MessageHandler(IWin32MessageHandler* pHandler);
		void RemoveWin32MessageHandler(IWin32MessageHandler* pHandler);

		void StoreMessage(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

		void ProcessMessage(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

		/*
		* Returns a Win32Window from a native windowhandle
		*	hWnd	- Native window handle
		*	return	- An instance of the Win32Window that has the 'hWnd' -handle. 
		*				Returns nullptr if handle is invalid
		*/
		Win32Window* 	GetWindowFromHandle(HWND hWnd) const;
		HINSTANCE 		GetInstanceHandle();

		// Application
		virtual void AddWindowHandler(IWindowHandler* pWindowHandler) 		override final;
		virtual void RemoveWindowHandler(IWindowHandler* pWindowHandler)	override final;
        
		virtual void ProcessStoredEvents() override final;

		virtual void MakeMainWindow(IWindow* pMainWindow) override final;

        virtual IWindow* GetForegroundWindow()   const override final;
        virtual IWindow* GetMainWindow()         const override final;

	private:
		void AddWindow(Win32Window* pWindow);

	public:
		static bool PreInit(HINSTANCE hInstance);
		static bool PostRelease();

		static bool Tick();
		static bool ProcessMessages();

		static IWindow*			CreateWindow(const char* pTitle, uint32 width, uint32 height);
		static IInputDevice* 	CreateInputDevice(EInputMode inputType);

		static void Terminate();

		static FORCEINLINE Win32Application* Get()
		{
			return s_pApplication;
		}

	private:
		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	private:
		Win32Window*	m_pMainWindow		= nullptr;
		HINSTANCE		m_hInstance 		= 0;
		bool			m_IsTrackingMouse	= false;
		
		TArray<Win32Window*>			m_Windows;
		TArray<Win32Message> 			m_StoredMessages;
		TArray<IWindowHandler*> 		m_WindowHandlers;
		TArray<IWin32MessageHandler*>	m_MessageHandlers;

	private:
		static Win32Application* s_pApplication;
	};

	typedef Win32Application PlatformApplication;
}

#endif