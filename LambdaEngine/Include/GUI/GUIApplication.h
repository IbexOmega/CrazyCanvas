#pragma once

namespace LambdaEngine
{
	class GUIRenderer;

	class GUIApplication
	{
	public:
		DECL_STATIC_CLASS(GUIApplication);

		static bool Init();
		static bool Release();

	private:
		static bool InitNoesis();
		static void NoesisLogHandler(const char* file, uint32_t line, uint32_t level, const char* channel, const char* message);
		static void NoesisErrorHandler(const char* file, uint32_t line, const char* message, bool fatal);

	private:
		static GUIRenderer* s_pRenderer;
	};
}