#pragma once
#include "LambdaEngine.h"
#include "Window.h"
#include "EventHandler.h"

#include "Core/TSharedRef.h"

#include "Containers/TSharedPtr.h"

#ifdef LAMBDA_VISUAL_STUDIO
	#pragma warning(push)
	#pragma warning(disable : 4100) // Disable unreferenced variable warning
#endif

namespace LambdaEngine
{
	struct WindowDesc;

	/*
	* EInputMode - Different input devices that can be created
	*/
	enum class EInputMode
	{
		INPUT_MODE_NONE		= 0, 
		INPUT_MODE_RAW		= 1, // Raw input on supported platforms (Mouse movement)
		INPUT_MODE_STANDARD	= 2, // Standard input from the applications event-loop
	};

	/*
	* Application
	*/
	class LAMBDA_API Application
	{
	public:
		DECL_UNIQUE_CLASS(Application);
		
		virtual ~Application() = default;

		virtual bool Create() = 0;
		virtual TSharedRef<Window> CreateWindow(const WindowDesc* pDesc)	= 0;
		
		virtual void SetEventHandler(TSharedPtr<EventHandler> eventHandler) 
		{ 
			m_EventHandler = eventHandler; 
		}

		virtual TSharedPtr<EventHandler> GetEventHandler() const
		{ 
			return m_EventHandler; 
		}
		
		virtual bool Tick() = 0;
		
		virtual bool ProcessStoredEvents() = 0;

		virtual void Terminate() = 0;
		
		virtual bool SupportsRawInput() const = 0;

		virtual void SetMouseVisibility(bool) = 0;
		virtual void SetMousePosition(int32 x, int32 y) = 0;
		virtual void SetInputMode(TSharedRef<Window> window, EInputMode inputMode) = 0;
		virtual EInputMode GetInputMode(TSharedRef<Window> window) const = 0;

		virtual void SetActiveWindow(TSharedRef<Window> window)
		{
		}
		
		virtual TSharedRef<Window> GetActiveWindow() const
		{
			return TSharedRef<Window>();
		}
		
		virtual void SetCapture(TSharedRef<Window> window)
		{ 
		}
		
		virtual TSharedRef<Window> GetCapture() const
		{ 
			return nullptr;
		}
		
	protected:
		Application() = default;

	public:
		/*
		* Processes all event from the OS and bufferes them up
		*/
		static void PeekEvents()
		{
		}

		static Application* CreateApplication()	
		{ 
			return nullptr; 
		}
		
	protected:
		TSharedPtr<EventHandler> m_EventHandler = nullptr;
	};
}

#ifdef LAMBDA_VISUAL_STUDIO
	#pragma warning(pop)
#endif
