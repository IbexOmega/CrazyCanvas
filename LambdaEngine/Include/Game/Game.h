#pragma once
#include "LambdaEngine.h"

#include "Time/API/Timestamp.h"

namespace LambdaEngine
{
	class Game 
	{
	public:
		DECL_ABSTRACT_CLASS(Game);
		
		virtual void Tick(Timestamp dt) = 0;
	};
}
