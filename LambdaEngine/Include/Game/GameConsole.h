#pragma once

#include "LambdaEngine.h"

struct ImGuiInputTextCallbackData;


namespace LambdaEngine
{
	struct Command
	{
		char* text;
	};

	class LAMBDA_API GameConsole
	{
	public:
		DECL_REMOVE_COPY(GameConsole);
		DECL_REMOVE_MOVE(GameConsole);


		static bool Init();
		static bool Release();

		static void Render();
		
	private:
		static int ParseText(const char* data);


	private:
		static TArray<char*> m_Items;
	};
}
