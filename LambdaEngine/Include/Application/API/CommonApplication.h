#pragma once
#include "Application.h"
#include "EventHandler.h"

#include "Containers/TArray.h"

namespace LambdaEngine
{
	class LAMBDA_API CommonApplication : public EventHandler
	{
	public:
		DECL_UNIQUE_CLASS(CommonApplication);

		bool Create(Application* pPlatformApplication);

		void RemoveEventHandler(EventHandler* pEventHandler);
		void AddEventHandler(EventHandler* pEventHandler);

		/*
		* Application ticks one frame, processes OS- events and then processes the buffered events
		*   return - Returns false if the OS- sent a quit message. Happens when terminate is called.
		*/
		bool Tick();

		/*
		* Application buffers all OS-events, and gets processed in a batch with this function
		*/
		void ProcessStoredEvents();

		/*
		* Sends a quit message to the application. This results in the application will begin the termination
		* sequence the next tick-iteration.
		*/
		static void Terminate();

		/*
		* Sets the window to be the current main window, this is not the same as the window that has
		* currently has input focus, that would be the active window.
		*   pMainWindow - New main window
		*/
		void MakeMainWindow(Window* pMainWindow);

		bool SupportsRawInput() const;

		/*
		* Sets the input mode for the selected window
		*/
		void		SetInputMode(Window* pWindow, EInputMode inputMode);
		EInputMode	GetInputMode(Window* pWindow) { return m_pPlatformApplication->GetInputMode(); }

		void	SetFocus(Window* pWindow);
		Window* GetFocus() const { return nullptr; }

		void	SetCapture(Window* pWindow);
		Window* GetCapture() const { return nullptr; }
		
		void	SetActiveWindow(Window* pWindow);
		Window* GetActiveWindow()	const { return nullptr; }
		Window* GetMainWindow()		const { return m_pMainWindow; }

	public:
		// EventHandler Interface
		virtual void FocusChanged(Window* pWindow, bool hasFocus)									override final;
		virtual void WindowMoved(Window* pWindow, int16 x, int16 y)									override final;
		virtual void WindowResized(Window* pWindow, uint16 width, uint16 height, EResizeType type)	override final;
		virtual void WindowClosed(Window* pWindow)													override final;
		virtual void MouseEntered(Window* pWindow)													override final;
		virtual void MouseLeft(Window* pWindow)														override final;

		virtual void MouseMoved(int32 x, int32 y)								override final;
		virtual void ButtonPressed(EMouseButton button, uint32 modifierMask)	override final;
		virtual void ButtonReleased(EMouseButton button)						override final;
		virtual void MouseScrolled(int32 deltaX, int32 deltaY)					override final;

		virtual void KeyPressed(EKey key, uint32 modifierMask, bool isRepeat)	override final;
		virtual void KeyReleased(EKey key)										override final;
		virtual void KeyTyped(uint32 character)									override final;
		
	private:
		// Hide constructor
		CommonApplication();
		~CommonApplication();

	public:
		static bool PreInit();
		static bool PostRelease();

		static CommonApplication* CreateApplication(Application* pPlatformApplication);
		static CommonApplication* Get();
		
	private:
		Window*					m_pMainWindow			= nullptr;
		Application*			m_pPlatformApplication	= nullptr;
		TArray<EventHandler*> 	m_EventHandlers;

	private:
		static CommonApplication* s_pCommonApplication;
	};
}
