#pragma once
#include "Input/API/InputCodes.h"
#include "Input/API/InputState.h"

#include "Application/API/Window.h"
#include "Core/TSharedRef.h"
#include "Window.h"

#include "Window.h"

#ifdef LAMBDA_VISUAL_STUDIO
	#pragma warning(push)
	#pragma warning(disable : 4100) // Disable unreferenced variable warning
#endif

namespace LambdaEngine
{
	/*
	* EResizeType
	*/
	enum class EResizeType
	{
		RESIZE_TYPE_NONE		= 0,
		RESIZE_TYPE_MAXIMIZE	= 1,
		RESIZE_TYPE_MINIMIZE	= 2,
	};

	/*
	* EventHandler
	*/
	class LAMBDA_API ApplicationEventHandler
	{
	public:
		DECL_INTERFACE(ApplicationEventHandler);

		/*
		* Called when window focus changed.
		*	window		- The window that changed focus status
		*	hasFocus	- True if pWindow got focus, otherwise false
		*/
		virtual void OnFocusChanged(TSharedRef<Window> window, bool hasFocus)
		{
		}

		/*
		* Called when window moved.
		*	window	- The window that moved
		*	x		- New x position of the window
		*	y		- New y position of the window
		*/
		virtual void OnWindowMoved(TSharedRef<Window> window, int16 x, int16 y)
		{
		}

		/*
		* Called when window focus changed.
		*	window	- The window that got focus
		*	width	- The new width of the window
		*	height	- The new height of the window
		*/
		virtual void OnWindowResized(TSharedRef<Window> window, uint16 width, uint16 height, EResizeType type)
		{
		}

		/*
		* Called when a window is closed
		*  window - The closed window
		*/
		virtual void OnWindowClosed(TSharedRef<Window> window)
		{
		}

		/*
		* Called when the mousecursor entered a window
		*  window - The window that the mouse entered
		*/
		virtual void OnMouseEntered(TSharedRef<Window> window)
		{
		}

		/*
		* Called when the mousecursor left a window
		*	window - The window that the mouse left
		*/
		virtual void OnMouseLeft(TSharedRef<Window> window)
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
		*	button			- Which button was pressed
		*	modifierState	- Current state of the modifer keys at the same time as key was pressed.
		*/
		virtual void OnButtonPressed(EMouseButton button, ModifierKeyState modifierState)
		{
		}

		/*
		* Will be called when a mouse button released event occurs
		*	button - Which button was released
		*/
		virtual void OnButtonReleased(EMouseButton button, ModifierKeyState modifierState)
		{
		}

		/*
		* Will be called when a mouse scroll event occurs
		*	deltaX - The amount of scrolling delta < 0 for left scrolling and delta > 0 for right scrolling
		*	deltaY - The amount of scrolling delta < 0 for downwards scrolling and delta > 0 for upwards scrolling
		*/
		virtual void OnMouseScrolled(int32 deltaX, int32 deltaY)
		{
		}

		/*
		* Will be called when a key pressed event occurs
		*	key				- Which key was pressed.
		*	modifierState	- Current state of the modifer keys at the same time as key was pressed.
		*	isRepeat		- True if the key already were down and this message is a repeat message.
		*					  Sent when a key is continuously held down.
		*/
		virtual void OnKeyPressed(EKey key, ModifierKeyState modifierState, bool isRepeat)
		{
		}

		/*
		 * Will be called when a key released event occurs
		 *	key - Which key was released
		 */
		virtual void OnKeyReleased(EKey key, ModifierKeyState modifierState)
		{
		}

		/*
		* Will be called once for each event that occurs when a key is continually held down
		*	character - Unicode character for the key that recently were pressed
		*/
		virtual void OnKeyTyped(uint32 character)
		{
		}
	};
}

#ifdef LAMBDA_VISUAL_STUDIO
	#pragma warning(pop)
#endif