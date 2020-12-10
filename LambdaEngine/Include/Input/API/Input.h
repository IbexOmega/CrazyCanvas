#pragma once
#include "InputState.h"

#include "Application/API/Events/Event.h"

#include "Threading/API/SpinLock.h"

#include <stack>

namespace LambdaEngine
{
	#define STATE_READ_INDEX 0
	#define STATE_WRITE_INDEX 1

	enum class EInputLayer : uint8
	{
		GUI		= 0,
		GAME	= 1,
		DEBUG	= 2,
		DEAD	= 3,
		NONE	= UINT8_MAX,
	};

	/*
	* Input
	*/
	class LAMBDA_API Input
	{
	public:
		DECL_STATIC_CLASS(Input);

		static bool Init();
		static bool Release();

		static void Tick();

		FORCEINLINE static void Enable()
		{
			s_InputEnabled = true;
		}

		FORCEINLINE static void PushInputMode(EInputLayer inputMode)
		{
			if (!s_InputModeStack.empty())
			{
				bool* pKeyStates	= s_KeyboardStates[ConvertInputModeUINT8(s_InputModeStack.top())][STATE_WRITE_INDEX].KeyStates;
				bool* pMouseStates	= s_MouseStates[ConvertInputModeUINT8(s_InputModeStack.top())][STATE_WRITE_INDEX].ButtonStates;

				for (int k = 0; k < EKey::KEY_COUNT; k++)
				{
					pKeyStates[k] = false;
				}

				for (int m = 0; m < EMouseButton::MOUSE_BUTTON_COUNT; m++)
				{
					pMouseStates[m] = false;
				}

				UpdateReadIndex(ConvertInputModeUINT8(s_InputModeStack.top()));
			}

			s_InputModeStack.push(inputMode);
		}

		FORCEINLINE static void PopInputMode()
		{
			s_InputModeStack.pop();
		}

		FORCEINLINE static EInputLayer GetCurrentInputmode()
		{
			return s_InputModeStack.top();
		}

		static void Disable();

		FORCEINLINE static bool IsKeyDown(EInputLayer inputMode, EKey key)
		{
			return s_KeyboardStates[ConvertInputModeUINT8(inputMode)][STATE_READ_INDEX].IsKeyDown(key);
		}

		FORCEINLINE static bool IsKeyUp(EInputLayer inputMode, EKey key)
		{
			return s_KeyboardStates[ConvertInputModeUINT8(inputMode)][STATE_READ_INDEX].IsKeyUp(key);
		}

		FORCEINLINE static bool IsInputEnabled()
		{
			return s_InputEnabled;
		}

		FORCEINLINE static void UpdateReadIndex(uint8 inputMode)
		{
			std::scoped_lock<SpinLock> keyboardLock(s_WriteBufferLockKeyboard);
			std::scoped_lock<SpinLock> mouseLock(s_WriteBufferLockMouse);


			s_KeyboardStates[inputMode][0] = s_KeyboardStates[inputMode][1];
			s_MouseStates[inputMode][0] = s_MouseStates[inputMode][1];
		}

		FORCEINLINE static void UpdateMouseScrollButton()
		{
			uint8 inputMode = ConvertInputModeUINT8(s_InputModeStack.top());

			if (IsInputEnabled())
			{
				MouseState& mouseState = s_MouseStates[inputMode][STATE_WRITE_INDEX];

				if (s_HasScrolled)
				{
					EMouseButton mouseButton = mouseState.ScrollY > 0 ? EMouseButton::MOUSE_BUTTON_SCROLL_UP : EMouseButton::MOUSE_BUTTON_SCROLL_DOWN;
					s_MouseStates[inputMode][STATE_WRITE_INDEX].ButtonStates[mouseButton] = true;
				}
				else
				{
					mouseState.ScrollX = mouseState.ScrollY = 0;
					s_MouseStates[inputMode][STATE_WRITE_INDEX].ButtonStates[EMouseButton::MOUSE_BUTTON_SCROLL_UP] = false;
					s_MouseStates[inputMode][STATE_WRITE_INDEX].ButtonStates[EMouseButton::MOUSE_BUTTON_SCROLL_DOWN] = false;
				}
			}
		}

		FORCEINLINE static const KeyboardState& GetKeyboardState(EInputLayer inputMode)
		{
			return s_KeyboardStates[ConvertInputModeUINT8(inputMode)][STATE_READ_INDEX];
		}

		FORCEINLINE static const MouseState& GetMouseState(EInputLayer inputMode)
		{
			return s_MouseStates[ConvertInputModeUINT8(inputMode)][STATE_READ_INDEX];
		}

	private:
		static bool HandleEvent(const Event& event);
		static uint8 ConvertInputModeUINT8(EInputLayer inputMode);

	private:
		inline static bool s_HasScrolled = false;

		// Input states are double buffered. The first one is read from, the second is written to.
		static KeyboardState s_KeyboardStates[4][4];
		static MouseState s_MouseStates[4][4];

		// Make sure nothing is being written to the write buffer when copying write buffer to read buffer in Input::Tick
		static SpinLock s_WriteBufferLockMouse;
		static SpinLock s_WriteBufferLockKeyboard;
		static std::atomic_bool s_InputEnabled;

		static std::stack<EInputLayer> s_InputModeStack;
	};
}
