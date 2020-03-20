#pragma once
#include "LambdaEngine.h"

namespace LambdaEngine
{
	class LAMBDA_API Application
	{
	public:
		DECL_STATIC_CLASS(Application);

		static bool PreInit() 		{ return true; }
		static bool PostRelease() 	{ return true; }
		
		static bool Tick() { return false; }
        
        static void Terminate() {}
	};
}
