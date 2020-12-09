#include "GUI/Core/GUIApplication.h"
#include "GUI/Core/GUIShaderManager.h"
#include "GUI/Core/GUIRenderer.h"
#include "GUI/Core/GUIInputMapper.h"

#include "NsApp/LocalXamlProvider.h"
#include "NsApp/LocalFontProvider.h"
#include "NsApp/LocalTextureProvider.h"
#include "NsApp/ThemeProviders.h"
#include "NoesisPCH.h"

#include "Application/API/CommonApplication.h"
#include "Application/API/Window.h"

#include "Application/API/Events/EventQueue.h"

namespace LambdaEngine
{
	Noesis::Ptr<Noesis::IView> GUIApplication::s_pView = nullptr;
	GUIRenderer* GUIApplication::s_pRenderer = nullptr;

	NoesisApp::LocalXamlProvider*		GUIApplication::s_pXAMLProvider		= nullptr;
	NoesisApp::LocalFontProvider*		GUIApplication::s_pFontProvider		= nullptr;
	NoesisApp::LocalTextureProvider*	GUIApplication::s_pTextureProvider	= nullptr;

	bool GUIApplication::Init()
	{
		EventQueue::RegisterEventHandler<PostSwapChainRecreatedEvent>(&GUIApplication::OnPostSwapChainRecreated);
		EventQueue::RegisterEventHandler<KeyPressedEvent>(&GUIApplication::OnKeyPressed);
		EventQueue::RegisterEventHandler<KeyReleasedEvent>(&GUIApplication::OnKeyReleased);
		EventQueue::RegisterEventHandler<KeyTypedEvent>(&GUIApplication::OnKeyTyped);
		EventQueue::RegisterEventHandler<MouseButtonClickedEvent>(&GUIApplication::OnMouseButtonClicked);
		EventQueue::RegisterEventHandler<MouseButtonReleasedEvent>(&GUIApplication::OnMouseButtonReleased);
		EventQueue::RegisterEventHandler<MouseScrolledEvent>(&GUIApplication::OnMouseScrolled);
		EventQueue::RegisterEventHandler<MouseMovedEvent>(&GUIApplication::OnMouseMoved);

		if (!GUIInputMapper::Init())
		{
			LOG_ERROR("[GUIApplication] Failed to initialize GUIInputMapper");
			return false;
		}

		if (!GUIShaderManager::Init())
		{
			LOG_ERROR("[GUIApplication] Failed to initialize GUIShaderManager");
			return false;
		}

		if (!InitNoesis())
		{
			LOG_ERROR("[GUIApplication] Failed to initialize NoesisGUI");
			return false;
		}

		return true;
	}

	bool GUIApplication::Release()
	{
		EventQueue::UnregisterEventHandler<PostSwapChainRecreatedEvent>(&GUIApplication::OnPostSwapChainRecreated);
		EventQueue::UnregisterEventHandler<KeyPressedEvent>(&GUIApplication::OnKeyPressed);
		EventQueue::UnregisterEventHandler<KeyReleasedEvent>(&GUIApplication::OnKeyReleased);
		EventQueue::UnregisterEventHandler<KeyTypedEvent>(&GUIApplication::OnKeyTyped);
		EventQueue::UnregisterEventHandler<MouseButtonClickedEvent>(&GUIApplication::OnMouseButtonClicked);
		EventQueue::UnregisterEventHandler<MouseButtonReleasedEvent>(&GUIApplication::OnMouseButtonReleased);
		EventQueue::UnregisterEventHandler<MouseScrolledEvent>(&GUIApplication::OnMouseScrolled);
		EventQueue::UnregisterEventHandler<MouseMovedEvent>(&GUIApplication::OnMouseMoved);

		if (s_pView.GetPtr() != nullptr)
		{
			s_pView->GetRenderer()->Shutdown();
			s_pView.Reset();
		}

		SAFEDELETE(s_pRenderer);

		s_pXAMLProvider->Release();
		s_pFontProvider->Release();
		s_pTextureProvider->Release();
		Noesis::GUI::Shutdown();

		return true;
	}

	void GUIApplication::SetView(Noesis::Ptr<Noesis::IView> view)
	{
		s_pView.Reset();

		s_pView = view;
		if (s_pView != nullptr)
		{
			TSharedRef<Window> mainWindow = CommonApplication::Get()->GetMainWindow();
			s_pView->SetFlags(Noesis::RenderFlags_PPAA | Noesis::RenderFlags_LCD);
			s_pView->SetSize(uint32(mainWindow->GetWidth()), uint32(mainWindow->GetHeight()));
		}

		s_pRenderer->SetView(view);
	}

