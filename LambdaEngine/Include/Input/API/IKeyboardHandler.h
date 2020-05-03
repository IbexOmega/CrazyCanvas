#pragma once
#include "InputCodes.h"

namespace LambdaEngine
{
	class IKeyboardHandler
	{
	public:
		DECL_INTERFACE(IKeyboardHandler);
		
		/*
		* Will be called when a key pressed event occurs
		*	key             - Which key was pressed.
        *   modifierMask    - A mask of values from FModiferFlag- enumeration indicating what modifer
                              keys were pressed at the same time as key.
        *   isRepeat        - True if the key already were down and this message is a repeat message.
                              Sent when a key is continuously held down.
		*/
		virtual void KeyPressed(EKey key, uint32 modifierMask, bool isRepeat) = 0;

        /*
         * Will be called when a key released event occurs
         *    key - Which key was released
         */
        virtual void KeyReleased(EKey key) = 0;

        /*
        * Will be called once for each event that occurs when a key is continually held down
        *    character - Unicode character for the key that recently were pressed
        */
        virtual void KeyTyped(uint32 character) = 0;

	};
}
