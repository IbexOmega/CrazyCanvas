#include "Application/API/CommonApplication.h"
#include "Application/API/PlatformApplication.h"
#include "Application/API/Window.h"

#include "Application/API/Events/EventQueue.h"
#include "Application/API/Events/MouseEvents.h"
#include "Application/API/Events/KeyEvents.h"
#include "Application/API/Events/WindowEvents.h"

namespace LambdaEngine
{
	TSharedPtr<CommonApplication> CommonApplication::s_CommonApplication = nullptr;

	/*
	* CommonApplication
	*/
	CommonApplication::CommonApplication()
		: ApplicationEventHandler()
		, m_EventHandlers()
	{
		VALIDATE(s_CommonApplication == nullptr);
		s_CommonApplication = this;
	}

	void CommonApplication::ReleasePlatform()
	{
		// HACK: For now to relase the cycle between the commonapplication and platformapplication
		SAFEDELETE(m_pPlatformApplication);
	}

	CommonApplication::~CommonApplication()
	{
		VALIDATE(s_CommonApplication != nullptr);
		s_CommonApplication = nullptr;

		SAFEDELETE(m_pPlatformApplication);
	}

	bool CommonApplication::Create()
	{
		// Create platform application
		m_pPlatformApplication = PlatformApplication::CreateApplication();
		if (m_pPlatformApplication->Create())
		{
			m_pPlatformApplication->SetEventHandler(s_CommonApplication);
		}
		else
		{
			return false;
		}

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

	void CommonApplication::AddEventHandler(ApplicationEventHandler* pEventHandler)
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

	void CommonApplication::RemoveEventHandler(ApplicationEventHandler* pEventHandler)
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

	void CommonApplication::SetMousePosition(int32 x, int32 y)
	{
		m_pPlatformApplication->SetMousePosition(x, y);
	}

	ModifierKeyState CommonApplication::GetModifierKeyState() const
	{
		return m_pPlatformApplication->GetModiferKeyState();
	}

	void CommonApplication::OnFocusChanged(TSharedRef<Window> window, bool hasFocus)
	{
		for (ApplicationEventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnFocusChanged(window, hasFocus);
		}

		FocusChangedEvent event(window, hasFocus);
		EventQueue::SendEvent(event);
	}

	void CommonApplication::OnWindowMoved(TSharedRef<Window> window, int16 x, int16 y)
	{
		for (ApplicationEventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnWindowMoved(window, x, y);
		}

		WindowMovedEvent event(window, x, y);
		EventQueue::SendEvent(event);
	}

	void CommonApplication::OnWindowResized(TSharedRef<Window> window, uint16 width, uint16 height, EResizeType resizeType)
	{
		for (ApplicationEventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnWindowResized(window, width, height, resizeType);
		}

		WindowResizedEvent event(window, width, height, resizeType);
		EventQueue::SendEvent(event);
	}

	void CommonApplication::OnWindowClosed(TSharedRef<Window> window)
	{
		if (window == m_MainWindow.Get())
		{
			Terminate();
		}

		for (ApplicationEventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnWindowClosed(window);
		}

		WindowClosedEvent event(window);
		EventQueue::SendEvent(event);
	}

	void CommonApplication::OnMouseEntered(TSharedRef<Window> window)
	{
		for (ApplicationEventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnMouseEntered(window);
		}

		WindowMouseEnteredEvent event(window);
		EventQueue::SendEvent(event);
	}

	void CommonApplication::OnMouseLeft(TSharedRef<Window> window)
	{
		for (ApplicationEventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnMouseLeft(window);
		}

		WindowMouseLeftEvent event(window);
		EventQueue::SendEvent(event);
	}

	void CommonApplication::OnMouseMoved(int32 x, int32 y)
	{
		for (ApplicationEventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnMouseMoved(x, y);
		}

		MouseMovedEvent event(x, y);
		EventQueue::SendEvent(event);
	}

	void CommonApplication::OnMouseMovedRaw(int32 deltaX, int32 deltaY)
	{
		for (ApplicationEventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnMouseMovedRaw(deltaX, deltaY);
		}

		RawMouseMovedEvent event(deltaX, deltaY);
		EventQueue::SendEvent(event);
	}

	void CommonApplication::OnButtonPressed(EMouseButton button, ModifierKeyState modifierState)
	{
		TSharedRef<Window> CaptureWindow = GetCapture();
		if (!CaptureWindow)
		{
			TSharedRef<Window> ActiveWindow = GetActiveWindow();
			SetCapture(ActiveWindow);
		}

		for (ApplicationEventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnButtonPressed(button, modifierState);
		}

		MouseClickedEvent event(button, modifierState);
		EventQueue::SendEvent(event);
	}

	void CommonApplication::OnButtonReleased(EMouseButton button)
	{
		TSharedRef<Window> CaptureWindow = GetCapture();
		if (CaptureWindow)
		{
			SetCapture(nullptr);
		}

		for (ApplicationEventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnButtonReleased(button);
		}

		MouseReleasedEvent event(button);
		EventQueue::SendEvent(event);
	}

	void CommonApplication::OnMouseScrolled(int32 deltaX, int32 deltaY)
	{
		for (ApplicationEventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnMouseScrolled(deltaX, deltaY);
		}

		MouseScrolledEvent event(deltaX, deltaY);
		EventQueue::SendEvent(event);
	}

	void CommonApplication::OnKeyPressed(EKey key, ModifierKeyState modifierState, bool isRepeat)
	{
		for (ApplicationEventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnKeyPressed(key, modifierState, isRepeat);
		}

		KeyPressedEvent event(key, modifierState, isRepeat);
		EventQueue::SendEvent(event);
	}

	void CommonApplication::OnKeyReleased(EKey key)
	{
		for (ApplicationEventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnKeyReleased(key);
		}

		KeyReleasedEvent event(key);
		EventQueue::SendEvent(event);
	}

	void CommonApplication::OnKeyTyped(uint32 character)
	{
		for (ApplicationEventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnKeyTyped(character);
		}

		KeyTypedEvent event(character);
		EventQueue::SendEvent(event);
	}

	bool CommonApplication::PreInit()
	{
		// Create application
		CommonApplication* pApplication = DBG_NEW CommonApplication();
		if (!pApplication->Create())
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
		s_CommonApplication->ReleasePlatform();
		s_CommonApplication.Reset();
		return true;
	}
}
