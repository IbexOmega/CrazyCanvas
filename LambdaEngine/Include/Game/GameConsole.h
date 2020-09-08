#pragma once

#include "LambdaEngine.h"
#include "Math/Math.h"

namespace LambdaEngine
{
	class LAMBDA_API GameConsole
	{
	public:
		DECL_REMOVE_COPY(GameConsole);
		DECL_REMOVE_MOVE(GameConsole);


		static bool Init();
		static bool Release();

		static void Render();
		
	private:
		struct Item
		{
			std::string str;
			glm::vec4 color;
		};

		static int ExecCommand(char* data);


	private:
		static TArray<Item> m_Items;
		static bool m_ScrollToBottom;
	};
}
