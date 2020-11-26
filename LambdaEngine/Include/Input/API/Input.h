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
				bool* keyStates		= s_KeyboardStates[ConvertInputModeUINT8(s_InputModeStack.top())][STATE_WRITE_INDEX].KeyStates;
				bool* mouseStates = s_MouseStates[ConvertInputModeUINT8(s_InputModeStack.top())][STATE_WRITE_INDEX].ButtonStates;

				for (int k = 0; k < EKey::KEY_COUNT; k++)
				{
					keyStates[k] = false;
				}

				for (int m = 0; m < EMouseButton::MOUSE_BUTTON_COUNT; m++)
				{
					mouseStates[m] = false;
				}
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
