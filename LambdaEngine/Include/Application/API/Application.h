#pragma once
#include "LambdaEngine.h"

namespace LambdaEngine
{
	struct WindowDesc;
	class Window;
	class EventHandler;

	// Different input devices that can be created
	enum class EInputMode
	{
		INPUT_MODE_NONE      = 0, 
		INPUT_MODE_RAW       = 1, // Raw input on supported platforms (Mouse movement)
		INPUT_MODE_STANDARD  = 2, // Standard input from the applications event-loop
	};

	class LAMBDA_API Application
	{
	public:
		DECL_UNIQUE_CLASS(Application);
		
		virtual bool 	Create() = 0;
		virtual Window*	CreateWindow(const WindowDesc* pDesc) = 0;
		
		virtual void 			SetEventHandler(EventHandler* pEventHandler) 	{ m_pEventHandler = pEventHandler; }
		virtual EventHandler* 	GetEventHandler() const 						{ return m_pEventHandler; }
		
		virtual void Tick() = 0;
		
		virtual void ProcessStoredEvents() = 0;

		virtual void Terminate() = 0;
		
		virtual bool SupportsRawInput() const = 0;

		virtual void 		SetInputMode(Window* pWindow, EInputMode inputMode) = 0;
		virtual EInputMode	GetInputMode(Window* pWindow) const 				= 0;

		virtual void 	SetActiveWindow(Window* pWindow) 	= 0;
		virtual Window* GetActiveWindow() const 			= 0;
		
		virtual void 	SetCapture(Window* pWindow) { }
		virtual Window* GetCapture() const 			{ return nullptr; };
		
	protected:
		Application()	= default;
		~Application() 	= default;

	public:
		static bool PreInit() 		{ return true; }
		static bool PostRelease() 	{ return true; }

		/*
		* Processes all event from the OS and bufferes them up
		*   return - Returns false if the OS- sent a quit message
		*/
		static bool PeekMessages() { return false; }

		static Application* CreateApplication()	{ return nullptr; }
		static Application* Get() 				{ return nullptr; }
		
	protected:
		EventHandler* m_pEventHandler = nullptr;
	};
}
