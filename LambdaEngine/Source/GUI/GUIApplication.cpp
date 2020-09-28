#include "GUI/GUIApplication.h"
#include "GUI/GUIShaderManager.h"
#include "GUI/GUIRenderer.h"

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

	void GUIApplication::Tick(Timestamp delta)
	{
		s_pView->Update(delta.AsSeconds());
	}

	bool GUIApplication::InitNoesis()
	{
#if defined(LAMBDA_DEBUG)
		Noesis::GUI::SetLogHandler(GUIApplication::NoesisLogHandler);
		Noesis::GUI::SetErrorHandler(GUIApplication::NoesisErrorHandler);
#elif defined(LAMBDA_RELEASE)
		Noesis::GUI::SetErrorHandler(GUIApplication::NoesisErrorHandler);
#endif

		//Init 25/9
		Noesis::GUI::Init("IbexOmega", "Uz25EdN1uRHmmJJyF0SjbeNtuCheNvKnJoeAhCTyh/NxhLSa");

		//Application Resources
		NoesisApp::SetThemeProviders(
			new NoesisApp::LocalXamlProvider("../Assets/NoesisGUI/Xaml"),
			new NoesisApp::LocalFontProvider("../Assets/NoesisGUI/Fonts"),
			new NoesisApp::LocalTextureProvider("../Assets/NoesisGUI/Textures"));

		Noesis::GUI::LoadApplicationResources("Theme/NoesisTheme.DarkBlue.xaml");


		//View Creation
		TSharedRef<Window> mainWindow = CommonApplication::Get()->GetMainWindow();
		//Noesis::Ptr<Noesis::FrameworkElement> xaml = Noesis::GUI::LoadXaml<Noesis::FrameworkElement>("App.xaml");
		Noesis::Ptr<Noesis::Grid> xaml(Noesis::GUI::ParseXaml<Noesis::Grid>(R"(
			<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation">
				<Grid.Background>
					<LinearGradientBrush StartPoint="0,0" EndPoint="0,1">
						<GradientStop Offset="0" Color="#FF123F61"/>
						<GradientStop Offset="0.6" Color="#FF0E4B79"/>
						<GradientStop Offset="0.7" Color="#FF106097"/>
					</LinearGradientBrush>
				</Grid.Background>
				<Viewbox>
					<StackPanel Margin="50">
						<Button Content="Hello World!" Margin="0,30,0,0"/>
						<Rectangle Height="5" Margin="-10,20,-10,0">
							<Rectangle.Fill>
								<RadialGradientBrush>
									<GradientStop Offset="0" Color="#40000000"/>
									<GradientStop Offset="1" Color="#00000000"/>
								</RadialGradientBrush>
							</Rectangle.Fill>
						</Rectangle>
					</StackPanel>
				</Viewbox>
			</Grid>
		)"));
		s_pView = Noesis::GUI::CreateView(xaml);
		s_pView->SetFlags(Noesis::RenderFlags_PPAA | Noesis::RenderFlags_LCD);
		s_pView->SetSize(uint32(mainWindow->GetWidth()), uint32(mainWindow->GetHeight()));

		//Renderer Initialization
		s_pRenderer = new GUIRenderer();

		if (!s_pRenderer->Init())
		{
			LOG_ERROR("[GUIApplication]: Failed to initialize Renderer");
			return false;
		}

		s_pRenderer->SetView(s_pView);

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