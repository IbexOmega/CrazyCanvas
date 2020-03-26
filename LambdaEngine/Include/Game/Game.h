#pragma once

namespace LambdaEngine
{
	class Game 
	{
	public:
		Game() = default;
		~Game() = default;

		virtual void Tick() = 0;
	};
}