	bool GUIApplication::InitNoesis()
	{
#if defined(LAMBDA_DEVELOPMENT)
		Noesis::GUI::SetLogHandler(GUIApplication::NoesisLogHandler);
		Noesis::GUI::SetErrorHandler(GUIApplication::NoesisErrorHandler);
#endif
		// Init 26/11
		Noesis::GUI::Init("IbexOmega", "ukOSbAQLkmwv1TwFCEZinEuObNKKI+AKaCgtsw+CiLlyW7f4");

		s_pXAMLProvider		= new NoesisApp::LocalXamlProvider("../Assets/NoesisGUI/Xaml");
		s_pFontProvider		= new NoesisApp::LocalFontProvider("../Assets/NoesisGUI/Fonts");
		s_pTextureProvider	= new NoesisApp::LocalTextureProvider("../Assets/NoesisGUI/Textures");

		//Application Resources
		NoesisApp::SetThemeProviders(
			s_pXAMLProvider,
			s_pFontProvider,
			s_pTextureProvider);

		Noesis::GUI::LoadApplicationResources("Theme/NoesisTheme.DarkBlue.xaml");

		//Renderer Initialization
		s_pRenderer = new GUIRenderer();
		if (!s_pRenderer->Init())
		{
			LOG_ERROR("[GUIApplication]: Failed to initialize Renderer");
			return false;
		}

		return true;
	}

	void GUIApplication::NoesisLogHandler(const char* file, uint32_t line, uint32_t level, const char* channel, const char* message)
	{
		if (level == 0) // [TRACE]
		{
			LOG_MESSAGE("[TRACE] In \"%s\", at L%d and channel \"%s\":\n \"%s\"", file, line, channel, message);
		}
		else if (level == 1) // [DEBUG]
		{
			LOG_MESSAGE("[DEBUG] In \"%s\", at L%d and channel \"%s\":\n \"%s\"", file, line, channel, message);
		}
		else if (level == 2) // [INFO]
		{
			LOG_INFO("[INFO] In \"%s\", at L%d and channel \"%s\":\n \"%s\"", file, line, channel, message);
		}
		else if (level == 3) // [WARNING]
		{
			LOG_WARNING("[WARNING] In \"%s\", at L%d and channel \"%s\":\n \"%s\"", file, line, channel, message);
		}
		else if (level == 4) // [ERROR]
		{
			LOG_ERROR("[ERROR] In \"%s\", at L%d and channel \"%s\":\n \"%s\"", file, line, channel, message);
		}
	}

	void GUIApplication::NoesisErrorHandler(const char* file, uint32_t line, const char* message, bool fatal)
	{
		if (fatal)
		{
			LOG_ERROR("[FATAL] In \"%s\", at L%d:\n \"%s\"", file, line, message);
		}
		else
		{
			LOG_ERROR("In \"%s\", at L%d:\n \"%s\"", file, line, message);
		}
	}

	bool GUIApplication::OnPostSwapChainRecreated(const PostSwapChainRecreatedEvent& postSwapChainRecreatedEvent)
	{
		if (s_pView.GetPtr() != nullptr)
			s_pView->SetSize(postSwapChainRecreatedEvent.NewWidth, postSwapChainRecreatedEvent.NewHeight);
		return true;
	}

	bool GUIApplication::OnKeyPressed(const KeyPressedEvent& keyPressedEvent)
	{
		if (s_pView.GetPtr() != nullptr)
			s_pView->KeyDown(GUIInputMapper::GetKey(keyPressedEvent.Key));
		return true;
	}

	bool GUIApplication::OnKeyReleased(const KeyReleasedEvent& keyReleasedEvent)
	{
		if (s_pView.GetPtr() != nullptr)
			s_pView->KeyUp(GUIInputMapper::GetKey(keyReleasedEvent.Key));
		return true;
	}

	bool GUIApplication::OnKeyTyped(const KeyTypedEvent& keyTypedEvent)
	{
		if (s_pView.GetPtr() != nullptr)
			s_pView->Char(keyTypedEvent.Character);
		return true;
	}

	bool GUIApplication::OnMouseButtonClicked(const MouseButtonClickedEvent& mouseButtonClickedEvent)
	{
		if (s_pView.GetPtr() != nullptr)
			s_pView->MouseButtonDown(mouseButtonClickedEvent.Position.x, mouseButtonClickedEvent.Position.y, GUIInputMapper::GetMouseButton(mouseButtonClickedEvent.Button));
		return true;
	}

	bool GUIApplication::OnMouseButtonReleased(const MouseButtonReleasedEvent& mouseButtonReleasedEvent)
	{
		if (s_pView.GetPtr() != nullptr)
			s_pView->MouseButtonUp(mouseButtonReleasedEvent.Position.x, mouseButtonReleasedEvent.Position.y, GUIInputMapper::GetMouseButton(mouseButtonReleasedEvent.Button));
		return true;
	}

	bool GUIApplication::OnMouseScrolled(const MouseScrolledEvent& mouseScrolledEvent)
	{
		if (s_pView.GetPtr() == nullptr)
			return true;

		if (glm::abs(mouseScrolledEvent.DeltaY) > 0)		s_pView->MouseWheel(mouseScrolledEvent.Position.x, mouseScrolledEvent.Position.y, 120 * mouseScrolledEvent.DeltaY);
		else if (glm::abs(mouseScrolledEvent.DeltaX) > 0)	s_pView->MouseHWheel(mouseScrolledEvent.Position.x, mouseScrolledEvent.Position.y, 120 * mouseScrolledEvent.DeltaX);

		return true;
	}

	bool GUIApplication::OnMouseMoved(const MouseMovedEvent& mouseMovedEvent)
	{
		if (s_pView.GetPtr() != nullptr)
			s_pView->MouseMove(mouseMovedEvent.Position.x, mouseMovedEvent.Position.y);
		return true;
	}
}