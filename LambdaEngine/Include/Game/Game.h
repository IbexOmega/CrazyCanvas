#pragma once
#include "LambdaEngine.h"

namespace LambdaEngine
{
	class Game 
	{
	public:
		DECL_ABSTRACT_CLASS(Game);

		virtual void Tick() = 0;
	};
}
