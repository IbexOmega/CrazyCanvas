#pragma once
#include "LambdaEngine.h"

#include "Time/API/Timestamp.h"

namespace LambdaEngine
{
	class Game 
	{
	public:
        Game();
        virtual ~Game();
        
        DECL_REMOVE_COPY(Game);
        DECL_REMOVE_MOVE(Game);
		
		virtual void Tick(Timestamp delta)      = 0;
        virtual void FixedTick(Timestamp delta) = 0;
        
    public:
        FORCEINLINE static Game* Get()
        {
            return s_pInstance;
        }
        
    private:
        static Game* s_pInstance;
	};
}
