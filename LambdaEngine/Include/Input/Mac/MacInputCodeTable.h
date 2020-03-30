#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#include "Input/API/InputCodes.h"

namespace LambdaEngine
{
    class MacInputCodeTable
    {
    public:
        DECL_STATIC_CLASS(MacInputCodeTable);
    
		static bool Init();

		static EKey 		GetKey(int32 keyCode);
		static EMouseButton GetMouseButton(int32 mouseButtonCode);

    private:
        static EKey 		s_KeyCodeTable[256];
		static EMouseButton s_MouseButtonCodeTable[5];
    };
}

#endif
