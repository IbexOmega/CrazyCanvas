#pragma once
#include "LambdaEngine.h"

namespace LambdaEngine
{
	struct WindowDesc;
	class Window;
	class IInputDevice;
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
		DECL_ABSTRACT_CLASS(Application);

		virtual bool Create(EventHandler* pEventHandler) = 0;

		virtual void ProcessStoredEvents() = 0; 

		virtual void MakeMainWindow(Window* pMainWindow) = 0;

		virtual bool SupportsRawInput() const = 0;

		virtual void SetInputMode(EInputMode inputMode) = 0;
		
		virtual EInputMode GetInputMode() const = 0;

		virtual Window* GetForegroundWindow()   const = 0;
		virtual Window* GetMainWindow()         const = 0;
		
		public:
		static bool PreInit() 		{ return true; }
		static bool PostRelease() 	{ return true; }

		/*
		* Processes all event from the OS and bufferes them up
		*   return - Returns false if the OS- sent a quit message
		*/
		static bool ProcessMessages() { return false; }

		static void Terminate() { }

		static Window* 		CreateWindow(const WindowDesc*)	{ return nullptr; }
		static Application* CreateApplication()				{ return nullptr; }
		
		static Application* Get() { return nullptr; }
	};
}
