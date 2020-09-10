#pragma once
#include "InputState.h"

#include "Application/API/Events/Event.h"

namespace LambdaEngine
{
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

		static void Disable();

		FORCEINLINE static bool IsKeyDown(EKey key)
		{
			return s_KeyboardState.IsKeyDown(key);
		}

		FORCEINLINE static bool IsKeyUp(EKey key)
		{
			return s_KeyboardState.IsKeyUp(key);
		}

		FORCEINLINE static const KeyboardState& GetKeyboardState()
		{
			return s_KeyboardState;
		}

		FORCEINLINE static const MouseState& GetMouseState()
		{
			return s_MouseState;
		}

	private:
		static bool HandleEvent(const Event& event);

	private:
		static KeyboardState s_KeyboardState;
		static MouseState s_MouseState;
		static bool s_InputEnabled;
	};
}
