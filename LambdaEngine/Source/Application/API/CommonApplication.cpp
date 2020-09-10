#include "Application/API/CommonApplication.h"
#include "Application/API/PlatformApplication.h"
#include "Application/API/Window.h"

namespace LambdaEngine
{
	TSharedPtr<CommonApplication> CommonApplication::s_CommonApplication = nullptr;

	/*
	* CommonApplication
	*/
	CommonApplication::CommonApplication()
		: EventHandler()
		, m_EventHandlers()
	{
		VALIDATE(s_CommonApplication == nullptr);
		s_CommonApplication = this;
	}

	CommonApplication::~CommonApplication()
	{
		VALIDATE(s_CommonApplication != nullptr);
		s_CommonApplication = nullptr;
	}

	bool CommonApplication::Create(Application* pApplication)
	{
		// Create platform application
		m_pPlatformApplication = pApplication;
		m_pPlatformApplication->SetEventHandler(s_CommonApplication);

		// Create mainwindow
		WindowDesc windowDesc = { };
		windowDesc.Title 	= "Lambda Engine";
		windowDesc.Width 	= 1920;
		windowDesc.Height 	= 1080;
		windowDesc.Style	= 
			WINDOW_STYLE_FLAG_TITLED		| 
			//WINDOW_STYLE_FLAG_MINIMIZABLE	|
			//WINDOW_STYLE_FLAG_MAXIMIZABLE	|
			//WINDOW_STYLE_FLAG_RESIZEABLE	|
			WINDOW_STYLE_FLAG_CLOSABLE;

		TSharedRef<Window> window = CreateWindow(&windowDesc);
		if (window)
		{
			MakeMainWindow(window);
			SetInputMode(window, EInputMode::INPUT_MODE_STANDARD);
			
			window->Show();
		}
		else
		{
			return false;
		}

		// Set prefered inputmode
		if (m_pPlatformApplication->SupportsRawInput())
		{
			//m_pPlatformApplication->SetInputMode(EInputMode::INPUT_MODE_RAW);
		}

		return true;
	}

	TSharedRef<Window> CommonApplication::CreateWindow(const WindowDesc* pDesc)
	{
		return m_pPlatformApplication->CreateWindow(pDesc);
	}

	void CommonApplication::AddEventHandler(EventHandler* pEventHandler)
	{
		// Check first so that this handler is not already added
		const uint32 count = uint32(m_EventHandlers.GetSize());
		for (uint32 i = 0; i < count; i++)
		{
			if (pEventHandler == m_EventHandlers[i])
			{
				return;
			}
		}

		// Add new handler
		m_EventHandlers.EmplaceBack(pEventHandler);
	}

	void CommonApplication::RemoveEventHandler(EventHandler* pEventHandler)
	{
		const uint32 count = uint32(m_EventHandlers.GetSize());
		for (uint32 i = 0; i < count; i++)
		{
			if (pEventHandler == m_EventHandlers[i])
			{
				m_EventHandlers.Erase(m_EventHandlers.Begin() + i);
				break;
			}
		}
	}

	void CommonApplication::MakeMainWindow(TSharedRef<Window> window)
	{
		VALIDATE(window != nullptr);
		m_MainWindow = window;
	}

	bool CommonApplication::SupportsRawInput() const
	{
		return m_pPlatformApplication->SupportsRawInput();
	}

	void CommonApplication::SetInputMode(TSharedRef<Window> window, EInputMode inputMode)
	{
		m_pPlatformApplication->SetInputMode(window, inputMode);
	}

	void CommonApplication::SetCapture(TSharedRef<Window> window)
	{
		m_pPlatformApplication->SetCapture(window);
	}

	void CommonApplication::SetActiveWindow(TSharedRef<Window> window)
	{
		m_pPlatformApplication->SetActiveWindow(window);
	}

	void CommonApplication::SetMouseVisibility(bool visible)
	{
		m_pPlatformApplication->SetMouseVisibility(visible);
	}

