#include "Application/API/CommonApplication.h"
#include "Application/API/PlatformApplication.h"
#include "Application/API/Window.h"

namespace LambdaEngine
{
	CommonApplication* CommonApplication::s_pCommonApplication = nullptr;

	CommonApplication::CommonApplication()
		: m_EventHandlers()
	{
		VALIDATE(s_pCommonApplication == nullptr);
		s_pCommonApplication = this;
	}

	CommonApplication::~CommonApplication()
	{
		VALIDATE(s_pCommonApplication != nullptr);
		s_pCommonApplication = nullptr;

		SAFEDELETE(m_pPlatformApplication);
	}

	bool CommonApplication::Create()
	{
		// Create platform applciation
		m_pPlatformApplication = PlatformApplication::CreateApplication();
		if (m_pPlatformApplication->Create())
		{
			m_pPlatformApplication->SetEventHandler(this);
		}
		else
		{
			return false;
		}

		// Create mainwindow
		WindowDesc windowDesc = { };
		windowDesc.Title 	= "Lambda Engine";
		windowDesc.Width 	= 2560;
		windowDesc.Height 	= 1080;
		windowDesc.Style	= WINDOW_STYLE_FLAG_TITLED | WINDOW_STYLE_FLAG_CLOSABLE;

		Window* pWindow = CreateWindow(&windowDesc);
		if (pWindow)
		{
			MakeMainWindow(pWindow);
			SetInputMode(pWindow, EInputMode::INPUT_MODE_STANDARD);
			
			pWindow->Show();
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
		else
		{
		}

		return true;
	}

	Window* CommonApplication::CreateWindow(const WindowDesc* pDesc)
	{
		return m_pPlatformApplication->CreateWindow(pDesc);
	}

	void CommonApplication::AddEventHandler(EventHandler* pEventHandler)
	{
		// Check first so that this handler is not already added
		const uint32 count = uint32(m_EventHandlers.size());
		for (uint32 i = 0; i < count; i++)
		{
			if (pEventHandler == m_EventHandlers[i])
			{
				return;
			}
		}

		// Add new handler
		m_EventHandlers.emplace_back(pEventHandler);
	}

	void CommonApplication::RemoveEventHandler(EventHandler* pEventHandler)
	{
		const uint32 count = uint32(m_EventHandlers.size());
		for (uint32 i = 0; i < count; i++)
		{
			if (pEventHandler == m_EventHandlers[i])
			{
				m_EventHandlers.erase(m_EventHandlers.begin() + i);
				break;
			}
		}
	}

	void CommonApplication::MakeMainWindow(Window* pMainWindow)
	{
		VALIDATE(pMainWindow != nullptr);
		m_pMainWindow = pMainWindow;
	}

	bool CommonApplication::SupportsRawInput() const
	{
		return m_pPlatformApplication->SupportsRawInput();
	}

	void CommonApplication::SetInputMode(Window* pWindow, EInputMode inputMode)
	{
		m_pPlatformApplication->SetInputMode(pWindow, inputMode);
	}

	void CommonApplication::SetCapture(Window* pWindow)
	{
		m_pPlatformApplication->SetCapture(pWindow);
	}

	void CommonApplication::SetActiveWindow(Window* pWindow)
	{
		m_pPlatformApplication->SetActiveWindow(pWindow);
	}

	void CommonApplication::OnFocusChanged(Window* pWindow, bool hasFocus)
	{
		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnFocusChanged(pWindow, hasFocus);
		}
	}

	void CommonApplication::OnWindowMoved(Window* pWindow, int16 x, int16 y)
	{
		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnWindowMoved(pWindow, x, y);
		}
	}

	void CommonApplication::OnWindowResized(Window* pWindow, uint16 width, uint16 height, EResizeType type)
	{
		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnWindowResized(pWindow, width, height, type);
		}
	}

	void CommonApplication::OnWindowClosed(Window* pWindow)
	{
		if (pWindow == m_pMainWindow)
		{
			Terminate();
		}

		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnWindowClosed(pWindow);
		}
	}

	void CommonApplication::OnMouseEntered(Window* pWindow)
	{
		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnMouseEntered(pWindow);
		}
	}

	void CommonApplication::OnMouseLeft(Window* pWindow)
	{
		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnMouseLeft(pWindow);
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
		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->OnButtonPressed(button, modifierMask);
		}
	}

	void CommonApplication::OnButtonReleased(EMouseButton button)
	{
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
		return m_pPlatformApplication->Tick();
	}

	void CommonApplication::Terminate()
	{
		m_pPlatformApplication->Terminate();
	}

	CommonApplication* CommonApplication::Get()
	{
		VALIDATE(s_pCommonApplication != nullptr);
		return s_pCommonApplication;
	}

	bool CommonApplication::PostRelease()
	{
		SAFEDELETE(s_pCommonApplication);
		return true;
	}
}
