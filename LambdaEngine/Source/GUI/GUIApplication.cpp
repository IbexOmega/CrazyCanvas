#include "GUI/GUIApplication.h"
#include "GUI/GUIShaderManager.h"
#include "GUI/GUIRenderer.h"

#include "NsApp/LocalXamlProvider.h"
#include "NsApp/LocalFontProvider.h"
#include "NsApp/LocalTextureProvider.h"
#include "NoesisPCH.h"

#include "Application/API/CommonApplication.h"
#include "Application/API/Window.h"

#include "Application/API/Events/EventQueue.h"

namespace LambdaEngine
{
	Noesis::Ptr<Noesis::IView> GUIApplication::s_pView = nullptr;
	GUIRenderer* GUIApplication::s_pRenderer = nullptr;

	bool GUIApplication::Init()
	{
		EventQueue::RegisterEventHandler<WindowResizedEvent>(&GUIApplication::OnWindowResized);

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
		s_pView->GetRenderer()->Shutdown();
		SAFEDELETE(s_pRenderer);
		s_pView.Reset();
		Noesis::GUI::Shutdown();

		return true;
	}

	bool GUIApplication::InitNoesis()
	{
#ifdef LAMBDA_DEBUG
		Noesis::GUI::SetLogHandler(GUIApplication::NoesisLogHandler);
		Noesis::GUI::SetErrorHandler(GUIApplication::NoesisErrorHandler);
#elif LAMBDA_RELEASE
		Noesis::GUI::SetErrorHandler(GUIApplication::NoesisErrorHandler);
#endif

		//Init
		Noesis::GUI::Init("IbexOmega", "Uz25EdN1uRHmmJJyF0SjbeNtuCheNvKnJoeAhCTyh/NxhLSa");

		//Set Providers
		Noesis::GUI::SetXamlProvider(Noesis::MakePtr<NoesisApp::LocalXamlProvider>("..\Assets\NoesisGUI\Xaml"));
		Noesis::GUI::SetFontProvider(Noesis::MakePtr<NoesisApp::LocalFontProvider>("..\Assets\NoesisGUI\Fonts"));
		Noesis::GUI::SetTextureProvider(Noesis::MakePtr<NoesisApp::LocalTextureProvider>("..\Assets\NoesisGUI\Textures"));

		//Set Fallbacks
		const char* fonts[] = { "Arial", "Segoe UI Emoji" };
		Noesis::GUI::SetFontFallbacks(fonts, ARR_SIZE(fonts));
		Noesis::GUI::SetFontDefaultProperties(15.0f, Noesis::FontWeight_Normal, Noesis::FontStretch_Normal, Noesis::FontStyle_Normal);

		//Application Resources
		Noesis::GUI::LoadApplicationResources("Theme/NoesisTheme.DarkBlue.xaml");

		//Renderer Initialization
		s_pRenderer = new GUIRenderer();

		if (!s_pRenderer->Init())
		{
			LOG_ERROR("[GUIApplication]: Failed to initialize Renderer");
			return false;
		}

		//View Creation
		TSharedRef<Window> mainWindow = CommonApplication::Get()->GetMainWindow();
		Noesis::Ptr<Noesis::FrameworkElement> xaml = Noesis::GUI::LoadXaml<Noesis::FrameworkElement>("ThemePreview.xaml");
		s_pView = Noesis::GUI::CreateView(xaml);
		s_pView->SetFlags(Noesis::RenderFlags_PPAA | Noesis::RenderFlags_LCD);
		s_pView->SetSize(uint32(mainWindow->GetWidth()), uint32(mainWindow->GetHeight()));
		s_pView->GetRenderer()->Init(s_pRenderer);

		return true;
	}

	void GUIApplication::NoesisLogHandler(const char* file, uint32_t line, uint32_t level, const char* channel, const char* message)
	{
		if (level == 0) // [TRACE]
		{
			LOG_MESSAGE("[NoesisGUI]: [TRACE] In \"%s\", at L%d and channel \"%s\":\n \"%s\"", file, line, channel, message);
		}
		else if (level == 1) // [DEBUG]
		{
			LOG_MESSAGE("[NoesisGUI]: [DEBUG] In \"%s\", at L%d and channel \"%s\":\n \"%s\"", file, line, channel, message);
		}
		else if (level == 2) // [INFO]
		{
			LOG_INFO("[NoesisGUI]: [INFO] In \"%s\", at L%d and channel \"%s\":\n \"%s\"", file, line, channel, message);
		}
		else if (level == 3) // [WARNING]
		{
			LOG_WARNING("[NoesisGUI]: [WARNING] In \"%s\", at L%d and channel \"%s\":\n \"%s\"", file, line, channel, message);
		}
		else if (level == 4) // [ERROR]
		{
			LOG_ERROR("[NoesisGUI]: [ERROR] In \"%s\", at L%d and channel \"%s\":\n \"%s\"", file, line, channel, message);
		}
	}

	void GUIApplication::NoesisErrorHandler(const char* file, uint32_t line, const char* message, bool fatal)
	{
		if (fatal)
		{
			LOG_ERROR("[NoesisGUI]: [FATAL] In \"%s\", at L%d:\n \"%s\"", file, line, message);
		}
		else
		{
			LOG_ERROR("[NoesisGUI]: In \"%s\", at L%d:\n \"%s\"", file, line, message);
		}
	}

	bool GUIApplication::OnWindowResized(const WindowResizedEvent& windowEvent)
	{
		s_pView->SetSize(windowEvent.Width, windowEvent.Height);
		return true;
	}
}