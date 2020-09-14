#include "Application/API/CommonApplication.h"
#include "Application/API/PlatformApplication.h"
#include "Application/API/Window.h"
#include "Engine/EngineConfig.h"

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
		windowDesc.Width 	= EngineConfig::GetArrayProperty("WindowSize").GetFront();
		windowDesc.Height 	= EngineConfig::GetArrayProperty("WindowSize").GetBack();
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
		WindowFocusChangedEvent event(window, hasFocus);
		EventQueue::SendEvent(event);
	}

	void CommonApplication::OnWindowMoved(TSharedRef<Window> window, int16 x, int16 y)
	{
		WindowMovedEvent event(window, x, y);
		EventQueue::SendEvent(event);
	}

	void CommonApplication::OnWindowResized(TSharedRef<Window> window, uint16 width, uint16 height, EResizeType resizeType)
	{
		WindowResizedEvent event(window, width, height, resizeType);
		EventQueue::SendEvent(event);
	}

	void CommonApplication::OnWindowClosed(TSharedRef<Window> window)
	{
		if (window == m_MainWindow.Get())
		{
			Terminate();
		}

		WindowClosedEvent event(window);
		EventQueue::SendEvent(event);
	}

	void CommonApplication::OnMouseEntered(TSharedRef<Window> window)
	{
		MouseEnteredWindowEvent event(window);
		EventQueue::SendEvent(event);
	}

	void CommonApplication::OnMouseLeft(TSharedRef<Window> window)
	{
		MouseLeftWindowEvent event(window);
		EventQueue::SendEvent(event);
	}

	void CommonApplication::OnMouseMoved(int32 x, int32 y)
	{
		MouseMovedEvent event(x, y);
		EventQueue::SendEvent(event);
	}

	void CommonApplication::OnMouseMovedRaw(int32 deltaX, int32 deltaY)
	{
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

		MouseButtonClickedEvent event(button, modifierState);
		EventQueue::SendEvent(event);
	}

	void CommonApplication::OnButtonReleased(EMouseButton button, ModifierKeyState modifierState)
	{
		TSharedRef<Window> CaptureWindow = GetCapture();
		if (CaptureWindow)
		{
			SetCapture(nullptr);
		}

		MouseButtonReleasedEvent event(button, modifierState);
		EventQueue::SendEvent(event);
	}

	void CommonApplication::OnMouseScrolled(int32 deltaX, int32 deltaY)
	{
		MouseScrolledEvent event(deltaX, deltaY);
		EventQueue::SendEvent(event);
	}

	void CommonApplication::OnKeyPressed(EKey key, ModifierKeyState modifierState, bool isRepeat)
	{
		KeyPressedEvent event(key, modifierState, isRepeat);
		EventQueue::SendEvent(event);
	}

	void CommonApplication::OnKeyReleased(EKey key, ModifierKeyState modifierState)
	{
		KeyReleasedEvent event(key, modifierState);
		EventQueue::SendEvent(event);
	}

	void CommonApplication::OnKeyTyped(uint32 character)
	{
		KeyTypedEvent event(character);
		EventQueue::SendEvent(event);
	}

	bool CommonApplication::PreInit()
	{
		// Create platform application
		Application* pPlatformApplication = PlatformApplication::CreateApplication();
		if (!pPlatformApplication->Create())
		{
			return false;
		}

		// Create application
		CommonApplication* pApplication = DBG_NEW CommonApplication();
		if (!pApplication->Create(pPlatformApplication))
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
		Application* pPlatformApplication = s_CommonApplication->GetPlatformApplication();
		pPlatformApplication->SetEventHandler(nullptr);

		s_CommonApplication.Reset();

		SAFEDELETE(pPlatformApplication);
		return true;
	}
}
