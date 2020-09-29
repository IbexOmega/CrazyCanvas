#pragma once

#include "NsCore/Ptr.h"
#include "NsGui/IView.h"

#include "Time/API/Timestamp.h"

#include "Application/API/Events/WindowEvents.h"
#include "Application/API/Events/KeyEvents.h"
#include "Application/API/Events/MouseEvents.h"

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

		static void SetView(Noesis::Ptr<Noesis::IView> view);

		FORCEINLINE static GUIRenderer* GetRenderer() { return s_pRenderer; }

	private:
		static bool InitNoesis();
		static void NoesisLogHandler(const char* file, uint32_t line, uint32_t level, const char* channel, const char* message);
		static void NoesisErrorHandler(const char* file, uint32_t line, const char* message, bool fatal);

		static bool OnWindowResized(const WindowResizedEvent& windowEvent);
		static bool OnKeyPressed(const KeyPressedEvent& keyPressedEvent);
		static bool OnKeyReleased(const KeyReleasedEvent& keyReleasedEvent);
		static bool OnKeyTyped(const KeyTypedEvent& keyTypedEvent);

		static bool OnMouseButtonClicked(const MouseButtonClickedEvent& mouseButtonClickedEvent);
		static bool OnMouseButtonReleased(const MouseButtonReleasedEvent& mouseButtonReleasedEvent);
		static bool OnMouseScrolled(const MouseScrolledEvent& mouseScrolledEvent);
		static bool OnMouseMoved(const MouseMovedEvent& mouseMovedEvent);

	private:
		static Noesis::Ptr<Noesis::IView> s_pView;
		static GUIRenderer* s_pRenderer;
	};
}