	void CommonApplication::SetMousePosition(int x, int y)
	{
		m_pPlatformApplication->SetMousePosition(x, y);
	}

	void CommonApplication::OnFocusChanged(TSharedRef<Window> window, bool hasFocus)
	{
		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnFocusChanged(window, hasFocus);
		}
	}

	void CommonApplication::OnWindowMoved(TSharedRef<Window> window, int16 x, int16 y)
	{
		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnWindowMoved(window, x, y);
		}
	}

	void CommonApplication::OnWindowResized(TSharedRef<Window> window, uint16 width, uint16 height, EResizeType type)
	{
		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnWindowResized(window, width, height, type);
		}
	}

	void CommonApplication::OnWindowClosed(TSharedRef<Window> window)
	{
		if (window == m_MainWindow.Get())
		{
			Terminate();
		}

		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnWindowClosed(window);
		}
	}

	void CommonApplication::OnMouseEntered(TSharedRef<Window> window)
	{
		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnMouseEntered(window);
		}
	}

	void CommonApplication::OnMouseLeft(TSharedRef<Window> window)
	{
		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnMouseLeft(window);
		}
	}

	void CommonApplication::OnMouseMoved(int32 x, int32 y)
	{
		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnMouseMoved(x, y);
		}
	}

	void CommonApplication::OnMouseMovedRaw(int32 deltaX, int32 deltaY)
	{
		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnMouseMovedRaw(deltaX, deltaY);
		}
	}

	void CommonApplication::OnButtonPressed(EMouseButton button, uint32 modifierMask)
	{
		TSharedRef<Window> CaptureWindow = GetCapture();
		if (!CaptureWindow)
		{
			TSharedRef<Window> ActiveWindow = GetActiveWindow();
			SetCapture(ActiveWindow);
		}

		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnButtonPressed(button, modifierMask);
		}
	}

	void CommonApplication::OnButtonReleased(EMouseButton button)
	{
		TSharedRef<Window> CaptureWindow = GetCapture();
		if (CaptureWindow)
		{
			SetCapture(nullptr);
		}

		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnButtonReleased(button);
		}
	}

	void CommonApplication::OnMouseScrolled(int32 deltaX, int32 deltaY)
	{
		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnMouseScrolled(deltaX, deltaY);
		}
	}

	void CommonApplication::OnKeyPressed(EKey key, uint32 modifierMask, bool isRepeat)
	{
		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnKeyPressed(key, modifierMask, isRepeat);
		}
	}

	void CommonApplication::OnKeyReleased(EKey key)
	{
		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnKeyReleased(key);
		}
	}

	void CommonApplication::OnKeyTyped(uint32 character)
	{
		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnKeyTyped(character);
		}
	}

	bool CommonApplication::PreInit()
	{
		// Create platform application
		Application* pPlatformApplcation = PlatformApplication::CreateApplication();
		if (!pPlatformApplcation->Create())
		{
			return false;
		}

		// Create application
		CommonApplication* pApplication = DBG_NEW CommonApplication();
		if (!pApplication->Create(pPlatformApplcation))
		{
			DELETE_OBJECT(pApplication);
			return false;
		}

		return true;
	}

	bool CommonApplication::Tick()
	{
		PlatformApplication::PeekEvents();
		
		bool shouldExit = m_pPlatformApplication->Tick();
		if (shouldExit)
		{
			m_IsExiting = true;
		}

		return shouldExit;
	}

	void CommonApplication::Terminate()
	{
		m_pPlatformApplication->Terminate();
	}

	CommonApplication* CommonApplication::Get()
	{
		VALIDATE(s_CommonApplication != nullptr);
		return s_CommonApplication.Get();
	}

	bool CommonApplication::PostRelease()
	{
		Application* pPlatformApplcation = s_CommonApplication->GetPlatformApplication();
		pPlatformApplcation->SetEventHandler(nullptr);
		
		s_CommonApplication.Reset();

		SAFEDELETE(pPlatformApplcation);
		return true;
	}
}
