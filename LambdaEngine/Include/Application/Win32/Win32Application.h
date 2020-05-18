#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Containers/TArray.h"

#include "Application/API/Application.h"

#include "Windows.h"

namespace LambdaEngine
{
	class Win32Window;
	class IWin32MessageHandler;

	/*
	* Struct used to buffer events from the OS
	*/
	struct Win32Message
	{
		// Window handle
		HWND hWnd = 0;
		
		// Type of message
		UINT uMessage = 0;

		// Parameters for message
		WPARAM 	wParam 	= 0;
		LPARAM 	lParam 	= 0;

		// Raw Input parameters
		struct
		{
			int32	MouseDeltaX	= 0;
			int32	MouseDeltaY	= 0;
		} RawInput;
	};

	/*
	* Struct used handle rawinput
	*/

	struct RawInputState
	{
		byte*	pInputBuffer	= nullptr;
		uint32	BufferSize		= 0;

		int32 MouseX = 0;
		int32 MouseY = 0;
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

		void StoreMessage(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam, int32 mouseDeltaX, int32 mouseDeltaY);

		/*
		* Returns a Win32Window from a native windowhandle
		*	hWnd	- Native window handle
		*	return	- An instance of the Win32Window that has the 'hWnd' -handle. 
		*				Returns nullptr if handle is invalid
		*/
		Win32Window* 	GetWindowFromHandle(HWND hWnd) const;
		HINSTANCE 		GetInstanceHandle();

		// Application interface
		virtual bool Create(IEventHandler* pEventHandler) override final;

		virtual void ProcessStoredEvents() override final;

		virtual void MakeMainWindow(IWindow* pMainWindow) override final;

		virtual bool SupportsRawInput() const override final;

		virtual void SetInputMode(EInputMode inputMode) override final;

		virtual EInputMode GetInputMode() const override final;

        virtual IWindow* GetForegroundWindow()   const override final;
        virtual IWindow* GetMainWindow()         const override final;

	private:
		void AddWindow(Win32Window* pWindow);

		void	ProcessStoredMessage(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam, int32 mouseDeltaX, int32 mouseDeltaY);
		LRESULT ProcessMessage(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
		LRESULT ProcessRawInput(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

	public:
		static bool PreInit();
		static bool PostRelease();

		static bool ProcessMessages();

		static IWindow*		CreateWindow(const WindowDesc* pDesc);
		static Application* CreateApplication();

		static void Terminate();

		static Win32Application* Get();

	private:
		static bool RegisterWindowClass();
		static bool RegisterRawInputDevices(HWND hwnd);
		static bool UnregisterRawInputDevices();
		
		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	private:
		Win32Window*	m_pMainWindow	= nullptr;
		IEventHandler*	m_pEventHandler = nullptr;
		HINSTANCE		m_hInstance		= 0;

		RawInputState	m_RawInput			= { };
		EInputMode		m_InputMode			= EInputMode::INPUT_MODE_NONE;
		bool			m_IsTrackingMouse	= false;
		
		TArray<Win32Window*>			m_Windows;
		TArray<Win32Message> 			m_StoredMessages;
		TArray<IWin32MessageHandler*>	m_MessageHandlers;

	private:
		static Win32Application* s_pApplication;
	};

	typedef Win32Application PlatformApplication;
}

#endif