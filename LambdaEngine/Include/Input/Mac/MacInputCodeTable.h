#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#include "Input/API/InputCodes.h"

namespace LambdaEngine
{
    class MacInputCodeTable
    {
    public:
        DECL_STATIC_CLASS(MacInputCodeTable);
    
        /*
        * Initializes KeyCode- and MouseButton table
        *
        * return - Returns true if successful
        */
		static bool Init();

        /*
        * Returns a LambdaEngine- EKey from a Mac- virtualkey
        *   keyCode - A virtualkey to be converted. See https://stackoverflow.com/questions/3202629/where-can-i-find-a-list-of-mac-virtual-key-codes
        *   return  - Returns a keycode of type EKey.
        */
		static EKey GetKey(int32 keyCode);

        /*
        * Returns a LambdaEngine- EMouseButton from a Mac- virtualbutton
        *   mouseButtonCode - A virtualbutton to be converted.
        *   return          - Returns a mousebutton of type EMouseButton.
        */
		static EMouseButton GetMouseButton(int32 mouseButtonCode);
        
        /*
         * Returns a mask with modiferFlags
         *  modiferFlags    - Modiferflags from a macOS event
         *  return          - Returns a LambdaEngine modiferMask
         */
        static uint32 GetModiferMask(uint32 modiferFlags);

    private:
        static EKey 		s_KeyCodeTable[256];
		static EMouseButton s_MouseButtonCodeTable[5];
    };
}

#endif
