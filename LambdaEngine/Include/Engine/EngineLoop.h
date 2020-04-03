#pragma once
#include "Game/Game.h"

#include "Time/API/Timestamp.h"

#include "Application/API/PlatformApplication.h"

namespace LambdaEngine
{
	/*
	* This class controls the game-loop and makes sure that all engine
	* submodules gets runned atleast once per frame
	*/
	class LAMBDA_API EngineLoop
	{
	public:
		DECL_STATIC_CLASS(EngineLoop);

		/*
		* Initializes modules that are needed in EngineLoop::Init()
		*
		* return - Returns tru if initialization is successfull
		*/
#ifdef LAMBDA_PLATFORM_WINDOWS
		static bool PreInit(HINSTANCE hInstance);
#else
		static bool PreInit();
#endif
		static bool Init();
		static void Run(Game* pGame);
		static bool Release();
		static bool PostRelease();

	private:
		static bool Tick(Timestamp dt);
	};
}
