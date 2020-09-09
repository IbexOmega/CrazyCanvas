#pragma once
#include "LambdaEngine.h"

#include "Time/API/Timestamp.h"

namespace LambdaEngine
{
	class LAMBDA_API Game 
	{
	public:
		DECL_REMOVE_COPY(Game);
		DECL_REMOVE_MOVE(Game);

		Game();
		virtual ~Game();
		
		virtual void Tick(Timestamp delta) = 0;
		virtual void FixedTick(Timestamp delta) = 0;
		
	public:
		FORCEINLINE static Game& Get()
		{
			VALIDATE(s_pInstance != nullptr);
			return *s_pInstance;
		}
		
	private:
		static Game* s_pInstance;
	};
}
