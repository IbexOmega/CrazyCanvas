#pragma once
#include "Application.h"
#include "IEventHandler.h"

#include "Containers/TArray.h"

namespace LambdaEngine
{
	class LAMBDA_API CommonApplication : public IEventHandler
	{
	public:
		DECL_UNIQUE_CLASS(CommonApplication);
		
		CommonApplication();
		~CommonApplication();

		bool Create();

		void RemoveEventHandler(IEventHandler* pEventHandler);
		void AddEventHandler(IEventHandler* pEventHandler);

		void ProcessStoredEvents();

		void MakeMainWindow(IWindow* pMainWindow);

		bool SupportsRawInput() const;

		void SetInputMode(EInputMode inputMode);

		EInputMode GetInputMode() const;

		IWindow* GetForegroundWindow()   const;
		IWindow* GetMainWindow()         const;

		// IEventHandler Interface
        virtual void FocusChanged(IWindow* pWindow, bool hasFocus)									override final;
        virtual void WindowMoved(IWindow* pWindow, int16 x, int16 y)								override final;
        virtual void WindowResized(IWindow* pWindow, uint16 width, uint16 height, EResizeType type) override final;
        virtual void WindowClosed(IWindow* pWindow)													override final;
        virtual void MouseEntered(IWindow* pWindow)													override final;
        virtual void MouseLeft(IWindow* pWindow)													override final;

        virtual void MouseMoved(int32 x, int32 y)								override final;
        virtual void ButtonPressed(EMouseButton button, uint32 modifierMask)	override final;
        virtual void ButtonReleased(EMouseButton button)						override final;
        virtual void MouseScrolled(int32 deltaX, int32 deltaY)					override final;

        virtual void KeyPressed(EKey key, uint32 modifierMask, bool isRepeat)	override final;
        virtual void KeyReleased(EKey key)										override final;
        virtual void KeyTyped(uint32 character)									override final;
		
	public:
		static bool PreInit();
		static bool PostRelease();
		
        /*
        * Application ticks one frame, processes OS- events and then processes the buffered events
        *   return - Returns false if the OS- sent a quit message. Happens when terminate is called.
        */
		static bool Tick();
        
        /*
        * Sends a quit message to the application
        */
		static void Terminate();

		static CommonApplication* Get();
		
	private:
		Application*			m_pPlatformApplication = nullptr;
		TArray<IEventHandler*> 	m_EventHandlers;

	private:
		static CommonApplication* s_pCommonApplication;
	};
}
