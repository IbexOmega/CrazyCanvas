#pragma once
#include "IWindow.h"

#include "Input/API/InputCodes.h"

namespace LambdaEngine
{
    enum class EResizeType
    {
        RESIZE_TYPE_NONE = 0,
        RESIZE_TYPE_MAXIMIZE = 1,
        RESIZE_TYPE_MINIMIZE = 2,
    };

	class IEventHandler
	{
	public:
		DECL_INTERFACE(IEventHandler);

        /*
        * Called when window focus changed.
        *   pWindow     - The window that changed focus status
        *   hasFocus    - True if pWindow got focus, otherwise false
        */
        virtual void FocusChanged(IWindow* pWindow, bool hasFocus) = 0;

        /*
        * Called when window moved.
        *   pWindow - The window that moved
        *   x       - New x position of the window
        *   y       - New y position of the window
        */
        virtual void WindowMoved(IWindow* pWindow, int16 x, int16 y) = 0;

        /*
        * Called when window focus changed.
        *   pWindow - The window that got focus
        *   width   - The new width of the window
        *   height  - The new height of the window
        */
        virtual void WindowResized(IWindow* pWindow, uint16 width, uint16 height, EResizeType type) = 0;

        /*
            * Called when a window is closed
            *  pWindow - The closed window
            */
        virtual void WindowClosed(IWindow* pWindow) = 0;

        /*
            * Called when the mousecursor entered a window
            *  pWindow - The window that the mouse entered
            */
        virtual void MouseEntered(IWindow* pWindow) = 0;

        /*
            * Called when the mousecursor left a window
            *  pWindow - The window that the mouse left
            */
        virtual void MouseLeft(IWindow* pWindow) = 0;

        /*
        * Will be called when a mouse move event occurs. Mouse coordinates are in screen space.
        * When using RAW input this the coordinates are reletive to how much the mouse have moved and does not
        * correspond to either screen or client area space.
        *	x - The new horizontal coordinates of the mouse
        *	y - The new vertical coordinates of the mouse
        */
        virtual void MouseMoved(int32 x, int32 y) = 0;

        /*
        * Will be called when a mouse button pressed event occurs
        *	button - Which button was pressed
        */
        virtual void ButtonPressed(EMouseButton button, uint32 modifierMask) = 0;

        /*
        * Will be called when a mouse button released event occurs
        *	button - Which button was released
        */
        virtual void ButtonReleased(EMouseButton button) = 0;

        /*
        * Will be called when a mouse scroll event occurs
        *   deltaX - The amount of scrolling delta < 0 for left scrolling and delta > 0 for right scrolling
        *   deltaY - The amount of scrolling delta < 0 for downwards scrolling and delta > 0 for upwards scrolling
        */
        virtual void MouseScrolled(int32 deltaX, int32 deltaY) = 0;

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