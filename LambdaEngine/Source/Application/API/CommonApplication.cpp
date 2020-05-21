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
	}

	bool CommonApplication::Create(Application* pPlatformApplication)
	{
		VALIDATE(pPlatformApplication != nullptr);
		m_pPlatformApplication = pPlatformApplication;

		// Set prefered inputmode
		//if (m_pPlatformApplication->SupportsRawInput())
		//{
		//	m_pPlatformApplication->SetInputMode(EInputMode::INPUT_MODE_RAW);
		//}
		//else
		//{
		//}


		WindowDesc windowDesc = { };
		windowDesc.pTitle 	= "Lambda Engine";
		windowDesc.Width 	= 1440;
		windowDesc.Height 	= 900;
		windowDesc.Style	= WINDOW_STYLE_FLAG_TITLED | WINDOW_STYLE_FLAG_CLOSABLE;

		if (!pPlatformApplication->Create(this))
		{
			return false;
		}

		Window* pWindow = PlatformApplication::CreateWindow(&windowDesc);
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

		return true;
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

	void CommonApplication::ProcessStoredEvents()
	{
		m_pPlatformApplication->ProcessStoredEvents();
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
		m_pPlatformApplication->SetInputMode(inputMode);
	}

	void CommonApplication::SetFocus(Window* pWindow)
	{
		//TODO: Implement
	}

	void CommonApplication::SetCapture(Window* pWindow)
	{
	}

	void CommonApplication::SetActiveWindow(Window* pWindow)
	{
	}

	void CommonApplication::FocusChanged(Window* pWindow, bool hasFocus)
	{
		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->FocusChanged(pWindow, hasFocus);
		}
	}

	void CommonApplication::WindowMoved(Window* pWindow, int16 x, int16 y)
	{
		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->WindowMoved(pWindow, x, y);
		}
	}

	void CommonApplication::WindowResized(Window* pWindow, uint16 width, uint16 height, EResizeType type)
	{
		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->WindowResized(pWindow, width, height, type);
		}
	}

	void CommonApplication::WindowClosed(Window* pWindow)
	{
		if (pWindow == m_pMainWindow)
		{
			Terminate();
		}

		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->WindowClosed(pWindow);
		}
	}

	void CommonApplication::MouseEntered(Window* pWindow)
	{
		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->MouseEntered(pWindow);
		}
	}

	void CommonApplication::MouseLeft(Window* pWindow)
	{
		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->MouseLeft(pWindow);
		}
	}

	void CommonApplication::MouseMoved(int32 x, int32 y)
	{
		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->MouseMoved(x, y);
		}
	}

	void CommonApplication::MouseMovedRaw(int32 deltaX, int32 deltaY)
	{
		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->MouseMovedRaw(deltaX, deltaY);
		}
	}

	void CommonApplication::ButtonPressed(EMouseButton button, uint32 modifierMask)
	{
		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->ButtonPressed(button, modifierMask);
		}
	}

	void CommonApplication::ButtonReleased(EMouseButton button)
	{
		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->ButtonReleased(button);
		}
	}

	void CommonApplication::MouseScrolled(int32 deltaX, int32 deltaY)
	{
		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->MouseScrolled(deltaX, deltaY);
		}
	}

	void CommonApplication::KeyPressed(EKey key, uint32 modifierMask, bool isRepeat)
	{
		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->KeyPressed(key, modifierMask, isRepeat);
		}
	}

	void CommonApplication::KeyReleased(EKey key)
	{
		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->KeyReleased(key);
		}
	}

	void CommonApplication::KeyTyped(uint32 character)
	{
		for (EventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->KeyTyped(character);
		}
	}

	bool CommonApplication::PreInit()
	{
		Application*		pPlatformApplication	= PlatformApplication::CreateApplication();
		CommonApplication*	pApplication			= CreateApplication(pPlatformApplication);
		if (!pApplication)
		{
			return false;
		}
	
		if (!PlatformApplication::PreInit())
		{
			return false;
		}

		return true;
	}

	bool CommonApplication::Tick()
	{
		bool shouldRun = PlatformApplication::ProcessMessages();
		ProcessStoredEvents();
		
		return shouldRun;
	}

	void CommonApplication::Terminate()
	{
		PlatformApplication::Terminate();
	}

	CommonApplication* CommonApplication::CreateApplication(Application* pPlatformApplication)
	{
		CommonApplication* pApplication = DBG_NEW CommonApplication();
		if (!pApplication->Create(pPlatformApplication))
		{
			delete pApplication;
			return nullptr;
		}
		else
		{
			return pApplication;
		}
	}

	CommonApplication* CommonApplication::Get()
	{
		VALIDATE(s_pCommonApplication != nullptr);
		return s_pCommonApplication;
	}

	bool CommonApplication::PostRelease()
	{
		PlatformApplication::PostRelease();

		SAFEDELETE(s_pCommonApplication);
		return true;
	}
}
