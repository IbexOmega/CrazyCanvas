#pragma once
#include "Input/API/InputCodes.h"

#ifdef LAMBDA_VISUAL_STUDIO
	#pragma warning(push)
	#pragma warning(disable : 4100) // Disable unreferenced variable warning
#endif

namespace LambdaEngine
{
	enum class EResizeType
	{
		RESIZE_TYPE_NONE		= 0,
		RESIZE_TYPE_MAXIMIZE	= 1,
		RESIZE_TYPE_MINIMIZE	= 2,
	};

	class Window;

	class EventHandler
	{
	public:
		DECL_INTERFACE(EventHandler);

		/*
		* Called when window focus changed.
		*   pWindow     - The window that changed focus status
		*   hasFocus    - True if pWindow got focus, otherwise false
		*/
		virtual void OnFocusChanged(Window* pWindow, bool hasFocus)
		{
		}

		/*
		* Called when window moved.
		*   pWindow - The window that moved
		*   x       - New x position of the window
		*   y       - New y position of the window
		*/
		virtual void OnWindowMoved(Window* pWindow, int16 x, int16 y)
		{
		}

		/*
		* Called when window focus changed.
		*   pWindow - The window that got focus
		*   width   - The new width of the window
		*   height  - The new height of the window
		*/
		virtual void OnWindowResized(Window* pWindow, uint16 width, uint16 height, EResizeType type)
		{
		}

		/*
		* Called when a window is closed
		*  pWindow - The closed window
		*/
		virtual void OnWindowClosed(Window* pWindow)
		{
		}

		/*
		* Called when the mousecursor entered a window
		*  pWindow - The window that the mouse entered
		*/
		virtual void OnMouseEntered(Window* pWindow)
		{
		}

		/*
		* Called when the mousecursor left a window
		*  pWindow - The window that the mouse left
		*/
		virtual void OnMouseLeft(Window* pWindow)
		{
		}

		/*
		* Will be called when a mouse move event occurs. Mouse coordinates are in the active window's client space.
		*	x - The new horizontal coordinates of the mouse
		*	y - The new vertical coordinates of the mouse
		*/
		virtual void OnMouseMoved(int32 x, int32 y)
		{
		}

		/*
		* Will be called when a raw mouse move event occurs. Mouse coordinates are raw delta values of the mouse movement.
		* This function will only be called when Raw input is activated on a window
		*	deltaX - The raw mouse x-coordinate delta
		*	deltaY - The raw mouse y-coordinate delta
		*/
		virtual void OnMouseMovedRaw(int32 deltaX, int32 deltaY)
		{
		}

		/*
		* Will be called when a mouse button pressed event occurs
		*	button - Which button was pressed
		*/
		virtual void OnButtonPressed(EMouseButton button, uint32 modifierMask)
		{
		}

		/*
		* Will be called when a mouse button released event occurs
		*	button - Which button was released
		*/
		virtual void OnButtonReleased(EMouseButton button)
		{
		}

		/*
		* Will be called when a mouse scroll event occurs
		*   deltaX - The amount of scrolling delta < 0 for left scrolling and delta > 0 for right scrolling
		*   deltaY - The amount of scrolling delta < 0 for downwards scrolling and delta > 0 for upwards scrolling
		*/
		virtual void OnMouseScrolled(int32 deltaX, int32 deltaY)
		{
		}

		/*
		* Will be called when a key pressed event occurs
		*	key             - Which key was pressed.
		*   modifierMask    - A mask of values from FModiferFlag- enumeration indicating what modifer
							  keys were pressed at the same time as key.
		*   isRepeat        - True if the key already were down and this message is a repeat message.
							  Sent when a key is continuously held down.
		*/
		virtual void OnKeyPressed(EKey key, uint32 modifierMask, bool isRepeat)
		{
		}

		/*
		 * Will be called when a key released event occurs
		 *    key - Which key was released
		 */
		virtual void OnKeyReleased(EKey key)
		{
		}

		/*
		* Will be called once for each event that occurs when a key is continually held down
		*    character - Unicode character for the key that recently were pressed
		*/
		virtual void OnKeyTyped(uint32 character)
		{
		}
	};
}

#ifdef LAMBDA_VISUAL_STUDIO
	#pragma warning(pop)
#endif