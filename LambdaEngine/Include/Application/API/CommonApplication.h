#pragma once
#include "Application.h"
#include "ApplicationEventHandler.h"

#include "Window.h"

#include "Containers/TArray.h"

namespace LambdaEngine
{
	class LAMBDA_API CommonApplication : public ApplicationEventHandler
	{
	public:
		~CommonApplication();
		
		DECL_UNIQUE_CLASS(CommonApplication);

		bool Create();
		TSharedRef<Window> CreateWindow(const WindowDesc* pDesc);
		
		void RemoveEventHandler(ApplicationEventHandler* pEventHandler);
		void AddEventHandler(ApplicationEventHandler* pEventHandler);

		/*
		* Application ticks one frame, processes OS- events and then processes the buffered events
		*	return - Returns false if the OS- sent a quit message. Happens when terminate is called.
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
		*	pMainWindow - New main window
		*/
		void MakeMainWindow(TSharedRef<Window> window);
		
		FORCEINLINE TSharedRef<Window> GetMainWindow()
		{
			VALIDATE(m_MainWindow != nullptr);
			return m_MainWindow;
		}

		bool SupportsRawInput() const;

		/*
		* Sets the input mode for the selected window
		*/
		void SetInputMode(TSharedRef<Window> pWindow, EInputMode inputMode);
		
		FORCEINLINE EInputMode GetInputMode(TSharedRef<Window> window)
		{
			return m_pPlatformApplication->GetInputMode(window); 
		}

		void SetCapture(TSharedRef<Window> window);
		
		FORCEINLINE TSharedRef<Window> GetCapture() const
		{
			return m_pPlatformApplication->GetCapture();
		}
		
		void SetActiveWindow(TSharedRef<Window> window);
		
		FORCEINLINE TSharedRef<Window> GetActiveWindow() const
		{
			return m_pPlatformApplication->GetActiveWindow();
		}

		void SetMouseVisibility(bool visible);
		void SetMousePosition(int x, int y);

		ModifierKeyState GetModifierKeyState() const;
		
		FORCEINLINE bool IsExiting() const
		{
			return m_IsExiting;
		}

	public:
		// EventHandler Interface
		virtual void OnFocusChanged(TSharedRef<Window> window, bool hasFocus)									override final;
		virtual void OnWindowMoved(TSharedRef<Window> window, int16 x, int16 y)									override final;
		virtual void OnWindowResized(TSharedRef<Window> window, uint16 width, uint16 height, EResizeType type)	override final;
		virtual void OnWindowClosed(TSharedRef<Window> window)													override final;
		virtual void OnMouseEntered(TSharedRef<Window> window)													override final;
		virtual void OnMouseLeft(TSharedRef<Window> window)														override final;

		virtual void OnMouseMoved(int32 x, int32 y)											override final;
		virtual void OnMouseMovedRaw(int32 deltaX, int32 deltaY)							override final;
		virtual void OnButtonPressed(EMouseButton button, ModifierKeyState modifierState)	override final;
		virtual void OnButtonReleased(EMouseButton button)									override final;
		virtual void OnMouseScrolled(int32 deltaX, int32 deltaY)							override final;

		virtual void OnKeyPressed(EKey key, ModifierKeyState modifierState, bool isRepeat)	override final;
		virtual void OnKeyReleased(EKey key)												override final;
		virtual void OnKeyTyped(uint32 character)											override final;

	private:
		CommonApplication();

		void ReleasePlatform();

	public:
		static bool PreInit();
		static bool PostRelease();

		static CommonApplication* Get();

	private:
		TSharedRef<Window> m_MainWindow	= nullptr;
		TArray<ApplicationEventHandler*> m_EventHandlers;
		Application* m_pPlatformApplication	= nullptr;

		bool m_IsExiting = false;

	private:
		static TSharedPtr<CommonApplication> s_CommonApplication;
	};
}
