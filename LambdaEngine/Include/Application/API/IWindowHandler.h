#pragma once
#include "IWindow.h"

namespace LambdaEngine
{
    enum class EResizeType
    {
        RESIZE_TYPE_NONE        = 0,
        RESIZE_TYPE_MAXIMIZE    = 1,
        RESIZE_TYPE_MINIMIZE    = 2,
    };

    class IWindowHandler
    {
    public:
        DECL_INTERFACE(IWindowHandler);
        
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
    };
}
