#pragma once

#include "NsCore/Ptr.h"
#include "NsGui/IView.h"

#include "Time/API/Timestamp.h"

#include "Application/API/Events/WindowEvents.h"

namespace LambdaEngine
{
	class GUIRenderer;

	class GUIApplication
	{
	public:
		DECL_STATIC_CLASS(GUIApplication);

		static bool Init();
		static bool Release();

		static void Tick(Timestamp delta);

		FORCEINLINE static GUIRenderer* GetRenderer() { return s_pRenderer; }

	private:
		static bool InitNoesis();
		static void NoesisLogHandler(const char* file, uint32_t line, uint32_t level, const char* channel, const char* message);
		static void NoesisErrorHandler(const char* file, uint32_t line, const char* message, bool fatal);

		static bool OnWindowResized(const WindowResizedEvent& windowEvent);

	private:
		static Noesis::Ptr<Noesis::IView> s_pView;
		static GUIRenderer* s_pRenderer;
	};
}