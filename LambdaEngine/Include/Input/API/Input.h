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

		FORCEINLINE static void PushInputLayer(EInputLayer inputMode)
		{
			std::scoped_lock<SpinLock> inputStackLock(s_InputStackLock);

			if (!s_InputLayerStack.empty())
			{
				const uint8 currentInputMode = ConvertInputLayerUINT8(s_InputLayerStack.top());
				std::fill_n(s_KeyboardStates[currentInputMode][STATE_WRITE_INDEX].KeyStates, EKey::KEY_COUNT, false);
				std::fill_n(s_MouseStates[currentInputMode][STATE_WRITE_INDEX].ButtonStates, EMouseButton::MOUSE_BUTTON_COUNT, false);

				UpdateReadIndex(currentInputMode);
			}

			s_InputLayerStack.push(inputMode);
		}

		FORCEINLINE static void PopInputLayer()
		{
			std::scoped_lock<SpinLock> inputStackLock(s_InputStackLock);
			s_InputLayerStack.pop();
		}

		FORCEINLINE static EInputLayer GetCurrentInputLayer()
		{
			std::scoped_lock<SpinLock> inputStackLock(s_InputStackLock);
			return s_InputLayerStack.top();
		}

		static void Disable();

		FORCEINLINE static bool IsKeyDown(EInputLayer inputMode, EKey key)
		{
			return s_KeyboardStates[ConvertInputLayerUINT8(inputMode)][STATE_READ_INDEX].IsKeyDown(key);
		}

		FORCEINLINE static bool IsKeyUp(EInputLayer inputMode, EKey key)
		{
			return s_KeyboardStates[ConvertInputLayerUINT8(inputMode)][STATE_READ_INDEX].IsKeyUp(key);
		}

		FORCEINLINE static bool IsInputEnabled()
		{
			return s_InputEnabled;
		}

		FORCEINLINE static void UpdateReadIndex(uint8 inputMode)
		{
			std::scoped_lock<SpinLock> keyboardLock(s_WriteBufferLockKeyboard);
			std::scoped_lock<SpinLock> mouseLock(s_WriteBufferLockMouse);

			s_KeyboardStates[inputMode][0]	= s_KeyboardStates[inputMode][1];
			s_MouseStates[inputMode][0]		= s_MouseStates[inputMode][1];
		}

		FORCEINLINE static const KeyboardState& GetKeyboardState(EInputLayer inputMode)
		{
			return s_KeyboardStates[ConvertInputLayerUINT8(inputMode)][STATE_READ_INDEX];
		}

		FORCEINLINE static const MouseState& GetMouseState(EInputLayer inputMode)
		{
			return s_MouseStates[ConvertInputLayerUINT8(inputMode)][STATE_READ_INDEX];
		}

	private:
		static bool HandleEvent(const Event& event);
		static uint8 ConvertInputLayerUINT8(EInputLayer inputMode);

	private:
		// Input states are double buffered. The first one is read from, the second is written to.
		static KeyboardState	s_KeyboardStates[4][2];
		static MouseState		s_MouseStates[4][2];

		// Make sure nothing is being written to the write buffer when copying write buffer to read buffer in Input::Tick
		static SpinLock s_WriteBufferLockMouse;
		static SpinLock s_WriteBufferLockKeyboard;
		static SpinLock s_InputStackLock;

		static std::atomic_bool			s_InputEnabled;
		static std::stack<EInputLayer>	s_InputLayerStack;
	};
}
