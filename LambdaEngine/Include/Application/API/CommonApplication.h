#pragma once
#include "Application.h"
#include "EventHandler.h"

#include "Core/Ref.h"

#include "Containers/TArray.h"

namespace LambdaEngine
{
	class LAMBDA_API CommonApplication : public EventHandler
	{
	public:
		DECL_UNIQUE_CLASS(CommonApplication);

		bool 	Create();
		Window* CreateWindow(const WindowDesc* pDesc);
		
		void RemoveEventHandler(EventHandler* pEventHandler);
		void AddEventHandler(EventHandler* pEventHandler);

		/*
		* Application ticks one frame, processes OS- events and then processes the buffered events
		*   return - Returns false if the OS- sent a quit message. Happens when terminate is called.
		*/
		bool Tick();

		/*
		* Sends a quit message to the application. This results in the application will begin the termination
		* sequence the next tick-iteration.
		*/
		void Terminate();

		/*
		* Sets the window to be the current main window, this is not the same as the window that has
		* currently has input focus, that would be the active window.
		*   pMainWindow - New main window
		*/
		void MakeMainWindow(Window* pMainWindow);
		
		FORCEINLINE Window* GetMainWindow()
		{ 
			return m_MainWindow.Get(); 
		}

		bool SupportsRawInput() const;

		/*
		* Sets the input mode for the selected window
		*/
		void SetInputMode(Window* pWindow, EInputMode inputMode);
		
		FORCEINLINE EInputMode GetInputMode(Window* pWindow)
		{ 
			return m_pPlatformApplication->GetInputMode(pWindow); 
		}

		void SetCapture(Window* pWindow);
		
		FORCEINLINE Window* GetCapture() const
		{ 
			return m_pPlatformApplication->GetCapture();
		}
		
		void SetActiveWindow(Window* pWindow);
		
		FORCEINLINE Window* GetActiveWindow() const 
		{ 
			return m_pPlatformApplication->GetActiveWindow();
		}

	public:
		// EventHandler Interface
		virtual void OnFocusChanged(Window* pWindow, bool hasFocus)										override final;
		virtual void OnWindowMoved(Window* pWindow, int16 x, int16 y)									override final;
		virtual void OnWindowResized(Window* pWindow, uint16 width, uint16 height, EResizeType type)	override final;
		virtual void OnWindowClosed(Window* pWindow)													override final;
		virtual void OnMouseEntered(Window* pWindow)													override final;
		virtual void OnMouseLeft(Window* pWindow)														override final;

		virtual void OnMouseMoved(int32 x, int32 y)								override final;
		virtual void OnMouseMovedRaw(int32 deltaX, int32 deltaY)				override final;
		virtual void OnButtonPressed(EMouseButton button, uint32 modifierMask)	override final;
		virtual void OnButtonReleased(EMouseButton button)						override final;
		virtual void OnMouseScrolled(int32 deltaX, int32 deltaY)				override final;

		virtual void OnKeyPressed(EKey key, uint32 modifierMask, bool isRepeat)	override final;
		virtual void OnKeyReleased(EKey key)									override final;
		virtual void OnKeyTyped(uint32 character)								override final;
		
	private:
		CommonApplication();
		~CommonApplication();

	public:
		static bool PreInit();
		static bool PostRelease();

		static CommonApplication* Get();
		
	private:
		Ref<Window>				m_MainWindow			= nullptr;
		Application*			m_pPlatformApplication	= nullptr;
		TArray<EventHandler*> 	m_EventHandlers;

	private:
		static CommonApplication* s_pCommonApplication;
	};
}
