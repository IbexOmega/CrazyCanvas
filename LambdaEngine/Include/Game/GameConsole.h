#pragma once

#include "LambdaEngine.h"



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

		static void Render();



	private:
		static char buf[32];
		TArray<Command> commands;

	};
}
