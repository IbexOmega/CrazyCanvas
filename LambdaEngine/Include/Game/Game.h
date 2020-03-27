#pragma once

namespace LambdaEngine
{
	class Game 
	{
	public:
		Game()          = default;
		virtual ~Game() = default;

		virtual void Tick() = 0;
	};
}
