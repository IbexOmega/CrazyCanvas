#include "Application/API/CommonApplication.h"
#include "Application/API/PlatformApplication.h"
#include "Application/API/IWindow.h"

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

	bool CommonApplication::Create()
	{
		m_pPlatformApplication = PlatformApplication::CreateApplication();
		VALIDATE(m_pPlatformApplication != nullptr);

		// Init platform
		if (!m_pPlatformApplication->Create(this))
		{
			return false;
		}

		// Set prefered inputmode
		//if (m_pPlatformApplication->SupportsRawInput())
		//{
		//	m_pPlatformApplication->SetInputMode(EInputMode::INPUT_MODE_RAW);
		//}
		//else
		//{
		//}
		m_pPlatformApplication->SetInputMode(EInputMode::INPUT_MODE_STANDARD);

		WindowDesc windowDesc = { };
		windowDesc.pTitle 	= "Lambda Engine";
		windowDesc.Width 	= 1440;
		windowDesc.Height 	= 900;
		windowDesc.Style	= WINDOW_STYLE_FLAG_TITLED | WINDOW_STYLE_FLAG_CLOSABLE;

		IWindow* pWindow = PlatformApplication::CreateWindow(&windowDesc);
		if (pWindow)
		{
			m_pPlatformApplication->MakeMainWindow(pWindow);
			pWindow->Show();
		}
		else
		{
			return false;
		}

		return true;
	}

	void CommonApplication::AddEventHandler(IEventHandler* pEventHandler)
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

	void CommonApplication::RemoveEventHandler(IEventHandler* pEventHandler)
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

	void CommonApplication::MakeMainWindow(IWindow* pMainWindow)
	{
		m_pPlatformApplication->MakeMainWindow(pMainWindow);
	}

	bool CommonApplication::SupportsRawInput() const
	{
		return m_pPlatformApplication->SupportsRawInput();
	}

	void CommonApplication::SetInputMode(EInputMode inputMode)
	{
		m_pPlatformApplication->SetInputMode(inputMode);
	}

	EInputMode CommonApplication::GetInputMode() const
	{
		return m_pPlatformApplication->GetInputMode();
	}

	IWindow* CommonApplication::GetForegroundWindow() const
	{
		return m_pPlatformApplication->GetForegroundWindow();
	}

	IWindow* CommonApplication::GetMainWindow() const
	{
		return m_pPlatformApplication->GetMainWindow();
	}

	void CommonApplication::FocusChanged(IWindow* pWindow, bool hasFocus)
	{
		for (IEventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->FocusChanged(pWindow, hasFocus);
		}
	}

	void CommonApplication::WindowMoved(IWindow* pWindow, int16 x, int16 y)
	{
		for (IEventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->WindowMoved(pWindow, x, y);
		}
	}

	void CommonApplication::WindowResized(IWindow* pWindow, uint16 width, uint16 height, EResizeType type)
	{
		for (IEventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->WindowResized(pWindow, width, height, type);
		}
	}

	void CommonApplication::WindowClosed(IWindow* pWindow)
	{
		for (IEventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->WindowClosed(pWindow);
		}
	}

	void CommonApplication::MouseEntered(IWindow* pWindow)
	{
		for (IEventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->MouseEntered(pWindow);
		}
	}

	void CommonApplication::MouseLeft(IWindow* pWindow)
	{
		for (IEventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->MouseLeft(pWindow);
		}
	}

	void CommonApplication::MouseMoved(int32 x, int32 y)
	{
		for (IEventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->MouseMoved(x, y);
		}
	}

	void CommonApplication::ButtonPressed(EMouseButton button, uint32 modifierMask)
	{
		for (IEventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->ButtonPressed(button, modifierMask);
		}
	}

	void CommonApplication::ButtonReleased(EMouseButton button)
	{
		for (IEventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->ButtonReleased(button);
		}
	}

	void CommonApplication::MouseScrolled(int32 deltaX, int32 deltaY)
	{
		for (IEventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->MouseScrolled(deltaX, deltaY);
		}
	}

	void CommonApplication::KeyPressed(EKey key, uint32 modifierMask, bool isRepeat)
	{
		for (IEventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->KeyPressed(key, modifierMask, isRepeat);
		}
	}

	void CommonApplication::KeyReleased(EKey key)
	{
		for (IEventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->KeyReleased(key);
		}
	}

	void CommonApplication::KeyTyped(uint32 character)
	{
		for (IEventHandler* pEventHandler : m_EventHandlers)
		{
			pEventHandler->KeyTyped(character);
		}
	}

	bool CommonApplication::PreInit()
	{
		CommonApplication* pApplication = DBG_NEW CommonApplication();
		if (!pApplication->Create())
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
		CommonApplication::Get()->ProcessStoredEvents();
		
		return shouldRun;
	}

	void CommonApplication::Terminate()
	{
		PlatformApplication::Terminate();
	}

	CommonApplication* CommonApplication::Get()
	{
		return s_pCommonApplication;
	}

	bool CommonApplication::PostRelease()
	{
		PlatformApplication::PostRelease();

		SAFEDELETE(s_pCommonApplication);
		return true;
	}
}
