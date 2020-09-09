#pragma once

#include "LambdaEngine.h"
#include "Math/Math.h"
#include <unordered_map>

#include "ConsoleCommand.h"

namespace LambdaEngine
{
	class LAMBDA_API GameConsole
	{
	public:
		DECL_REMOVE_COPY(GameConsole);
		DECL_REMOVE_MOVE(GameConsole);

		bool Init();
		bool Release();

		void Render();
		
		void BindCommand(ConsoleCommand cmd, std::function<void(TArray<Arg>& arguments)> callback);

		static GameConsole& Get();

	private:
		GameConsole();
		~GameConsole();
		
		struct Item
		{
			std::string str;
			glm::vec4 color;
		};

		int ExecCommand(std::string& data);

	private:
		TArray<Item> m_Items;
		bool m_ScrollToBottom;
		std::unordered_map<std::string, std::pair<ConsoleCommand, std::function<void(TArray<Arg>& arguments)>>> m_CommandMap;
	};

	/*
	
	 struct LAMBDA_API Arg
	{
		enum EType { FLOAT, BOOL, INT, STRING };

		EType type;
		union Value
		{
			float f;
			bool b;
			int i;
			char str[64];
		} value;
	};

	Command cmd;
	cmd.name = "name";
	cmd.addArg(STRING);
	cmd.addArg(INT);
	GameConsole::BindCommand(cmd, [](TArray<Arg> args){});

	/setPosition 2 4 5
	/light activate 
	/name ol 3
	
	*/
}
